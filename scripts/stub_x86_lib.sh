#!/bin/bash
# Replace an x86-only third-party library with an aarch64 no-op STUB that exports the same
# symbols, so aarch64 UE modules linking against it resolve at link time. The stubbed
# functionality is non-operational — this is only for OPTIONAL plugins whose x86 libs have no
# arm64 build (OpenCV, EOS, Steam, OpenXR, WebRTC, libvpx, Bink, ONNX, msquic, protobuf, ...).
# Backs up the original once as <lib>.x86. Handles both shared (.so*) and static (.a) libs.
#
# Usage: stub_x86_lib.sh /path/to/libfoo.so[.N]   |   /path/to/libfoo.a
set -euo pipefail
LIB="${1:?usage: stub_x86_lib.sh <lib-path>}"
{ [ -f "$LIB" ] || [ -f "$LIB.x86" ]; } || { echo "no such lib: $LIB"; exit 1; }
[ -f "$LIB.x86" ] || cp -a "$LIB" "$LIB.x86"   # back up original once (before we overwrite it)
SRC="$LIB.x86"
CLANG=${CLANG:-clang-18}; NM=${NM:-llvm-nm-18}; AR=${AR:-llvm-ar-18}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT

# Defined global symbols from the x86 lib (llvm-nm reads symbols regardless of target arch).
# Shared libs: use the dynamic symbol table (-D) = the real exported API. Static archives (.a) have
# no dynamic table, so use the regular table. Strip @@VERSION tags (UE references these unversioned),
# keep only valid C identifiers, and drop linker-reserved symbols (they'd clash with ld's own).
case "$LIB" in
  *.a) $NM --defined-only "$SRC" 2>/dev/null | awk 'NF>=3{print $2, $3}' > "$W/s" || true ;;
  *)   $NM -D --defined-only "$SRC" 2>/dev/null | awk 'NF>=3{print $2, $3}' > "$W/s" || true ;;
esac
RESERVED='^(_DYNAMIC|_GLOBAL_OFFSET_TABLE_|_edata|_end|__bss_start|_init|_fini|_start|__data_start|__dso_handle|_IO_stdin_used|__TMC_END__|_GLOBAL__sub_I.*)$'
sed -E 's/@@?[A-Za-z0-9_.]+$//' "$W/s" | awk '$2 ~ /^[A-Za-z_][A-Za-z0-9_]*$/' \
  | grep -vE " ($RESERVED)$" | sort -u -k2,2 > "$W/syms"

{
  echo ".text"
  # functions (T/t) and weak text (W/w/i) → return 0
  awk 'toupper($1)=="T" || toupper($1)=="W" || $1=="i" {print ".globl "$2"\n.type "$2",@function\n"$2":\n\tmov x0, #0\n\tret"}' "$W/syms"
  echo ".data"
  # data/bss/rodata objects (D/B/G/S/V/R) → 64 zero bytes (link-satisfying placeholder)
  awk 'toupper($1)=="D" || toupper($1)=="B" || toupper($1)=="G" || toupper($1)=="S" || toupper($1)=="V" || toupper($1)=="R" {print ".globl "$2"\n"$2":\n\t.zero 64"}' "$W/syms"
} > "$W/stub.s"

NFUNC=$(grep -c "@function" "$W/stub.s" || true)
# Build into a temp first so a failed compile never destroys the target lib.
case "$LIB" in
  *.a)
    $CLANG -c -fPIC -o "$W/stub.o" "$W/stub.s"
    $AR rcs "$W/out.a" "$W/stub.o"
    cp "$W/out.a" "$LIB" ;;
  *)
    $CLANG -shared -fPIC -nostdlib -Wl,-soname,"$(basename "$LIB")" -o "$W/out.so" "$W/stub.s"
    cp "$W/out.so" "$LIB" ;;
esac
echo "stubbed $(basename "$LIB")  ($NFUNC funcs)  [orig kept at $SRC]"
