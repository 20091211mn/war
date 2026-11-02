// يعدّل مشروع iOS المولَّد: أفقي بملء الشاشة، إبقاء الشاشة مضاءة، السماح بالاتصال المحلي، أيقونة
import fs from 'node:fs';
import path from 'node:path';
import { execSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const app = path.join(root, 'ios', 'App', 'App');
const plist = path.join(app, 'Info.plist');
const pb = (a) => { try { execSync(`/usr/libexec/PlistBuddy -c "${a}" "${plist}"`, { stdio: 'pipe' }); return true; } catch { return false; } };
// أفقي فقط
for (const k of ['UISupportedInterfaceOrientations', 'UISupportedInterfaceOrientations~ipad']) {
  pb(`Delete :${k}`); pb(`Add :${k} array`);
  pb(`Add :${k}:0 string UIInterfaceOrientationLandscapeLeft`); pb(`Add :${k}:1 string UIInterfaceOrientationLandscapeRight`);
}
// ملء الشاشة وإخفاء شريط الحالة
pb('Delete :UIStatusBarHidden'); pb('Add :UIStatusBarHidden bool true');
pb('Delete :UIViewControllerBasedStatusBarAppearance'); pb('Add :UIViewControllerBasedStatusBarAppearance bool false');
pb('Delete :UIRequiresFullScreen'); pb('Add :UIRequiresFullScreen bool true');
// السماح بالاتصال HTTP/WS بالشبكة المحلية (ملاعب LAN)
pb('Delete :NSAppTransportSecurity'); pb('Add :NSAppTransportSecurity dict'); pb('Add :NSAppTransportSecurity:NSAllowsArbitraryLoads bool true');
pb('Delete :NSLocalNetworkUsageDescription'); pb('Add :NSLocalNetworkUsageDescription string للاتصال بلاعبين على نفس الشبكة');
// أيقونة (1024 مربعة بلا شفافية)
const icon = path.join(app, 'Assets.xcassets', 'AppIcon.appiconset', 'AppIcon-512@2x.png');
try { const tmp = path.join(root, 'icon-tmp.jpg'); const src = path.join(root, 'public', 'icon-512.png'); execSync(`sips -z 1024 1024 -s format jpeg "${src}" --out "${tmp}" && sips -s format png "${tmp}" --out "${icon}"`, { stdio: 'pipe' }); fs.rmSync(tmp, { force: true }); } catch { try { execSync(`convert "${path.join(root, 'public', 'icon-512.png')}" -resize 1024x1024 -background black -alpha remove -alpha off "${icon}"`); } catch { try { fs.copyFileSync(path.join(root, 'public', 'icon-512.png'), icon); } catch { /* */ } } }
console.log('تم تعديل مشروع iOS');
