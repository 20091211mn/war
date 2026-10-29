#!/data/data/com.termux/files/usr/bin/bash
# رفع اللعبة إلى GitHub بأمر واحد من Termux:  bash push.sh
set -e
cd "$(dirname "$0")"
git config user.name "20091211mn"; git config user.email "mohammedissa20090@gmail.com"
[ -d .git ] || git init -b main
git remote remove origin 2>/dev/null || true
git remote add origin https://github.com/20091211mn/war.git
git add -A
git commit -m "تحديث: خرائط جديدة وتحسينات" || true
echo "اكتب اسم المستخدم 20091211mn ثم كلمة المرور = التوكن"
git push -u origin main --force
