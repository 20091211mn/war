#!/data/data/com.termux/files/usr/bin/sh
# تشغيل سريع على Termux (أو أي لينكس/ماك)
cd "$(dirname "$0")"
command -v node >/dev/null 2>&1 || { echo "ثبّت node أولاً: pkg install nodejs"; exit 1; }
command -v termux-wake-lock >/dev/null 2>&1 && termux-wake-lock
exec node server/server.js
