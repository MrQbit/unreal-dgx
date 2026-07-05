# Status — In Progress

_Snapshot of milestones, the current blocker, and remaining work._

## Milestones reached

1. ✅ Epic GitHub access; UE 5.4 source cloned (`847de5e25`)
2. ✅ arm64 .NET SDK/runtimes; native `GitDependencies` (+ fixed its x86‑only `__xstat64` bug); ~16 GB deps fetched
3. ✅ `SetupDotnet.sh` / `Unreal.cs` use the system arm64 dotnet (bundled is x64)
4. ✅ **UnrealBuildTool builds & runs natively**; `GenerateProjectFiles` completes
5. ✅ **Linux is a valid build platform on aarch64** (fixed UBT's `which clang++` arg‑quoting bug + system‑compiler path)
6. ✅ **Entire engine C++ compiles** natively with clang‑18 + system libc++ (0 compile errors)
7. ✅ **ShaderCompileWorker built & runs natively** — first complete aarch64 UE target, incl. DXC/ShaderConductor
8. ✅ **~25 third‑party libraries ported** (rebuilt / arch‑fixed / stubbed) — see `THIRD_PARTY_LIBS.md`
9. ✅ **UnrealEditor links** — ELF aarch64, 415+ modules, ~7.6 GB, receipt `Arch: arm64`
10. ✅ **Editor boots natively** — loads engine, mounts plugins, compiles Vulkan shaders, starts Asset Registry
11. ✅ **Vulkan confirmed on the GB10 Blackwell GPU** — Vulkan 1.4, 16 graphics queues; UE detects the NVIDIA aarch64 driver + loads Vulkan shader formats

## Current blocker

- 🚧 **Runtime assert `RunningPlatform` (`StaticMesh.cpp:5554`)** — `GetRunningTargetPlatform()` returns
  null because the `LinuxTargetPlatform` module wasn't built (the minimal build drops essential
  non‑plugin modules; `ExtraModuleNames` doesn't pull target‑platform modules).
  **Fix in progress:** switched `UnrealEditor.Target.cs` to `bBuildAllModules=true` with USD disabled
  on arm64 (`UnrealUSDWrapper.Build.cs` `EnableUsdSdk`→false) so essential modules build with proper
  manifests. This surfaces a few more x86‑only plugin libs to disable/port (OpenCV, …).

## Remaining work

- ⬜ Finish the full‑module build (disable/port the remaining x86‑only plugin libs: OpenCV, others TBD)
- ⬜ Clear the `RunningPlatform` assert; boot the editor all the way to idle
- ⬜ Verify UE VulkanRHI device creation + a rendered viewport on the GB10 (raw Vulkan already confirmed)
- ⬜ **Unreal MCP + Claude Code** — install an MCP plugin, connect Claude Code, drive the editor
- ⬜ Build a sample game end‑to‑end via Claude Code

## Known functional limitations (by design, from stubs/disables)

- FBX import/export, Bink audio **encoding**, ISPC‑accelerated texture compression + physics SIMD
  (C++ fallbacks used), the CEF web browser, USD import, Perforce/Plastic source control, and Zen DDC
  are unavailable on this arm64 build. None block the editor from running or building a game.
