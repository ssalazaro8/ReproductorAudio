#include "wav.h"
#include <string.h>

static uint16_t read_u16(FILE *f) {
    unsigned char b[2];
    if (fread(b, 1, 2, f) != 2) return 0;
    return (uint16_t)(b[0] | ((uint16_t)b[1] << 8));
}

static uint32_t read_u32(FILE *f) {
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) return 0;
    return (uint32_t)b[0] |
           ((uint32_t)b[1] << 8) |
           ((uint32_t)b[2] << 16) |
           ((uint32_t)b[3] << 24);
}

static int chunk_id(FILE *f, char out[5]) {
    if (fread(out, 1, 4, f) != 4) return -1;
    out[4] = '\0';
    return 0;
}

int wav_open(WavFile *wav, const char *path) {
    if (!wav || !path) return -1;

    memset(wav, 0, sizeof(*wav));
    wav->file = fopen(path, "rb");
    if (!wav->file) return -1;

    char riff[5];
    if (chunk_id(wav->file, riff) != 0 ||
        strcmp(riff, "RIFF") != 0) {
        wav_close(wav);
        return -1;
    }

    (void)read_u32(wav->file);

    char wave[5];
    if (chunk_id(wav->file, wave) != 0 ||
        strcmp(wave, "WAVE") != 0) {
        wav_close(wav);
        return -1;
    }

    int found_fmt = 0;
    int found_data = 0;

    while (!found_data) {
        char id[5];
        if (chunk_id(wav->file, id) != 0) break;

        uint32_t size = read_u32(wav->file);
        long chunk_start = ftell(wav->file);
        if (chunk_start < 0) break;

        if (strcmp(id, "fmt ") == 0) {
            if (size < 16) {
                wav_close(wav);
                return -1;
            }

            wav->audio_format = read_u16(wav->file);
            wav->channels = read_u16(wav->file);
            wav->sample_rate = read_u32(wav->file);
            (void)read_u32(wav->file); // byte rate
            (void)read_u16(wav->file); // block align
            wav->bits_per_sample = read_u16(wav->file);

            if (fseek(wav->file, chunk_start + (long)size, SEEK_SET) != 0) {
                wav_close(wav);
                return -1;
            }
            found_fmt = 1;
        } else if (strcmp(id, "data") == 0) {
            wav->data_offset = chunk_start;
            wav->data_size = size;
            found_data = 1;
            if (fseek(wav->file, chunk_start, SEEK_SET) != 0) {
                wav_close(wav);
                return -1;
            }
        } else {
            if (fseek(wav->file, chunk_start + (long)size, SEEK_SET) != 0) {
                wav_close(wav);
                return -1;
            }
        }

        // WAV chunks are word-aligned.
        if ((size & 1u) && !found_data) {
            if (fseek(wav->file, 1, SEEK_CUR) != 0) {
                wav_close(wav);
                return -1;
            }
        }
    }

    if (!found_fmt || !found_data ||
        wav->audio_format != 1 ||
        wav->channels == 0 ||
        wav->sample_rate == 0 ||
        wav->bits_per_sample != 16) {
        wav_close(wav);
        return -1;
    }

    return 0;
}

size_t wav_read(WavFile *wav, unsigned char *buffer, size_t bytes) {
    if (!wav || !wav->file || !buffer || bytes == 0) return 0;

    long current = ftell(wav->file);
    if (current < 0) return 0;

    long relative = current - wav->data_offset;
    if (relative < 0 || (uint32_t)relative >= wav->data_size) return 0;

    uint32_t remaining = wav->data_size - (uint32_t)relative;
    if (bytes > remaining) bytes = remaining;

    return fread(buffer, 1, bytes, wav->file);
}

void wav_close(WavFile *wav) {
    if (!wav) return;
    if (wav->file) fclose(wav->file);
    wav->file = NULL;
}
