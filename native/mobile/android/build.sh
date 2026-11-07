#!/usr/bin/env bash
# يبني APK أصلياً بلا Gradle: NDK (clang) → libwarzone.so ، aapt2 → apk ، zipalign + apksigner (مفتاح تجريبي)
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"; OUT="${OUT:-$HERE/out}"; rm -rf "$OUT"; mkdir -p "$OUT/stage" "$OUT/res"
SDK="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-/usr/local/lib/android/sdk}}"; NDK="${ANDROID_NDK_LATEST_HOME:-${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-}}}"
[ -d "$NDK" ] || NDK="$(ls -d "$SDK"/ndk/* | sort -V | tail -1)"
BT="$(ls -d "$SDK"/build-tools/* | sort -V | tail -1)"; PLAT="$(ls -d "$SDK"/platforms/android-* | sort -V | tail -1)"
TC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"; GLUE="$NDK/sources/android/native_app_glue"
echo "SDK=$SDK NDK=$NDK BT=$BT PLAT=$PLAT"
for abi in arm64-v8a armeabi-v7a; do
  case $abi in arm64-v8a) CC=aarch64-linux-android26;; armeabi-v7a) CC=armv7a-linux-androideabi26;; esac
  mkdir -p "$OUT/stage/lib/$abi"
  "$TC/$CC-clang" -O2 -fPIC -c "$GLUE/android_native_app_glue.c" -o "$OUT/glue-$abi.o"
  "$TC/$CC-clang++" -std=c++17 -O2 -fPIC -shared -pthread -fno-math-errno -Wno-deprecated-declarations -I"$GLUE" "$HERE/main_android.cpp" "$OUT/glue-$abi.o" \
    -o "$OUT/stage/lib/$abi/libwarzone.so" -landroid -llog -laaudio -lm -Wl,-z,max-page-size=16384 -Wl,--no-undefined
  echo "built $abi: $(stat -c %s "$OUT/stage/lib/$abi/libwarzone.so") bytes"
done
# أيقونة
ICON="$HERE/../../../public/icon-512.png"; for d in "mdpi:48" "hdpi:72" "xhdpi:96" "xxhdpi:144" "xxxhdpi:192"; do n=${d%%:*}; px=${d##*:}; mkdir -p "$OUT/res/mipmap-$n"; convert "$ICON" -resize ${px}x${px} "$OUT/res/mipmap-$n/ic_launcher.png" 2>/dev/null || cp "$ICON" "$OUT/res/mipmap-$n/ic_launcher.png"; done
"$BT/aapt2" compile --dir "$OUT/res" -o "$OUT/res.zip"
"$BT/aapt2" link -o "$OUT/base.apk" -I "$PLAT/android.jar" --manifest "$HERE/AndroidManifest.xml" --min-sdk-version 26 --target-sdk-version 33 "$OUT/res.zip"
(cd "$OUT/stage" && zip -qr ../base.apk lib)
"$BT/zipalign" -p -f 4 "$OUT/base.apk" "$OUT/aligned.apk"
[ -f "$OUT/dbg.jks" ] || keytool -genkeypair -keystore "$OUT/dbg.jks" -alias dbg -keyalg RSA -keysize 2048 -validity 10000 -storepass android -keypass android -dname "CN=Warzone" >/dev/null 2>&1
"$BT/apksigner" sign --ks "$OUT/dbg.jks" --ks-pass pass:android --key-pass pass:android --out "$OUT/Warzone-native.apk" "$OUT/aligned.apk"
"$BT/apksigner" verify "$OUT/Warzone-native.apk" && ls -la "$OUT/Warzone-native.apk"
