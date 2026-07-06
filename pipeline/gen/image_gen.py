#!/usr/bin/env python3
"""Local text->image on the GB10 (diffusers). Textures, sprites, UI, concept art.
Usage: image_gen.py "<prompt>" <out.png> [--w 1024 --h 1024 --steps 25 --seed 0 --model sdxl|flux]
Model weights download on first run (cached under HF_HOME). No external API — runs on the Blackwell GPU."""
import argparse, torch
from diffusers import AutoPipelineForText2Image

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("prompt"); ap.add_argument("out")
    ap.add_argument("--w", type=int, default=1024); ap.add_argument("--h", type=int, default=1024)
    ap.add_argument("--steps", type=int, default=25); ap.add_argument("--seed", type=int, default=0)
    ap.add_argument("--model", default="sdxl", choices=["sdxl", "flux"])
    a = ap.parse_args()
    repo = ("stabilityai/stable-diffusion-xl-base-1.0" if a.model == "sdxl"
            else "black-forest-labs/FLUX.1-schnell")
    dtype = torch.bfloat16 if torch.cuda.is_available() else torch.float32
    pipe = AutoPipelineForText2Image.from_pretrained(repo, torch_dtype=dtype)
    pipe = pipe.to("cuda" if torch.cuda.is_available() else "cpu")
    g = torch.Generator(device=pipe.device).manual_seed(a.seed)
    img = pipe(a.prompt, width=a.w, height=a.h, num_inference_steps=a.steps, generator=g).images[0]
    img.save(a.out); print("IMAGE_OK", a.out)

if __name__ == "__main__":
    main()
