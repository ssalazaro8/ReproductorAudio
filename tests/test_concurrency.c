#include "buffer.h"
#include "playlist.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRODUCERS 4
#define ITEMS_PER_PRODUCER 5000

typedef struct {
    CircularBuffer *buffer;
    int id;
} ProducerArgs;

static void *test_producer(void *arg) {
    ProducerArgs *args = arg;
    unsigned char value = (unsigned char)args->id;

    for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
        assert(buffer_write(args->buffer, &value, 1) == 1);
    }

    return NULL;
}

static void *test_consumer(void *arg) {
    CircularBuffer *buffer = arg;
    unsigned char value;

    int total = PRODUCERS * ITEMS_PER_PRODUCER;
    for (int i = 0; i < total; ++i) {
        assert(buffer_read(buffer, &value, 1) == 1);
    }

    return NULL;
}

static void test_playlist(void) {
    Playlist playlist;
    assert(playlist_init(&playlist) == 0);

    assert(playlist_add(&playlist, "a.wav") == 0);
    assert(playlist_add(&playlist, "b.wav") == 0);
    assert(playlist_add(&playlist, "c.wav") == 0);

    assert(playlist_count(&playlist) == 3);

    Track track;
    assert(playlist_current(&playlist, &track) == 0);
    assert(strcmp(track.path, "a.wav") == 0);

    assert(playlist_next(&playlist) == 0);
    assert(playlist_current(&playlist, &track) == 0);
    assert(strcmp(track.path, "b.wav") == 0);

    assert(playlist_prev(&playlist) == 0);
    assert(playlist_current(&playlist, &track) == 0);
    assert(strcmp(track.path, "a.wav") == 0);

    assert(playlist_remove(&playlist, 0) == 0);
    assert(playlist_count(&playlist) == 2);

    playlist_clear(&playlist);
    assert(playlist_count(&playlist) == 0);

    playlist_destroy(&playlist);
}

int main(void) {
    printf("=== TESTS DE CONCURRENCIA ===\n");

    CircularBuffer buffer;
    assert(buffer_init(&buffer, 64) == 0);

    pthread_t producers[PRODUCERS];
    pthread_t consumer;

    ProducerArgs args[PRODUCERS];

    for (int i = 0; i < PRODUCERS; ++i) {
        args[i].buffer = &buffer;
        args[i].id = i;
        assert(pthread_create(&producers[i], NULL, test_producer, &args[i]) == 0);
    }

    assert(pthread_create(&consumer, NULL, test_consumer, &buffer) == 0);

    for (int i = 0; i < PRODUCERS; ++i) {
        pthread_join(producers[i], NULL);
    }

    pthread_join(consumer, NULL);

    printf("[OK] Buffer circular concurrente.\n");

    buffer_destroy(&buffer);

    test_playlist();
    printf("[OK] Playlist con rwlock.\n");

    printf("\nTODAS LAS PRUEBAS PASARON.\n");
    return 0;
}
