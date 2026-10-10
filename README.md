# Reproductor Concurrente — Parcial 2 de Sistemas Operativos

**Alternativa 2: Reproductor de audio y gestión concurrente de lista de reproducción**

## 1. Descripción del proyecto

Reproductor Concurrente es una aplicación de consola desarrollada en lenguaje C que permite reproducir archivos de audio WAV y administrar una lista de reproducción mediante comandos interactivos.

El proyecto aplica conceptos fundamentales de sistemas operativos, especialmente concurrencia, sincronización entre hilos, exclusión mutua, variables de condición y coordinación de recursos compartidos.

La reproducción utiliza el patrón productor-consumidor. Un hilo productor lee los datos de audio y los deposita en un buffer circular compartido, mientras que un hilo consumidor extrae esos datos y los envía al sistema de audio mediante ALSA (*Advanced Linux Sound Architecture*).

La aplicación permite administrar la playlist y controlar la reproducción sin depender de una interfaz gráfica.

### Objetivos

- Implementar concurrencia mediante hilos POSIX (`pthread`).
- Desarrollar un buffer circular para intercambiar datos de audio entre hilos.
- Utilizar `pthread_mutex_t` y `pthread_cond_t` para sincronizar el acceso a los recursos compartidos.
- Gestionar una lista de reproducción protegida mediante `pthread_rwlock_t`.
- Implementar controles de reproducción desde una interfaz CLI.
- Reproducir archivos WAV PCM de 16 bits a través de ALSA.
- Incorporar pruebas automatizadas para el buffer circular y la lista de reproducción.
- Automatizar la compilación y ejecución de pruebas mediante un `Makefile`.
- Aplicar principios de modularidad y separación de responsabilidades.

## 2. Tecnologías utilizadas

| Tecnología | Propósito |
|---|---|
| C (C11) | Lenguaje de programación |
| POSIX Threads (`pthread`) | Creación y coordinación de hilos |
| `pthread_mutex_t` | Exclusión mutua y protección de recursos compartidos |
| `pthread_cond_t` | Coordinación de hilos mediante variables de condición |
| `pthread_rwlock_t` | Protección concurrente de la lista de reproducción |
| Buffer circular | Almacenamiento temporal de bloques de audio |
| ALSA | Salida de audio en Linux |
| GCC | Compilación del código fuente |
| GNU Make | Automatización de compilación y pruebas |
| Ubuntu / WSL2 | Entorno de desarrollo y ejecución |
| WAV PCM | Formato de audio utilizado en las pruebas |

## 3. Requisitos del sistema

Para compilar y ejecutar el proyecto se necesita:

- Ubuntu, otra distribución Linux compatible o Ubuntu mediante WSL2.
- GCC con soporte para C11 y POSIX Threads.
- GNU Make.
- Bibliotecas de desarrollo de ALSA.
- Un dispositivo o servidor de audio compatible para escuchar la reproducción.

Python 3 es opcional y puede utilizarse para generar archivos WAV de prueba.

### Instalación de dependencias

En Ubuntu o WSL2, ejecutar:

```bash
sudo apt update
sudo apt install build-essential libasound2-dev alsa-utils
```

Comprobar que las herramientas estén instaladas:

```bash
gcc --version
make --version
aplay -L
```

En WSL2, la salida de audio depende de la configuración del entorno y de la integración con Windows. Si el reproductor compila, pero no se escucha el audio, consultar la sección de solución de problemas.

## 4. Estructura del proyecto

La aplicación está organizada en módulos que separan la interfaz, la reproducción, la gestión de la playlist, el manejo de archivos WAV y la sincronización del buffer.

```text
reproductor_concurrente/
├── Makefile
├── README.md
├── .gitignore
├── audio/
│   ├── sonido_1_440hz.wav
│   ├── sonido_2_660hz.wav
│   └── sonido_3_880hz.wav
├── include/
│   ├── buffer.h
│   ├── player.h
│   ├── playlist.h
│   └── wav.h
├── src/
│   ├── main.c
│   ├── buffer.c
│   ├── playlist.c
│   ├── wav.c
│   └── player.c
├── tests/
│   └── test_concurrency.c
├── scripts_generate_test_wav.sh
└── bin/
    ├── reproductor
    └── test_concurrency
```

Los archivos ejecutables y los archivos objeto se generan durante la compilación. El contenido de `bin/` puede no estar presente en un clon limpio del repositorio hasta ejecutar los comandos de compilación.

### Descripción de los módulos

| Archivo | Responsabilidad |
|---|---|
| `src/main.c` | Punto de entrada, interfaz de consola y procesamiento de comandos. |
| `src/buffer.c` | Implementación del buffer circular y su sincronización. |
| `src/playlist.c` | Operaciones de administración y acceso concurrente a la playlist. |
| `src/wav.c` | Lectura y procesamiento de archivos WAV admitidos. |
| `src/player.c` | Coordinación de la reproducción, los hilos y la salida de audio. |
| `include/buffer.h` | Declaraciones de las funciones y estructuras del buffer. |
| `include/player.h` | Declaraciones de la interfaz del reproductor. |
| `include/playlist.h` | Declaraciones de la interfaz de la playlist. |
| `include/wav.h` | Declaraciones para el manejo de archivos WAV. |
| `tests/test_concurrency.c` | Pruebas automatizadas de concurrencia y estructuras compartidas. |
| `Makefile` | Reglas para compilar, ejecutar y probar el proyecto. |
| `scripts_generate_test_wav.sh` | Script auxiliar para generar archivos WAV de prueba. |

## 5. Arquitectura del sistema

El reproductor utiliza una arquitectura basada en productor-consumidor y una capa de control encargada de coordinar las operaciones de reproducción.

```text
                       +-------------------+
                       |      Usuario      |
                       |    Interfaz CLI   |
                       +---------+---------+
                                 |
                                 v
                       +-------------------+
                       |      main.c       |
                       | Comandos y estado |
                       +---------+---------+
                                 |
                   +-------------+-------------+
                   |                           |
                   v                           v
          +----------------+          +----------------+
          |   playlist.c   |          |    player.c    |
          | Gestión de     |          | Control de     |
          | reproducción   |          | reproducción   |
          +----------------+          +--------+-------+
                                               |
                                  +------------+------------+
                                  |                         |
                                  v                         v
                           Hilo productor            Hilo consumidor
                                  |                         ^
                                  v                         |
                           +-------------------------------+
                           |        Buffer circular       |
                           | Mutex y variables de         |
                           | condición                    |
                           +-------------------------------+
                                                             |
                                                             v
                                                      Salida de audio
                                                          mediante
                                                           ALSA
```

### 5.1. Hilo productor

El productor lee los datos del archivo WAV seleccionado y los deposita en el buffer circular compartido.

Sus principales responsabilidades son:

1. Leer bloques de audio desde el archivo.
2. Comprobar si hay espacio suficiente en el buffer.
3. Esperar cuando el buffer está lleno.
4. Escribir los datos disponibles en el buffer.
5. Coordinarse con el consumidor mediante los mecanismos de sincronización.
6. Responder a los cambios de estado y de pista contemplados por el reproductor.

### 5.2. Hilo consumidor

El consumidor extrae los datos del buffer y los envía al sistema de audio mediante ALSA.

Sus principales responsabilidades son:

1. Comprobar si hay datos disponibles.
2. Esperar cuando el buffer está vacío.
3. Extraer bloques de audio respetando los límites de la estructura.
4. Entregar los datos al dispositivo de audio.
5. Coordinar el consumo con el productor.
6. Participar en el control del estado de reproducción y el cierre ordenado de los recursos.

### 5.3. Buffer circular

El buffer circular permite almacenar temporalmente los datos producidos y entregarlos al consumidor sin que ambos hilos tengan que operar directamente sobre el archivo y el dispositivo de audio al mismo tiempo.

La estructura utiliza posiciones de lectura y escritura que avanzan circularmente dentro de su capacidad. Cuando una posición alcanza el final del espacio disponible, vuelve al inicio, de acuerdo con la lógica de implementación.

La sincronización protege los datos, los índices y la información necesaria para determinar si hay espacio para escribir o datos para leer.

### 5.4. Lista de reproducción

La playlist almacena las pistas que el usuario agrega y administra desde la interfaz de consola.

El uso de `pthread_rwlock_t` permite coordinar las operaciones concurrentes de lectura y escritura. Las consultas pueden compartir el acceso de lectura, mientras que las modificaciones requieren acceso exclusivo.

Esta protección es importante porque el reproductor puede necesitar consultar la playlist mientras el usuario agrega, elimina o limpia elementos.

## 6. Concurrencia y sincronización

La concurrencia es uno de los componentes centrales del proyecto. Su propósito es permitir que la lectura de audio, el consumo de datos y la gestión de la reproducción se coordinen sin acceder de forma descontrolada a los recursos compartidos.

### 6.1. Mutex: `pthread_mutex_t`

El mutex proporciona exclusión mutua para proteger secciones críticas.

Cuando un hilo necesita acceder a un recurso protegido, adquiere el mutex antes de modificarlo y lo libera al finalizar la operación correspondiente.

En el proyecto, los mutexes se utilizan en las estructuras y operaciones compartidas que requieren acceso sincronizado.

### 6.2. Variables de condición: `pthread_cond_t`

Las variables de condición permiten que un hilo suspenda su ejecución hasta que exista una condición que le permita continuar.

En el patrón productor-consumidor se utilizan para coordinar situaciones como:

- El productor necesita esperar cuando no hay espacio suficiente en el buffer.
- El consumidor necesita esperar cuando no hay datos disponibles.
- Las operaciones sobre el buffer pueden notificar al otro hilo cuando cambia la disponibilidad de espacio o de datos.

La condición debe comprobarse de nuevo después de despertar, ya que una notificación no garantiza por sí sola que el recurso siga disponible.

### 6.3. Bloqueo de lectura y escritura: `pthread_rwlock_t`

La lista de reproducción utiliza un bloqueo de lectura y escritura para proteger las operaciones concurrentes.

Este mecanismo diferencia entre:

- **Lectura:** consultar los elementos de la playlist.
- **Escritura:** agregar, eliminar o modificar elementos de la playlist.

La protección busca mantener la integridad de la estructura y evitar accesos incompatibles durante modificaciones concurrentes.

### 6.4. Espera bloqueante y uso de CPU

La coordinación mediante variables de condición permite que los hilos esperen cuando no pueden continuar, en lugar de consultar repetidamente el estado de una condición mediante ciclos de espera activa.

Este enfoque favorece un uso más eficiente de los recursos del procesador.

### 6.5. Riesgos que aborda el diseño

| Riesgo | Descripción | Mecanismo de prevención |
|---|---|---|
| Condición de carrera | Accesos concurrentes incompatibles a datos compartidos. | Mutexes y bloqueos adecuados. |
| Buffer lleno | El productor intenta escribir sin capacidad disponible. | Comprobación de capacidad y espera sincronizada. |
| Buffer vacío | El consumidor intenta leer sin datos disponibles. | Comprobación de disponibilidad y espera sincronizada. |
| Interbloqueo | Los hilos quedan bloqueados esperando recursos o condiciones. | Orden coherente de bloqueos y gestión correcta de las condiciones. |
| Inconsistencia de la playlist | Las modificaciones concurrentes afectan las consultas. | `pthread_rwlock_t` y coordinación con el reproductor. |

El comportamiento correcto depende de que todos los accesos relevantes respeten las reglas de sincronización y de que los hilos puedan finalizar correctamente.

## 7. Formato de audio y ALSA

El reproductor está orientado a la lectura de archivos WAV con audio PCM de 16 bits y utiliza ALSA como interfaz de salida de audio.

Los archivos utilizados en las pruebas de desarrollo tienen las siguientes características:

| Propiedad | Configuración de prueba |
|---|---|
| Formato | WAV |
| Codificación | PCM |
| Frecuencia de muestreo | 44.100 Hz |
| Canales | Mono |
| Profundidad | 16 bits |
| Duración de los tonos de prueba | 20 segundos |

Estas características corresponden a los archivos de prueba y no significan que todos los archivos WAV existentes sean compatibles.

### Funcionamiento de la salida de audio

El consumidor obtiene bloques de datos del buffer circular y los envía a ALSA, que se encarga de interactuar con el dispositivo de sonido configurado en el sistema.

En un entorno Linux convencional, ALSA puede utilizar directamente un dispositivo de audio. En WSL2, la salida puede depender de la integración con WSLg y de un servidor de audio disponible.

## 8. Compilación

Para compilar la aplicación, situarse en la raíz del proyecto y ejecutar:

```bash
make
```

El ejecutable principal se genera en:

```text
bin/reproductor
```

Para limpiar los archivos generados y realizar una compilación desde cero:

```bash
make clean
make
```

El `Makefile` también contiene una regla para ejecutar las pruebas automatizadas.

## 9. Ejecución

Iniciar el reproductor desde la raíz del proyecto:

```bash
./bin/reproductor
```

Si se proporciona una ruta de archivo al iniciar, la aplicación puede utilizarla como pista inicial, según el comportamiento implementado:

```bash
./bin/reproductor audio/sonido_1_440hz.wav
```

### Ejemplo de una sesión

```text
=== REPRODUCTOR CONCURRENTE ===

> add audio/sonido_1_440hz.wav
> add audio/sonido_2_660hz.wav
> add audio/sonido_3_880hz.wav
> list
> play
> status
> pause
> play
> next
> prev
> stop
> quit
```

Los mensajes exactos que muestra la aplicación pueden variar según el estado del reproductor.

## 10. Comandos disponibles

| Comando | Descripción |
|---|---|
| `add <archivo.wav>` | Agrega un archivo WAV a la playlist. |
| `list` | Muestra los elementos de la playlist. |
| `play` | Inicia o reanuda la reproducción. |
| `pause` | Solicita pausar la reproducción. |
| `stop` | Detiene la reproducción. |
| `next` | Avanza a la siguiente pista. |
| `prev` | Regresa a la pista anterior. |
| `remove <indice>` | Elimina un elemento de la playlist. |
| `clear` | Limpia la lista de reproducción. |
| `status` | Muestra información del estado del reproductor. |
| `help` | Muestra la ayuda disponible. |
| `quit` | Finaliza la aplicación. |

El índice utilizado por `remove` debe corresponder a la numeración que muestra el comando `list`.

### Ejemplo de administración de pistas

```text
> add audio/sonido_1_440hz.wav
> add audio/sonido_2_660hz.wav
> list
> remove 1
> list
> clear
> list
```

### Ejemplo de controles

```text
> play
> status
> pause
> status
> play
> next
> prev
> stop
> status
```

Estos ejemplos sirven como guía para la demostración manual de las operaciones disponibles.

## 11. Pruebas automatizadas

El proyecto incluye pruebas automatizadas que se ejecutan mediante el `Makefile`.

Para ejecutar las pruebas:

```bash
make test
```

La regla compila y ejecuta:

```text
bin/test_concurrency
```

### Pruebas de concurrencia

El archivo `tests/test_concurrency.c` contiene pruebas relacionadas con el comportamiento concurrente de las estructuras compartidas.

Las principales áreas cubiertas son:

1. **Buffer circular concurrente:** validación de las operaciones de lectura y escritura realizadas por varios hilos.
2. **Lista de reproducción:** validación de las operaciones y del acceso concurrente a la estructura.

Una ejecución exitosa indica que los casos implementados finalizaron satisfactoriamente. La cobertura de la prueba depende de los escenarios incluidos en el código.

### Secuencia de validación

Para reconstruir el proyecto, ejecutar las pruebas y luego iniciar la aplicación:

```bash
make clean
make
make test
./bin/reproductor
```

Después de las pruebas automatizadas, se recomienda comprobar manualmente la reproducción de audio y los comandos de control.

## 12. Generar un archivo WAV de prueba

Si no se dispone de un archivo WAV compatible, se puede generar un tono de 440 Hz utilizando Python 3.

Desde la raíz del proyecto, ejecutar:

```bash
mkdir -p audio

python3 - <<'PY'
import math
import os
import struct
import wave

rate = 44100
seconds = 3
frequency = 440
amplitude = 16000

os.makedirs("audio", exist_ok=True)

with wave.open("audio/tono_440hz.wav", "wb") as wav:
    wav.setnchannels(1)
    wav.setsampwidth(2)
    wav.setframerate(rate)

    frames = bytearray()

    for i in range(rate * seconds):
        sample = int(
            amplitude * math.sin(
                2 * math.pi * frequency * i / rate
            )
        )
        frames.extend(struct.pack("<h", sample))

    wav.writeframes(frames)

print("Archivo generado: audio/tono_440hz.wav")
PY
```

Este procedimiento genera un archivo WAV PCM de 16 bits, mono, a 44.100 Hz y con una duración de tres segundos.

Para reproducirlo, iniciar la aplicación:

```bash
./bin/reproductor
```

Después, ejecutar:

```text
> add audio/tono_440hz.wav
> play
```

## 13. Solución de problemas

### 13.1. No se encuentra `make`

Instalar las herramientas de compilación:

```bash
sudo apt update
sudo apt install build-essential
```

### 13.2. Error relacionado con ALSA durante la compilación

Instalar la biblioteca de desarrollo:

```bash
sudo apt install libasound2-dev
```

Después, compilar nuevamente:

```bash
make clean
make
```

### 13.3. ALSA no encuentra un dispositivo de audio

Comprobar los dispositivos disponibles:

```bash
aplay -L
```

En WSL2 con WSLg, revisar si existe el socket de audio:

```bash
ls -l /mnt/wslg/PulseServer
```

Consultar la configuración de PulseAudio, si está disponible:

```bash
echo "$PULSE_SERVER"
pactl info
```

El comando `pactl` requiere que estén instaladas las herramientas correspondientes.

En el entorno WSL2 utilizado durante el desarrollo, la reproducción se probó mediante la integración de audio de WSLg. La configuración concreta puede variar según la instalación.

Antes de modificar `~/.asoundrc`, revisar su contenido y conservar una copia de seguridad si se necesita realizar cambios.

### 13.4. No existe el ejecutable

Compilar el proyecto:

```bash
make
```

Comprobar los archivos generados:

```bash
ls -l bin/
```

### 13.5. Las pruebas no se ejecutan

Ejecutar:

```bash
make clean
make test
```

Si aparece un error de compilación, revisar el mensaje mostrado por GCC.

### 13.6. El reproductor no emite sonido

Comprobar que el archivo exista:

```bash
ls -l audio/
```

Probar el archivo directamente mediante ALSA:

```bash
aplay -D default audio/tono_440hz.wav
```

Si esta prueba falla, comprobar primero la configuración de audio del entorno antes de investigar la lógica del reproductor.

## 14. Evidencias para la sustentación

Para la demostración del parcial, se recomienda mostrar los siguientes elementos:

1. **Compilación:** ejecutar `make` y mostrar que termina correctamente.
2. **Pruebas automatizadas:** ejecutar `make test`.
3. **Inicio del programa:** ejecutar `./bin/reproductor`.
4. **Gestión de la playlist:** demostrar `add`, `list`, `remove` y `clear`.
5. **Control de reproducción:** demostrar `play`, `pause` y `stop`.
6. **Cambio de pistas:** demostrar `next` y `prev`.
7. **Consulta de estado:** utilizar `status`.
8. **Salida de audio:** reproducir uno o varios archivos WAV.
9. **Creación de hilos:** mostrar el uso de `pthread_create`.
10. **Sincronización:** explicar el uso de `pthread_mutex_t` y `pthread_cond_t`.
11. **Protección de la playlist:** mostrar las operaciones que utilizan `pthread_rwlock_t`.
12. **Productor-consumidor:** explicar cómo los datos pasan del archivo WAV al buffer y del buffer a ALSA.

Para demostrar la existencia de varios hilos en ejecución, se puede utilizar un depurador o una herramienta de inspección de procesos e hilos. La presencia de `pthread_create` en el código, por sí sola, no demuestra que ambos hilos estén ejecutándose simultáneamente en un momento específico.

## 15. Relación con los criterios de evaluación

| Criterio | Elementos que se deben demostrar |
|---|---|
| Gestión concurrente de playlist | Operaciones de lectura y modificación protegidas mediante `pthread_rwlock_t`. |
| Productor-consumidor | Hilos productor y consumidor, y buffer circular compartido. |
| Sincronización | Uso de mutexes y variables de condición para coordinar los recursos compartidos. |
| Controles de reproducción | Funcionamiento de `play`, `pause`, `stop`, `next` y `prev`. |
| Robustez | Resultados de pruebas, manejo de errores y revisión de condiciones de carrera, interbloqueos y recursos. |
| Modularidad | Separación de responsabilidades entre los módulos de la aplicación. |

La evaluación de cada criterio debe apoyarse en el comportamiento observado y en el código correspondiente.

## 16. Alcance y limitaciones

El proyecto está orientado a la reproducción de archivos WAV compatibles con el lector implementado.

La reproducción depende de la validez del archivo, del formato admitido y de la disponibilidad de un dispositivo de audio funcional.

Las pruebas automatizadas se concentran en el buffer circular y la lista de reproducción. Para ampliar la cobertura, se pueden incorporar escenarios específicos para los controles de reproducción, cambios de pista durante la ejecución, eliminación de elementos en uso, errores de lectura y cierre de los hilos.

No se debe asumir compatibilidad con formatos como MP3 o AAC si no se han implementado sus mecanismos de decodificación.

## 17. Limpieza y reconstrucción

Para eliminar los ejecutables y archivos objeto generados:

```bash
make clean
```

Para reconstruir el proyecto y ejecutar las pruebas:

```bash
make
make test
```

## 18. Conclusión

El Reproductor Concurrente integra conceptos fundamentales de sistemas operativos mediante una aplicación práctica desarrollada en C/POSIX.

La arquitectura utiliza hilos POSIX, un buffer circular productor-consumidor, primitivas de sincronización y una lista de reproducción con acceso concurrente. La separación modular facilita comprender y mantener la implementación.

La aplicación permite estudiar cómo coordinar la lectura y el consumo de datos, proteger estructuras compartidas y controlar una operación de reproducción mediante una interfaz de consola.

Las pruebas automatizadas y la demostración manual complementan el análisis del código y permiten verificar el comportamiento del proyecto en el entorno de ejecución utilizado.
