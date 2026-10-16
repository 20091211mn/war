// يجمع ملفات الواجهة + shared داخل مجلد www ليحزمها Capacitor داخل التطبيق
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const www = path.join(root, 'www');
fs.rmSync(www, { recursive: true, force: true });
fs.cpSync(path.join(root, 'public'), www, { recursive: true });
fs.cpSync(path.join(root, 'shared'), path.join(www, 'shared'), { recursive: true });
console.log('www جاهز:', fs.readdirSync(www).join(', '));
