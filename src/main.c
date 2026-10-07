#include "playlist.h"
#include "player.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 1024

static void print_help(void) {
    printf(
        "\nComandos disponibles:\n"
        "  add <archivo.wav>   Agrega una canción\n"
        "  list                Muestra la playlist\n"
        "  play                Reproducir\n"
        "  pause               Pausar\n"
        "  stop                Detener\n"
        "  next                Siguiente canción\n"
        "  prev                Canción anterior\n"
        "  remove <indice>     Eliminar canción\n"
        "  clear               Vaciar playlist\n"
        "  status              Mostrar estado e información del buffer\n"
        "  help                Mostrar ayuda\n"
        "  quit                 Salir\n\n"
    );
}

static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s);
    while (end > s && (end[-1] == '\n' || end[-1] == '\r' ||
                       end[-1] == ' ' || end[-1] == '\t')) {
        end--;
    }
    *end = '\0';
    return s;
}

int main(int argc, char **argv) {
    Playlist playlist;
    Player player;

    if (playlist_init(&playlist) != 0) {
        fprintf(stderr, "No se pudo inicializar playlist.\n");
        return EXIT_FAILURE;
    }

    if (player_init(&player, &playlist) != 0) {
        fprintf(stderr, "No se pudo inicializar player.\n");
        playlist_destroy(&playlist);
        return EXIT_FAILURE;
    }

    if (argc > 1) {
        if (playlist_add(&playlist, argv[1]) != 0) {
            fprintf(stderr, "No se pudo agregar: %s\n", argv[1]);
        }
    }

    if (player_start(&player) != 0) {
        fprintf(stderr, "No se pudieron iniciar los hilos.\n");
        player_destroy(&player);
        playlist_destroy(&playlist);
        return EXIT_FAILURE;
    }

    printf("\n=== REPRODUCTOR CONCURRENTE ===\n");
    printf("Escribe 'help' para ver los comandos.\n");

    char input[INPUT_SIZE];

    while (1) {
        printf("\n> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;

        char *command = trim(input);

        if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0) {
            break;
        }

        if (strcmp(command, "help") == 0) {
            print_help();
        } else if (strcmp(command, "list") == 0) {
            playlist_print(&playlist);
        } else if (strcmp(command, "play") == 0) {
            player_play(&player);
        } else if (strcmp(command, "pause") == 0) {
            player_pause(&player);
        } else if (strcmp(command, "stop") == 0) {
            player_stop(&player);
        } else if (strcmp(command, "next") == 0) {
            player_next(&player);
        } else if (strcmp(command, "prev") == 0) {
            player_previous(&player);
        } else if (strcmp(command, "clear") == 0) {
            playlist_clear(&playlist);
            player_stop(&player);
            printf("[Playlist] Lista vaciada.\n");
        } else if (strcmp(command, "status") == 0) {
            player_status(&player);
        } else if (strncmp(command, "add ", 4) == 0) {
            char *path = trim(command + 4);
            if (playlist_add(&playlist, path) == 0) {
                printf("[Playlist] Agregada: %s\n", path);
            } else {
                printf("[Playlist] No se pudo agregar.\n");
            }
        } else if (strncmp(command, "remove ", 7) == 0) {
            char *value = trim(command + 7);
            char *end = NULL;
            unsigned long index = strtoul(value, &end, 10);

            if (end == value || *trim(end) != '\0') {
                printf("Índice inválido.\n");
            } else if (playlist_remove(&playlist, (size_t)index) == 0) {
                printf("[Playlist] Canción eliminada.\n");
            } else {
                printf("[Playlist] Índice fuera de rango.\n");
            }
        } else if (*command != '\0') {
            printf("Comando desconocido. Usa 'help'.\n");
        }
    }

    printf("\nCerrando reproductor...\n");
    player_destroy(&player);
    playlist_destroy(&playlist);
    printf("Recursos liberados correctamente.\n");

    return EXIT_SUCCESS;
}
