#!/usr/bin/env bash
# يبني IPA أصلياً (غير موقَّع) بلا Xcode project: clang لـ iphoneos مباشرة → Warzone.app → Payload → zip
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"; OUT="$HERE/out"; rm -rf "$OUT"; APP="$OUT/Payload/Warzone.app"; mkdir -p "$APP"
SDK="$(xcrun --sdk iphoneos --show-sdk-path)"; echo "SDK=$SDK"
xcrun --sdk iphoneos clang++ -arch arm64 -miphoneos-version-min=13.0 -isysroot "$SDK" -std=c++17 -O2 -fobjc-arc -fno-math-errno -Wno-deprecated-declarations -x objective-c++ "$HERE/main_ios.mm" \
  -framework UIKit -framework Foundation -framework QuartzCore -framework CoreGraphics -framework AudioToolbox -framework AVFoundation -o "$APP/Warzone"
cp "$HERE/Info.plist" "$APP/Info.plist"; ICON="$HERE/../../../public/icon-512.png"
sips -z 120 120 "$ICON" --out "$APP/AppIcon60x60@2x.png" >/dev/null; sips -z 180 180 "$ICON" --out "$APP/AppIcon60x60@3x.png" >/dev/null
codesign --force --sign - "$APP" || true
(cd "$OUT" && zip -qry Warzone-native.ipa Payload); ls -la "$OUT/Warzone-native.ipa"; file "$APP/Warzone"
