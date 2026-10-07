#ifndef PLAYER_H
#define PLAYER_H

#include "playlist.h"
#include "buffer.h"
#include <pthread.h>
#include <stdint.h>

typedef enum {
    PLAYER_STOPPED = 0,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} PlayerState;

typedef struct {
    Playlist *playlist;
    CircularBuffer buffer;

    pthread_t producer_thread;
    pthread_t consumer_thread;

    pthread_mutex_t control_mutex;
    pthread_cond_t control_cond;

    PlayerState state;
    int shutdown;
    int skip_requested;
    int previous_requested;

    char current_path[MAX_PATH];
    int track_finished;

    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
} Player;

int player_init(Player *player, Playlist *playlist);
int player_start(Player *player);
void player_destroy(Player *player);

void player_play(Player *player);
void player_pause(Player *player);
void player_stop(Player *player);
void player_next(Player *player);
void player_previous(Player *player);

void player_status(Player *player);

#endif
