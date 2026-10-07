#include "playlist.h"
#include <stdio.h>
#include <string.h>

int playlist_init(Playlist *playlist) {
    if (!playlist) return -1;
    playlist->count = 0;
    playlist->current = 0;
    return pthread_rwlock_init(&playlist->lock, NULL);
}

void playlist_destroy(Playlist *playlist) {
    if (!playlist) return;
    pthread_rwlock_destroy(&playlist->lock);
}

int playlist_add(Playlist *playlist, const char *path) {
    if (!playlist || !path || path[0] == '\0') return -1;

    pthread_rwlock_wrlock(&playlist->lock);

    if (playlist->count >= MAX_PLAYLIST || strlen(path) >= MAX_PATH) {
        pthread_rwlock_unlock(&playlist->lock);
        return -1;
    }

    snprintf(playlist->tracks[playlist->count].path, MAX_PATH, "%s", path);
    playlist->count++;

    pthread_rwlock_unlock(&playlist->lock);
    return 0;
}

int playlist_remove(Playlist *playlist, size_t index) {
    if (!playlist) return -1;

    pthread_rwlock_wrlock(&playlist->lock);

    if (index >= playlist->count) {
        pthread_rwlock_unlock(&playlist->lock);
        return -1;
    }

    for (size_t i = index; i + 1 < playlist->count; ++i) {
        playlist->tracks[i] = playlist->tracks[i + 1];
    }

    playlist->count--;

    if (playlist->count == 0) {
        playlist->current = 0;
    } else if (playlist->current >= playlist->count) {
        playlist->current = playlist->count - 1;
    } else if (index < playlist->current) {
        playlist->current--;
    }

    pthread_rwlock_unlock(&playlist->lock);
    return 0;
}

void playlist_clear(Playlist *playlist) {
    if (!playlist) return;

    pthread_rwlock_wrlock(&playlist->lock);
    playlist->count = 0;
    playlist->current = 0;
    pthread_rwlock_unlock(&playlist->lock);
}

size_t playlist_count(Playlist *playlist) {
    if (!playlist) return 0;

    pthread_rwlock_rdlock(&playlist->lock);
    size_t count = playlist->count;
    pthread_rwlock_unlock(&playlist->lock);
    return count;
}

int playlist_get(Playlist *playlist, size_t index, Track *out) {
    if (!playlist || !out) return -1;

    pthread_rwlock_rdlock(&playlist->lock);
    if (index >= playlist->count) {
        pthread_rwlock_unlock(&playlist->lock);
        return -1;
    }
    *out = playlist->tracks[index];
    pthread_rwlock_unlock(&playlist->lock);
    return 0;
}

int playlist_current(Playlist *playlist, Track *out) {
    if (!playlist || !out) return -1;

    pthread_rwlock_rdlock(&playlist->lock);
    if (playlist->count == 0) {
        pthread_rwlock_unlock(&playlist->lock);
        return -1;
    }
    *out = playlist->tracks[playlist->current];
    pthread_rwlock_unlock(&playlist->lock);
    return 0;
}

int playlist_next(Playlist *playlist) {
    if (!playlist) return -1;

    pthread_rwlock_wrlock(&playlist->lock);
    if (playlist->count == 0) {
        pthread_rwlock_unlock(&playlist->lock);
        return -1;
    }
    playlist->current = (playlist->current + 1) % playlist->count;
    pthread_rwlock_unlock(&playlist->lock);
    return 0;
}

int playlist_prev(Playlist *playlist) {
    if (!playlist) return -1;

    pthread_rwlock_wrlock(&playlist->lock);
    if (playlist->count == 0) {
        pthread_rwlock_unlock(&playlist->lock);
        return -1;
    }
    playlist->current = (playlist->current == 0)
        ? playlist->count - 1
        : playlist->current - 1;
    pthread_rwlock_unlock(&playlist->lock);
    return 0;
}

int playlist_set_current(Playlist *playlist, size_t index) {
    if (!playlist) return -1;

    pthread_rwlock_wrlock(&playlist->lock);
    if (index >= playlist->count) {
        pthread_rwlock_unlock(&playlist->lock);
        return -1;
    }
    playlist->current = index;
    pthread_rwlock_unlock(&playlist->lock);
    return 0;
}

void playlist_print(Playlist *playlist) {
    if (!playlist) return;

    pthread_rwlock_rdlock(&playlist->lock);

    printf("\n--- PLAYLIST (%zu canciones) ---\n", playlist->count);
    for (size_t i = 0; i < playlist->count; ++i) {
        printf("%s%zu. %s\n",
               (i == playlist->current ? "-> " : "   "),
               i,
               playlist->tracks[i].path);
    }
    if (playlist->count == 0) {
        printf("(vacía)\n");
    }
    printf("--------------------------------\n");

    pthread_rwlock_unlock(&playlist->lock);
}
