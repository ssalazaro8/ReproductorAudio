#!/usr/bin/env bash
set -euo pipefail

mkdir -p audio

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
        sample = int(16000 * math.sin(2 * math.pi * freq * i / rate))
        w.writeframes(struct.pack("<h", sample))

print("Creado: audio/tono_440hz.wav")
PY
