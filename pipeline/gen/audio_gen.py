#!/usr/bin/env python3
"""Local audio generation on the GB10. Music/SFX via MusicGen (transformers, uses the verified torch
CUDA stack — no AudioCraft version pins); voice via Piper TTS. Outputs 16-bit WAV for UE import.
Usage:
  audio_gen.py music "<description>" <out.wav> [--secs 8]
  audio_gen.py voice "<line of dialogue>" <out.wav>"""
import argparse, os, subprocess, sys


def music(desc, out, secs):
    import torch, numpy as np, soundfile as sf
    from transformers import AutoProcessor, MusicgenForConditionalGeneration
    dev = "cuda" if torch.cuda.is_available() else "cpu"
    proc = AutoProcessor.from_pretrained("facebook/musicgen-small")
    model = MusicgenForConditionalGeneration.from_pretrained("facebook/musicgen-small").to(dev)
    inp = proc(text=[desc], padding=True, return_tensors="pt").to(dev)
    tokens = max(64, int(secs * 50))                       # MusicGen ~50 tokens/sec
    with torch.no_grad():
        audio = model.generate(**inp, max_new_tokens=tokens, do_sample=True, guidance_scale=3.0)
    sr = model.config.audio_encoder.sampling_rate
    wav = audio[0, 0].cpu().numpy().astype(np.float32)
    sf.write(out, wav, sr, subtype="PCM_16")
    print("AUDIO_OK", out)


def voice(line, out):
    # Piper: needs a voice .onnx (auto-downloaded to ~/gameforge/models on first use). Light + fast on arm64.
    forge = os.environ.get("FORGE_HOME", os.path.expanduser("~/gameforge"))
    model = os.environ.get("PIPER_VOICE", os.path.join(forge, "models", "en_US-amy-medium.onnx"))
    if not os.path.exists(model):
        base = "https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium"
        os.makedirs(os.path.dirname(model), exist_ok=True)
        for suf in ("", ".json"):
            subprocess.run(["curl", "-sL", "-o", model + suf, f"{base}/en_US-amy-medium.onnx{suf}"], check=True)
    # piper 1.4.x CLI: python -m piper -m <onnx> -f <out.wav>, text on stdin
    subprocess.run([sys.executable, "-m", "piper", "-m", model, "-f", out],
                   input=line.encode(), check=True)
    print("AUDIO_OK", out)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("mode", choices=["music", "voice"]); ap.add_argument("text"); ap.add_argument("out")
    ap.add_argument("--secs", type=int, default=8)
    a = ap.parse_args()
    (music(a.text, a.out, a.secs) if a.mode == "music" else voice(a.text, a.out))
