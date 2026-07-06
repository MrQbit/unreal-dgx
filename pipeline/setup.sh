#!/bin/bash
# One-shot installer for GameForge's LOCAL AI asset generators on the DGX Spark (GB10, aarch64+CUDA).
# Run once on the DGX. Big first-run: torch (CUDA), diffusers, audio models, plus model downloads.
# Everything runs locally on the Blackwell GPU — no external APIs.
set -u
FORGE="${FORGE_HOME:-$HOME/gameforge}"
mkdir -p "$FORGE/assets" "$FORGE/models"
cd "$FORGE"

echo "== 1. Blender (3D, headless) =="
command -v blender >/dev/null || sudo apt-get install -y blender python3-numpy
blender --version | head -1

echo "== 2. Python venv for the AI generators (uv) =="
command -v uv >/dev/null || curl -LsSf https://astral.sh/uv/install.sh | sh
uv venv "$FORGE/.venv" --python 3.11
VENV="$FORGE/.venv"

echo "== 3. PyTorch (CUDA, aarch64 / Blackwell) =="
# GB10 is Blackwell (sm_12x) — needs a recent torch CUDA build. NVIDIA publishes aarch64 CUDA wheels;
# the cu128 index carries Blackwell-capable builds. Adjust the index if a newer one is required.
VIRTUAL_ENV="$VENV" uv pip install --index-url https://download.pytorch.org/whl/cu128 torch torchvision torchaudio \
  || VIRTUAL_ENV="$VENV" uv pip install torch torchvision torchaudio
"$VENV/bin/python" -c "import torch;print('torch',torch.__version__,'cuda',torch.cuda.is_available())"

echo "== 4. Image generation (diffusers: SDXL / FLUX) =="
VIRTUAL_ENV="$VENV" uv pip install diffusers transformers accelerate safetensors sentencepiece pillow

echo "== 5. Audio generation (music/SFX) + voice (Piper TTS) =="
VIRTUAL_ENV="$VENV" uv pip install audiocraft soundfile      # MusicGen for music/SFX
VIRTUAL_ENV="$VENV" uv pip install piper-tts                 # lightweight local TTS

echo "== 6. Local LLM (design/dialogue) — ollama =="
command -v ollama >/dev/null || (curl -fsSL https://ollama.com/install.sh | sh)
ollama list | grep -q nemotron || echo "  (pull a model, e.g. 'ollama pull llama3.1:8b' or use the installed nemotron)"

echo
echo "Done. Generators live in $FORGE/.venv. First image/audio run downloads model weights (several GB)."
echo "Smoke test:  $VENV/bin/python $(dirname "$0")/gen/image_gen.py 'a mossy stone brick texture, seamless' $FORGE/assets/stone.png"
