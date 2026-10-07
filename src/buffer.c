#include "buffer.h"
#include <stdlib.h>
#include <string.h>

int buffer_init(CircularBuffer *buffer, size_t capacity) {
    if (!buffer || capacity == 0) return -1;

    buffer->data = malloc(capacity);
    if (!buffer->data) return -1;

    buffer->capacity = capacity;
    buffer->read_pos = 0;
    buffer->write_pos = 0;
    buffer->count = 0;
    buffer->closed = 0;

    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        free(buffer->data);
        return -1;
    }
    if (pthread_cond_init(&buffer->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->data);
        return -1;
    }
    if (pthread_cond_init(&buffer->not_full, NULL) != 0) {
        pthread_cond_destroy(&buffer->not_empty);
        pthread_mutex_destroy(&buffer->mutex);
        free(buffer->data);
        return -1;
    }

    return 0;
}

void buffer_destroy(CircularBuffer *buffer) {
    if (!buffer) return;
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
    pthread_mutex_destroy(&buffer->mutex);
    free(buffer->data);
    buffer->data = NULL;
}

size_t buffer_write(CircularBuffer *buffer, const unsigned char *data, size_t bytes) {
    if (!buffer || !data || bytes == 0) return 0;

    size_t written = 0;
    pthread_mutex_lock(&buffer->mutex);

    while (written < bytes && !buffer->closed) {
        while (buffer->count == buffer->capacity && !buffer->closed) {
            pthread_cond_wait(&buffer->not_full, &buffer->mutex);
        }
        if (buffer->closed) break;

        size_t space = buffer->capacity - buffer->count;
        size_t chunk = bytes - written;
        if (chunk > space) chunk = space;

        size_t until_end = buffer->capacity - buffer->write_pos;
        if (chunk > until_end) chunk = until_end;

        memcpy(buffer->data + buffer->write_pos, data + written, chunk);
        buffer->write_pos = (buffer->write_pos + chunk) % buffer->capacity;
        buffer->count += chunk;
        written += chunk;

        pthread_cond_signal(&buffer->not_empty);
    }

    pthread_mutex_unlock(&buffer->mutex);
    return written;
}

size_t buffer_read(CircularBuffer *buffer, unsigned char *data, size_t bytes) {
    if (!buffer || !data || bytes == 0) return 0;

    size_t read = 0;
    pthread_mutex_lock(&buffer->mutex);

    while (read < bytes) {
        while (buffer->count == 0 && !buffer->closed) {
            pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
        }

        if (buffer->count == 0 && buffer->closed) break;

        size_t available = buffer->count;
        size_t chunk = bytes - read;
        if (chunk > available) chunk = available;

        size_t until_end = buffer->capacity - buffer->read_pos;
        if (chunk > until_end) chunk = until_end;

        memcpy(data + read, buffer->data + buffer->read_pos, chunk);
        buffer->read_pos = (buffer->read_pos + chunk) % buffer->capacity;
        buffer->count -= chunk;
        read += chunk;

        pthread_cond_signal(&buffer->not_full);
    }

    pthread_mutex_unlock(&buffer->mutex);
    return read;
}

void buffer_close(CircularBuffer *buffer) {
    if (!buffer) return;

    pthread_mutex_lock(&buffer->mutex);
    buffer->closed = 1;
    pthread_cond_broadcast(&buffer->not_empty);
    pthread_cond_broadcast(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
}

void buffer_reset(CircularBuffer *buffer) {
    if (!buffer) return;

    pthread_mutex_lock(&buffer->mutex);
    buffer->read_pos = 0;
    buffer->write_pos = 0;
    buffer->count = 0;
    buffer->closed = 0;
    pthread_cond_broadcast(&buffer->not_empty);
    pthread_cond_broadcast(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
}

size_t buffer_size(CircularBuffer *buffer) {
    if (!buffer) return 0;

    pthread_mutex_lock(&buffer->mutex);
    size_t value = buffer->count;
    pthread_mutex_unlock(&buffer->mutex);
    return value;
}
