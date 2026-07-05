# Porting UE 5.4 to native aarch64 — method & fixes

This is the ordered account of what breaks when you try to build the UE 5.4 editor natively on
aarch64 Linux, and how each was fixed. All patches are in
`patches/unreal-engine-5.4-aarch64.patch`.

## The core problem

Epic supports the Linux **editor** on x86_64 only. The bundled toolchain (clang, .NET, dotnet host,
and every prebuilt third‑party binary) is x86_64. There is no aarch64 Autodesk FBX SDK, no aarch64
ISPC, no aarch64 Zen/Chromium, etc. So a native ARM64 build requires: (1) replacing the bundled
toolchain with system tools, (2) teaching UnrealBuildTool to treat aarch64 as a first‑class host,
and (3) making every third‑party library available for aarch64.

## Host toolchain (system clang-18 + arm64 .NET)

- **System toolchain**: `clang-18`, `libc++-18`, `lld-18`, `ninja`. Created unversioned symlinks in
  `/usr/local/bin` (`clang`, `clang++`, `ld.lld`, `llvm-ar`, …) because UBT's `WhichClang` runs
  `which clang++`.
- **.NET (arm64)**: installed .NET 8 SDK + .NET 6/8 runtimes to `~/.dotnet` (UBT targets net6.0).
- **`GitDependencies`** (fetches ~16 GB of deps) ships only as a linux‑x64 binary. Rebuilt it
  natively for arm64 (retarget csproj net6→net8, self‑contained publish) and fixed a real bug:
  `Program.cs` used a legacy `__xstat64` P/Invoke with the x86_64 `struct stat`/`_STAT_VER` layout →
  EINVAL on aarch64. Replaced with managed `File.GetUnixFileMode`/`SetUnixFileMode`.
- **`SetupDotnet.sh`**: forces the system arm64 dotnet on aarch64 (the bundled linux dotnet is x64).
- **`Unreal.cs` (`DotnetPath`)**: recursive UBT actions (WriteMetadata / build receipts) spawned the
  x86_64 bundled dotnet. On aarch64 now uses the system dotnet already hosting UBT
  (`Environment.ProcessPath`). Without this, builds fail with `WriteMetadata … canceled`.

## UnrealBuildTool — make aarch64 a valid host

(`Engine/Source/Programs/UnrealBuildTool/Platform/Linux/*`)

- **`UEBuildLinux.cs`**: `DefaultHostArchitecture` is detected from `RuntimeInformation.OSArchitecture`
  (was hardcoded `X64`); the `Linux` platform arch config uses the host arch; fixed a copy‑paste bug
  where `LinuxName` checked the Apple arch dictionary. Also `Target.bCompileISPC = false` on arm64
  (bundled ISPC is x86 — modules use their C++ fallbacks). **Keep the Arm64 triple as
  `aarch64-unknown-linux-gnueabi`** — Epic's prebuilt arm64 third‑party libs live under that dir name.
- **`LinuxPlatformSDK.cs`**: `ForceUseSystemCompiler()` defaults true on aarch64 → the SDK is "valid"
  when a system clang exists.
- **`LinuxCommon.cs`**: fixed `Which()` — it built `StartInfo.Arguments = "-c 'which name'"`, but
  .NET's Linux arg parser doesn't honor single quotes, so the shell got mangled argv and the lookup
  always failed. Now uses `ArgumentList` + `command -v`. **This is what made "Platform Linux is not a
  valid platform to build" persist even with system clang installed.**
- **`LinuxToolChain.cs`**: raised the clang version gate to allow clang 18; use the **system libc++**
  (`-stdlib=libc++`) instead of UE's stale bundled libc++ (too old for clang‑18); added the
  `-stdlib=libc++` at the global compile level so the shared PCH and every TU use the same std lib
  (mixing causes `redefinition of byte`); silence the unused‑arg warning on C files; skip the x86‑only
  `dump_syms`/`BreakpadEncoder` on arm64; `-Wno-vla-cxx-extension` for clang‑18.
- **`UEBuildLinux.cs` SetUpEnvironment**: don't inject `-isystem /usr/include` on aarch64 — it shadows
  libc++'s own `math.h`/`stdint.h` wrappers (`<cmath> didn't find libc++'s <math.h>`).

## Engine C++ (arm64 correctness)

- **`UnixPlatform.h`**: enable NEON for any `__aarch64__` target (was gated on `PLATFORM_LINUXARM64`),
  so the "Linux" platform uses `UnrealMathNeon.h`, not the broken scalar FPU fallback.
- **`UnixPlatformRunnableThread.h`**: `SIGSTKSZ` is no longer a compile‑time constant in glibc ≥ 2.34;
  replaced with a fixed floor so it can initialize an enum.
- **Embree** (x86‑only): `USE_EMBREE=0` on arm64. Fixed inconsistent fallbacks —
  `MeshDistanceFieldUtilities.cpp` (`FBoxSphereBounds` → `FBoxSphereBounds3f` to match the decl) and
  `NaniteRayTracingEncode.cpp` (wrapped the whole Embree impl in `#if USE_EMBREE` + a no‑op stub).

## Runtime (make the editor boot)

- **`PluginManager.cpp`**: a plugin module binary that was never built (optional plugins excluded from
  the build) is now a **warning that skips the plugin**, not a fatal exit.
- **`BaseEngine.ini` DDC**: UE 5.4's default local DDC is **Zen**, whose server is an x86 binary that
  can't auto‑launch on arm64 → the DDC had "no readable or writable nodes" (fatal). Dropped
  ZenLocal/ZenShared from the graph and made the filesystem `Local` node read/write.
- Several plugins whose modules aren't built were disabled via `EnabledByDefault=false`
  (FastBuildController, PlatformCrypto, WmfMedia, …) or via `DisablePlugins` in the editor target
  (Perforce/Changelist/KDevelop — x86 P4 API).

## The editor target (`UnrealEditor.Target.cs`)

Gated on `UE_ARM64_MINIMAL_EDITOR=1`: `bCompileCEF3=false` (Chromium is x86‑only), disable the
proprietary‑dep plugins, and **`bBuildAllModules=true` with USD disabled on arm64** (see
`UnrealUSDWrapper.Build.cs` `EnableUsdSdk`) so essential modules build without the un‑portable USD SDK.

## Third‑party libraries

The bulk of the work. See [`THIRD_PARTY_LIBS.md`](THIRD_PARTY_LIBS.md) — ~25 libraries, each either
rebuilt from source for aarch64 (`-fPIC`, libc++), fixed in its `.Build.cs` to select the arm64 lib
Epic already ships, or stubbed (FBX, Bink encode, ISPC texcomp) where no arm64 build is possible.
