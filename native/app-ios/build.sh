#!/usr/bin/env bash
# يبني IPA غير موقَّع: الواجهة كلها Swift ، والمحرك C++ يُترجم إلى مكتبة ويُربط بجسر C. بلا Xcode project.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"; MOB="$HERE/../mobile"; OUT="$HERE/out"; rm -rf "$OUT"; APP="$OUT/Payload/Warzone.app"; mkdir -p "$APP"
SDK="$(xcrun --sdk iphoneos --show-sdk-path)"; echo "SDK=$SDK"; xcrun swiftc --version
xcrun --sdk iphoneos clang++ -arch arm64 -miphoneos-version-min=13.0 -isysroot "$SDK" -std=c++17 -O2 -fno-math-errno -Wno-deprecated-declarations -c "$MOB/bridge/wz_bridge.cpp" -o "$OUT/engine.o"
xcrun --sdk iphoneos swiftc -target arm64-apple-ios13.0 -sdk "$SDK" -Ounchecked -parse-as-library -module-name Warzone \
  -import-objc-header "$HERE/WZ-Bridging-Header.h" "$HERE"/*.swift "$OUT/engine.o" -lc++ -o "$APP/Warzone"
sed 's/com.warzone.game/com.warzone.swift/' "$MOB/ios/Info.plist" > "$APP/Info.plist"; ICON="$HERE/../../public/icon-512.png"
sips -z 120 120 "$ICON" --out "$APP/AppIcon60x60@2x.png" >/dev/null; sips -z 180 180 "$ICON" --out "$APP/AppIcon60x60@3x.png" >/dev/null
codesign --force --sign - "$APP" || true
(cd "$OUT" && zip -qry Warzone-swift.ipa Payload); ls -la "$OUT/Warzone-swift.ipa"; file "$APP/Warzone"
