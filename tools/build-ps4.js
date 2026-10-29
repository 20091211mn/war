// يبني صفحة واحدة مستقلة (HTML+CSS+JS مصغّرة ومحوّلة لمتصفح PS4/Safari القديم) ثم يولّد ps4/src/game.h
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import esbuild from 'esbuild';
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const target = ['es2017'];
const shared = { name: 'shared', setup(b) { b.onResolve({ filter: /^\/shared\// }, (a) => ({ path: path.join(root, a.path) })); } };
const js = await esbuild.build({ entryPoints: [path.join(root, 'public/js/main.js')], bundle: true, format: 'iife', target, minify: true, write: false, plugins: [shared], legalComments: 'none',
  banner: { js: `(function(){var w=window;
if(!w.queueMicrotask)w.queueMicrotask=function(f){Promise.resolve().then(f)};
if(!w.AbortController){w.AbortController=function(){this.signal={aborted:false};this.abort=function(){this.signal.aborted=true}}}
if(!Array.prototype.flat)Array.prototype.flat=function(){return [].concat.apply([],this)};
if(!Object.fromEntries)Object.fromEntries=function(a){var o={};for(var i of a)o[i[0]]=i[1];return o};
if(!String.prototype.replaceAll)String.prototype.replaceAll=function(a,b){return this.split(a).join(b)};
var box;function dbg(m){try{if(!box){box=document.createElement('pre');box.style.cssText='position:fixed;left:0;top:0;z-index:99999;max-width:100%;margin:0;padding:6px;background:rgba(120,0,0,.85);color:#fff;font:14px monospace;white-space:pre-wrap;pointer-events:none';document.body.appendChild(box)}box.textContent=(box.textContent+'\\n'+m).slice(-900)}catch(e){}}
w.addEventListener('error',function(e){dbg('ERR '+e.message+' @'+(e.lineno||'')+':'+(e.colno||''))});
w.addEventListener('unhandledrejection',function(e){dbg('REJ '+(e.reason&&e.reason.message||e.reason))});
w.__dbg=dbg})();` } });
const css = await esbuild.transform(fs.readFileSync(path.join(root, 'public/style.css'), 'utf8'), { loader: 'css', target, minify: true });
let html = fs.readFileSync(path.join(root, 'public/index.html'), 'utf8');
html = html.replace(/<link rel="manifest"[^>]*>/, '').replace(/<link rel="stylesheet" href="style.css">/, () => '<style>' + css.code + '</style>');
const code = js.outputFiles[0].text.replace(/<\/script/gi, '<\\/script');
html = html.replace('<script type="module" src="js/main.js"></script>', () => '<script>window.__PS4=1;' + code + '</script>');
const buf = Buffer.from(html, 'utf8');
fs.mkdirSync(path.join(root, 'ps4/out'), { recursive: true });
fs.writeFileSync(path.join(root, 'ps4/out/page.html'), buf);
fs.mkdirSync(path.join(root, 'ps4/assets'), { recursive: true });
fs.writeFileSync(path.join(root, 'ps4/assets/page.html'), buf);
let h = '// مولَّد تلقائيًا من tools/build-ps4.js\n#define GAME_HTML_LEN ' + buf.length + 'u\nstatic const unsigned char GAME_HTML[] = {\n';
for (let i = 0; i < buf.length; i += 24) h += Array.from(buf.subarray(i, i + 24)).join(',') + ',\n';
h += '0};\n';
fs.writeFileSync(path.join(root, 'ps4/out/game.h'), h);
console.log('PS4 page:', (buf.length / 1024).toFixed(0), 'KB');
