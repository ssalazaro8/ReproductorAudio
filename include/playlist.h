#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <stddef.h>
#include <pthread.h>

#define MAX_PLAYLIST 128
#define MAX_PATH 512

typedef struct {
    char path[MAX_PATH];
} Track;

typedef struct {
    Track tracks[MAX_PLAYLIST];
    size_t count;
    size_t current;
    pthread_rwlock_t lock;
} Playlist;

int playlist_init(Playlist *playlist);
void playlist_destroy(Playlist *playlist);

int playlist_add(Playlist *playlist, const char *path);
int playlist_remove(Playlist *playlist, size_t index);
void playlist_clear(Playlist *playlist);

size_t playlist_count(Playlist *playlist);
int playlist_get(Playlist *playlist, size_t index, Track *out);
int playlist_current(Playlist *playlist, Track *out);
int playlist_next(Playlist *playlist);
int playlist_prev(Playlist *playlist);
int playlist_set_current(Playlist *playlist, size_t index);

void playlist_print(Playlist *playlist);

#endif
