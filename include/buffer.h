#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <pthread.h>

typedef struct {
    unsigned char *data;
    size_t capacity;
    size_t read_pos;
    size_t write_pos;
    size_t count;
    int closed;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;

int buffer_init(CircularBuffer *buffer, size_t capacity);
void buffer_destroy(CircularBuffer *buffer);

size_t buffer_write(CircularBuffer *buffer, const unsigned char *data, size_t bytes);
size_t buffer_read(CircularBuffer *buffer, unsigned char *data, size_t bytes);

void buffer_close(CircularBuffer *buffer);
void buffer_reset(CircularBuffer *buffer);
size_t buffer_size(CircularBuffer *buffer);

#endif
