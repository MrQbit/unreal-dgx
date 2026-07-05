# Building the aarch64 editor

Reproduction steps on an aarch64 Ubuntu 24.04 host (e.g. DGX Spark / GB10).

## 1. Toolchain

```bash
sudo apt-get install -y clang-18 clang-tools-18 libc++-18-dev libc++abi-18-dev lld-18 \
    build-essential cmake ninja-build
# unversioned symlinks UBT expects (which clang++)
for t in clang clang++ ld.lld lld llvm-ar llvm-ranlib llvm-nm llvm-objcopy llvm-strip; do
  sudo ln -sf /usr/bin/$t-18 /usr/local/bin/$t
done
# arm64 .NET (SDK 8 + runtimes 6 & 8)
curl -fsSL https://dot.net/v1/dotnet-install.sh -o dotnet-install.sh
bash dotnet-install.sh --channel 8.0 --arch arm64 --install-dir ~/.dotnet
bash dotnet-install.sh --channel 6.0 --runtime dotnet     --arch arm64 --install-dir ~/.dotnet
bash dotnet-install.sh --channel 6.0 --runtime aspnetcore --arch arm64 --install-dir ~/.dotnet
# runtime libs used via the system (arm64)
sudo apt-get install -y libfontconfig-dev libfreeimage-dev libsndfile1 vulkan-tools
```

## 2. Source + patches

```bash
git clone -b 5.4 https://github.com/EpicGames/UnrealEngine.git   # needs Epic GitHub access
cd UnrealEngine
git checkout 847de5e25                                            # see patches/BASE_COMMIT.txt
git apply /path/to/unreal-dgx/patches/unreal-engine-5.4-aarch64.patch
```

## 3. Rebuild the native GitDependencies, then fetch deps

```bash
export DOTNET_ROOT=$HOME/.dotnet PATH=$HOME/.dotnet:$PATH
dotnet publish Engine/Source/Programs/GitDependencies/GitDependencies.csproj \
  -c Release -r linux-arm64 --self-contained true -p:PublishTrimmed=false \
  -o Engine/Binaries/DotNET/GitDependencies/linux-arm64
bash Engine/Build/BatchFiles/Linux/GitDependencies.sh --force     # ~16 GB
```

## 4. Third‑party libraries

Rebuild the arm64 third‑party libs (see `docs/THIRD_PARTY_LIBS.md`). At minimum: DXC/ShaderConductor,
OpenEXR+Imath, TBB (with libc++), Vorbis, libpng, etc2comp, astcenc, SPIRV‑Reflect, hlslcc,
nvTextureTools, VHACD, SpeedTree, METIS, Draco, Kiss_FFT; generate the FBX/Bink stubs. The `.Build.cs`
patches already point each module at the expected `aarch64-unknown-linux-gnueabi` output path.

## 5. Build the editor

```bash
export DOTNET_ROOT=$HOME/.dotnet \
       PATH=/usr/local/bin:$HOME/.dotnet:/usr/bin:/bin:/usr/sbin:/sbin \
       UE_ARM64_MINIMAL_EDITOR=1
./Engine/Build/BatchFiles/Linux/Build.sh UnrealEditor Linux Development -ForceUseSystemCompiler
# → Engine/Binaries/Linux/UnrealEditor (ELF aarch64)
```

`UE_ARM64_MINIMAL_EDITOR=1` activates the arm64 target tweaks (bBuildAllModules, CEF/USD off, plugin
disables) in `UnrealEditor.Target.cs`. `-ForceUseSystemCompiler` tells UBT to use system clang.

## 6. Run (headless smoke test)

```bash
export DOTNET_ROOT=$HOME/.dotnet
./Engine/Binaries/Linux/UnrealEditor-Cmd -run=CompileAllBlueprints -unattended -nopause -nosplash
```

Verify Vulkan first if unsure: a 30‑line `vkEnumeratePhysicalDevices` test links
`/usr/lib/aarch64-linux-gnu/libvulkan.so.1` and should print `NVIDIA GB10 | Vulkan 1.4`.

## Notes / gotchas

- Build logs: `Engine/Programs/UnrealBuildTool/Log.txt` is authoritative; the console log truncates
  the linker's undefined‑symbol lists.
- Use a **persistent** log path (not a session scratch dir) for long background builds.
- After any `.Build.cs`/`.Target.cs` change UBT recompiles the rules automatically; after a UBT `.cs`
  change, `dotnet build Engine/Source/Programs/UnrealBuildTool/UnrealBuildTool.csproj -c Development`.
