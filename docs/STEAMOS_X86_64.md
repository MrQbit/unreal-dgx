# Targeting SteamOS / Steam Deck (x86_64 Linux) from the arm64 DGX

**Status: the cross-toolchain is set up and proven — the DGX builds real x86_64 Linux binaries.**
Full UE *engine* cross-packaging needs one more integration step in UnrealBuildTool (below).

SteamOS / Steam Deck is **x86_64 Linux + Vulkan** — a natural fit for this editor (it already cooks
Vulkan content). The only missing piece was compiling x86_64 *binaries* on an ARM64 host. That now
works: **`clang-18` cross-compiles to x86_64, and links a valid x86_64 ELF with either libstdc++ or
libc++** (libc++ is UE's stdlib).

## Proof (run on this DGX)

```
$ clang-18 --target=x86_64-linux-gnu -stdlib=libc++ -fuse-ld=lld \
    -L~/x86_64-sysroot/usr/lib/x86_64-linux-gnu hello.cpp -o hello -lc++ -lc++abi
$ file hello
hello: ELF 64-bit LSB pie executable, x86-64, dynamically linked,
       interpreter /lib64/ld-linux-x86-64.so.2, for GNU/Linux   ← runs on SteamOS/Steam Deck
```

## What was installed (one-time, done on this DGX)

1. **amd64 multiarch** — `dpkg --add-architecture amd64` + an `archive.ubuntu.com` amd64 apt source
   (arm64 uses `ports.ubuntu.com`, which has no amd64):
   ```
   /etc/apt/sources.list.d/amd64.sources:
     Types: deb
     URIs: http://archive.ubuntu.com/ubuntu/
     Suites: noble noble-updates noble-backports noble-security
     Components: main restricted universe multiverse
     Architectures: amd64
     Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg
   ```
   (and add `Architectures: arm64` to the existing `ubuntu.sources` so apt doesn't seek amd64 on ports.)
2. **x86_64 glibc + gcc cross toolchain** (crt objects, libgcc, glibc headers/libs):
   `apt-get install libc6-dev:amd64 libstdc++-13-dev:amd64 libgcc-13-dev:amd64 gcc-x86-64-linux-gnu g++-x86-64-linux-gnu`
3. **libc++ for x86_64** (UE's stdlib) — the `-dev` package conflicts with the arm64 one, so *extract*
   it into a sysroot instead of installing: `apt-get download libc++-18-dev:amd64 …` then `dpkg -x`
   into `~/x86_64-sysroot`. `scripts/x86_64_cross_env.sh` wires the paths (`x64cxx` helper).

## Why libc++ is easy here

The libc++/libc++abi **headers are architecture-independent** (same for arm64 and x86_64) — only the
`.so` you link against is arch-specific. So the system clang-18's headers + the extracted x86_64
`libc++.so` link cleanly. UBT already emits `-lc++ -lc++abi` (from the arm64 port patch), which is
exactly what the cross-link needs.

## Remaining step for a full UE x86_64 *package*

The toolchain works; wiring it into a full engine/game build needs UnrealBuildTool to treat **x86_64
as a target arch when the host is arm64**:

- Our `LinuxToolChain`/`UEBuildLinux` patches currently assume **host arch == target arch** (they force
  the system arm64 clang and the `aarch64-unknown-linux-gnueabi` triple). For an x86_64 *target*, UBT
  must instead emit `--target=x86_64-unknown-linux-gnu`, point `--sysroot`/`-L` at the x86_64 sysroot
  above (or Epic's bundled `v22_clang-16.0.6-centos7` toolchain, which ships a matched x86_64 libc++),
  and keep `-fuse-ld=lld`.
- The engine's **x86_64 third-party libs are Epic's stock** (FBX, ICU, PhysX, etc. all ship prebuilt
  for x86_64 Linux) — so **none of the arm64 stubbing/rebuilding is needed** for the x86_64 target.
  That side is actually simpler than the arm64 editor was.
- Then: `Build.sh <Game> Linux Development -Architecture=x64` (or the LinuxArm64/x64 arch selector) and
  `RunUAT BuildCookRun -platform=Linux`. The cook (Vulkan) already works.

In short: **cross-toolchain = done and proven; the follow-on is a UBT arch-targeting patch** (bounded,
well-understood) plus a long first x86_64 build. Alternatively, package the x86_64 build on any small
x86_64 Linux runner — the project is portable.

## Recap of the platform picture

| Target | From this DGX |
|---|---|
| Linux **arm64** | ✅ native (done — see PongGame) |
| Linux **x86_64 / SteamOS** | ✅ toolchain proven; UBT arch-targeting patch pending |
| **Android** | ✅ with the NDK (Vulkan/GLES cook already works) |
| **Windows / macOS / iOS** | ❌ package on those OSes (D3D/Metal + toolchains are OS-bound) |
