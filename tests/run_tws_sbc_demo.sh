#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT/tmp"
cd "$ROOT"

# 1. fake song: 3 s melody, 44.1 kHz mono
python3 - <<'PY'
import wave, math, struct
rate=44100; dur=3.0
notes=[440,494,523,587,659,587,523,494]
with wave.open('tmp/song.wav','wb') as w:
    w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
    n=int(rate*dur); frames=bytearray()
    for i in range(n):
        note=notes[int(i/rate*2)%len(notes)]
        v=int(12000*math.sin(2*math.pi*note*i/rate))
        frames+=struct.pack('<h',v)
    w.writeframes(frames)
PY

# 2. encode to SBC, then decode back to raw PCM (host codec pipeline)
ffmpeg -y -loglevel error -i tmp/song.wav -ac 1 -c:a sbc -b:a 128k -f sbc tmp/song.sbc
ffmpeg -y -loglevel error -i tmp/song.sbc -f s16le -ac 1 -ar 44100 tmp/song.pcm

# 3. TWS two-ear distribution + sync (src_ai code)
gcc -std=c11 -Wall -Wextra -I include \
    tests/tws_sbc_demo.c bt/profiles/tws.c bt/core/buf.c \
    -o tmp/tws_sbc_demo
./tmp/tws_sbc_demo tmp/song.pcm

# 4. both ears must be bit identical
md5sum tmp/tws_sbc_primary.wav tmp/tws_sbc_secondary.wav
cmp -s tmp/tws_sbc_primary.wav tmp/tws_sbc_secondary.wav && echo "WAV identical: PASS" || { echo "WAV identical: FAIL"; exit 1; }
