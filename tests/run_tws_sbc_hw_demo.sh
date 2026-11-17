#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"; cd "$ROOT"; mkdir -p tmp
python3 - <<'PY'
import wave,math,struct
rate=44100; dur=3.0; notes=[440,494,523,587,659,587,523,494]
with wave.open('tmp/song.wav','wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate); fr=bytearray()
    for i in range(int(rate*dur)):
        n=notes[int(i/rate*2)%len(notes)]
        fr+=struct.pack('<h',int(12000*math.sin(2*math.pi*n*i/rate)))
    w.writeframes(fr)
PY
ffmpeg -y -loglevel error -i tmp/song.wav -ac 1 -c:a sbc -b:a 128k -f sbc tmp/song.sbc
ffmpeg -y -loglevel error -i tmp/song.sbc -f s16le -ac 1 -ar 44100 tmp/song.pcm
gcc -std=c11 -Wall -Wextra -I include tests/tws_sbc_hw_demo.c \
    bt/profiles/tws.c bt/core/buf.c media/codec/sbc_hw.c -o tmp/tws_sbc_hw_demo
./tmp/tws_sbc_hw_demo tmp/song.sbc tmp/song.pcm
md5sum tmp/tws_hw_primary.wav tmp/tws_hw_secondary.wav
cmp -s tmp/tws_hw_primary.wav tmp/tws_hw_secondary.wav && echo "WAV identical: PASS"
