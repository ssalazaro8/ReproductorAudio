#include "player.h"
#include "wav.h"

#include <alsa/asoundlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BUFFER_CAPACITY (1024 * 1024)
#define IO_CHUNK 16384

static void sleep_ms(long ms)
{
    struct timespec ts;

    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;

    nanosleep(&ts, NULL);
}

static int open_alsa(snd_pcm_t **handle, uint16_t channels, uint32_t sample_rate)
{
    int rc = snd_pcm_open(
        handle,
        "default",
        SND_PCM_STREAM_PLAYBACK,
        0
    );

    if (rc < 0) {
        fprintf(stderr, "[ALSA] No se pudo abrir el dispositivo: %s\n",
                snd_strerror(rc));
        return -1;
    }

    rc = snd_pcm_set_params(
        *handle,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        channels,
        sample_rate,
        1,
        500000
    );

    if (rc < 0) {
        fprintf(stderr, "[ALSA] No se pudo configurar audio: %s\n",
                snd_strerror(rc));

        snd_pcm_close(*handle);
        *handle = NULL;
        return -1;
    }

    return 0;
}

static void close_alsa(snd_pcm_t **handle)
{
    if (*handle) {
        snd_pcm_drain(*handle);
        snd_pcm_close(*handle);
        *handle = NULL;
    }
}

static int wait_until_playing(Player *player)
{
    pthread_mutex_lock(&player->control_mutex);

    while (!player->shutdown && player->state == PLAYER_PAUSED) {
        pthread_cond_wait(
            &player->control_cond,
            &player->control_mutex
        );
    }

    int ok = !player->shutdown &&
             player->state == PLAYER_PLAYING;

    pthread_mutex_unlock(&player->control_mutex);

    return ok;
}

static int get_control_flags(Player *player, int *skip, int *previous)
{
    pthread_mutex_lock(&player->control_mutex);

    *skip = player->skip_requested;
    *previous = player->previous_requested;

    player->skip_requested = 0;
    player->previous_requested = 0;

    int shutdown = player->shutdown;

    pthread_mutex_unlock(&player->control_mutex);

    return shutdown;
}

static void set_state(Player *player, PlayerState state)
{
    pthread_mutex_lock(&player->control_mutex);

    player->state = state;
    pthread_cond_broadcast(&player->control_cond);

    pthread_mutex_unlock(&player->control_mutex);
}

static int is_shutdown(Player *player)
{
    pthread_mutex_lock(&player->control_mutex);

    int value = player->shutdown;

    pthread_mutex_unlock(&player->control_mutex);

    return value;
}

static void reset_buffer(Player *player)
{
    buffer_reset(&player->buffer);
}

static void *producer_main(void *arg)
{
    Player *player = arg;
    unsigned char data[IO_CHUNK];

    while (!is_shutdown(player)) {
        if (!wait_until_playing(player)) {
            break;
        }

        Track track;

        if (playlist_current(player->playlist, &track) != 0) {
            set_state(player, PLAYER_PAUSED);

            fprintf(stderr,
                    "[Producer] Playlist vacía. Agrega una canción.\n");

            sleep_ms(100);
            continue;
        }

        WavFile wav;

        if (wav_open(&wav, track.path) != 0) {
            fprintf(stderr,
                    "[Producer] WAV no soportado o no encontrado: %s\n",
                    track.path);

            set_state(player, PLAYER_PAUSED);
            continue;
        }

        player->channels = wav.channels;
        player->sample_rate = wav.sample_rate;
        player->bits_per_sample = wav.bits_per_sample;

        snprintf(player->current_path, MAX_PATH, "%s", track.path);

        fprintf(stderr, "\n[Producer] Cargando: %s\n", track.path);
        fprintf(stderr, "[Producer] %u Hz | %u canales | %u bits\n",
                wav.sample_rate,
                wav.channels,
                wav.bits_per_sample);

        int ended = 0;

        while (!ended && !is_shutdown(player)) {
            if (!wait_until_playing(player)) {
                break;
            }

            int skip = 0;
            int previous = 0;

            get_control_flags(player, &skip, &previous);

            if (skip || previous) {
                reset_buffer(player);
                ended = 1;

                if (previous) {
                    playlist_prev(player->playlist);
                } else {
                    playlist_next(player->playlist);
                }

                break;
            }

            size_t n = wav_read(&wav, data, sizeof(data));

            if (n == 0) {
                ended = 1;
                break;
            }

            size_t written = buffer_write(
                &player->buffer,
                data,
                n
            );

            if (written < n) {
                ended = 1;
            }
        }

        wav_close(&wav);

        if (is_shutdown(player)) {
            break;
        }

        if (!ended) {
            continue;
        }

        if (!is_shutdown(player)) {
            playlist_next(player->playlist);
        }

        reset_buffer(player);
    }

    buffer_close(&player->buffer);

    return NULL;
}

static void *consumer_main(void *arg)
{
    Player *player = arg;
    unsigned char data[IO_CHUNK];

    snd_pcm_t *pcm = NULL;

    uint32_t opened_rate = 0;
    uint16_t opened_channels = 0;

    while (!is_shutdown(player)) {
        if (!wait_until_playing(player)) {
            break;
        }

        if (!pcm ||
            opened_rate != player->sample_rate ||
            opened_channels != player->channels) {

            close_alsa(&pcm);

            if (player->sample_rate == 0 ||
                player->channels == 0) {

                sleep_ms(100);
                continue;
            }

            if (open_alsa(
                    &pcm,
                    player->channels,
                    player->sample_rate
                ) != 0) {

                fprintf(stderr,
                        "[Consumer] Se continuará sin salida de audio.\n");

                size_t n = buffer_read(
                    &player->buffer,
                    data,
                    sizeof(data)
                );

                if (n == 0) {
                    sleep_ms(50);
                }

                continue;
            }

            opened_rate = player->sample_rate;
            opened_channels = player->channels;

            fprintf(stderr,
                    "[Consumer] ALSA listo: %u Hz, %u canales\n",
                    opened_rate,
                    opened_channels);
        }

        size_t n = buffer_read(
            &player->buffer,
            data,
            sizeof(data)
        );

        if (n == 0) {
            sleep_ms(10);
            continue;
        }

        snd_pcm_sframes_t frames =
            (snd_pcm_sframes_t)(
                n / (player->channels * sizeof(int16_t))
            );

        if (frames <= 0) {
            continue;
        }

        snd_pcm_sframes_t offset = 0;

        while (offset < frames && !is_shutdown(player)) {
            snd_pcm_sframes_t rc = snd_pcm_writei(
                pcm,
                data + offset *
                    player->channels *
                    sizeof(int16_t),
                frames - offset
            );

            if (rc == -EPIPE) {
                snd_pcm_prepare(pcm);
                continue;
            }

            if (rc < 0) {
                fprintf(stderr,
                        "[ALSA] Error de reproducción: %s\n",
                        snd_strerror((int)rc));

                snd_pcm_prepare(pcm);
                break;
            }

            offset += rc;
        }
    }

    close_alsa(&pcm);

    return NULL;
}

int player_init(Player *player, Playlist *playlist)
{
    if (!player || !playlist) {
        return -1;
    }

    memset(player, 0, sizeof(*player));

    player->playlist = playlist;
    player->state = PLAYER_STOPPED;

    if (buffer_init(&player->buffer, BUFFER_CAPACITY) != 0) {
        return -1;
    }

    if (pthread_mutex_init(&player->control_mutex, NULL) != 0) {
        buffer_destroy(&player->buffer);
        return -1;
    }

    if (pthread_cond_init(&player->control_cond, NULL) != 0) {
        pthread_mutex_destroy(&player->control_mutex);
        buffer_destroy(&player->buffer);
        return -1;
    }

    return 0;
}

int player_start(Player *player)
{
    if (!player) {
        return -1;
    }

    if (pthread_create(
            &player->producer_thread,
            NULL,
            producer_main,
            player
        ) != 0) {
        return -1;
    }

    if (pthread_create(
            &player->consumer_thread,
            NULL,
            consumer_main,
            player
        ) != 0) {

        pthread_mutex_lock(&player->control_mutex);

        player->shutdown = 1;

        pthread_cond_broadcast(&player->control_cond);

        pthread_mutex_unlock(&player->control_mutex);

        pthread_join(player->producer_thread, NULL);

        return -1;
    }

    return 0;
}

void player_destroy(Player *player)
{
    if (!player) {
        return;
    }

    pthread_mutex_lock(&player->control_mutex);

    player->shutdown = 1;
    player->state = PLAYER_STOPPED;

    pthread_cond_broadcast(&player->control_cond);

    pthread_mutex_unlock(&player->control_mutex);

    buffer_close(&player->buffer);

    pthread_join(player->producer_thread, NULL);
    pthread_join(player->consumer_thread, NULL);

    pthread_cond_destroy(&player->control_cond);
    pthread_mutex_destroy(&player->control_mutex);

    buffer_destroy(&player->buffer);
}

void player_play(Player *player)
{
    if (!player) {
        return;
    }

    set_state(player, PLAYER_PLAYING);

    fprintf(stderr, "[Control] PLAY\n");
}

void player_pause(Player *player)
{
    if (!player) {
        return;
    }

    set_state(player, PLAYER_PAUSED);

    fprintf(stderr, "[Control] PAUSE\n");
}

void player_stop(Player *player)
{
    if (!player) {
        return;
    }

    pthread_mutex_lock(&player->control_mutex);

    player->state = PLAYER_STOPPED;
    player->skip_requested = 0;
    player->previous_requested = 0;

    pthread_cond_broadcast(&player->control_cond);

    pthread_mutex_unlock(&player->control_mutex);

    reset_buffer(player);

    fprintf(stderr, "[Control] STOP\n");
}

void player_next(Player *player)
{
    if (!player) {
        return;
    }

    pthread_mutex_lock(&player->control_mutex);

    player->skip_requested = 1;

    pthread_cond_broadcast(&player->control_cond);

    pthread_mutex_unlock(&player->control_mutex);

    fprintf(stderr, "[Control] NEXT solicitado\n");
}

void player_previous(Player *player)
{
    if (!player) {
        return;
    }

    pthread_mutex_lock(&player->control_mutex);

    player->previous_requested = 1;

    pthread_cond_broadcast(&player->control_cond);

    pthread_mutex_unlock(&player->control_mutex);

    fprintf(stderr, "[Control] PREVIOUS solicitado\n");
}

void player_status(Player *player)
{
    if (!player) {
        return;
    }

    pthread_mutex_lock(&player->control_mutex);

    PlayerState state = player->state;

    pthread_mutex_unlock(&player->control_mutex);

    const char *name =
        state == PLAYER_PLAYING ? "PLAYING" :
        state == PLAYER_PAUSED ? "PAUSED" :
        "STOPPED";

    printf("\n[STATUS]\n");
    printf("Estado: %s\n", name);
    printf("Canción: %s\n",
           player->current_path[0]
               ? player->current_path
               : "(ninguna)");

    printf("Buffer: %zu bytes\n",
           buffer_size(&player->buffer));

    printf("Audio: %u Hz | %u canales | %u bits\n",
           player->sample_rate,
           player->channels,
           player->bits_per_sample);
}