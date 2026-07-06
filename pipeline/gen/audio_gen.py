#!/usr/bin/env python3
"""Local audio generation on the GB10. Music/SFX via MusicGen; voice via Piper TTS. Outputs WAV for UE.
Usage:
  audio_gen.py music "<description>" <out.wav> [--secs 8]
  audio_gen.py voice "<line of dialogue>" <out.wav>"""
import argparse, sys

def music(desc, out, secs):
    import torchaudio, torch
    from audiocraft.models import MusicGen
    m = MusicGen.get_pretrained("facebook/musicgen-small")
    m.set_generation_params(duration=secs)
    wav = m.generate([desc])[0].cpu()
    torchaudio.save(out, wav, m.sample_rate); print("AUDIO_OK", out)

def voice(line, out):
    import subprocess, os
    # Piper: expects a downloaded voice model (see setup). Falls back to a clear default voice.
    model = os.environ.get("PIPER_VOICE", os.path.expanduser("~/gameforge/models/en_US-amy-medium.onnx"))
    subprocess.run(["piper", "--model", model, "--output_file", out], input=line.encode(), check=True)
    print("AUDIO_OK", out)

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["music", "voice"]); ap.add_argument("text"); ap.add_argument("out")
    ap.add_argument("--secs", type=int, default=8)
    a = ap.parse_args()
    (music(a.text, a.out, a.secs) if a.mode == "music" else voice(a.text, a.out))
