# PongGame — code-driven Pong (native aarch64 test game)

A minimal, fully **C++ / code-driven** Pong for the DGX Spark — no hand-authored binary assets. The
game mode spawns everything at `BeginPlay` (paddles, ball, walls, top-down camera), so it builds,
cooks, and packages headlessly. Proves the whole pipeline: native aarch64 game C++ → cook → package.

## Controls
- **W / S** or **Up / Down** — move the left paddle. The right paddle is a simple AI.
- First-to-score logging in the output; the ball resets toward the loser.

## Structure
```
PongGame.uproject
Config/{DefaultEngine,DefaultInput,DefaultGame}.ini   # default map + game mode + input axes
Source/PongGame.Target.cs  PongGameEditor.Target.cs
Source/PongGame/
  PongField.h        # field constants
  PongPaddle.{h,cpp} # APawn — player input + AI tracking, clamped movement
  PongBall.{h,cpp}   # AActor — velocity, wall/paddle bounce, scoring
  PongGameMode.{h,cpp} # spawns camera/paddles/ball/walls, possesses the player paddle, scoring
```
No `.umap` of its own — it uses an engine default map and `GlobalDefaultGameMode`, so the whole game
is code. (`DefaultEngine.ini`: `GameDefaultMap=/Engine/Maps/Templates/Template_Default`,
`GlobalDefaultGameMode=/Script/PongGame.PongGameMode`.)

## Build + run (on the DGX)
```bash
cp -r /path/to/unreal-dgx/game/PongGame  $HOME/PongGame
cd $HOME/UnrealEngine
export DOTNET_ROOT=$HOME/.dotnet PATH=/usr/local/bin:$HOME/.dotnet:$PATH UE_ARM64_MINIMAL_EDITOR=1

# compile the game (native aarch64)
./Engine/Build/BatchFiles/Linux/Build.sh PongGame Linux Development \
   -Project=$HOME/PongGame/PongGame.uproject -ForceUseSystemCompiler

# run it (needs a display; -RenderOffScreen to run headless)
./Engine/Binaries/Linux/UnrealEditor $HOME/PongGame/PongGame.uproject -game

# or package a standalone build (see ../../docs/PACKAGING.md)
/path/to/unreal-dgx/scripts/package_game.sh $HOME/PongGame/PongGame.uproject Linux
```
