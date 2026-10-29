// يعدّل مشروع أندرويد المولَّد: أفقي بملء الشاشة، إبقاء الشاشة مضاءة، أيقونة التطبيق
import fs from 'node:fs';
import path from 'node:path';
import { execSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const app = path.join(root, 'android', 'app', 'src', 'main');
const manifestPath = path.join(app, 'AndroidManifest.xml');
let m = fs.readFileSync(manifestPath, 'utf8');
if (!m.includes('screenOrientation')) m = m.replace('<activity', '<activity\n            android:screenOrientation="sensorLandscape"');
if (!m.includes('INTERNET')) m = m.replace('</manifest>', '    <uses-permission android:name="android.permission.INTERNET" />\n</manifest>');
if (!m.includes('ACCESS_NETWORK_STATE')) m = m.replace('</manifest>', '    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />\n    <uses-permission android:name="android.permission.WAKE_LOCK" />\n</manifest>');
if (!m.includes('usesCleartextTraffic')) m = m.replace('<application', '<application\n        android:usesCleartextTraffic="true"');
fs.writeFileSync(manifestPath, m);
// ملء الشاشة وإبقاء الشاشة مضاءة
const stylesPath = path.join(app, 'res', 'values', 'styles.xml');
let st = fs.readFileSync(stylesPath, 'utf8');
if (!st.includes('windowFullscreen')) st = st.replace(/(<style name="AppTheme.NoActionBar"[^>]*>)/, '$1\n        <item name="android:windowFullscreen">true</item>\n        <item name="android:windowLayoutInDisplayCutoutMode">shortEdges</item>\n        <item name="android:keepScreenOn">true</item>\n        <item name="android:windowBackground">@android:color/black</item>\n        <item name="android:statusBarColor">@android:color/transparent</item>\n        <item name="android:navigationBarColor">@android:color/transparent</item>\n        <item name="android:windowDrawsSystemBarBackgrounds">true</item>');
fs.writeFileSync(stylesPath, st);
// إضافة LanHost الأصلية (خادم LAN داخل التطبيق) + MainActivity التي تسجّلها
const javaDir = path.join(app, 'java', 'com', 'warzone', 'lan');
fs.mkdirSync(javaDir, { recursive: true });
for (const f of ['LanHostPlugin.java', 'MainActivity.java']) fs.copyFileSync(path.join(root, 'tools', 'native', f), path.join(javaDir, f));
// أيقونة
const src = path.join(root, 'public', 'icon-512.png');
const sizes = { mdpi: 48, hdpi: 72, xhdpi: 96, xxhdpi: 144, xxxhdpi: 192 };
try { fs.rmSync(path.join(app, 'res', 'mipmap-anydpi-v26'), { recursive: true, force: true }); } catch { /* */ }
for (const [d, px] of Object.entries(sizes)) {
  const dir = path.join(app, 'res', `mipmap-${d}`);
  if (!fs.existsSync(dir)) continue;
  for (const n of ['ic_launcher.png', 'ic_launcher_round.png', 'ic_launcher_foreground.png']) {
    try { execSync(`convert "${src}" -resize ${px}x${px} "${path.join(dir, n)}"`); } catch { fs.copyFileSync(src, path.join(dir, n)); }
  }
}
console.log('تم تعديل مشروع أندرويد');
