# Third‑party libraries — aarch64 status

How each third‑party dependency was made available for aarch64. Three categories:
**(R)** rebuilt from in‑tree/downloaded source, **(S)** Build.cs selection/path fix for an arm64 lib
Epic already ships, **(X)** stubbed / disabled (no arm64 build possible).

Common build flags for rebuilt libs: `clang-18`/`clang++-18`, `-fPIC`, and **`-stdlib=libc++` for C++**
(the engine links system libc++ — mixing libstdc++ breaks linking). The arm64 triple dir name is
`aarch64-unknown-linux-gnueabi` (Epic's LinuxArm64 convention).

| Library | Cat | What was wrong | Resolution |
|---|---|---|---|
| **DXC + ShaderConductor** | R | x86‑only; source is in‑tree (LLVM‑3.7‑fork DXC + SPIRV‑Cross/Tools) | Native CMake build (host==target, no cross): `-DSC_ARCH_NAME=arm64 -DCMAKE_CXX_FLAGS=-stdlib=libc++`. Patched `Source/CMakeLists.txt` (arm64 branch was falling to `-m32`; excluded `-msse2`). → `Build-arm64-native/Lib/lib{ShaderConductor,dxcompiler}.so` → `Binaries/ThirdParty/ShaderConductor/Linux/<triple>/`. Build.cs selects by arch. |
| **OpenEXR 3.2.1 + Imath 3.1.9** | R | x86‑only; source in‑tree | CMake, `-stdlib=libc++`, static PIC. → `Deploy/.../Unix/<triple>/lib/lib{Imath-3_1,Iex,IlmThread,OpenEXR,OpenEXRCore,OpenEXRUtil}-*.a`. Enabled arm64 in ImgMedia's 3 Build.cs gates. |
| **Boost 1.82** | S | arm64 libs exist but named `-mt-a64.a`; Build.cs used `-x64` by platform | `Boost.Build.cs`: select `a64` suffix by `Architecture==Arm64`; skip `boost_python*` on a64 (not shipped; only USD needs it). |
| **Intel TBB 2019u8** | R | Epic's arm64 `libtbb.a` was built against **libstdc++** (ABI clash with engine libc++) | Rebuilt `src/tbb/*.cpp` + `src/rml/client/rml_tbb.cpp` and `src/tbbmalloc/*` (minus proxy.cpp — it overrides global new/delete) with `-stdlib=libc++`. `IntelTBB.Build.cs` path → arm64 subdir. |
| **Oodle (data + network)** | S | arm64 libs exist under `LinuxArm64/` | `OodleDataCompression.Build.cs` + `OodleNetworkHandlerComponent.Build.cs`: select arm64 lib + `PlatformDir="LinuxArm64"` by arch. |
| **libpng 1.5.27** | R | Epic's arm64 lib was **non‑PIC** | Rebuilt from source `-fPIC` (`-DPNG_ARM_NEON_OPT=0`). |
| **Vorbis 1.3.2** | R | Epic's arm64 `libvorbis.a` was non‑PIC | Rebuilt vorbis/vorbisenc/vorbisfile `-fPIC`. |
| **etc2comp** | R | no arm64 lib | Rebuilt EtcLib `-fPIC -stdlib=libc++`. |
| **astcenc 4.2.0** | R | only x86 SSE4.1 lib | Rebuilt core with `-DASTCENC_NEON=1 -DASTCENC_SSE=0 …`. |
| **SPIRV‑Reflect** | R | mislabeled x86 in arm64 dir | Rebuilt `spirv_reflect.c` `-fPIC`. |
| **hlslcc** | R | no arm64 lib | Rebuilt `src/hlslcc_lib/*.cpp` `-std=c++17 -stdlib=libc++`. |
| **nvTextureTools 2.0.6** | R | no arm64 lib; POSH doesn't know aarch64 | Patched `posh.h`/`nvcore.h` (add `__aarch64__`), `Memory.h` (drop `throw()` from `operator new`), `ImageIO.cpp` (`return false`→`NULL` in pointer fns). CMake shared, PIC, `-DNV_USE_SSE=0`. |
| **VHACD** | R | no arm64 lib | Rebuilt `src/*.cpp` `-fPIC -stdlib=libc++` (scalar path, no SSE/OpenCL). |
| **SpeedTree v7.0** | R | no arm64 lib (source in‑tree) | Rebuilt `Source/Core` `-fPIC -stdlib=libc++`. |
| **METIS 5.1.0** | R | x86‑only, no in‑tree source | Downloaded **5.1.0** (matched Epic's header — note Epic swaps `CONTIG`/`MINCONN` enum; built against the in‑tree header). `metis.Build.cs` path → arm64. Used by NaniteBuilder. |
| **Draco 1.5.6** | R | x86‑only, no in‑tree source | Downloaded matching tag, CMake `-stdlib=libc++`, `draco_static`. |
| **Kiss_FFT** | R | no arm64 lib | Rebuilt `kiss_fft.c + tools/{kiss_fftr,kiss_fftnd,kiss_fftndr}.c` `-fPIC`. |
| **FontConfig** | S | no arm64 lib | Use system `libfontconfig` (`apt install libfontconfig-dev`); Build.cs → `PublicSystemLibraries.Add("fontconfig")` on arm64. |
| **FreeImage 3.18** | S | x86‑only .so | Use system `libfreeimage.so.3` (exact version match); symlinked into `Binaries/ThirdParty/FreeImage/Linux/`. |
| **libsndfile** | S | x86‑only .so | Symlinked system `libsndfile.so.1`. |
| **FBX SDK 2020.2** | X | **proprietary, no arm64 Linux SDK exists** | Patched `fbxarch.h` for `__aarch64__` (headers compile). Generated a **stub `libfbxsdk.so`** exporting the 459 undefined `fbxsdk::` symbols (asm `ret` no‑ops) so FBX‑using modules link. FBX import/export is non‑functional. |
| **Bink audio encoder** | X | proprietary, arm64 decode exists but not encode | Asm stub `.a` for the 25 encoder symbols; `AudioFormatBink.Build.cs` selects it on arm64. (Decode already had an arm64 lib.) |
| **Intel ISPC (texcomp + SIMD)** | X | ISPC compiler is x86 SIMD only | Texcomp: stub `.a` (`IntelISPCTexComp.Build.cs`). SIMD codegen: `bCompileISPC=false` on arm64 → Chaos/Niagara C++ fallbacks. |
| **CEF3 (Chromium)** | X | x86‑only, cannot rebuild | `bCompileCEF3=false` → WebBrowser non‑CEF path. |
| **USD (Pixar)** | X | needs USD/Python/Boost.Python builds not ported | `UnrealUSDWrapper.Build.cs` `EnableUsdSdk` returns false on arm64 → USD plugins compile with SDK unavailable. |
| **Perforce P4 API** | X | x86‑only | Disabled Perforce/Changelist/KDevelop plugins. |
| **OpenCV** | X (TODO) | x86‑only .so | Can't be `DisablePlugins`'d cleanly (hard-depended by CameraCalibration / MediaFrameworkUtilities / MixedRealityCaptureFramework / nDisplay). Needs a stub `.so` or system OpenCV — see `STATUS.md`. |
| **EOS / Steam / OpenXR / WebRTC / libvpx / Bink / ONNX-IREE / msquic / protobuf** | X (TODO) | x86‑only, pulled in only by `bBuildAllModules=true` | Optional plugins; needed only for the full-module build. `DisablePlugins` is soft (loses to hard deps), so reaching a full build needs these libs stubbed. Not needed for the minimal booting editor. |

## Regenerating the rebuilt libraries

The rebuilt `.a`/`.so` files are **not** in this repo (binary, and belong in the UE tree). After
applying the patch to a UE 5.4 checkout, rebuild them with the commands above. The exact per‑lib
build recipes (source file lists, defines) are captured in the project memory / commit history; the
`.Build.cs` patches point each module at the expected arm64 output path.
