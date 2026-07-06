#!/bin/bash
# Toolchain env for cross-compiling x86_64 Linux (SteamOS / Steam Deck) binaries FROM the arm64 DGX.
# Proven: `source` this, then clang-18 --target=x86_64-linux-gnu builds valid x86-64 ELF binaries
# with either libstdc++ or libc++ (UE's stdlib). See docs/STEAMOS_X86_64.md.
#
# Prereqs (one-time, already done on this DGX):
#   sudo dpkg --add-architecture amd64
#   # add archive.ubuntu.com amd64 apt source (see docs/STEAMOS_X86_64.md), then:
#   sudo apt-get install -y libc6-dev:amd64 libstdc++-13-dev:amd64 libgcc-13-dev:amd64 \
#        gcc-x86-64-linux-gnu g++-x86-64-linux-gnu
#   # libc++ for x86_64 (extract, don't install — the -dev pkg conflicts across arches):
#   mkdir -p ~/x86_64-sysroot && cd ~/x86_64-sysroot && \
#     apt-get download libc++-18-dev:amd64 libc++abi-18-dev:amd64 libc++1-18:amd64 \
#        libc++abi1-18:amd64 libunwind-18:amd64 && for d in *.deb; do dpkg -x "$d" .; done

export X64_SYSROOT="${X64_SYSROOT:-$HOME/x86_64-sysroot}"
export X64_TARGET="x86_64-linux-gnu"

# Compile+link an x86_64 C++ file with libc++ (UE-style). Usage: x64cxx foo.cpp -o foo
x64cxx() {
  clang-18 --target="$X64_TARGET" -stdlib=libc++ -fuse-ld=lld \
    -L"$X64_SYSROOT/usr/lib/x86_64-linux-gnu" \
    -Wl,-rpath-link,"$X64_SYSROOT/usr/lib/x86_64-linux-gnu" \
    "$@" -lc++ -lc++abi
}
# libstdc++ variant (links via the system x86_64 cross-gcc). Usage: x64cxx_gnu foo.cpp -o foo
x64cxx_gnu() {
  clang-18 --target="$X64_TARGET" -stdlib=libstdc++ -c "$@" 2>/dev/null
}
echo "x86_64 cross env ready: X64_SYSROOT=$X64_SYSROOT ; helpers: x64cxx, x64cxx_gnu"
