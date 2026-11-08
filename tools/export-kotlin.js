#!/usr/bin/env node
// Exports game data for the Kotlin port: native/gen/data.h -> JSON, native/src/gunmodels.h -> binary meshes.
// Usage: node tools/export-kotlin.js
import fs from 'fs';import path from 'path';import {fileURLToPath} from 'url';
const root=path.join(path.dirname(fileURLToPath(import.meta.url)),'..');
const outDir=path.join(root,'native/kotlin/core/src/main/resources/warzone');
fs.mkdirSync(outDir,{recursive:true});
const data=fs.readFileSync(path.join(root,'native/gen/data.h'),'utf8');
function cToJson(s){
  s=s.replace(/'(.)'/g,'"$1"');
  s=s.replace(/"(?:[^"\\]|\\.)*"|\b[A-Za-z_]\w*\b/g,m=>m[0]==='"'?m:JSON.stringify(m));
  s=s.replace(/\{/g,'[').replace(/\}/g,']').replace(/(\d)f\b/g,'$1');
  return JSON.parse(s);
}
function grab(name){ // static const T name[]={...};
  const re=new RegExp('static const [\\w\\s\\*\\(\\)\\[\\]]*?\\b'+name+'(?:\\[\\d*\\])*\\s*=\\s*\\{');
  const m=re.exec(data); if(!m)throw new Error('missing '+name);
  let i=m.index+m[0].length-1,d=0,j=i;
  for(;j<data.length;j++){ if(data[j]==='{')d++; else if(data[j]==='}'){d--; if(!d)break;} }
  return cToJson(data.slice(i,j+1));
}
const maps=[];
const mm=/MAPS\[\]=(\{[\s\S]*?\}\});/.exec(data)[1];
const ml=cToJson(mm);
ml.forEach((f,k)=>{
  const [W,D,nb,nd,nv,nr,nbl,bx,dc,red,blue,veh,cols,ncol,sky,hor,ground,fog,fogN,fogF,sun,amb,sunI]=f;
  maps.push({W,D,boxes:grab(bx).slice(0,nb).flat(),deco:grab(dc).slice(0,nd).flat(),red:grab(red).slice(0,nr).flat(),blue:grab(blue).slice(0,nbl).flat(),
    veh:grab(veh).slice(0,nv).flat(),cols:grab(cols).slice(0,ncol).flat(),sky,hor,ground,fog,fogN,fogF,sun,amb,sunI});
});
fs.writeFileSync(path.join(outDir,'maps.json'),JSON.stringify(maps));
const wn=['id','dmg','head','rpm','autof','pellets','mag','mags','reload','hip','ads','recoilV','recoilH','r0','r1','minMul','move','zoom','type','name'];
const W=grab('WEAPONS').map(a=>Object.fromEntries(wn.map((n,i)=>[n,a[i]])));
const mount=Object.fromEntries(wn.map((n,i)=>[n,grab('MOUNTDEF')[i]]));
const wk=grab('WKIND');
const gd=grab('GUNDEFS').map(a=>({mz:a[0],sy:a[1],np:a[2],p:a[3].slice(0,a[2]).flat(),hasMag:a[4],mag:a[5]}));
fs.writeFileSync(path.join(outDir,'weapons.json'),JSON.stringify({weapons:W,mount,wkind:wk,gundefs:gd}));
// gun meshes
const gm=fs.readFileSync(path.join(root,'native/src/gunmodels.h'),'utf8');
function arr(name){ const m=new RegExp(name+'(?:\\[\\d+\\])?\\s*=\\s*\\{([^}]*)\\}').exec(gm); if(!m)throw new Error('gm '+name); return m[1].split(',').map(s=>Number(s.trim().replace(/f$/i,''))); }
for(const n of ['sniper','rifle','dmr','launcher']){
  const nv=+new RegExp(n+'_NV=(\\d+)').exec(gm)[1];
  const grp=arr(n+'_GRP'),lo=arr(n+'_LO'),rng=arr(n+'_RNG'),pos=arr(n+'_pos'),nor=arr(n+'_nor'),col=arr(n+'_col'),idx=arr(n+'_idx');
  const sz=4+16+24+pos.length*2+nor.length+col.length+4+idx.length*2;
  const b=Buffer.alloc(sz); let o=0;
  b.writeInt32LE(nv,o);o+=4; grp.forEach(v=>{b.writeInt32LE(v,o);o+=4;}); lo.forEach(v=>{b.writeFloatLE(v,o);o+=4;}); rng.forEach(v=>{b.writeFloatLE(v,o);o+=4;});
  pos.forEach(v=>{b.writeInt16LE(v,o);o+=2;}); nor.forEach(v=>{b.writeInt8(v,o);o+=1;}); col.forEach(v=>{b.writeUInt8(v,o);o+=1;});
  b.writeInt32LE(idx.length,o);o+=4; idx.forEach(v=>{b.writeUInt16LE(v,o);o+=2;});
  if(pos.length!==nv*3)throw new Error('pos len '+n);
  fs.writeFileSync(path.join(outDir,'gm_'+n+'.bin'),b);
}
console.log('exported',maps.length,'maps,',W.length,'weapons');
