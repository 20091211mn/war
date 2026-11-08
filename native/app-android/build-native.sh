#!/usr/bin/env bash
# يبني libwarzone.so (محرك C++ + جسر JNI) لكل معماريات أندرويد ويضعها في jniLibs ليحزمها Gradle
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"; MOB="$HERE/../mobile"; OUT="$HERE/app/src/main/jniLibs"; rm -rf "$OUT"
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-/usr/local/lib/android/sdk}}"; NDK="${ANDROID_NDK_LATEST_HOME:-${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}}"
[ -d "$NDK" ] || NDK="$(ls -d "$SDK"/ndk/* | sort -V | tail -1)"; TC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
for abi in arm64-v8a armeabi-v7a; do
  case $abi in arm64-v8a) CC=aarch64-linux-android26;; armeabi-v7a) CC=armv7a-linux-androideabi26;; esac
  mkdir -p "$OUT/$abi"
  "$TC/$CC-clang++" -std=c++17 -O2 -fPIC -shared -pthread -fno-math-errno -Wno-deprecated-declarations \
    "$MOB/bridge/wz_bridge.cpp" "$HERE/jni/native_lib.cpp" -o "$OUT/$abi/libwarzone.so" -llog -lm -Wl,-z,max-page-size=16384 -Wl,--no-undefined
  echo "built $abi: $(stat -c %s "$OUT/$abi/libwarzone.so") bytes"
done
