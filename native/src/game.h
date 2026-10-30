// لعبة Warzone الأصلية: لاعب + بوتات + أسلحة + تسجيل نقاط + واجهة
#pragma once
#include "render.h"
#include "font.h"
#include <string>
#include <cstdlib>

struct Input{
  float mx=0,my=0;      // حركة: x يمين، y أمام
  float lx=0,ly=0;      // نظر: x يمين، y أعلى (معدّل)
  bool fire=false,ads=false,jump=false,crouch=false,reload=false,sprint=false,swapP=false,swapN=false,grenade=false,start=false,confirm=false,back=false;
};

static float frand(){return (float)rand()/(float)RAND_MAX;}

struct Ent{
  Body b; float yaw=0,pitch=0; int team=0; bool bot=true,alive=false,crouch=false,sprint=false,moving=false;
  float hp=100,prot=0,respawn=0,phase=0,flash=0,cd=0,reloadT=-1,rkP=0,rkY=0;
  int wpn=0,ammo=0,mags=0,kills=0,deaths=0;
  float think=0,tYaw=0,sx=0,sz=0,stuck=0,turnT=0,turnDir=1,burst=0,react=0,strafe=1,strafeT=0; int target=-1; float lastX=0,lastZ=0,lastDmgT=0; int lastHitBy=-1;
  char name[16]={0};
};
struct Tracer{float x0,y0,z0,x1,y1,z1,t;};

struct Game{
  World w; Renderer r; std::vector<Ent> e; std::vector<Tracer> tr;
  int mapIdx=0,scoreLim=30,score[2]={0,0},nBots=7,diff=1; float t=0,hitMark=0,dmgFlash=0; bool over=false; int winner=0;
  int prim=0; Input prev; bool paused=false; float msgT=0; char msg[64]={0}; int shotsFeed[4]={-1,-1,-1,-1}; std::string feed[4]; float feedT[4]={};
  int menu=1,menuSel=0; // 1 = قائمة البداية
  void init(int rw,int rh){ r.init(rw,rh); startMatch(mapIdx); }
  void say(const char*s){snprintf(msg,64,"%s",s);msgT=2.5f;}
  void addFeed(const std::string&s){for(int i=3;i>0;i--){feed[i]=feed[i-1];feedT[i]=feedT[i-1];}feed[0]=s;feedT[0]=5;}
  void startMatch(int idx){
    mapIdx=idx%MAP_COUNT; w.load(mapIdx); e.assign(1+nBots*2+ (0),Ent()); // لاعب + فريقان
    int total=1+nBots*2-1; e.assign(total,Ent());
    for(int i=0;i<total;i++){Ent&p=e[i];p.bot=i>0;p.team=(i==0)?0:((i&1)?1:0);snprintf(p.name,16,i==0?"YOU":"BOT%d",i);p.wpn=i==0?prim:rand()%WEAPON_COUNT;}
    score[0]=score[1]=0;over=false;t=0;tr.clear();
    for(auto&p:e)spawn(p);
  }
  void giveWeapon(Ent&p,int wi){p.wpn=wi;p.ammo=WEAPONS[wi].mag;p.mags=WEAPONS[wi].mags;p.reloadT=-1;p.cd=0;}
  void spawn(Ent&p){
    const P2*sp=p.team?w.m->blue:w.m->red; int n=p.team?w.m->nbl:w.m->nr; P2 s=sp[rand()%n];
    // اختر نقطة بعيدة عن الأعداء
    float bestD=-1;for(int k=0;k<4;k++){P2 c=sp[rand()%n];float md=1e9;for(auto&o:e)if(o.alive&&o.team!=p.team){md=std::min(md,(o.b.x-c.x)*(o.b.x-c.x)+(o.b.z-c.z)*(o.b.z-c.z));}if(md>bestD){bestD=md;s=c;}}
    p.b=Body();p.b.x=s.x;p.b.z=s.z;p.b.y=20;for(int i=0;i<120;i++){moveBody(w,p.b,0,0,0.04f,false,false);if(p.b.onGround)break;}
    p.yaw=p.team?1.57f:-1.57f; if(w.m->W<w.m->D*0.0)p.yaw=0; p.pitch=0;
    // الاتجاه نحو وسط الخريطة
    p.yaw=std::atan2(-(0-p.b.x),-(0-p.b.z)); p.alive=true;p.hp=100;p.prot=2;p.crouch=false;giveWeapon(p,p.bot?p.wpn:prim);
  }
  static float dmgAt(const WeaponDef&d,float dist){ if(dist<=d.r0)return d.dmg; if(dist>=d.r1)return d.dmg*d.minMul; return d.dmg*(1-(1-d.minMul)*(dist-d.r0)/(d.r1-d.r0)); }
  bool visible(const Ent&a,const Ent&b2){
    float ax=a.b.x,ay=a.b.y+PK.eye,az=a.b.z,bx=b2.b.x,by=b2.b.y+1.2f,bz=b2.b.z;
    float dx=bx-ax,dy=by-ay,dz=bz-az,d=std::sqrt(dx*dx+dy*dy+dz*dz); if(d<0.01f)return true;
    return raycastMap(w,ax,ay,az,dx/d,dy/d,dz/d,d)>=1e29f;
  }
  void hurt(int vi,int ai,float dmg,bool head){
    Ent&v=e[vi]; if(!v.alive||v.prot>0)return;
    v.hp-=dmg; v.lastHitBy=ai; v.lastDmgT=t; if(vi==0)dmgFlash=0.5f;
    if(ai==0)hitMark=0.25f;
    if(v.hp<=0){ v.alive=false;v.deaths++;v.respawn=4;
      if(ai>=0&&ai!=vi){Ent&k=e[ai];k.kills++;score[k.team]++; char b[64];snprintf(b,64,"%s > %s%s",k.name,v.name,head?" HS":"");addFeed(b);} 
      if(score[0]>=scoreLim||score[1]>=scoreLim){over=true;winner=score[0]>=scoreLim?0:1;}
    }
  }
  void fire(int i){
    Ent&p=e[i]; const WeaponDef&d=WEAPONS[p.wpn];
    p.ammo--; p.cd=60.f/d.rpm; p.flash=0.06f; p.prot=0;
    float moveF=p.moving?1.6f:1.f; bool ads=(i==0)&&adsHeld;
    float sp=(ads?d.ads:d.hip)*moveF; if(p.crouch)sp*=0.7f;
    float eyeY=p.b.y+(p.crouch?PK.eyec:PK.eye);
    for(int k=0;k<d.pellets;k++){
      float a=sp; if(p.bot){a=(0.022f+0.05f*0.5f)*(diff==0?1.7f:diff==1?1.f:0.55f)*0.8f;}
      float yw=p.yaw+(frand()-0.5f)*2*a, pt=p.pitch+(frand()-0.5f)*2*a; if(d.pellets>1){yw+=(frand()-0.5f)*0.09f;pt+=(frand()-0.5f)*0.09f;}
      float cp=std::cos(pt); float dx=-std::sin(yw)*cp,dy=std::sin(pt),dz=-std::cos(yw)*cp;
      float tw=raycastMap(w,p.b.x,eyeY,p.b.z,dx,dy,dz,d.r1*1.4f); float best=std::min(tw,d.r1*1.4f); int hv=-1; bool hh=false;
      for(int j=0;j<(int)e.size();j++){ if(j==i||!e[j].alive||e[j].team==p.team)continue;
        RayHit h=rayPlayer(e[j].b.x,e[j].b.y,e[j].b.z,e[j].crouch,p.b.x,eyeY,p.b.z,dx,dy,dz,best); if(h.ok&&h.t<best){best=h.t;hv=j;hh=h.head;} }
      if(hv>=0)hurt(hv,i,dmgAt(d,best)*(hh?d.head:1.f),hh);
      if(tr.size()<64)tr.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best,0.07f});
    }
    if(i==0){ p.rkP+=d.recoilV*(ads?0.6f:1.f)*1.6f; p.rkY+=(frand()-0.5f)*d.recoilH*3.f; }
  }
  bool adsHeld=false;
  void reloadStart(Ent&p){const WeaponDef&d=WEAPONS[p.wpn]; if(p.reloadT>=0||p.ammo>=d.mag||p.mags<=0)return; p.reloadT=0;}
  void updateWeapon(int i,float dt,bool trig){
    Ent&p=e[i]; const WeaponDef&d=WEAPONS[p.wpn]; if(p.cd>0)p.cd-=dt; if(p.flash>0)p.flash-=dt;
    if(p.reloadT>=0){p.reloadT+=dt; if(p.reloadT>=d.reload){p.reloadT=-1;p.mags--;p.ammo=d.mag;} return;}
    if(trig&&p.cd<=0){ if(p.ammo>0)fire(i); else reloadStart(p); }
    if(p.ammo==0&&p.mags<=0&&i!=0){p.mags=d.mags;}
  }
  void botThink(int i,float dt){
    Ent&p=e[i]; p.think-=dt;
    if(p.think<=0){ p.think=0.25f+frand()*0.25f; int best=-1;float bd=1e9; bool vis=false;
      for(int j=0;j<(int)e.size();j++){ if(!e[j].alive||e[j].team==p.team)continue;
        float dx=e[j].b.x-p.b.x,dz=e[j].b.z-p.b.z,dd=dx*dx+dz*dz; if(dd<bd){ if(visible(p,e[j])||best<0){best=j;bd=dd;vis=visible(p,e[j]);} } }
      if(best!=p.target){p.react=(diff==0?0.5f:diff==1?0.3f:0.15f);} p.target=best; p.sx=vis?1.f:0.f; p.sz=std::sqrt(bd);
    }
  }
  void updateBot(int i,float dt){
    Ent&p=e[i]; botThink(i,dt); p.react-=dt; p.strafeT-=dt; if(p.strafeT<=0){p.strafeT=0.8f+frand()*1.5f;p.strafe=frand()<0.5f?-1.f:1.f;}
    float mx=0,mz=0; bool trig=false; p.sprint=false;
    if(p.target>=0&&e[p.target].alive){ Ent&t2=e[p.target];
      float dx=t2.b.x-p.b.x,dz=t2.b.z-p.b.z,d=std::sqrt(dx*dx+dz*dz); float want=std::atan2(-dx,-dz);
      float er=WEAPONS[p.wpn].r0>1?(p.wpn==2?9.f:p.wpn==1?14.f:22.f):22.f; bool vis=p.sx>0.5f;
      float dy=want-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*std::min(1.f,dt*(vis?10.f:5.f));
      // ارتفاع الهدف
      float pitchW=std::atan2((t2.b.y+1.3f)-(p.b.y+PK.eye),std::max(1.f,d)); p.pitch+=(pitchW-p.pitch)*std::min(1.f,dt*8);
      if(vis&&d<er){ // اشتباك: تحرّك جانبياً
        mx=p.strafe*0.6f; mz=d<er*0.4f?0.4f:(d>er*0.8f?-0.4f:0);
        if(p.react<=0&&std::fabs(dy)<0.25f&&d<WEAPONS[p.wpn].r1*1.1f){ p.burst-=dt; if(p.burst<=-0.35f)p.burst=0.35f+frand()*0.5f; trig=p.burst>0; if(trig&&p.burst<=0)trig=false; }
      } else { mz=-1; p.sprint=!vis&&d>20; }
    } else mz=-0.5f;
    // تفادي العوائق
    float fxw=-std::sin(p.yaw),fzw=-std::cos(p.yaw),rxw=std::cos(p.yaw),rzw=-std::sin(p.yaw);
    float vx=(-mz)*fxw+mx*rxw,vz=(-mz)*fzw+mx*rzw;
    float sp=(p.sprint?PK.sprint:PK.walk)*(trig?0.6f:1.f)*WEAPONS[p.wpn].move; float vl=std::sqrt(vx*vx+vz*vz);
    if(vl>0.01f){vx=vx/vl*sp;vz=vz/vl*sp;}
    if(p.turnT>0){p.turnT-=dt; float c=std::cos(p.turnDir*1.2f),s=std::sin(p.turnDir*1.2f); float nx=vx*c-vz*s,nz=vx*s+vz*c; vx=nx;vz=nz;}
    float la=0.9f; bool jump=false;
    if(vl>0.01f&&blocked(w,p.b.x+vx/sp*la,p.b.y,p.b.z+vz/sp*la,PK.h)){ if(p.turnT<=0){p.turnT=0.5f+frand()*0.6f;p.turnDir=frand()<0.5f?-1.f:1.f;} jump=frand()<0.03f; }
    float ox=p.b.x,oz=p.b.z; moveBody(w,p.b,vx,vz,dt,jump,false);
    float mv=std::hypot(p.b.x-ox,p.b.z-oz); p.moving=mv>0.001f; p.phase+=mv*3.4f;
    if(vl>0.01f&&mv<sp*dt*0.3f){p.stuck+=dt; if(p.stuck>0.6f){p.turnT=0.8f+frand();p.turnDir=frand()<0.5f?-1.f:1.f;p.stuck=0;}} else p.stuck=0;
    updateWeapon(i,dt,trig); if(p.ammo<=WEAPONS[p.wpn].mag/6&&!trig)reloadStart(p);
  }
  // ---- اللاعب ----
  void updatePlayer(float dt,const Input&in){
    Ent&p=e[0]; adsHeld=in.ads;
    float zoom=in.ads?WEAPONS[p.wpn].zoom:1.f; float sens=1.f/ (in.ads?std::max(1.f,zoom*0.8f):1.f);
    p.yaw-=in.lx*dt*sens; p.pitch+=in.ly*dt*sens; p.yaw+=p.rkY*std::min(1.f,dt*30); p.pitch+=p.rkP*std::min(1.f,dt*30); p.rkP*=0.8f; p.rkY*=0.8f;
    p.pitch=std::max(-1.45f,std::min(1.45f,p.pitch));
    bool wantCrouch=in.crouch; if(!wantCrouch&&p.crouch&&blocked(w,p.b.x,p.b.y,p.b.z,PK.h))wantCrouch=true; p.crouch=wantCrouch;
    float mx=in.mx,mz=in.my; float ml=std::hypot(mx,mz); if(ml>1){mx/=ml;mz/=ml;}
    p.sprint=in.sprint&&mz>0.3f&&!in.ads&&!p.crouch;
    float sp=(p.crouch?PK.crouch:(p.sprint?PK.sprint:PK.walk))*WEAPONS[p.wpn].move*(in.ads?0.65f:1.f);
    float fxw=-std::sin(p.yaw),fzw=-std::cos(p.yaw),rxw=std::cos(p.yaw),rzw=-std::sin(p.yaw);
    float vx=(mz*fxw+mx*rxw)*sp,vz=(mz*fzw+mx*rzw)*sp; float ox=p.b.x,oz=p.b.z;
    moveBody(w,p.b,vx,vz,dt,in.jump,p.crouch); float mv=std::hypot(p.b.x-ox,p.b.z-oz); p.moving=mv>0.001f&&p.b.onGround; p.phase+=mv*3.4f;
    if(in.reload)reloadStart(p);
    if((in.swapP&&!prev.swapP)||(in.swapN&&!prev.swapN)){ prim=(prim+(in.swapN&&!prev.swapN?1:WEAPON_COUNT-1))%WEAPON_COUNT; giveWeapon(p,prim); }
    bool trig=in.fire&&!p.sprint; if(!WEAPONS[p.wpn].autof&&prev.fire)trig=false;
    updateWeapon(0,dt,trig);
  }
  void update(float dt,const Input&in){
    if(menu){ // قائمة بسيطة
      if(in.mx>0.5f&&prev.mx<=0.5f)menuSel=(menuSel+1)%MAP_COUNT; if(in.mx<-0.5f&&prev.mx>=-0.5f)menuSel=(menuSel+MAP_COUNT-1)%MAP_COUNT;
      if((in.confirm&&!prev.confirm)||(in.start&&!prev.start)){menu=0;startMatch(menuSel);} prev=in; return; }
    if(in.start&&!prev.start)paused=!paused;
    if(paused){ if(in.back&&!prev.back){menu=1;paused=false;} prev=in; return; }
    if(over){ if(in.confirm&&!prev.confirm){startMatch(mapIdx);} prev=in; return; }
    t+=dt; if(msgT>0)msgT-=dt; if(hitMark>0)hitMark-=dt; if(dmgFlash>0)dmgFlash-=dt;
    for(auto&q:tr)q.t-=dt; tr.erase(std::remove_if(tr.begin(),tr.end(),[](Tracer&q){return q.t<=0;}),tr.end());
    for(int k=0;k<4;k++)if(feedT[k]>0)feedT[k]-=dt;
    for(int i=0;i<(int)e.size();i++){ Ent&p=e[i];
      if(p.prot>0)p.prot-=dt;
      if(!p.alive){ p.respawn-=dt; if(p.respawn<=0){spawn(p);} continue; }
      if(i==0)updatePlayer(dt,in); else updateBot(i,dt);
    }
    prev=in;
  }
  // ---- الرسم ----
  void drawAvatar(const Ent&a){
    float tc[2][3]={{0.82f,0.14f,0.14f},{0.15f,0.36f,0.9f}}; const float*c=tc[a.team]; float sw=std::sin(a.phase)*0.6f*(a.moving?1:0); float cs=a.crouch?0.69f:1.f;
    float x=a.b.x,y=a.b.y,z=a.b.z,yw=a.yaw;
    auto part=[&](float lx,float ly,float lz,float hx,float hy,float hz,float pitch,float R,float G,float B){float o[3];Renderer::rot(yw,0,lx,ly*cs,lz,o);r.obb(x+o[0],y+o[1],z+o[2],hx,hy*cs,hz,yw,pitch,R,G,B);};
    if(!a.alive){ r.obb(x,y+0.15f,z,0.25f,0.15f,0.9f,yw,0,c[0]*0.5f,c[1]*0.5f,c[2]*0.5f); return; }
    part(-0.12f,0.4f,sw*0.15f,0.1f,0.4f,0.11f,0,0.17f,0.18f,0.15f); part(0.12f,0.4f,-sw*0.15f,0.1f,0.4f,0.11f,0,0.17f,0.18f,0.15f);
    part(0,1.1f,0,0.22f,0.3f,0.13f,0,c[0],c[1],c[2]); part(0,1.12f,0,0.25f,0.23f,0.15f,0,0.2f,0.22f,0.2f);
    part(0,1.58f,0,0.11f,0.12f,0.11f,0,0.86f,0.68f,0.55f); part(0,1.66f,0,0.17f,0.08f,0.17f,0,c[0]*0.5f,c[1]*0.5f,c[2]*0.5f);
    part(-0.28f,1.1f,-0.1f,0.06f,0.25f,0.06f,0,c[0]*0.6f,c[1]*0.6f,c[2]*0.6f); part(0.28f,1.1f,-0.2f,0.06f,0.06f,0.28f,0,c[0]*0.6f,c[1]*0.6f,c[2]*0.6f);
    part(0.2f,1.15f,-0.45f,0.03f,0.04f,0.35f,0,0.1f,0.1f,0.12f);
  }
  void render(){
    Ent&me=e[0]; float eyeH=me.alive?(me.b.y+(me.crouch?PK.eyec:PK.eye)):me.b.y+0.6f;
    float fov=1.3f; if(adsHeld&&me.alive){fov=1.3f/WEAPONS[me.wpn].zoom;}
    Cam c{me.b.x,eyeH,me.b.z,me.yaw,me.pitch,fov}; r.setCam(c); r.world(w);
    for(int i=1;i<(int)e.size();i++){const Ent&a=e[i];float dx=a.b.x-c.x,dz=a.b.z-c.z; if(dx*dx+dz*dz>r.fogF*r.fogF)continue; drawAvatar(a);}
    for(auto&q:tr){ float dx=q.x1-q.x0,dy=q.y1-q.y0,dz=q.z1-q.z0,l=std::sqrt(dx*dx+dy*dy+dz*dz); if(l<0.1f)continue;
      float yw=std::atan2(-dx,-dz),pt=std::asin(dy/l); float cx=(q.x0+q.x1)/2,cy=(q.y0+q.y1)/2,cz=(q.z0+q.z1)/2; r.obb(cx,cy,cz,0.012f,0.012f,std::min(l,6.f)/2,yw,pt,1,0.9f,0.5f,false); }
    // مجسّم السلاح
    if(me.alive){ std::fill(r.zb.begin(),r.zb.end(),0.f); float k=adsHeld?1.f:0.f; float hx=0.2f*(1-k),hy=-0.2f+(-0.0f)*k-0.02f*k,hz=-0.45f;
      float kick=me.flash>0?0.03f:0.f; float rl=me.reloadT>=0?std::sin(me.reloadT/WEAPONS[me.wpn].reload*3.14159f):0; hy-=rl*0.25f;
      float O[3];Renderer::rot(me.yaw,me.pitch,hx,hy,hz+kick,O); float ox=c.x+O[0],oy=c.y+O[1],oz=c.z+O[2];
      float len=0.18f+(me.wpn==3||me.wpn==5?0.18f:0)+(me.wpn==4?0.1f:0); if(me.wpn>=6)len=0.1f;
      auto gp=[&](float lx,float ly,float lz,float sx,float sy,float sz,float R,float G,float B){float o[3];Renderer::rot(me.yaw,me.pitch,lx,ly,lz,o);r.obb(ox+o[0],oy+o[1],oz+o[2],sx,sy,sz,me.yaw,me.pitch,R,G,B,false);};
      gp(0,0,-len*0.3f,0.025f,0.035f,len,0.14f,0.14f,0.16f); gp(0,-0.06f,0.08f,0.015f,0.05f,0.02f,0.1f,0.1f,0.1f); gp(0,0,len*0.3f+0.18f,0.02f,0.04f,0.1f,0.4f,0.28f,0.14f);
      float sl[3]={0.15f,0.36f,0.9f}; if(!me.team){sl[0]=0.5f;sl[1]=0.08f;sl[2]=0.08f;} gp(0.03f,-0.1f,0.22f,0.035f,0.04f,0.16f,sl[0],sl[1],sl[2]); gp(-0.03f,-0.05f,-0.08f,0.03f,0.03f,0.05f,0.86f,0.68f,0.55f);
      if(me.flash>0)gp(0,0,-len*1.3f,0.04f,0.04f,0.05f,1,0.8f,0.3f);
    }
    hud();
  }
  // ---- واجهة ----
  void px(int x,int y,uint32_t c){ if(x>=0&&y>=0&&x<r.W&&y<r.H)r.fb[y*r.W+x]=c; }
  void rect(int x,int y,int w2,int h2,uint32_t c,float a=1){ for(int j=std::max(0,y);j<std::min(r.H,y+h2);j++)for(int i=std::max(0,x);i<std::min(r.W,x+w2);i++){ if(a>=1)r.fb[j*r.W+i]=c; else { uint32_t o=r.fb[j*r.W+i]; auto m=[&](int sh){int A=(o>>sh)&255,B=(c>>sh)&255;return (uint32_t)(A+(B-A)*a)<<sh;}; r.fb[j*r.W+i]=0xFF000000u|m(16)|m(8)|m(0);} } }
  void text(int x,int y,const char*s,int sc,uint32_t col){ int cx=x; for(;*s;s++){ if(*s=='\n'){cx=x;y+=8*sc;continue;} const uint8_t*g=glyphOf(*s); if(g)for(int j=0;j<7;j++)for(int i=0;i<5;i++)if(g[j]&(16>>i))rect(cx+i*sc,y+j*sc,sc,sc,col); cx+=6*sc; } }
  int tw(const char*s,int sc){return (int)strlen(s)*6*sc;}
  void textC(int cx,int y,const char*s,int sc,uint32_t col){text(cx-tw(s,sc)/2,y,s,sc,col);}
  void hud(){
    int W=r.W,H=r.H,sc=std::max(1,H/270); Ent&me=e[0]; const WeaponDef&d=WEAPONS[me.wpn];
    if(dmgFlash>0)rect(0,0,W,H,0xFFB01010,dmgFlash*0.9f);
    if(!me.alive){ rect(0,0,W,H,0xFF300000,0.5f); char b[32];snprintf(b,32,"RESPAWN %d",(int)std::ceil(me.respawn));textC(W/2,H/2-4*sc,b,sc*2,0xFFFFFFFF); }
    else { int cx=W/2,cy=H/2; uint32_t cc=hitMark>0?0xFFFF4040:0xFFFFFFFF; int g=3*sc; rect(cx-g-4*sc,cy,4*sc,sc,cc);rect(cx+g,cy,4*sc,sc,cc);rect(cx,cy-g-4*sc,sc,4*sc,cc);rect(cx,cy+g,sc,4*sc,cc); if(hitMark>0){rect(cx-6*sc,cy-6*sc,2*sc,2*sc,0xFFFF4040);rect(cx+5*sc,cy+5*sc,2*sc,2*sc,0xFFFF4040);rect(cx-6*sc,cy+5*sc,2*sc,2*sc,0xFFFF4040);rect(cx+5*sc,cy-6*sc,2*sc,2*sc,0xFFFF4040);} }
    // الصحة
    rect(8*sc,H-14*sc,60*sc,6*sc,0xFF000000,0.5f); rect(8*sc,H-14*sc,(int)(60*sc*std::max(0.f,me.hp)/100),6*sc,me.hp>35?0xFF40D060:0xFFE04030); char b[48]; snprintf(b,48,"%d",(int)std::max(0.f,me.hp)); text(10*sc,H-13*sc,b,sc,0xFFFFFFFF);
    snprintf(b,48,"%d / %d",me.ammo,me.mags*d.mag); text(W-tw(b,sc*2)-8*sc,H-16*sc,b,sc*2,me.reloadT>=0?0xFFFFC040:0xFFFFFFFF); text(W-tw(d.id,sc)-8*sc,H-26*sc,d.id,sc,0xFFB0B0B0);
    if(me.reloadT>=0)textC(W/2,H/2+20*sc,"RELOADING",sc,0xFFFFC040);
    // النتيجة
    snprintf(b,48,"RED %d",score[0]);text(W/2-tw(b,sc*2)-10*sc,4*sc,b,sc*2,0xFFFF6060); snprintf(b,48,"%d BLUE",score[1]);text(W/2+10*sc,4*sc,b,sc*2,0xFF6090FF);
    for(int k=0;k<4;k++)if(feedT[k]>0)text(W-tw(feed[k].c_str(),sc)-6*sc,(4+k*9)*sc,feed[k].c_str(),sc,0xFFFFFFFF);
    // رادار
    int rs=34*sc,rx0=6*sc,ry0=6*sc; rect(rx0,ry0,rs*2,rs*2,0xFF000000,0.4f);
    for(auto&o:e){ if(!o.alive)continue; float dx=o.b.x-me.b.x,dz=o.b.z-me.b.z; float fxw=-std::sin(me.yaw),fzw=-std::cos(me.yaw),rxw=std::cos(me.yaw),rzw=-std::sin(me.yaw);
      float lr=dx*rxw+dz*rzw,lf=dx*fxw+dz*fzw; float sc2=rs/40.f; int X=rx0+rs+(int)(lr*sc2),Y=ry0+rs-(int)(lf*sc2); if(std::fabs(lr)>38||std::fabs(lf)>38)continue; rect(X-sc,Y-sc,2*sc,2*sc,o.team?0xFF5080FF:0xFFFF5050); }
    rect(rx0+rs-sc,ry0+rs-sc,2*sc,2*sc,0xFFFFFFFF);
    if(msgT>0)textC(W/2,H/3,msg,sc*2,0xFFFFFFFF);
    if(over){ rect(0,0,W,H,0xFF000000,0.55f); textC(W/2,H/2-20*sc,winner==0?"RED TEAM WINS":"BLUE TEAM WINS",sc*3,winner==0?0xFFFF6060:0xFF6090FF); textC(W/2,H/2+10*sc,"X = PLAY AGAIN",sc*2,0xFFFFFFFF); }
    if(paused){ rect(0,0,W,H,0xFF000000,0.6f); textC(W/2,H/2-12*sc,"PAUSED",sc*3,0xFFFFFFFF); textC(W/2,H/2+14*sc,"OPTIONS = RESUME   O = MENU",sc*2,0xFFB0B0B0); }
  }
  void renderMenu(){
    Cam c{0,6,0,t*0.2f,-0.15f,1.2f}; t+=0.016f; r.setCam(c); // خلفية: الخريطة المختارة
    if(w.id!=menuSel){w.load(menuSel);} r.setCam(c); r.world(w); int W=r.W,H=r.H,sc=std::max(1,H/270); rect(0,0,W,H,0xFF000000,0.5f);
    textC(W/2,H/5,"WARZONE",sc*6,0xFFFF5040); textC(W/2,H/5+50*sc,"NATIVE PS4",sc*2,0xFFB0B0B0);
    char b[64]; snprintf(b,64,"< MAP %d / %d >",menuSel+1,MAP_COUNT); textC(W/2,H/2,b,sc*3,0xFFFFFFFF);
    textC(W/2,H/2+34*sc,"X = START   OPTIONS = PAUSE   DPAD = SELECT",sc,0xFFB0B0B0);
    textC(W/2,H/2+48*sc,"L STICK MOVE  R STICK LOOK  R2 FIRE  L2 AIM  X JUMP  O CROUCH  SQ RELOAD  L1/R1 WEAPON",sc,0xFF909090);
  }
};
