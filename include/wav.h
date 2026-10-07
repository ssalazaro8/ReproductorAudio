#ifndef WAV_H
#define WAV_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

typedef struct {
    FILE *file;
    uint16_t audio_format;
    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
    uint32_t data_size;
    long data_offset;
} WavFile;

int wav_open(WavFile *wav, const char *path);
size_t wav_read(WavFile *wav, unsigned char *buffer, size_t bytes);
void wav_close(WavFile *wav);

#endif
