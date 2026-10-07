# Reproductor Concurrente - Parcial 2 Sistemas Operativos

Proyecto de la **Alternativa 2: Reproductor de Audio y Gestión Concurrente de Lista de Reproducción**.

## Objetivo

Implementar en C/POSIX un reproductor CLI con:

- Hilos POSIX (`pthread`).
- Buffer circular productor-consumidor.
- `pthread_mutex_t` y `pthread_cond_t`.
- Gestión concurrente de playlist.
- Comandos Play/Pause/Stop/Next/Previous/Add/Delete/Clear.
- Reproducción de WAV PCM 16-bit mediante ALSA.
- Pruebas de concurrencia y de la estructura de playlist.
- Makefile para compilar y ejecutar.

La estructura sigue los requisitos del parcial: la lectura/decodificación produce muestras al buffer circular y el consumidor las envía al dispositivo de audio; la playlist se protege ante modificaciones concurrentes.

## Requisitos

Ubuntu/WSL2 o Linux, GCC, Make y ALSA:

```bash
sudo apt update
sudo apt install build-essential libasound2-dev
```

Para WSL2 con WSLg, el audio normalmente se integra con el entorno gráfico de Windows. Si ALSA no encuentra un dispositivo, consulte la sección de diagnóstico.

## Compilar

```bash
make
```

## Ejecutar

Desde la raíz del proyecto:

```bash
./bin/reproductor
```

Ejemplo:

```text
=== REPRODUCTOR CONCURRENTE ===
> add audio/ejemplo.wav
> list
> play
> pause
> next
> stop
> quit
```

También se puede iniciar con una canción:

```bash
./bin/reproductor audio/ejemplo.wav
```

## Comandos

- `add <archivo.wav>`
- `list`
- `play`
- `pause`
- `stop`
- `next`
- `prev`
- `remove <indice>`
- `clear`
- `status`
- `help`
- `quit`

## Pruebas

```bash
make test
```

Las pruebas verifican principalmente la seguridad de la playlist y el buffer circular bajo acceso concurrente.

## Generar una canción WAV de prueba

Si no tienes un WAV, en Linux puedes usar Python para crear un tono de prueba:

```bash
python3 - <<'PY'
import wave, math, struct
rate = 44100
seconds = 3
freq = 440
with wave.open("audio/tono_440hz.wav", "wb") as w:
    w.setnchannels(1)
    w.setsampwidth(2)
    w.setframerate(rate)
    for i in range(rate * seconds):
        value = int(16000 * math.sin(2 * math.pi * freq * i / rate))
        w.writeframes(struct.pack("<h", value))
PY
```

Luego:

```bash
./bin/reproductor
> add audio/tono_440hz.wav
> play
```

## Arquitectura

```text
                         +--------------------+
                         |      Usuario       |
                         |      CLI/main      |
                         +---------+----------+
                                   |
                         comandos / estado
                                   |
                  +----------------+----------------+
                  |                                 |
           +------v------+                    +-----v------+
           |  Playlist   |                    | Playback   |
           | mutex/rwlock|                    | control    |
           +-------------+                    +-----+------+
                                                    |
                              +---------------------+---------------------+
                              |                                           |
                        Producer thread                              Consumer thread
                        WAV -> buffer                                buffer -> ALSA
                              |                                           ^
                              v                                           |
                    +----------------------+                              |
                    |   Buffer circular   |------------------------------+
                    | mutex + not_empty   |
                    | + not_full          |
                    +----------------------+
```

## Concurrencia

- **Producer:** lee bloques del WAV y espera cuando el buffer está lleno.
- **Consumer:** espera cuando el buffer está vacío y envía los datos a ALSA.
- **Playlist:** se protege con `pthread_rwlock_t`.
- **Control de reproducción:** usa mutex + variable de condición para despertar el hilo productor cuando cambia el estado.
- No se usa espera activa (`busy waiting`).

## Evidencias recomendadas

Para la sustentación, mostrar:

1. `make`
2. `make test`
3. `./bin/reproductor`
4. `list`
5. `play`
6. `pause`
7. `next`
8. `prev`
9. `add`
10. `remove`
11. `clear`
12. `status`
13. Dos o más hilos ejecutándose.
14. Código donde aparecen `pthread_create`, `pthread_mutex`, `pthread_cond`, `pthread_rwlock`.
15. Explicar productor-consumidor y cómo se evita el underrun/overrun.

