// حرب طائرات F-35 (خريطة 8): فيزياء طيران (رفع/سحب/دفع/انهيار)، مدفع، ذكاء اصطناعي، نزول بقاذفة RPG (5 طلقات) وعودة للطائرة، فوز بتدمير 30 طائرة
#pragma once
static inline float wrapPiA(float a){ while(a>3.14159f)a-=6.28318f; while(a<-3.14159f)a+=6.28318f; return a; }
static inline float clampA(float v,float lo,float hi){ return v<lo?lo:v>hi?hi:v; }

inline int Game::jetNew(int team,bool airborne,int slot){
  int j=-1; for(int k=0;k<(int)jets.size();k++) if(!jets[k].alive&&!jets[k].spare&&jets[k].respT<=0){ j=k; break; }
  if(j<0){ jets.push_back(Jet()); j=(int)jets.size()-1; }
  Jet J; J.team=team; J.alive=true; J.hp=300; J.pilot=-1; J.yaw=team?1.5708f:-1.5708f;
  if(airborne){ J.x=(team?1:-1)*(w.m->W*0.5f-150.f-frand()*70.f); J.z=(frand()-0.5f)*w.m->D*0.5f; J.y=110.f+frand()*60.f; J.vx=-sinf(J.yaw)*105.f; J.vz=-cosf(J.yaw)*105.f; J.th=0.7f; }
  else { const P2*sp=team?w.m->blue:w.m->red; int n=team?w.m->nbl:w.m->nr; P2 s=sp[slot%n]; J.x=s.x; J.z=s.z; J.y=0; J.th=0; J.spare=true; J.gnd=true; }
  jets[j]=J; return j;
}
inline void Game::airInit(){
  jets.clear(); entJet.assign(e.size(),-1); footT=0; jetMsgT=0; rpgIdx=0;
  for(int k=0;k<(int)WEAPON_COUNT;k++) if(!strcmp(WEAPONS[k].id,"rpg")) rpgIdx=k;
  for(int i=0;i<(int)e.size();i++) airSpawn(i);
  for(int k=0;k<4;k++) jetNew(k/2,false,6+k%2);
  jetSay("دمّر 30 طائرة معادية بالمدفع");
}
inline void Game::airSpawn(int i){
  Ent&p=e[i]; int j=jetNew(p.team,true,0); Jet&J=jets[j]; J.pilot=i; entJet[i]=j;
  p.alive=true; p.hp=100; p.prot=2; p.b=Body(); p.b.x=J.x; p.b.y=J.y; p.b.z=J.z; p.yaw=J.yaw; p.pitch=0; p.mnt=false; p.reloadT=-1;
  if(i==meI){ adsHeld=false; drive=-1; }
}
inline void Game::jetHurt(int j,float dmg,int by,bool inc){
  Jet&J=jets[j]; if(!J.alive||dmg<=0)return; J.hp-=dmg; J.lastBy=by; if(inc&&frand()<0.3f)J.fire=std::max(J.fire,5.f);
  if(J.pilot==meI){ dmgFlash=0.35f; SFX::play(SFX::HURT,0.8f); } if(by==meI){ hitMark=0.25f; SFX::play(SFX::HIT,0.6f); }
  if(J.hp<=0)jetDie(j,by);
}
inline void Game::jetDie(int j,int by){
  Jet&J=jets[j]; if(!J.alive)return; J.alive=false; J.respT=J.spare?25.f:0.f; float ex=J.x,ey=J.y,ez=J.z;
  for(int k=0;k<18;k++)puffs.push_back({ex+(frand()-0.5f)*8,ey+(frand()-0.5f)*4,ez+(frand()-0.5f)*8,1.f+frand()*3.f,1.4f+frand()*1.4f,2.5f+frand()*3.f,k<11});
  sparkAt(ex,ey,ez,0,-1,0,40); { float dd=hypotf(ex-e[meI].b.x,ez-e[meI].b.z); SFX::play(SFX::EXPLODE,std::max(0.25f,1.f-dd/900.f)); }
  int p=J.pilot; J.pilot=-1;
  if(p>=0){ Ent&pe=e[p]; pe.alive=false; pe.deaths++; pe.respawn=5.f; entJet[p]=-1; if(p==meI){ streak=0; jetSay("أُسقطت طائرتك!"); } }
  if(by>=0&&by<(int)e.size()&&e[by].team!=J.team){ Ent&k=e[by]; k.kills++; score[k.team]++; char b[64]; snprintf(b,64,"%s > F-35 (%s)",nm(by).c_str(),p>=0?nm(p).c_str():"طائرة"); addFeed(b);
    if(by==meI){ SFX::play(SFX::KILL,0.8f); streak++; char m[48]; snprintf(m,48,T("أسقطت طائرة! %d / %d"),score[k.team],scoreLim); say(m); }
    if(score[0]>=scoreLim||score[1]>=scoreLim){ over=true; winner=score[0]>=scoreLim?0:1; } }
}
// فيزياء الطيران: دفع + سحب + رفع يعتمد زاوية الهجوم (مع انهيار) + جاذبية + قصور؛ الدوران بالانحدار الجانبي يأتي من اتجاه قوة الرفع
inline bool Game::jetStep(Jet&J,float dt,float rc,float pc,float yc,float thd,bool ab){
  const float G=9.8f,KL=0.00165f,CD0=0.00065f,KI=0.00015f;
  J.th=clampA(J.th+thd*0.6f*dt,0.f,1.f); J.ab+=((ab&&J.th>0.9f?1.f:0.f)-J.ab)*std::min(1.f,4.f*dt);
  float cp=cosf(J.pitch),sp=sinf(J.pitch),cy=cosf(J.yaw),sy=sinf(J.yaw);
  float f[3]={-sy*cp,sp,-cy*cp},r0[3]={cy,0.f,-sy}; float u0[3]={r0[1]*f[2]-r0[2]*f[1],r0[2]*f[0]-r0[0]*f[2],r0[0]*f[1]-r0[1]*f[0]};
  float s=jetSpeed(J); float thrust=15.f*J.th+12.f*J.ab;
  if(J.gnd){ // على الأرض: عجلات + احتكاك + توجيه
    float hs=hypotf(J.vx,J.vz); float ax=thrust*0.8f-0.7f-CD0*hs*hs; hs=std::max(0.f,hs+ax*dt); if(J.th<0.02f)hs=std::max(0.f,hs-6.f*dt);
    J.yaw-=rc*0.6f*std::min(1.f,hs/15.f)*dt; J.roll*=expf(-6.f*dt);
    if(hs>55.f&&pc>0.1f)J.pitch+=pc*0.5f*dt; else J.pitch=std::max(0.f,J.pitch*expf(-3.f*dt)); J.pitch=clampA(J.pitch,0.f,0.45f);
    float hy=J.yaw; J.vx=-sinf(hy)*hs; J.vz=-cosf(hy)*hs; float L=KL*hs*hs*std::min(1.f,J.pitch*5.2f+0.05f); if(L>G)J.vy+=(L-G)*dt; else J.vy=0.f;
    J.stall=false;
  } else {
    float auth=clampA((s-22.f)/60.f,0.12f,1.f);
    J.roll+=rc*2.4f*dt*auth; if(fabsf(rc)<0.05f)J.roll-=J.roll*0.8f*dt; J.roll=clampA(J.roll,-1.5f,1.5f);
    float cr=cosf(J.roll),sr=sinf(J.roll); float up[3],rt[3]; for(int k=0;k<3;k++){ up[k]=u0[k]*cr-r0[k]*sr; rt[k]=r0[k]*cr+u0[k]*sr; }
    float vf=J.vx*f[0]+J.vy*f[1]+J.vz*f[2],vu=J.vx*up[0]+J.vy*up[1]+J.vz*up[2],vr=J.vx*rt[0]+J.vy*rt[1]+J.vz*rt[2];
    float aoa=atan2f(-vu,std::max(1.f,vf)); float CL=aoa*5.2f; J.stall=fabsf(aoa)>0.30f||s<40.f; if(fabsf(aoa)>0.30f){ float sg=aoa>0?1.f:-1.f; CL=sg*std::max(0.4f,0.30f*5.2f-(fabsf(aoa)-0.30f)*6.f); }
    { float al=fabsf(aoa); if(al>0.2f&&((pc>0&&aoa>0)||(pc<0&&aoa<0)))pc*=clampA((0.30f-al)/0.10f,0.f,1.f); } // حدّ الحِمل: يمنع تجاوز زاوية الانهيار
    float q=pc*0.9f*auth,ry=yc*0.25f*auth; float nf[3]; for(int k=0;k<3;k++)nf[k]=f[k]+up[k]*q*dt+rt[k]*ry*dt; if(s>15.f){ float k=1.3f*dt; for(int q2=0;q2<3;q2++)nf[q2]+=((q2==0?J.vx:q2==1?J.vy:J.vz)/s-f[q2])*k; } /* استقرار: الأنف يتبع اتجاه السرعة */ if(s<40.f)nf[1]-=0.55f*dt;
    float nl=sqrtf(nf[0]*nf[0]+nf[1]*nf[1]+nf[2]*nf[2])+1e-6f; for(int k=0;k<3;k++)nf[k]/=nl; J.pitch=clampA(asinf(clampA(nf[1],-1.f,1.f)),-1.35f,1.35f); J.yaw=atan2f(-nf[0],-nf[2]);
    float L=KL*s*s*CL,D=(CD0+KI*CL*CL)*s*s; float ax=f[0]*thrust+up[0]*L-rt[0]*vr*2.5f,ay=f[1]*thrust+up[1]*L-rt[1]*vr*2.5f-G,az=f[2]*thrust+up[2]*L-rt[2]*vr*2.5f;
    if(s>0.5f){ ax-=J.vx/s*D; ay-=J.vy/s*D; az-=J.vz/s*D; } J.vx+=ax*dt; J.vy+=ay*dt; J.vz+=az*dt;
  }
  { float ex=fabsf(J.x)-w.m->W*0.5f,ez=fabsf(J.z)-w.m->D*0.5f; if(ex>0)J.vx-=(J.x>0?1.f:-1.f)*std::min(15.f,ex*0.3f)*dt; if(ez>0)J.vz-=(J.z>0?1.f:-1.f)*std::min(15.f,ez*0.3f)*dt; if(J.y>300.f)J.vy-=(J.y-300.f)*0.2f*dt+20.f*dt; }
  J.x+=J.vx*dt; J.y+=J.vy*dt; J.z+=J.vz*dt; float hx=w.m->W*0.5f+80.f,hz=w.m->D*0.5f+80.f; J.x=clampA(J.x,-hx,hx); J.z=clampA(J.z,-hz,hz);
  if(J.y<=0.f){ float vd=-J.vy; if(!J.gnd&&(vd>8.5f||fabsf(J.roll)>0.5f||(J.pitch<-0.35f&&s>30.f)))return true; J.y=0.f; if(J.vy<0)J.vy=0.f; J.gnd=true; if(!J.gnd)J.pitch=std::max(0.f,J.pitch); }
  else if(J.y>0.4f)J.gnd=false;
  if(J.y>260.f&&J.vy>0)J.vy-=20.f*dt;
  if(J.y<30.f){ const std::vector<int>&cl=w.cellAt(J.x,J.z); for(int k:cl){ const Box&b=w.boxes[k]; if(J.x>b.x0-2&&J.x<b.x1+2&&J.z>b.z0-2&&J.z<b.z1+2&&J.y<b.y1+1.2f&&b.y1>2.f)return true; } }
  return false;
}
inline void Game::jetGun(int j,int aim){
  Jet&J=jets[j]; if(J.cdG>0||J.gun<=0)return; J.cdG=0.045f; J.gun--; float cp=cosf(J.pitch);
  float dx=-sinf(J.yaw)*cp+(frand()-0.5f)*0.016f,dy=sinf(J.pitch)+(frand()-0.5f)*0.016f,dz=-cosf(J.yaw)*cp+(frand()-0.5f)*0.016f; float l=sqrtf(dx*dx+dy*dy+dz*dz); dx/=l;dy/=l;dz/=l;
  if(aim>=0&&aim<(int)jets.size()&&jets[aim].alive){ const Jet&T=jets[aim]; float ax=T.x+T.vx*0.25f-J.x,ay=T.y+T.vy*0.25f-J.y,az=T.z+T.vz*0.25f-J.z,al=sqrtf(ax*ax+ay*ay+az*az)+1e-4f; float er=diff==0?0.03f:diff==1?0.02f:0.012f; dx=ax/al+(frand()-0.5f)*er; dy=ay/al+(frand()-0.5f)*er; dz=az/al+(frand()-0.5f)*er; l=sqrtf(dx*dx+dy*dy+dz*dz); dx/=l;dy/=l;dz/=l; }
  float ox=J.x+dx*9.f,oy=J.y+dy*9.f-0.3f,oz=J.z+dz*9.f,best=700.f; int hj=-1;
  for(int k=0;k<(int)jets.size();k++){ const Jet&T=jets[k]; if(k==j||!T.alive||T.team==J.team)continue; float px=T.x-ox,py=T.y+1-oy,pz=T.z-oz,tc=px*dx+py*dy+pz*dz; if(tc<0||tc>=best)continue; float qx=px-dx*tc,qy=py-dy*tc,qz=pz-dz*tc; if(qx*qx+qy*qy+qz*qz<45.f){best=tc;hj=k;} }
  float tw=raycastMap(w,ox,oy,oz,dx,dy,dz,best); if(tw<best){best=tw;hj=-1;} if(oy+dy*best<0.f&&dy<0)best=std::max(1.f,-oy/dy);
  if(tr.size()<64)tr.push_back({ox,oy,oz,ox+dx*std::min(best,260.f),oy+dy*std::min(best,260.f),oz+dz*std::min(best,260.f),0.09f});
  if(hj>=0){ jetHurt(hj,16.f,J.pilot,true); sparkAt(ox+dx*best,oy+dy*best,oz+dz*best,dx,dy,dz,3); }
  if(J.pilot==meI||frand()<0.15f){ float dd=hypotf(J.x-e[meI].b.x,J.z-e[meI].b.z); SFX::shot(4,false,J.pilot==meI?0.8f:std::max(0.f,0.6f-dd/1200.f),0); }
}
// ذكاء الطيار: اختيار أقرب هدف، انحدار نحوه، تجنّب الأرض والحدود، إطلاق المدفع عند الاصطفاف
inline void Game::jetAI(int j,float dt){
  Jet&J=jets[j]; J.aiT-=dt; float skill=diff==0?0.7f:diff==1?0.85f:1.f,cone=diff==0?0.035f:diff==1?0.05f:0.07f;
  if(J.aiT<=0){ J.aiT=0.6f; bool keep=J.tgt>=0&&jets[J.tgt].alive&&jets[J.tgt].pilot>=0&&J.wob>0; J.wob-=0.6f; if(!keep){ J.wob=7.f; J.tgt=-1; float bs=1e9f; for(int k=0;k<(int)jets.size();k++){ const Jet&T=jets[k]; if(!T.alive||T.team==J.team||T.pilot<0)continue; float dx=T.x-J.x,dy=T.y-J.y,dz=T.z-J.z,d=sqrtf(dx*dx+dy*dy+dz*dz); if(d<bs){bs=d;J.tgt=k;} } } }
  float tx=0,ty=140.f,tz=0; bool have=J.tgt>=0&&jets[J.tgt].alive; float dist=1e9f,cosA=-1.f;
  float cp=cosf(J.pitch),fx=-sinf(J.yaw)*cp,fy=sinf(J.pitch),fz=-cosf(J.yaw)*cp;
  if(have){ const Jet&T=jets[J.tgt]; float lead=std::min(1.2f,hypotf(T.x-J.x,T.z-J.z)/300.f); tx=T.x+T.vx*lead; ty=T.y+T.vy*lead; tz=T.z+T.vz*lead; float dx=tx-J.x,dy=ty-J.y,dz=tz-J.z; dist=sqrtf(dx*dx+dy*dy+dz*dz); cosA=(dx*fx+dy*fy+dz*fz)/(dist+1e-4f); }
  float W2=w.m->W*0.5f-260.f,D2=w.m->D*0.5f-220.f; bool oob=fabsf(J.x)>W2||fabsf(J.z)>D2; if(oob||!have){ tx=oob?0.f:tx; tz=oob?0.f:tz; if(!have&&!oob){ tx=(J.team?-1:1)*200.f; tz=0; } ty=std::max(ty,130.f); }
  ty=std::max(ty,95.f); float dx=tx-J.x,dy=ty-J.y,dz=tz-J.z,hd=hypotf(dx,dz)+1e-3f; float ey=wrapPiA(atan2f(-dx,-dz)-J.yaw); float des=clampA(atan2f(dy,hd),-0.3f,0.35f);
  if(J.y>220.f)des=std::min(des,-0.15f); if(J.y<90.f)des=std::max(des,0.1f);
  bool turnM=fabsf(ey)>0.25f&&J.y>100.f; float rmax=J.y<100.f?0.5f:clampA(1.3f+J.vy*0.02f,0.8f,1.45f); if(fabsf(ey)>2.4f){ if(J.evade==0.f)J.evade=ey>0?1.f:-1.f; ey=J.evade*2.4f; } else J.evade=0.f; float roll=clampA(-ey*2.2f,-rmax,rmax); float rc=clampA((roll-J.roll)*2.5f,-1.f,1.f)*skill;
  float cr=std::max(0.4f,cosf(J.roll)); float pc=turnM?(fabsf(J.roll)>0.7f?1.f:0.4f):clampA((des-J.pitch)*2.2f+(1.f/cr-1.f)*0.6f,-1.f,1.f); pc*=skill; if(J.y>190.f)pc=std::min(pc,0.3f-(J.y-190.f)*0.02f); if(J.pitch<-0.45f)pc=1.f; if(J.pitch>0.5f)pc=std::min(pc,-0.1f);
  if(J.vy<0&&J.y<J.vy*J.vy/20.f+70.f&&!J.gnd){ pc=1.f; rc=clampA(-J.roll*2.f,-1.f,1.f); }
  float s=jetSpeed(J); float thd=s<105.f?1.f:s>135.f?-1.f:0.f; bool ab=have&&dist>700.f&&diff>0;
  if(J.stall&&J.y>120.f&&J.pitch>0.f){ pc=-0.4f; rc=0; thd=1.f; }
  if(J.y<60.f&&J.vy<-12.f){ J.vy*=1.f-3.f*dt; }
  bool dead=jetStep(J,dt,rc,pc,0.f,thd,ab); if(dead){ jetDie(j,-1); return; }
  if(J.y<24.f&&!J.gnd){ J.y=24.f; if(J.vy<0)J.vy=0.f; if(J.pitch<0)J.pitch*=0.5f; } /* مساعد تفادي الأرض للطيار الآلي */
  if(have&&dist<650.f&&cosA>0.8f&&frand()<(diff==0?0.5f:0.9f))jetGun(j,J.tgt);
}
inline void Game::airUpdate(float dt,const Input&in){
  Ent&me=e[meI]; if(jetMsgT>0)jetMsgT-=dt; bool triE=in.tri&&!airPrev.tri; airPrev=in;
  for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(p.prot>0)p.prot-=dt;
    if(!p.alive){ p.respawn-=dt; if(p.respawn<=0)airSpawn(i); continue; }
    int j=entJet[i];
    if(i==meI&&j<0){ // على الأرض: قاذفة RPG
      adsHeld=in.ads; updatePlayer(dt,in); footT+=dt;
      int best=-1; float bd=18.f; for(int k=0;k<(int)jets.size();k++){ const Jet&J=jets[k]; if(!J.alive||J.pilot>=0||J.team!=p.team||!J.gnd||jetSpeed(J)>8.f)continue; float d=hypotf(J.x-p.b.x,J.z-p.b.z); if(d<bd){bd=d;best=k;} }
      if(best>=0&&triE){ jets[best].pilot=i; entJet[i]=best; jets[best].th=0; jets[best].vx=jets[best].vz=0; footT=0; adsHeld=false; jetSay("ارفع الدفع (القفز +) ثم اسحب الذراع للإقلاع"); continue; }
      if(footT>10.f&&best<0){ int nj=jetNew(p.team,false,0); Jet&J=jets[nj]; J.x=p.b.x+cosf(p.yaw)*10.f; J.z=p.b.z-sinf(p.yaw)*10.f; J.y=0; J.yaw=p.yaw; footT=0; jetSay("وصلت طائرة بديلة بجانبك"); }
      continue; }
    if(j<0)continue;
    Jet&J=jets[j];
    if(i==meI){ adsHeld=false;
      if(triE){ bool onGnd=J.gnd&&jetSpeed(J)<10.f; float rx=cosf(J.yaw),rz=-sinf(J.yaw); J.pilot=-1; entJet[i]=-1; J.th=0;
        p.b=Body(); if(onGnd){ p.b.x=J.x+rx*9.f; p.b.z=J.z+rz*9.f; p.b.y=0.1f; J.vx=J.vz=0; jetSay("نزلت من الطائرة — لديك RPG بخمس طلقات"); } else { p.b.x=J.x; p.b.z=J.z; p.b.y=0.1f; jetSay("قفزت بالمظلة — الطائرة تحلّق بلا طيار"); }
        p.yaw=J.yaw; p.pitch=0; giveWeapon(p,rpgIdx); p.ammo=WEAPONS[rpgIdx].mag; p.mags=5; footT=0; continue; }
      float rc=in.mx,pc=in.my,yc=clampA(in.lx*0.4f,-1.f,1.f); float thd=(in.jump?1.f:0.f)-(in.crouch?1.f:0.f);
      bool dead=jetStep(J,dt,rc,pc,yc,thd,in.ads||in.sprint); if(dead){ jetDie(j,-1); continue; }
      if(in.fire)jetGun(j);
    } else jetAI(j,dt);
    if(!J.alive)continue;
    p.b.x=J.x; p.b.y=J.y; p.b.z=J.z; p.yaw=J.yaw; p.pitch=J.pitch; p.hp=J.hp/3.f; p.moving=false;
  }
  for(int j=0;j<(int)jets.size();j++){ Jet&J=jets[j];
    if(!J.alive){ if(J.spare&&J.respT>0){ J.respT-=dt; if(J.respT<=0){ int nj=jetNew(J.team,false,6+(j&1)); (void)nj; } } continue; }
    if(J.cdG>0)J.cdG-=dt; else if(J.gun<600)J.gunF+=dt*(J.pilot==meI?10.f:40.f); if(J.gunF>=1.f){ int add=(int)J.gunF; J.gun=std::min(600,J.gun+add); J.gunF-=add; }
    if(J.pilot<0&&!(J.gnd&&jetSpeed(J)<1.f)){ if(jetStep(J,dt,0,0,0,0,false)){ jetDie(j,-1); continue; } }
    if(J.fire>0){ J.fire-=dt; J.hp-=12.f*dt; if(frand()<dt*14.f)puffs.push_back({J.x+(frand()-0.5f)*2,J.y,J.z+(frand()-0.5f)*2,1.4f,1.0f,1.2f,frand()<0.4f}); if(J.hp<=0)jetDie(j,J.lastBy); }
  }
  for(auto&q:puffs){ q.t-=dt; q.y+=q.vy*dt; q.sz+=dt*0.8f; } puffs.erase(std::remove_if(puffs.begin(),puffs.end(),[](Puff&q){return q.t<=0;}),puffs.end());
  for(auto&o:e)if(o.revealT>0)o.revealT-=dt;
}
inline void Game::airCam(Cam&c){
  int j=myJet(); if(j<0||!e[meI].alive)return; const Jet&J=jets[j]; float cp=cosf(J.pitch),fx=-sinf(J.yaw)*cp,fy=sinf(J.pitch),fz=-cosf(J.yaw)*cp;
  c.x=J.x-fx*26.f; c.y=std::max(2.5f,J.y-fy*26.f+7.f); c.z=J.z-fz*26.f; c.yaw=J.yaw; c.pitch=J.pitch*0.9f-0.12f; c.fov=1.3f*fovMul;
}
inline bool Game::projP(float x,float y,float z,int&X,int&Y,float&zc){
  float ddx=x-r.cam.x,ddy=y-r.cam.y,ddz=z-r.cam.z; zc=ddx*r.fx+ddy*r.fy+ddz*r.fz; if(zc<1.f)return false; float xc=ddx*r.rx+ddz*r.rz,yc=ddx*r.ux+ddy*r.uy+ddz*r.uz;
  X=r.W/2+(int)(xc/zc*r.focal); Y=r.H/2-(int)(yc/zc*r.focal); return true;
}
inline void Game::airDraw(const Cam&c){
  struct Pt{ float x,y,z,hx,hy,hz; int col; };
  static const Pt P[]={ {0,0,0.5f,0.95f,0.8f,4.8f,0},{0,0,-5.6f,0.75f,0.62f,1.4f,0},{0,-0.05f,-7.3f,0.4f,0.35f,0.7f,0},{0,0.85f,-2.6f,0.5f,0.38f,1.5f,2},
    {-1.0f,-0.1f,-2.2f,0.4f,0.5f,1.5f,0},{1.0f,-0.1f,-2.2f,0.4f,0.5f,1.5f,0},{-2.9f,-0.2f,1.4f,1.9f,0.07f,2.1f,0},{2.9f,-0.2f,1.4f,1.9f,0.07f,2.1f,0},{-5.0f,-0.2f,2.6f,1.0f,0.06f,1.2f,0},{5.0f,-0.2f,2.6f,1.0f,0.06f,1.2f,0},
    {-2.2f,-0.1f,5.9f,1.3f,0.06f,0.9f,0},{2.2f,-0.1f,5.9f,1.3f,0.06f,0.9f,0},{-1.0f,1.5f,5.1f,0.06f,1.1f,1.1f,1},{1.0f,1.5f,5.1f,0.06f,1.1f,1.1f,1},{0,0,6.2f,0.6f,0.55f,0.5f,3} };
  for(int j=0;j<(int)jets.size();j++){ const Jet&J=jets[j]; if(!J.alive)continue; float dx=J.x-c.x,dz=J.z-c.z; if(dx*dx+dz*dz>r.fogF*r.fogF)continue;
    float cr=cosf(J.roll),sr=sinf(J.roll); r.obbRoll=J.roll; float tc[3]={J.team?0.2f:0.85f,J.team?0.4f:0.18f,J.team?0.9f:0.18f};
    for(const Pt&p:P){ float lx=p.x*cr-p.y*sr,ly=p.x*sr+p.y*cr; float o[3]; Renderer::rot(J.yaw,J.pitch,lx,ly,p.z,o); static const float CG[3]={0.48f,0.52f,0.57f},CC[3]={0.12f,0.2f,0.3f},CE[3]={0.1f,0.1f,0.11f}; const float*col=p.col==1?tc:p.col==2?CC:p.col==3?CE:CG;
      r.obb(J.x+o[0],J.y+o[1],J.z+o[2],p.hx,p.hy,p.hz,J.yaw,J.pitch,col[0],col[1],col[2]); }
    if(J.ab>0.3f){ float o[3]; Renderer::rot(J.yaw,J.pitch,0,0,7.2f,o); r.obb(J.x+o[0],J.y+o[1],J.z+o[2],0.35f,0.35f,0.9f*J.ab,J.yaw,J.pitch,1.f,0.6f,0.15f,false,false); }
    r.obbRoll=0; }
}
inline void Game::airHud(int sc){
  int W=r.W,H=r.H; Ent&me=e[meI]; int sm=std::max(1,sc-1); char b[64]; int j=myJet();
  if(jetMsgT>0)textC(W/2,H/3+14*sc,jetMsg,sc,C_ACC);
  if(!me.alive||over)return;
  if(j<0){ for(int k=0;k<(int)jets.size();k++){ const Jet&J=jets[k]; if(!J.alive||J.pilot>=0||J.team!=me.team||!J.gnd)continue; float d=hypotf(J.x-me.b.x,J.z-me.b.z); if(d<18.f&&jetSpeed(J)<8.f){ textC(W/2,H*2/3,"Triangle: ركوب الطائرة",sc,C_ACC); break; } } return; }
  const Jet&J=jets[j]; float s=jetSpeed(J);
  for(int k=0;k<(int)jets.size();k++){ const Jet&T=jets[k]; if(!T.alive||k==j)continue; int X,Y; float zc; if(!projP(T.x,T.y,T.z,X,Y,zc)||X<10||X>W-10||Y<10||Y>H-10)continue; bool en=T.team!=J.team; uint32_t col=en?0xFFFF4A4A:0xFF60E080; int h2=std::max(3*sc,(int)(28.f*r.focal/zc/3.f));
    h2=std::min(h2,14*sc); rect(X-h2,Y-h2,h2*2,sc,col,0.9f); rect(X-h2,Y+h2,h2*2,sc,col,0.9f); rect(X-h2,Y-h2,sc,h2*2,col,0.9f); rect(X+h2,Y-h2,sc,h2*2+sc,col,0.9f);
    if(en){ snprintf(b,64,"%dm",(int)zc); textC(X,Y+h2+2*sc,b,sm,col); } }
  { int X,Y; float zc; if(s>5.f&&projP(J.x+J.vx*3.f,J.y+J.vy*3.f,J.z+J.vz*3.f,X,Y,zc)){ rect(X-3*sc,Y-sc/2,2*sc,sc,0xFF80FF80); rect(X+sc,Y-sc/2,2*sc,sc,0xFF80FF80); rect(X-sc/2,Y-3*sc,sc,2*sc,0xFF80FF80); rect(X-sc/2-sc,Y-sc,3*sc,3*sc,0xFF80FF80,0.25f); } }
  int px=W/2-60*sc,py=H-34*sc; rrect(px,py,120*sc,26*sc,8*sc,0xFF0A0E14,0.78f);
  snprintf(b,64,"%d",(int)(s*3.6f)); text(px+8*sc,py+5*sc,b,sc*2,0xFFFFFFFF); text(px+8*sc,py+19*sc,"km/h",sm,C_MUT); snprintf(b,64,"%d",(int)J.y); text(px+70*sc,py+5*sc,b,sc*2,0xFFFFFFFF); text(px+70*sc,py+19*sc,"m",sm,C_MUT);
  rrect(px,py-9*sc,120*sc,6*sc,3*sc,0xFF0A0E14,0.78f); rect(px+2*sc,py-8*sc,(int)((120*sc-4*sc)*J.th),4*sc,J.ab>0.5f?0xFFFF8A20:C_ACC); text(px+2*sc,py-20*sc,"دفع",sm,C_MUT);
  snprintf(b,64,"HEI %d",J.gun); text(px+120*sc+6*sc,py+8*sc,b,sc,J.gun>60?C_TXT:0xFFFF6060);
  if(J.stall)textC(W/2,H/2-34*sc,"STALL",sc*2,0xFFFF4040); else if(J.y<45.f&&J.vy<-4.f&&!J.gnd)textC(W/2,H/2-34*sc,"PULL UP",sc*2,0xFFFF4040);
  if(fabsf(J.x)>w.m->W*0.5f||fabsf(J.z)>w.m->D*0.5f)textC(W/2,H/2-50*sc,"عد إلى منطقة القتال",sc,0xFFFFC040);
  if(J.gnd&&s<10.f)textC(W/2,H*2/3,"Triangle: النزول من الطائرة",sm,C_MUT);
}
