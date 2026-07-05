# Status — In Progress

_Snapshot of milestones, the current blocker, and remaining work._

## Milestones reached

1. Epic GitHub access; UE 5.4 source cloned (`847de5e25`)
2. arm64 .NET SDK/runtimes; native `GitDependencies` (+ fixed its x86-only `__xstat64` bug); ~16 GB deps fetched
3. `SetupDotnet.sh` / `Unreal.cs` use the system arm64 dotnet (bundled is x64)
4. **UnrealBuildTool builds & runs natively**; `GenerateProjectFiles` completes
5. **Linux is a valid build platform on aarch64** (fixed UBT's `which clang++` arg-quoting bug + system-compiler path)
6. **Entire engine C++ compiles** natively with clang-18 + system libc++ (0 compile errors)
7. **ShaderCompileWorker built & runs natively** — first complete aarch64 UE target, incl. DXC/ShaderConductor
8. **~25 third-party libraries ported** (rebuilt / arch-fixed / stubbed) — see `THIRD_PARTY_LIBS.md`
9. **UnrealEditor links** — ELF aarch64, 415+ modules, ~7.6 GB, receipt `Arch: arm64`
10. **Editor boots natively** — loads engine, mounts plugins, compiles Vulkan shaders, starts Asset Registry
11. **Vulkan confirmed on the GB10 Blackwell GPU** — Vulkan 1.4, 16 graphics queues; UE detects the NVIDIA aarch64 driver + loads Vulkan shader formats

## ✅ SOLVED: `RunningPlatform` assert — root cause was a DDPI host-key gap

**The editor now boots past the assert, registers the Linux target platform, and compiles Blueprints.**

Root cause: `Engine/Config/Linux/DataDrivenPlatformInfo.ini` gates `bIsEnabled` by **host** platform
(`Windows:bIsEnabled=true`, `Linux:bIsEnabled=true`, `Mac:bIsEnabled=false`). On arm64,
`FLinuxPlatformProperties::IniPlatformName()` returns **`"LinuxArm64"`** (not `"Linux"`) —
`IS_ARM64 ? "LinuxArm64" : "Linux"` — so none of those host keys matched and the Linux target
platform was never enabled → `GetRunningTargetPlatform()` returned null. **Fix:** add
`LinuxArm64:bIsEnabled=true` + `LinuxArm64:bUsesHostCompiler=true` to the Linux DDPI. Now
`Loaded TargetPlatform 'Linux' / 'LinuxEditor' / 'LinuxServer' / 'LinuxClient'` and the assert is gone.

This required the full-module build (`bBuildAllModules=true`) so `LinuxTargetPlatform` is actually
built + manifested, achieved by **stubbing the x86-only plugin libs** (see below) rather than
disabling (DisablePlugins is soft). Path to the full link:
- **12 x86 libs stubbed in-place** with aarch64 no-op exports (`scripts/stub_x86_lib.sh`): OpenCV,
  EOSSDK, steam_api (×2 paths), OpenXR loader, IREE (libireert/libflatcc), protobuf-lite, libproj,
  onnx/onnx_proto, coremod, libvpx, mpcdi — symbols extracted from each x86 lib, `@@VERSION` tags
  stripped, linker-reserved symbols excluded.
- **23-plugin closure disabled** (`UnrealEditor.Target.cs`) — the module-level dependent closure of
  the plugins that can't build on arm64 (WebRTC/PixelStreaming, msquic/QuicMessaging, BlackmagicMedia
  SSE2, MovieRenderPipeline, nDisplay/mpcdi ABI, NNE-ORT/IREE) — computed so nothing hard-references
  a disabled plugin.
- **XMPP `__res_query` shim**: glibc ≥2.34 dropped the default-version `__res_query`; added a
  one-function forwarder (`res_compat.o`) into the arm64 `libstrophe.a`.

### New blocker (much later): package-load crash

Signal 11 in `FLinkerLoad::CreateLinker` / `GetPackageLinker` (CoreUObject) while
`CompileAllBlueprints` loads `/Engine/Tutorial/.../TutorialCharacter`. Note `CompileAllBlueprints` is
a torture test over *every* asset; the editor GUI loads a different set, so this may not block a GUI
boot. Under investigation.

<details><summary>Historical: the original RunningPlatform investigation</summary>

Runtime assert `check(RunningPlatform)` at `StaticMesh.cpp:5554` — `GetRunningTargetPlatform()`
returns null: **no Linux target platform registers**. Findings:

- `TargetPlatformManagerModule::DiscoverAvailablePlatforms` loops `GetAllPlatformInfos()` and loads a
  platform's TP module if `Info.bEnabledForUse`. Linux's DDPI has `bIsEnabled=true`, so it should try.
- `HasCompiledSupportForPlatform("Linux")` needs `ModuleExists("LinuxTargetPlatform")`. At runtime
  only **Android** target platforms load (20 variants) — Linux/Windows/Mac/iOS TP modules do not register.
- Attempts that did NOT resolve it: (1) `ExtraModuleNames += LinuxTargetPlatform` (builds the .so but
  not into the main manifest); (2) forced clean rebuild; (3) cleared `BinariesSubFolder="Linux"` so the
  .so lands in the main `Binaries/Linux/`; (4) hand-adding the module to the main `UnrealEditor.modules`
  (BuildIds all match) — the module still never loads.
- Conclusion: the minimal (`bBuildAllModules=false`) build leaves the target-platform module set
  incomplete in a way `ExtraModuleNames`/manifest edits can't repair — TP discovery has
  registration/ordering dependencies satisfied only by a full editor build.

### Two candidate real fixes (next session)

**Key finding this session:** `bBuildAllModules=true` (the normal editor path that registers all TP
modules) does **not** converge via `DisablePlugins`, because **`DisablePlugins` is a *soft* disable —
it loses to hard plugin dependencies.** Optional plugins with x86-only libs (OpenCV, EOS, Steam,
OpenXR, WebRTC/PixelStreaming, WebM/VPx, Bink, NNE/ONNX, msquic, protobuf, …) are hard-depended-on by
other enabled plugins (e.g. OpenCV←CameraCalibration/MediaFrameworkUtilities/MixedRealityCaptureFramework/
nDisplay; Steam←SteamSockets/SteamController; WebRTC←PixelStreamingPlayer), so they stay enabled and
fail to link. Disabling the transitive closure was attempted and did **not** converge (the closure
keeps growing; some are hard-referenced by non-disableable modules).

So the two real paths are:

1. **Stub the ~10 x86-only third-party libs** (OpenCV, EOS SDK, steam_api, openxr_loader, WebRTC,
   libvpx, Bink, ONNX/IREE, msquic, protobuf) the same way FBX/Bink-encode/ISPC were stubbed
   (asm no-op exports generated from the linker's undefined-symbol list), so `bBuildAllModules=true`
   *builds* everything and registers all TP modules. Also XMPP needs `-lresolv`, and a couple of
   plugins use x86 MMX/SSE intrinsics (`__builtin_ia32_*`) that need NEON/scalar fallbacks or guarding.
2. **Fix the target-platform discovery under the minimal build** — trace why
   `ModuleExists("LinuxTargetPlatform")` is false at DDPI-parse time (`bHasCompiledTargetSupport`)
   despite the module being built, placed in the main `Binaries/Linux/`, and hand-added to the main
   `UnrealEditor.modules` (BuildIds match). At runtime only Android TP modules register. This is the
   lower-effort path if the module-registration timing can be understood.

The committed `UnrealEditor.Target.cs` is left in the minimal (`bBuildAllModules=false`) state — the
furthest point where the editor links and boots.

</details>

## Remaining work

- Clear the `RunningPlatform` assert; boot the editor all the way to idle
- Verify UE VulkanRHI device creation + a rendered viewport on the GB10 (raw Vulkan already confirmed)
- **Unreal MCP + Claude Code** — install an MCP plugin, connect Claude Code, drive the editor
- Build a sample game end-to-end via Claude Code

## Known functional limitations (by design, from stubs/disables)

FBX import/export, Bink audio encoding, ISPC-accelerated texture compression + physics SIMD (C++
fallbacks used), the CEF web browser, USD import, Perforce/Plastic source control, and Zen DDC are
unavailable on this arm64 build. None block the editor from running or building a game.
