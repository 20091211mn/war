// لعبة Warzone الأصلية: لاعب + بوتات + أسلحة + تسجيل نقاط + واجهة
#pragma once
#include "render.h"
#include "veh.h"
#include "font.h"
#include "ar.h"
#include "net.h"
#include "ws.h"
#include <string>
#include <cstdlib>

inline int g_aclTime=0;
#ifndef BUILD_ID
#define BUILD_ID 0
#endif
struct Input{
  bool tri=false;
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
  char name[16]={0}; bool remote=false,gone=false; uint8_t spawnSeq=0; float tx=0,ty=0,tz=0,tyaw=0; int lastAmmo=0; int lastHp=100; NetInput rin{}; float lastPkt=0; int lastKills=0;
};
struct Tracer{float x0,y0,z0,x1,y1,z1,t;};

struct Game{
  World w; Renderer r; std::vector<Ent> e; std::vector<Tracer> tr;
  int mapIdx=0,scoreLim=30,score[2]={0,0},nBots=6,diff=0; float sens=1.0f; bool swapTrig=false,swapL=false,swapSticks=false; float t=0,hitMark=0,dmgFlash=0; bool over=false; int winner=0;
  float fpsShow=0; std::vector<VH::Veh> vh; int drive=-1; float lookIdle=0; bool nearVeh=false; struct Puff{float x,y,z,vy,t,sz;bool fire;}; std::vector<Puff> puffs; struct Spark{float x,y,z,vx,vy,vz,t;}; std::vector<Spark> sparks;
  void sparkAt(float x,float y,float z,float nx,float ny,float nz,int n){ for(int k=0;k<n&&sparks.size()<120;k++)sparks.push_back({x,y,z,-nx*1.5f+(frand()-0.5f)*3.f,-ny*1.5f+frand()*2.5f,-nz*1.5f+(frand()-0.5f)*3.f,0.22f+frand()*0.15f}); }
  int prim=0; Input prev; bool paused=false; float msgT=0; char msg[64]={0}; int shotsFeed[4]={-1,-1,-1,-1}; std::string feed[4]; float feedT[4]={};
  int netGoal=0,netSt=0,netPhase=0; const char*netWhat=""; sockaddr_in pendJoin;
  struct ACl; ACl* ac=nullptr; void aclStart(uint32_t ip,int port); void aclStop(); void aclUpdate(float dt,const Input&in); void aclShoot(const std::string&dj); void aclReload(); void aclSwapSlot(); void aclGrenade(); void aclRender3D(); void aclRenderMenu(); void aclMenuNav(const Input&in);
  int menu=1,menuSel=0,menuRow=0,mode=0; // menu: 1 رئيسية ، 2 ردهة الانضمام
  // ---- شبكة ----
  int role=0,meI=0; uint8_t matchId=0,spawnCtr=0,evtId=0,lastEvt=0; std::vector<NetEvt> evq; uint16_t netSeq=0;
  Udp sock,beaconRx,beaconTx; float snapT=0,inT=0,beaconT=0,srvSilence=0,joinT=0; uint32_t myIp=0; char ipTxt[24]={0}; bool joining=false; sockaddr_in hostAddr; Input lastIn;
  std::vector<NetTr> trOut; struct Cli{bool on=false;sockaddr_in a;int ent=-1;float last=0;}; Cli cli[8];
  struct HostEnt{sockaddr_in a;uint8_t map,players,maxp;char name[16];float seen;}; std::vector<HostEnt> hosts; int hostSel=0;
  std::string nm(int i)const{ char b[16]; if(role==3&&i>=0&&i<(int)e.size())return i==meI?"YOU":e[i].name; if(i==meI)return "YOU"; if(i==0)return "HOST"; if(i<(int)e.size()&&e[i].remote)snprintf(b,16,"P%d",i); else snprintf(b,16,"BOT%d",i); return b; }
  void init(int rw,int rh){ r.init(rw,rh); startMatch(mapIdx); }
  void say(const char*s){snprintf(msg,64,"%s",s);msgT=2.5f;}
  void addFeed(const std::string&s){for(int i=3;i>0;i--){feed[i]=feed[i-1];feedT[i]=feedT[i-1];}feed[0]=s;feedT[0]=5;}
  void startMatch(int idx){
    mapIdx=idx%MAP_COUNT; w.load(mapIdx); e.assign(1+nBots*2+ (0),Ent()); // لاعب + فريقان
    int total=1+nBots*2-1; e.assign(total,Ent());
    for(int i=0;i<total;i++){Ent&p=e[i];p.bot=i>0;p.team=(i==0)?0:((i&1)?1:0);snprintf(p.name,16,"%s",nm(i).c_str());p.wpn=i==0?prim:rand()%WEAPON_COUNT;}
    if(role==1)for(auto&c:cli)if(c.on&&c.ent>0&&c.ent<total){ e[c.ent].remote=true; e[c.ent].bot=false; }
    vh.clear(); drive=-1; puffs.clear(); if(role==0)for(int k=0;k<w.m->nv;k++){ VH::Veh v; v.x=v.sx=w.m->veh[k].x; v.z=v.sz=w.m->veh[k].z; v.h=v.sh=w.m->veh[k].h; vh.push_back(v); }
    matchId++; evq.clear(); score[0]=score[1]=0;over=false;t=0;tr.clear();
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
    p.yaw=atan2f(-(0-p.b.x),-(0-p.b.z)); p.alive=true;p.hp=100;p.prot=2;p.crouch=false;p.spawnSeq=++spawnCtr;giveWeapon(p,(p.bot||p.remote)?p.wpn:prim);
  }
  static float dmgAt(const WeaponDef&d,float dist){ if(dist<=d.r0)return d.dmg; if(dist>=d.r1)return d.dmg*d.minMul; return d.dmg*(1-(1-d.minMul)*(dist-d.r0)/(d.r1-d.r0)); }
  bool visible(const Ent&a,const Ent&b2){
    float ax=a.b.x,ay=a.b.y+PK.eye,az=a.b.z,bx=b2.b.x,by=b2.b.y+1.2f,bz=b2.b.z;
    float dx=bx-ax,dy=by-ay,dz=bz-az,d=sqrtf(dx*dx+dy*dy+dz*dz); if(d<0.01f)return true;
    return raycastMap(w,ax,ay,az,dx/d,dy/d,dz/d,d)>=1e29f;
  }
  void hurt(int vi,int ai,float dmg,bool head){
    Ent&v=e[vi]; if(!v.alive||v.prot>0)return;
    v.hp-=dmg; v.lastHitBy=ai; v.lastDmgT=t; if(vi==meI)dmgFlash=0.5f;
    if(ai==meI)hitMark=0.25f;
    if(v.hp<=0){ v.alive=false;v.deaths++;v.respawn=4;
      if(ai>=0&&ai!=vi){Ent&k=e[ai];k.kills++;score[k.team]++; char b[64];snprintf(b,64,"%s > %s%s",nm(ai).c_str(),nm(vi).c_str(),head?" HS":"");addFeed(b); evq.push_back({++evtId,(uint8_t)ai,(uint8_t)vi,(uint8_t)head}); if(evq.size()>6)evq.erase(evq.begin());} 
      if(score[0]>=scoreLim||score[1]>=scoreLim){over=true;winner=score[0]>=scoreLim?0:1;}
    }
  }
  // ---- الشاص ----
  void explodeVeh(VH::Veh&v,int by){
    v.dead=true; v.respawnT=30; v.burn=false; if(v.driver>=0){ if(v.driver==meI)drive=-1; v.driver=-1; }
    for(int k=0;k<14;k++)puffs.push_back({v.x+(frand()-0.5f)*3,0.6f+frand()*1.5f,v.z+(frand()-0.5f)*3,1.5f+frand()*2.f,1.2f+frand()*1.2f,1.2f+frand()*1.5f,k<8});
    for(int i=0;i<(int)e.size();i++){ if(!e[i].alive)continue; float dx=e[i].b.x-v.x,dz=e[i].b.z-v.z,dd=sqrtf(dx*dx+dz*dz); if(dd<7.f)hurt(i,by,260*powf(1-dd/7.f,1.1f),false); }
    if(by==meI)hitMark=0.4f; say("DESTROYED");
  }
  void hurtVeh(VH::Veh&v,float amt,int by){ if(v.dead||amt<=0)return; v.hp-=amt; if(v.hp<=0)explodeVeh(v,by); else if(v.hp<150)v.burn=true; }
  void damageVeh(VH::Veh&v,int part,float d,int by){ if(v.dead)return;
    if(part>=10){ int i=part-10; if((v.tires>>i)&1)return; v.tireHp[i]-=d; if(v.tireHp[i]<=0)v.tires|=1<<i; return; }
    hurtVeh(v,part==1?d*1.3f:d*0.35f,by); if(by==meI)hitMark=0.25f; }
  void exitVeh(){ if(drive<0)return; VH::Veh&v=vh[drive]; Ent&p=e[meI]; static const float SP[4][2]={{-1.9f,-0.25f},{1.9f,-0.25f},{0,3.6f},{0,-3.6f}};
    float bx=v.x,bz=v.z; for(auto&s:SP){ float wx,wz; VH::l2w(v,s[0],s[1],wx,wz); if(!blocked(w,wx,0.05f,wz,PK.h)){ bx=wx;bz=wz;break; } }
    p.b=Body(); p.b.x=bx;p.b.z=bz;p.b.y=0.3f; for(int k=0;k<30;k++){moveBody(w,p.b,0,0,0.04f,false,false);if(p.b.onGround)break;} p.pitch=0; v.driver=-1; drive=-1; }
  void updateVeh(float dt,const Input&in){
    Ent&me=e[meI]; nearVeh=false; bool tri=in.tri&&!prev.tri;
    if(drive>=0&&(!me.alive||vh[drive].dead)){ vh[drive].driver=-1; drive=-1; }
    if(me.alive&&drive<0){ int best=-1; float bd=2.3f; for(int k=0;k<(int)vh.size();k++){ auto&v=vh[k]; if(v.dead)continue; float lx,lz; VH::w2l(v,me.b.x,me.b.z,lx,lz); float dd=hypotf(std::max(0.f,fabsf(lx)-VH::HW),std::max(0.f,fabsf(lz)-2.5f));
        if(dd<bd&&me.b.y<1.5f&&v.driver<0&&(v.team<0||v.team==me.team)){bd=dd;best=k;} }
      nearVeh=best>=0; if(nearVeh&&tri){ drive=best; vh[best].driver=meI; vh[best].team=me.team; me.pitch=-0.18f; me.crouch=false; me.reloadT=-1; lookIdle=0; } }
    else if(me.alive&&drive>=0&&tri)exitVeh();
    for(int k=0;k<(int)vh.size();k++){ VH::Veh&v=vh[k];
      if(v.dead){ v.respawnT-=dt; if(v.respawnT<=0){ VH::Veh n; n.x=n.sx=v.sx;n.z=n.sz=v.sz;n.h=n.sh=v.sh; v=n; } continue; }
      bool dr=drive==k&&me.alive;
      float thr=0,st=0; bool hb=false; if(dr){ thr=std::max(-1.f,std::min(1.f,in.my+(in.fire?1.f:0.f)-(in.ads?1.f:0.f))); st=in.mx; hb=in.jump; }
      else hb=fabsf(v.speed)<2.f;
      float crash=VH::step(w,v,vh,thr,st,hb,dt);
      if(crash>VH::CRASHMIN&&v.crashT<=0){ v.crashT=0.45f; hurtVeh(v,(crash-VH::CRASHMIN)*9,v.driver); if(dr)dmgFlash=std::max(dmgFlash,0.15f); } if(v.crashT>0)v.crashT-=dt;
      if(v.dead)continue;
      if(v.burn){ v.hp-=6*dt; v.smoke-=dt; if(v.smoke<=0){ v.smoke=0.12f; puffs.push_back({v.x+(frand()-0.5f),1.3f,v.z+(frand()-0.5f),1.6f,0.9f,0.5f,frand()<0.3f}); } if(v.hp<=0){ explodeVeh(v,v.driver); continue; } }
      for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(!p.alive||(dr&&i==meI)||p.gone)continue; VH::pushOut(v,p.b.x,p.b.z,p.b.y);
        if(dr&&fabsf(v.speed)>5.f&&p.team!=me.team){ float lx,lz; VH::w2l(v,p.b.x,p.b.z,lx,lz); if(fabsf(lx)<VH::HW+0.45f&&fabsf(lz)<2.5f+0.45f&&p.b.y<1.8f&&p.prot<=0)hurt(i,meI,fabsf(v.speed)*14,false); } }
      if(dr){ float sx,sz; VH::l2w(v,-0.45f,-0.25f,sx,sz); me.b.x=sx;me.b.z=sz;me.b.y=0.35f;me.b.vy=0; me.moving=false; }
    }
    for(auto&q:puffs){ q.t-=dt; q.y+=q.vy*dt; q.sz+=dt*0.8f; } puffs.erase(std::remove_if(puffs.begin(),puffs.end(),[](Puff&q){return q.t<=0;}),puffs.end());
  }
  void fire(int i){
    Ent&p=e[i]; const WeaponDef&d=WEAPONS[p.wpn];
    p.ammo--; p.cd=60.f/d.rpm; p.flash=0.06f; p.prot=0;
    float moveF=p.moving?1.6f:1.f; bool ads=(i==meI)&&adsHeld;
    float sp=(ads?d.ads:d.hip)*moveF; if(p.crouch)sp*=0.7f;
    float eyeY=p.b.y+(p.crouch?PK.eyec:PK.eye);
    std::string dj;
    for(int k=0;k<d.pellets;k++){
      float a=sp; if(p.bot){a=(0.022f+0.05f*0.5f)*(diff==0?3.2f:diff==1?1.5f:0.8f);}
      float yw=p.yaw+(frand()-0.5f)*2*a, pt=p.pitch+(frand()-0.5f)*2*a; if(d.pellets>1){yw+=(frand()-0.5f)*0.09f;pt+=(frand()-0.5f)*0.09f;}
      float cp=cosf(pt); float dx=-sinf(yw)*cp,dy=sinf(pt),dz=-cosf(yw)*cp;
      if(role==3&&i==meI){ char jb[64]; snprintf(jb,64,"%s[%.4f,%.4f,%.4f]",dj.empty()?"":",",dx,dy,dz); dj+=jb; float tw2=std::min(raycastMap(w,p.b.x,eyeY,p.b.z,dx,dy,dz,d.r1*1.4f),d.r1*1.4f); if(tr.size()<64)tr.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*tw2,eyeY+dy*tw2,p.b.z+dz*tw2,0.09f}); continue; }
      float tw=raycastMap(w,p.b.x,eyeY,p.b.z,dx,dy,dz,d.r1*1.4f); float best=std::min(tw,d.r1*1.4f); int hv=-1; bool hh=false;
      for(int j=0;j<(int)e.size();j++){ if(j==i||!e[j].alive||e[j].team==p.team)continue;
        RayHit h=rayPlayer(e[j].b.x,e[j].b.y,e[j].b.z,e[j].crouch,p.b.x,eyeY,p.b.z,dx,dy,dz,best); if(h.ok&&h.t<best){best=h.t;hv=j;hh=h.head;} }
      { VH::Veh*vhit=nullptr;int vpart=0; for(auto&v:vh){ if(v.dead)continue; if(v.driver>=0&&v.driver!=i&&e[v.driver].team==p.team)continue; int pt2; float tv=VH::rayVeh(v,p.b.x,eyeY,p.b.z,dx,dy,dz,best,pt2); if(tv<best){best=tv;vhit=&v;vpart=pt2;hv=-1;} }
        if(vhit)damageVeh(*vhit,vpart,dmgAt(d,best),i); }
      if(hv>=0)hurt(hv,i,dmgAt(d,best)*(hh?d.head:1.f),hh);
      if(best<d.r1*1.39f)sparkAt(p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best,dx,dy,dz,hv>=0?0:5); if(tr.size()<64)tr.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best,0.09f}); if(role==1&&trOut.size()<10)trOut.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best});
    }
    if(role==3&&i==meI&&!dj.empty())aclShoot(dj);
    if(i==meI&&role!=2){ p.rkP+=d.recoilV*(ads?0.6f:1.f)*1.6f; p.rkY+=(frand()-0.5f)*d.recoilH*3.f; }
  }
  bool adsHeld=false;
  void reloadStart(Ent&p){const WeaponDef&d=WEAPONS[p.wpn]; if(p.reloadT>=0||p.ammo>=d.mag||p.mags<=0)return; p.reloadT=0; if(role==3&&&p==&e[meI])aclReload(); }
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
      if(best!=p.target){p.react=(diff==0?1.1f:diff==1?0.6f:0.3f);} p.target=best; p.sx=vis?1.f:0.f; p.sz=sqrtf(bd);
    }
  }
  void updateBot(int i,float dt){
    Ent&p=e[i]; botThink(i,dt); p.react-=dt; p.strafeT-=dt; if(p.strafeT<=0){p.strafeT=0.8f+frand()*1.5f;p.strafe=frand()<0.5f?-1.f:1.f;}
    float mx=0,mz=0; bool trig=false; p.sprint=false;
    if(p.target>=0&&e[p.target].alive){ Ent&t2=e[p.target];
      float dx=t2.b.x-p.b.x,dz=t2.b.z-p.b.z,d=sqrtf(dx*dx+dz*dz); float want=atan2f(-dx,-dz);
      float er=WEAPONS[p.wpn].r0>1?(p.wpn==2?9.f:p.wpn==1?14.f:22.f):22.f; bool vis=p.sx>0.5f;
      float dy=want-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*std::min(1.f,dt*(vis?(diff==0?3.5f:diff==1?6.f:10.f):3.f));
      // ارتفاع الهدف
      float pitchW=atan2f((t2.b.y+1.3f)-(p.b.y+PK.eye),std::max(1.f,d)); p.pitch+=(pitchW-p.pitch)*std::min(1.f,dt*8);
      if(vis&&d<er){ // اشتباك: تحرّك جانبياً
        mx=p.strafe*0.6f; mz=d<er*0.4f?0.4f:(d>er*0.8f?-0.4f:0);
        if(p.react<=0&&fabsf(dy)<0.25f&&d<WEAPONS[p.wpn].r1*1.1f){ p.burst-=dt; if(p.burst<=-0.35f)p.burst=0.35f+frand()*0.5f; trig=p.burst>0; }
      } else { mz=-1; p.sprint=!vis&&d>20; }
    } else mz=-0.5f;
    // تفادي العوائق
    float fxw=-sinf(p.yaw),fzw=-cosf(p.yaw),rxw=cosf(p.yaw),rzw=-sinf(p.yaw);
    float vx=(-mz)*fxw+mx*rxw,vz=(-mz)*fzw+mx*rzw;
    float sp=(p.sprint?PK.sprint:PK.walk)*(trig?0.6f:1.f)*WEAPONS[p.wpn].move; float vl=sqrtf(vx*vx+vz*vz);
    if(vl>0.01f){vx=vx/vl*sp;vz=vz/vl*sp;}
    if(p.turnT>0){p.turnT-=dt; float c=cosf(p.turnDir*1.2f),s=sinf(p.turnDir*1.2f); float nx=vx*c-vz*s,nz=vx*s+vz*c; vx=nx;vz=nz;}
    float la=0.9f; bool jump=false;
    if(vl>0.01f&&blocked(w,p.b.x+vx/sp*la,p.b.y,p.b.z+vz/sp*la,PK.h)){ if(p.turnT<=0){p.turnT=0.5f+frand()*0.6f;p.turnDir=frand()<0.5f?-1.f:1.f;} jump=frand()<0.03f; }
    float ox=p.b.x,oz=p.b.z; moveBody(w,p.b,vx,vz,dt,jump,false);
    float mv=hypotf(p.b.x-ox,p.b.z-oz); p.moving=mv>0.001f; p.phase+=mv*3.4f;
    if(vl>0.01f&&mv<sp*dt*0.3f){p.stuck+=dt; if(p.stuck>0.6f){p.turnT=0.8f+frand();p.turnDir=frand()<0.5f?-1.f:1.f;p.stuck=0;}} else p.stuck=0;
    updateWeapon(i,dt,trig); if(p.ammo<=WEAPONS[p.wpn].mag/6&&!trig)reloadStart(p);
  }
  // ---- اللاعب ----
  void updatePlayer(float dt,const Input&in){
    Ent&p=e[meI]; adsHeld=in.ads;
    float zoom=in.ads?WEAPONS[p.wpn].zoom:1.f; float sens=1.f/ (in.ads?std::max(1.f,zoom*0.8f):1.f);
    p.yaw-=in.lx*dt*sens*this->sens; p.pitch+=in.ly*dt*sens*this->sens; p.yaw+=p.rkY*std::min(1.f,dt*30); p.pitch+=p.rkP*std::min(1.f,dt*30); p.rkP*=0.8f; p.rkY*=0.8f;
    p.pitch=std::max(-1.45f,std::min(1.45f,p.pitch));
    if(drive>=0){ // أثناء القيادة: العصا اليمنى للكاميرا فقط ، وتعود خلف الشاص تلقائياً
      if(fabsf(in.lx)+fabsf(in.ly)<0.05f){ lookIdle+=dt; if(lookIdle>1.0f){ float dy=vh[drive].h-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*std::min(1.f,dt*1.6f); } } else lookIdle=0;
      p.pitch=std::max(-0.7f,std::min(0.5f,p.pitch)); p.crouch=false; p.sprint=false; p.moving=false; adsHeld=false; return; }
    bool wantCrouch=in.crouch; if(!wantCrouch&&p.crouch&&blocked(w,p.b.x,p.b.y,p.b.z,PK.h))wantCrouch=true; p.crouch=wantCrouch;
    float mx=in.mx,mz=in.my; float ml=hypotf(mx,mz); if(ml>1){mx/=ml;mz/=ml;}
    p.sprint=in.sprint&&mz>0.3f&&!in.ads&&!p.crouch;
    float sp=(p.crouch?PK.crouch:(p.sprint?PK.sprint:PK.walk))*WEAPONS[p.wpn].move*(in.ads?0.65f:1.f);
    float fxw=-sinf(p.yaw),fzw=-cosf(p.yaw),rxw=cosf(p.yaw),rzw=-sinf(p.yaw);
    float vx=(mz*fxw+mx*rxw)*sp,vz=(mz*fzw+mx*rzw)*sp; float ox=p.b.x,oz=p.b.z;
    moveBody(w,p.b,vx,vz,dt,in.jump,p.crouch); float mv=hypotf(p.b.x-ox,p.b.z-oz); p.moving=mv>0.001f&&p.b.onGround; p.phase+=mv*3.4f;
    if(in.reload&&role!=2)reloadStart(p);
    if(role==3){ if(in.tri&&!prev.tri)aclSwapSlot(); if(in.swapN&&!prev.swapN)aclGrenade(); }
    else if((in.swapP&&!prev.swapP)||(in.swapN&&!prev.swapN)){ prim=(prim+(in.swapN&&!prev.swapN?1:WEAPON_COUNT-1))%WEAPON_COUNT; giveWeapon(p,prim); }
    bool trig=in.fire&&!p.sprint; if(!WEAPONS[p.wpn].autof&&prev.fire)trig=false;
    if(role!=2)updateWeapon(meI,dt,trig);
  }
  // ======== شبكة ========
  void netClose(){ if(role==2&&sock.ready()){NetHello h{T_BYE,NET_VER};sock.send(hostAddr,&h,sizeof h);} sock.close_(); beaconRx.close_(); beaconTx.close_(); role=0; meI=0; joining=false; for(auto&c:cli)c=Cli(); }
  int humans()const{int n=0;for(auto&p:e)if(p.remote)n++;return n+(role==1?1:0);}
  // بدء مرحلي: menu=3 يعرض اسم الخطوة الجارية. الهدف: 1 مضيف ، 2 ردهة ، 3 انضمام
  void netBegin(int goal){ netGoal=goal; netSt=0; netPhase=0; menu=3; netWhat="START"; }
  void netStartStep(){
    Udp*u=nullptr; int id=0,port=0; bool bc=false;
    if(netGoal==1){ if(netPhase==0){u=&sock;id=0;port=PORT_GAME;} else {u=&beaconTx;id=2;bc=true;} }
    else if(netGoal==2){ u=&beaconRx;id=1;port=PORT_BEACON; }
    else { u=&sock;id=3; }
    if(netSt==0){ u->open(id,port,bc); netSt=1; netWhat="WORKER"; return; }
    netWhat=u->what(); int st=u->state();
    if(st<0){ char m[64]; snprintf(m,64,"NETWORK ERROR AT %s",netWhat); say(m); msgT=6; netClose(); menu=1; return; }
    if(st!=1)return; // ما زال الخيط يعمل (أو عالق: الشاشة تعرض الخطوة والمستخدم يلغي بـ O)
    netSt=0;
    if(netGoal==1&&netPhase==0){netPhase=1;return;}
    if(netGoal==1){ role=1; meI=0; for(auto&c:cli)c=Cli(); menu=0; startMatch(menuSel); say("HOSTING LAN GAME - WAITING FOR PLAYERS"); msgT=8; snapT=inT=beaconT=0; }
    else if(netGoal==2){ menu=2; hosts.clear(); hostSel=0; joining=false; }
    else { hostAddr=pendJoin; joining=true; joinT=0; menu=2; NetHello h{T_HELLO,NET_VER}; sock.send(hostAddr,&h,sizeof h); }
  }
  void hostStart(){ netBegin(1); }
  void lobbyStart(){ netBegin(2); }
  void joinAddr(const sockaddr_in&a){ pendJoin=a; netBegin(3); }
  void joinDirect(const char*ip){ joinAddr(mkAddr(inet_addr(ip),PORT_GAME)); }
  int assignSlot(){
    int hc[2]={0,0}; for(auto&p:e)if(p.remote)hc[p.team]++; hc[0]+=1; // المضيف في الأحمر
    int pref=hc[0]<=hc[1]?0:1;
    for(int pass=0;pass<2;pass++)for(int i=1;i<(int)e.size();i++){ Ent&p=e[i]; if(!p.remote&&p.bot&&p.team==(pass?1-pref:pref))return i; }
    return -1;
  }
  void packSnap(){
    uint8_t buf[1400]; NetSnapHead h; h.t=T_SNAP;h.map=mapIdx;h.matchId=matchId;h.n=(uint8_t)e.size();h.over=over;h.winner=winner;h.s0=score[0];h.s1=score[1];h.seq=++netSeq;
    int ne=(int)e.size(); h.nEvt=(uint8_t)evq.size(); h.nTr=(uint8_t)trOut.size(); int off=sizeof h; memcpy(buf,&h,off);
    for(int i=0;i<ne;i++){ const Ent&p=e[i]; NetEnt n; n.x=p.b.x;n.y=p.b.y;n.z=p.b.z;n.yaw=p.yaw;n.pitch=p.pitch;n.flash=p.flash;n.phase=p.phase;n.reloadT=p.reloadT;n.hp=(int16_t)p.hp;
      n.flags=(p.alive?1:0)|(p.crouch?2:0)|(p.moving?4:0)|(p.sprint?8:0)|(p.team?16:0)|((p.remote||i==0)?32:0)|(p.prot>0?64:0); n.wpn=p.wpn;n.ammo=p.ammo;n.mags=p.mags;n.spawnSeq=p.spawnSeq;n.kills=p.kills;n.deaths=p.deaths; memcpy(buf+off,&n,sizeof n);off+=sizeof n; }
    for(auto&v:evq){memcpy(buf+off,&v,sizeof v);off+=sizeof v;}
    for(auto&q:trOut){memcpy(buf+off,&q,sizeof q);off+=sizeof q;} trOut.clear();
    for(auto&c:cli)if(c.on)sock.send(c.a,buf,off);
  }
  void applySnap(const Pkt&pk){
    if(pk.n<(int)sizeof(NetSnapHead))return; NetSnapHead h; memcpy(&h,pk.d,sizeof h);
    int need=sizeof h+h.n*sizeof(NetEnt)+h.nEvt*sizeof(NetEvt)+h.nTr*sizeof(NetTr); if(pk.n<need||h.n>16||h.n<2)return;
    srvSilence=0; if(h.map!=mapIdx||(int)e.size()!=h.n){ mapIdx=h.map%MAP_COUNT; w.load(mapIdx); e.assign(h.n,Ent()); for(int i=0;i<h.n;i++){e[i].bot=false;e[i].alive=false;} }
    if(h.matchId!=matchId){ matchId=h.matchId; tr.clear(); evq.clear(); lastEvt=h.nEvt?lastEvt:lastEvt; }
    over=h.over;winner=h.winner;score[0]=h.s0;score[1]=h.s1; int off=sizeof h;
    for(int i=0;i<h.n;i++){ NetEnt n; memcpy(&n,pk.d+off,sizeof n); off+=sizeof n; Ent&p=e[i];
      p.team=(n.flags&16)?1:0; p.remote=(n.flags&32)&&i!=0; bool al=n.flags&1; p.kills=n.kills;p.deaths=n.deaths; snprintf(p.name,16,"%s",nm(i).c_str());
      if(i==meI){
        if(n.hp<p.hp&&al)dmgFlash=0.5f; p.hp=n.hp;
        if(n.spawnSeq!=p.spawnSeq||(al&&!p.alive)){ p.spawnSeq=n.spawnSeq; p.b.x=n.x;p.b.y=n.y;p.b.z=n.z;p.b.vy=0;p.yaw=n.yaw;p.pitch=0;p.crouch=false; prim=n.wpn; p.wpn=n.wpn; }
        p.alive=al; p.prot=(n.flags&64)?1.f:0.f; if(n.wpn==p.wpn){ if(n.ammo<p.ammo&&n.ammo>=0&&p.alive)p.rkP+=WEAPONS[p.wpn].recoilV*1.6f; p.ammo=n.ammo; p.mags=n.mags; p.reloadT=n.reloadT; p.flash=n.flash; }
      } else {
        bool first=!p.alive&&al; p.alive=al; p.hp=n.hp; p.tx=n.x;p.ty=n.y;p.tz=n.z;p.tyaw=n.yaw; p.pitch=n.pitch; p.crouch=n.flags&2; p.moving=n.flags&4; p.sprint=n.flags&8; p.wpn=n.wpn; p.flash=n.flash; p.phase=n.phase; p.ammo=n.ammo;
        if(first||n.spawnSeq!=p.spawnSeq){ p.spawnSeq=n.spawnSeq; p.b.x=n.x;p.b.y=n.y;p.b.z=n.z; p.yaw=n.yaw; }
      }
    }
    for(int k=0;k<h.nEvt;k++){ NetEvt v; memcpy(&v,pk.d+off,sizeof v); off+=sizeof v; if((int8_t)(v.id-lastEvt)>0){ lastEvt=v.id; if(v.killer==meI)hitMark=0.25f; char b[64]; snprintf(b,64,"%s > %s%s",nm(v.killer).c_str(),nm(v.victim).c_str(),v.head?" HS":""); addFeed(b); } }
    for(int k=0;k<h.nTr;k++){ NetTr q; memcpy(&q,pk.d+off,sizeof q); off+=sizeof q; tr.push_back({q.x0,q.y0,q.z0,q.x1,q.y1,q.z1,0.09f}); }
  }
  void netPump(float dt){
    Pkt pk;
    if(role==1){
      while(sock.recv(pk)){ if(pk.n<2)continue; uint8_t ty=pk.d[0];
        int ci=-1; for(int k=0;k<8;k++)if(cli[k].on&&cli[k].a.sin_addr.s_addr==pk.from.sin_addr.s_addr&&cli[k].a.sin_port==pk.from.sin_port)ci=k;
        if(ty==T_HELLO){ if(ci<0){ for(int k=0;k<8&&ci<0;k++)if(!cli[k].on){ int en=assignSlot(); if(en<0)break; ci=k; cli[k].on=true;cli[k].a=pk.from;cli[k].ent=en; Ent&p=e[en]; p.remote=true;p.bot=false; spawn(p); snprintf(p.name,16,"%s",nm(en).c_str()); char b[40];snprintf(b,40,"%s JOINED",nm(en).c_str());addFeed(b);} }
          if(ci>=0){ cli[ci].last=0; NetWelcome wl{T_WELCOME,(uint8_t)cli[ci].ent,(uint8_t)mapIdx,(uint8_t)e.size()}; sock.send(pk.from,&wl,sizeof wl); }
          else { NetWelcome wl{T_WELCOME,255,0,0}; sock.send(pk.from,&wl,sizeof wl); } }
        else if(ty==T_BYE&&ci>=0){ Ent&p=e[cli[ci].ent]; p.remote=false;p.bot=true; char b[40];snprintf(b,40,"%s LEFT",nm(cli[ci].ent).c_str());addFeed(b); cli[ci]=Cli(); }
        else if(ty==T_INPUT&&ci>=0&&pk.n>=(int)sizeof(NetInput)){ cli[ci].last=0; NetInput in; memcpy(&in,pk.d,sizeof in); if((int16_t)(in.seq-e[cli[ci].ent].rin.seq)>0||e[cli[ci].ent].rin.t==0)e[cli[ci].ent].rin=in; }
      }
      for(auto&c:cli)if(c.on){ c.last+=dt; if(c.last>5.f){ Ent&p=e[c.ent]; p.remote=false;p.bot=true; c=Cli(); } }
      snapT+=dt; if(snapT>=0.05f){snapT=0;packSnap();}
      beaconT+=dt; if(beaconT>=1.f){ beaconT=0; NetBeacon bc; memset(&bc,0,sizeof bc); bc.t=T_BEACON;bc.map=mapIdx;bc.players=(uint8_t)humans();bc.maxp=8; snprintf(bc.name,16,"LAN HOST");
        beaconTx.send(mkAddr(htonl(INADDR_BROADCAST),PORT_BEACON),&bc,sizeof bc); uint32_t sub=(myIp&0x00FFFFFFu)|0xFF000000u; if(myIp)beaconTx.send(mkAddr(sub,PORT_BEACON),&bc,sizeof bc); }
    } else if(role==2||joining){
      while(sock.recv(pk)){ if(pk.n<1)continue; uint8_t ty=pk.d[0];
        if(ty==T_WELCOME&&joining&&pk.n>=(int)sizeof(NetWelcome)){ NetWelcome wl; memcpy(&wl,pk.d,sizeof wl); joining=false;
          if(wl.slot==255){ say("HOST FULL"); sock.close_(); menu=2; } else { role=2; meI=wl.slot; mapIdx=wl.map%MAP_COUNT; w.load(mapIdx); e.assign(wl.total,Ent()); for(auto&p:e){p.bot=false;p.alive=false;} menu=0; srvSilence=0; inT=0; over=false; lastEvt=0; evq.clear(); say("CONNECTED"); } }
        else if(ty==T_SNAP&&role==2)applySnap(pk); }
      if(joining){ joinT+=dt; if(joinT>0.4f&&joinT<8.f){ static float ra=0; ra+=dt; if(ra>0.4f){ra=0;NetHello h{T_HELLO,NET_VER};sock.send(hostAddr,&h,sizeof h);} } if(joinT>=8.f){joining=false;sock.close_();say("NO REPLY FROM HOST");} }
      if(role==2){ srvSilence+=dt; if(srvSilence>4.f){ netClose(); menu=1; say("DISCONNECTED"); return; }
        inT+=dt; if(inT>=1.f/30){ inT=0; const Ent&p=e[meI]; NetInput n; n.t=T_INPUT;n.slot=(uint8_t)meI;n.seq=++netSeq;n.x=p.b.x;n.y=p.b.y;n.z=p.b.z;n.yaw=p.yaw;n.pitch=p.pitch;
          n.flags=(lastIn.fire?1:0)|(p.crouch?2:0)|(lastIn.reload?4:0)|(p.sprint?8:0)|(p.moving?16:0)|(lastIn.ads?32:0); n.wpn=prim; sock.send(hostAddr,&n,sizeof n); } }
    }
    if(menu==2&&beaconRx.ready()){ while(beaconRx.recv(pk)){ if(pk.n<(int)sizeof(NetBeacon)||pk.d[0]!=T_BEACON)continue; NetBeacon bc; memcpy(&bc,pk.d,sizeof bc); bool f=false;
        for(auto&h:hosts)if(h.a.sin_addr.s_addr==pk.from.sin_addr.s_addr){h.map=bc.map;h.players=bc.players;h.seen=0;f=true;}
        if(!f){ HostEnt he; he.a=mkAddr(pk.from.sin_addr.s_addr,PORT_GAME); he.map=bc.map;he.players=bc.players;he.maxp=bc.maxp;memcpy(he.name,bc.name,16);he.name[15]=0;he.seen=0; hosts.push_back(he);} }
      for(auto&h:hosts)h.seen+=dt; hosts.erase(std::remove_if(hosts.begin(),hosts.end(),[](HostEnt&h){return h.seen>4.f;}),hosts.end()); if(hostSel>=(int)hosts.size())hostSel=std::max(0,(int)hosts.size()-1); }
  }
  // لاعب بعيد (على المضيف): تطبيق آخر حالة وصلت منه
  void updateRemote(int i,float dt){
    Ent&p=e[i]; const NetInput&n=p.rin; if(n.t!=T_INPUT)return;
    p.b.x=std::max(-w.m->W/2.f,std::min(w.m->W/2.f,n.x)); p.b.y=n.y; p.b.z=std::max(-w.m->D/2.f,std::min(w.m->D/2.f,n.z)); p.yaw=n.yaw;p.pitch=n.pitch;p.crouch=n.flags&2;p.sprint=n.flags&8;p.moving=n.flags&16;
    if(p.moving)p.phase+=dt*8; if(n.wpn<WEAPON_COUNT&&n.wpn!=p.wpn)giveWeapon(p,n.wpn);
    if(n.flags&4)reloadStart(p); updateWeapon(i,dt,(n.flags&1)&&!p.sprint);
  }
  // ======== التحديث ========
  void update(float dt,const Input&in){
    lastIn=in;
    if(role==3||menu>=5){ aclUpdate(dt,in); return; }
    if(menu==1){
      bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f,rt=in.mx>0.5f&&prev.mx<=0.5f,lf=in.mx<-0.5f&&prev.mx>=-0.5f;
      if(up)menuRow=(menuRow+2)%3; if(dn)menuRow=(menuRow+1)%3; int d=rt?1:lf?-1:0;
      if(d){ if(menuRow==0)menuSel=(menuSel+d+MAP_COUNT)%MAP_COUNT; else if(menuRow==1)diff=(diff+d+3)%3; else mode=(mode+d+4)%4; }
      if(msgT>0)msgT-=dt;
      if((in.confirm&&!prev.confirm)||(in.start&&!prev.start)){ if(mode==0){menu=0;role=0;meI=0;startMatch(menuSel);} else if(mode==1)hostStart(); else if(mode==2)lobbyStart(); else aclStart(0,3000); }
      prev=in; return; }
    if(menu==3){ netStartStep(); if(in.back&&!prev.back){ netClose(); menu=1; } prev=in; return; }
    if(menu==2){
      netPump(dt); if(msgT>0)msgT-=dt;
      bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f; if(up&&hostSel>0)hostSel--; if(dn&&hostSel+1<(int)hosts.size())hostSel++;
      if(in.confirm&&!prev.confirm&&!joining&&!hosts.empty())joinAddr(hosts[hostSel].a);
      if(in.back&&!prev.back){ netClose(); menu=1; }
      prev=in; return; }
    if(in.start&&!prev.start)paused=!paused;
    bool net=role!=0; Input use=in;
    if(paused){ if(in.mx>0.5f&&prev.mx<=0.5f)sens=std::min(2.5f,sens+0.2f); if(in.mx<-0.5f&&prev.mx>=-0.5f)sens=std::max(0.3f,sens-0.2f); if(in.back&&!prev.back){ if(net)netClose(); menu=1;paused=false;}
      if(!net){prev=in;return;} use=Input(); }
    if(net)netPump(dt);
    if(menu){prev=in;return;} // انقطع الاتصال
    if(over&&role!=2){ if(in.confirm&&!prev.confirm&&!paused){ if(role==1)for(auto&c:cli)if(c.on&&c.ent>0)e[c.ent].remote=true; startMatch(mapIdx);} prev=in; return; }
    if(over&&role==2){ prev=in; return; }
    t+=dt; if(msgT>0)msgT-=dt; if(hitMark>0)hitMark-=dt; if(dmgFlash>0)dmgFlash-=dt;
    for(auto&q:tr)q.t-=dt; tr.erase(std::remove_if(tr.begin(),tr.end(),[](Tracer&q){return q.t<=0;}),tr.end());
    for(int k=0;k<4;k++)if(feedT[k]>0)feedT[k]-=dt;
    for(auto&k:sparks){ k.t-=dt; k.vy-=9.f*dt; k.x+=k.vx*dt; k.y=std::max(0.02f,k.y+k.vy*dt); k.z+=k.vz*dt; } sparks.erase(std::remove_if(sparks.begin(),sparks.end(),[](Spark&k){return k.t<=0;}),sparks.end());
    if(role==2||role==3){ // عميل: حركة محلية للاعب فقط + تنعيم الآخرين
      for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(i==meI){ if(p.alive)updatePlayer(dt,use); continue; } if(p.gone)continue;
        float k=std::min(1.f,dt*16.f); p.b.x+=(p.tx-p.b.x)*k;p.b.y+=(p.ty-p.b.y)*k;p.b.z+=(p.tz-p.b.z)*k; float dy=p.tyaw-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*k; if(p.flash>0)p.flash-=dt; }
      prev=in; return; }
    for(int i=0;i<(int)e.size();i++){ Ent&p=e[i];
      if(p.prot>0)p.prot-=dt;
      if(!p.alive){ p.respawn-=dt; if(p.respawn<=0){spawn(p);} continue; }
      if(i==meI)updatePlayer(dt,use); else if(p.remote)updateRemote(i,dt); else updateBot(i,dt);
    }
    if(role==0)updateVeh(dt,use);
    prev=in;
  }
  // ---- الرسم ----
  static void gcol(char c,float*o){ static const float T[7][3]={{.14f,.14f,.16f},{.34f,.35f,.38f},{.05f,.05f,.06f},{.42f,.27f,.13f},{.5f,.45f,.32f},{.9f,.5f,.1f},{.86f,.68f,.55f}}; int k=c=='d'?0:c=='g'?1:c=='b'?2:c=='w'?3:c=='t'?4:c=='o'?5:6; o[0]=T[k][0];o[1]=T[k][1];o[2]=T[k][2]; }
  // يرسم سلاحاً (من بيانات أندرويد) عند أصل عالمي مع دوران yaw/pitch
  void drawGunAt(float ox,float oy,float oz,float yaw,float pitch,int wi,float flash,bool lit,float k){
    const GunDef&g=GUNDEFS[wi];
    auto part=[&](float x,float y,float z,float sx,float sy,float sz,const float*c){float o[3];Renderer::rot(yaw,pitch,x,y,z,o);r.obb(ox+o[0],oy+o[1],oz+o[2],sx*.5f,sy*.5f,sz*.5f,yaw,pitch,c[0]*k,c[1]*k,c[2]*k,true,lit);};
    for(int i=0;i<g.np;i++){float c[3];gcol(g.p[i].c,c);part(g.p[i].x,g.p[i].y,g.p[i].z,g.p[i].sx,g.p[i].sy,g.p[i].sz,c);}
    if(g.hasMag){float c[3];gcol('b',c);part(g.mag[0],g.mag[1],g.mag[2],g.mag[3],g.mag[4],g.mag[5],c);}
    if(flash>0){float c[3]={1,0.8f,0.35f};float sz=0.07f+0.05f*flash;part(0,0.005f,g.mz-0.03f,sz,sz,sz*1.8f,c);}
  }
  void drawAvatar(const Ent&a,float camDist){
    float tc[2][3]={{0.82f,0.14f,0.14f},{0.15f,0.36f,0.9f}}; const float*c=tc[a.team]; const float td[2][3]={{0.5f,0.08f,0.08f},{0.08f,0.2f,0.55f}}; const float*d=td[a.team];
    float x=a.b.x,y=a.b.y,z=a.b.z,yw=a.yaw;
    static const float pants[3]={.17f,.18f,.15f},boot[3]={.08f,.07f,.07f},vest[3]={.2f,.22f,.2f},plate[3]={.26f,.28f,.25f},pocket[3]={.16f,.17f,.15f},pack[3]={.18f,.2f,.16f},belt[3]={.1f,.1f,.11f},skin[3]={.86f,.68f,.55f},glove[3]={.1f,.1f,.11f},visor[3]={.05f,.06f,.08f},mask[3]={.12f,.12f,.12f};
    // ---- سقوط عند الموت: ينهار للأمام بسلاسة ثم يبقى مستلقياً ----
    if(!a.alive){ float dt_=std::max(0.f,std::min(1.f,(4.f-a.respawn)/0.45f)); if(a.respawn>4.f||a.respawn<=0)dt_=1; float k=1-(1-dt_)*(1-dt_); float ph=-1.5708f*k;
      auto D=[&](float lx,float ly,float lz,float hx,float hy,float hz,float rx_,const float*col){ float y1=ly*cosf(ph)-lz*sinf(ph),z1=ly*sinf(ph)+lz*cosf(ph)+0.0f; float o[3];Renderer::rot(yw,0,lx,y1+0.17f*k,z1,o); r.obb(x+o[0],y+o[1],z+o[2],hx*.5f,hy*.5f,hz*.5f,yw,rx_+ph,col[0]*0.6f,col[1]*0.6f,col[2]*0.6f); };
      float sp=0.12f*k; D(-0.12f-sp,0.4f,0.02f,.2f,.8f,.22f,0,pants); D(0.12f+sp,0.4f,0.0f,.2f,.8f,.22f,0,pants); D(0,1.1f,0,.44f,.58f,.25f,0,c); D(0,1.13f,0.015f,.5f,.46f,.29f,0,vest); D(0,1.0f,0.2f,.36f,.42f,.14f,0,pack);
      D(0,1.58f,0,.21f,.23f,.22f,0,skin); D(0,1.66f,0.01f,.34f,.2f,.35f,0,d); D(0.36f+sp*2,1.2f,0,.12f,.54f,.12f,0,d); D(-0.36f-sp*2,1.2f,0,.12f,.54f,.12f,0,d); return; }
    if(camDist>38.f){ float o[3]; r.obb(x,y+0.42f,z,.2f,.42f,.12f,yw,0,pants[0],pants[1],pants[2]); r.obb(x,y+1.1f,z,.25f,.3f,.15f,yw,0,c[0],c[1],c[2]); r.obb(x,y+1.58f,z,.12f,.13f,.12f,yw,0,d[0],d[1],d[2]); (void)o; return; }
    // ---- مشية واقعية: فخذ + ساق مع ثني الركبة ، اهتزاز الجذع ، ميلان عند الركض ، وضعية انحناء حقيقية ----
    float mv=a.moving?1.f:0.f; bool cr=a.crouch; float amp=(a.sprint?0.95f:0.68f)*(cr?0.45f:1.f)*mv;
    float hipY=0.84f-(cr?0.34f:0.f); float bob=fabsf(sinf(a.phase))*0.032f*mv*(a.sprint?1.7f:1.f)*(cr?0.5f:1.f);
    float L=(a.sprint?0.2f:0.f)+(cr?0.1f:0.f)+0.02f*sinf(a.phase*2)*mv; float cl=cosf(L),sl=sinf(L);
    auto B=[&](float lx,float ly,float lz,float hx,float hy,float hz,float rx_,const float*col){float o[3];Renderer::rot(yw,0,lx,ly,lz,o);r.obb(x+o[0],y+o[1],z+o[2],hx*.5f,hy*.5f,hz*.5f,yw,rx_,col[0],col[1],col[2]);};
    auto UP=[&](float lx,float ly,float lz,float hx,float hy,float hz,const float*col){ float oy=ly-0.84f; float y2=oy*cl-lz*sl,z2=oy*sl+lz*cl; B(lx,hipY+bob+y2,z2,hx,hy,hz,L,col); };
    auto UPp=[&](float lx,float ly,float lz,float*o3){ float oy=ly-0.84f; o3[0]=lx; o3[1]=hipY+bob+oy*cl-lz*sl; o3[2]=oy*sl+lz*cl; };
    // الساقان
    for(int s2=0;s2<2;s2++){ float lx=s2?0.12f:-0.12f; float ph=a.phase+(s2?3.14159f:0.f);
      float th=sinf(ph)*amp+(cr?0.85f:0.f); float flex=std::max(0.f,cosf(ph))*0.85f*mv*(cr?0.5f:1.f)+(cr?1.7f:0.f); float sa=th-flex;
      float hy=hipY+bob*0.5f; float kx=hy-0.42f*cosf(th),kz=-0.42f*sinf(th);
      B(lx,hy-0.21f*cosf(th),-0.21f*sinf(th),0.2f,0.44f,0.22f,th,pants);
      B(lx,kx-0.21f*cosf(sa),kz-0.21f*sinf(sa),0.18f,0.44f,0.2f,sa,pants);
      float ay=kx-0.42f*cosf(sa),az=kz-0.42f*sinf(sa); B(lx,ay-0.05f,az-0.08f,0.21f,0.14f,0.3f,sa*0.35f,boot); }
    // الجذع
    UP(0,1.1f,0,.44f,.58f,.25f,c); UP(0,1.13f,0.015f,.5f,.46f,.29f,vest); UP(0,1.22f,-0.15f,.36f,.2f,.05f,plate);
    for(float px:{-0.14f,0.f,0.14f})UP(px,0.98f,-0.16f,.1f,.12f,.06f,pocket);
    UP(0,1.0f,0.2f,.36f,.42f,.14f,pack); UP(0,0.92f,0,.5f,.1f,.3f,belt); UP(0,1.44f,0,.11f,.1f,.11f,skin);
    // الرأس (يتبع النظر)
    { float nk[3]; UPp(0,1.58f,0,nk); float hp=a.pitch*0.5f; auto H=[&](float ox,float oy,float oz,float sx,float sy,float sz,const float*col){ float cx=cosf(hp),sx2=sinf(hp); float o3[3]={nk[0]+ox,nk[1]+oy*cx-oz*sx2,nk[2]+oy*sx2+oz*cx}; B(o3[0],o3[1],o3[2],sx,sy,sz,hp,col); };
      H(0,0,0,.21f,.23f,.22f,skin); float hel[3]={d[0]*.9f,d[1]*.9f,d[2]*.9f}; H(0,0.08f,0.01f,.34f,.2f,.35f,hel); H(0,0.02f,-0.115f,.23f,.07f,.03f,visor); H(0,-0.1f,-0.08f,.18f,.05f,.08f,mask); }
    // الذراعان والسلاح (ثابتان على السلاح مع تنفّس خفيف)
    float ap=a.sprint?0.2f:a.pitch; float base=a.sprint?0.9f:1.5708f; float br=0.015f*sinf(a.phase*0.5f+t*2.f);
    auto arm=[&](float ax,float rot){ float len=0.54f,sh[3]; UPp(ax,1.36f,0,sh); float cr_=cosf(rot),sr=sinf(rot);
      B(sh[0],sh[1]-len/2*cr_,sh[2]-len/2*sr,.12f,len,.12f,rot,d); B(sh[0],sh[1]-(len+0.02f)*cr_,sh[2]-(len+0.02f)*sr,.1f,.1f,.1f,rot,glove); };
    arm(0.27f,base+ap-0.12f+br); arm(-0.27f,base+ap+0.12f+br);
    float gs[3]; UPp(0.12f,1.3f,0,gs); float cA=cosf(ap),sA=sinf(ap); float wo[3]; Renderer::rot(yw,0,gs[0],gs[1]-0.05f*cA+0.38f*sA,gs[2]-0.05f*sA-0.38f*cA,wo);
    drawGunAt(x+wo[0],y+wo[1],z+wo[2],yw,ap,a.wpn,a.flash>0?1.f:0.f,true,1.f);
  }
  void render(){
    Ent&me=e[meI]; float eyeH=me.alive?(me.b.y+(me.crouch?PK.eyec:PK.eye)):me.b.y+0.6f;
    float fov=1.3f; if(adsHeld&&me.alive){fov=1.3f/WEAPONS[me.wpn].zoom;}
    Cam c{me.b.x,eyeH,me.b.z,me.yaw,me.pitch,fov};
    if(drive>=0&&me.alive){ const VH::Veh&v=vh[drive]; float cp=cosf(me.pitch),fxw=-sinf(me.yaw)*cp,fyw=sinf(me.pitch),fzw=-cosf(me.yaw)*cp; float cx0=v.x,cy0=2.2f,cz0=v.z; float D=8.f; float tw2=raycastMap(w,cx0,cy0,cz0,-fxw,-fyw,-fzw,D); D=std::min(D,tw2-0.3f); D=std::max(1.5f,D); c.x=cx0-fxw*D; c.y=std::max(0.5f,cy0-fyw*D); c.z=cz0-fzw*D; }
    r.setCam(c); r.world(w);
    for(auto&v:vh){ float dx=v.x-c.x,dz=v.z-c.z,dd=sqrtf(dx*dx+dz*dz); if(dd<r.fogF)VH::draw(r,v,dd); }
    for(auto&q:puffs){ float k=std::min(1.f,q.t); if(q.fire)r.obb(q.x,q.y,q.z,q.sz*.4f,q.sz*.4f,q.sz*.4f,0,0,1.0f,0.55f*k+0.2f,0.1f,false,false); else r.obb(q.x,q.y,q.z,q.sz*.5f,q.sz*.5f,q.sz*.5f,0,0,0.12f+0.1f*k,0.12f+0.1f*k,0.12f+0.1f*k,false,false); }
    for(int i=0;i<(int)e.size();i++){if(i==meI||e[i].gone)continue;const Ent&a=e[i];float dx=a.b.x-c.x,dz=a.b.z-c.z; if(dx*dx+dz*dz>r.fogF*r.fogF||dx*dx+dz*dz<0.5f)continue; drawAvatar(a,sqrtf(dx*dx+dz*dz));}
    for(auto&q:tr){ float dx=q.x1-q.x0,dy=q.y1-q.y0,dz=q.z1-q.z0,l=sqrtf(dx*dx+dy*dy+dz*dz); if(l<0.1f)continue; float p0=std::min(1.f,std::max(0.f,1.f-q.t/0.09f)); float hd=p0*l,tl=std::max(0.f,hd-std::min(5.f,l*0.7f)); float sg=hd-tl; if(sg<0.05f)continue;
      float yw=atan2f(-dx,-dz),pt=asinf(dy/l); float m=(hd+tl)*0.5f/l; float cx=q.x0+dx*m,cy=q.y0+dy*m,cz=q.z0+dz*m; float fa=1.f-p0*0.5f;
      r.obb(cx,cy,cz,0.022f,0.022f,sg/2,yw,pt,1.0f*fa,0.62f*fa,0.2f*fa,false,false); r.obb(cx,cy,cz,0.01f,0.01f,sg/2+0.01f,yw,pt,1,0.98f,0.85f,false,false);
      float hx=q.x0+dx*hd/l,hy=q.y0+dy*hd/l,hz=q.z0+dz*hd/l; r.obb(hx,hy,hz,0.035f,0.035f,0.06f,yw,pt,1,1,0.9f,false,false); }
    for(auto&k:sparks){ float a=std::min(1.f,k.t*5.f); r.obb(k.x,k.y,k.z,0.025f,0.025f,0.025f,0,0,1.f,0.7f*a+0.2f,0.2f,false,false); }
    if(role==3)aclRender3D();
    // مجسّم السلاح (نفس قطع نسخة أندرويد)
    if(me.alive&&drive<0){ r.flush(); std::fill(r.zb.begin(),r.zb.end(),0.f); const GunDef&g=GUNDEFS[me.wpn]; float k=adsHeld?1.f:0.f; float top=g.sy;
      static float bob=0; bob+=me.moving?(me.sprint?0.17f:0.11f):0; float bx=sinf(bob)*0.008f*(me.moving?1:0),by=fabsf(cosf(bob))*0.008f*(me.moving?1:0);
      float x=0.17f*(1-k)+0*k+bx,y=-0.17f*(1-k)+(-top-0.002f)*k+by,z=-0.33f*(1-k)-0.36f*k; float kick=me.flash>0?0.045f:0.f; z+=kick; float rx=kick*1.2f,ry=0.f;
      if(me.reloadT>=0){float sn=sinf(me.reloadT/WEAPONS[me.wpn].reload*3.14159f);y-=sn*0.13f;rx-=sn*0.5f;x+=sn*0.03f;}
      if(me.sprint){y-=0.06f;rx-=0.35f;ry=0.4f;x-=0.04f;}
      float O[3];Renderer::rot(me.yaw,me.pitch,x,y,z,O); float gy=me.yaw+ry,gp=me.pitch+rx; float ox=c.x+O[0],oy=c.y+O[1],oz=c.z+O[2];
      drawGunAt(ox,oy,oz,gy,gp,me.wpn,me.flash>0?std::min(1.f,me.flash*16.f):0.f,false,0.95f);
      float sl[3]={me.team?0.08f:0.5f,me.team?0.2f:0.08f,me.team?0.55f:0.08f},sk[3]={.86f,.68f,.55f};
      auto hp=[&](float px,float py,float pz,float sx,float sy,float sz,const float*cc){float o[3];Renderer::rot(gy,gp,px,py,pz,o);r.obb(ox+o[0],oy+o[1],oz+o[2],sx*.5f,sy*.5f,sz*.5f,gy,gp,cc[0]*0.95f,cc[1]*0.95f,cc[2]*0.95f,false,false);};
      float gz=(me.wpn>=6)?0.04f:0.06f; hp(0,-0.075f,gz,.05f,.05f,.06f,sk); hp(0.015f,-0.12f,gz+0.12f,.065f,.06f,.28f,sl);
      if(me.wpn<6){hp(-0.005f,-0.05f,g.mz*0.42f,.055f,.05f,.07f,sk);hp(-0.03f,-0.09f,g.mz*0.42f+0.2f,.06f,.055f,.3f,sl);} else hp(-0.03f,-0.075f,gz+0.01f,.05f,.05f,.06f,sk);
    }
    hud();
  }
  // ---- واجهة ----
  void px(int x,int y,uint32_t c){ if(x>=0&&y>=0&&x<r.W&&y<r.H)r.fb[y*r.W+x]=c; }
  void rect(int x,int y,int w2,int h2,uint32_t c,float a=1){ r.flush(); for(int j=std::max(0,y);j<std::min(r.H,y+h2);j++)for(int i=std::max(0,x);i<std::min(r.W,x+w2);i++){ if(a>=1)r.fb[j*r.W+i]=c; else { uint32_t o=r.fb[j*r.W+i]; auto m=[&](int sh){int A=(o>>sh)&255,B=(c>>sh)&255;return (uint32_t)(A+(B-A)*a)<<sh;}; r.fb[j*r.W+i]=0xFF000000u|m(16)|m(8)|m(0);} } }
  // خط حقيقي: sc*9 = حجم الخط بالبكسل ؛ يدعم العربية
  static int fontEm(int sc){ return std::max(8,sc*9); }
  static const FGlyph* fgFor(int em,uint32_t cp,int&base){ const FGlyph*a;int n; if(em<=22){a=FG_S;n=FN_S;base=16;} else if(em<=44||cp>=0x80){a=FG_M;n=FN_M;base=32;} else {a=FG_L;n=FN_L;base=64;}
    const FGlyph*g=AR::find(a,n,cp); if(!g&&cp!=0x20){ g=AR::find(a,n,'?'); } return g; }
  int tw(const char*s,int sc){ int em=fontEm(sc),best=0,cur=0; for(auto cp:AR::visual(s)){ if(cp=='\n'){best=std::max(best,cur);cur=0;continue;} int base; const FGlyph*g=fgFor(em,cp,base); cur+=g?(int)(g->adv16/16.0f*em/base+0.5f):em/3; } return std::max(best,cur); }
  void text(int x,int y,const char*s,int sc,uint32_t col){ r.flush(); int em=fontEm(sc); std::string line; const char*p=s; int ly=y;
    auto flush=[&](){ int cx=x,bl=ly+7*sc; for(auto cp:AR::visual(line.c_str())){ int base; const FGlyph*g=fgFor(em,cp,base); if(!g){cx+=em/3;continue;} float k=(float)em/base;
        if(g->w){ int dw=std::max(1,(int)(g->w*k+0.5f)),dh=std::max(1,(int)(g->h*k+0.5f)); int dx=cx+(int)(g->bx*k+(g->bx<0?-0.5f:0.5f)),dy=bl-(int)(g->by*k+0.5f); const uint8_t*src=FDATA+g->off;
          for(int j=0;j<dh;j++){ int py=dy+j; if(py<0||py>=r.H)continue; float fy=(j+0.5f)/k-0.5f; int y0=(int)floorf(fy); float ty=fy-y0; int y1=std::min(g->h-1,y0+1); y0=std::max(0,y0);
            for(int i=0;i<dw;i++){ int px=dx+i; if(px<0||px>=r.W)continue; float fx=(i+0.5f)/k-0.5f; int x0=(int)floorf(fx); float tx=fx-x0; int x1=std::min(g->w-1,x0+1); x0=std::max(0,x0);
              float a=(src[y0*g->w+x0]*(1-tx)+src[y0*g->w+x1]*tx)*(1-ty)+(src[y1*g->w+x0]*(1-tx)+src[y1*g->w+x1]*tx)*ty; a/=255.f; if(a<0.04f)continue;
              uint32_t o=r.fb[py*r.W+px]; auto m=[&](int sh){int A=(o>>sh)&255,B=(col>>sh)&255;return (uint32_t)(A+(B-A)*a)<<sh;}; r.fb[py*r.W+px]=0xFF000000u|m(16)|m(8)|m(0); } } }
        cx+=(int)(g->adv16/16.0f*k+0.5f); } line.clear(); };
    for(;*p;p++){ if(*p=='\n'){ flush(); ly+=em*3/2; } else line+=*p; } flush(); }
  void textC(int cx,int y,const char*s,int sc,uint32_t col){text(cx-tw(s,sc)/2,y,s,sc,col);}
  void hud(){
    int W=r.W,H=r.H,sc=std::max(1,H/270); Ent&me=e[meI]; const WeaponDef&d=WEAPONS[me.wpn];
    if(dmgFlash>0)rect(0,0,W,H,0xFFB01010,dmgFlash*0.9f);
    if(!me.alive){ rect(0,0,W,H,0xFF300000,0.5f); char b[32];snprintf(b,32,"RESPAWN %d",(int)ceilf(me.respawn));textC(W/2,H/2-4*sc,b,sc*2,0xFFFFFFFF); }
    else { int cx=W/2,cy=H/2; uint32_t cc=hitMark>0?0xFFFF4040:0xFFFFFFFF; int g=3*sc; rect(cx-g-4*sc,cy,4*sc,sc,cc);rect(cx+g,cy,4*sc,sc,cc);rect(cx,cy-g-4*sc,sc,4*sc,cc);rect(cx,cy+g,sc,4*sc,cc); if(hitMark>0){rect(cx-6*sc,cy-6*sc,2*sc,2*sc,0xFFFF4040);rect(cx+5*sc,cy+5*sc,2*sc,2*sc,0xFFFF4040);rect(cx-6*sc,cy+5*sc,2*sc,2*sc,0xFFFF4040);rect(cx+5*sc,cy-6*sc,2*sc,2*sc,0xFFFF4040);} }
    char b[64]; int sm=std::max(1,sc-1);
    // ---- شريط النتيجة والوقت (أعلى الوسط) ----
    { int pw=70*sc,ph=15*sc,px=W/2-pw/2,py=3*sc; rrect(px,py,pw,ph,ph/2,0xFF0B0F14,0.78f);
      snprintf(b,64,"%d",score[0]); textC(px+14*sc,py+ph/2-4*sc,b,sc,0xFFE23B3B); snprintf(b,64,"%d",score[1]); textC(px+pw-14*sc,py+ph/2-4*sc,b,sc,0xFF3B82F6);
      int secs=role==3?g_aclTime:std::max(0,600-(int)t); snprintf(b,64,"%d:%02d",secs/60,secs%60); textC(W/2,py+ph/2-4*sc,b,sc,0xFFE8EEF6); }
    // ---- رادار دائري (أعلى اليسار) ----
    int R=30*sc,rcx=8*sc+R,rcy=8*sc+R; float fxw=-sinf(me.yaw),fzw=-cosf(me.yaw),rxw=cosf(me.yaw),rzw=-sinf(me.yaw);
    for(int j=-R;j<=R;j++)for(int i=-R;i<=R;i++){ int d2=i*i+j*j; if(d2>R*R)continue; int X=rcx+i,Y=rcy+j; if(X<0||Y<0||X>=W||Y>=H)continue; uint32_t o=r.fb[Y*W+X]; float al=d2>(R-sc)*(R-sc)?0.85f:0.55f; uint32_t c2=d2>(R-sc)*(R-sc)?0xFF8FA0B4:0xFF0B0F14; auto m=[&](int sh){int A=(o>>sh)&255,B2=(c2>>sh)&255;return (uint32_t)(A+(B2-A)*al)<<sh;}; r.fb[Y*W+X]=0xFF000000u|m(16)|m(8)|m(0); }
    auto dot=[&](float wx,float wz,uint32_t col,int sz){ float dx=wx-me.b.x,dz=wz-me.b.z; float lr=dx*rxw+dz*rzw,lf=dx*fxw+dz*fzw; float k=(R-2*sc)/40.f; int X=rcx+(int)(lr*k),Y=rcy-(int)(lf*k); int ddx=X-rcx,ddy=Y-rcy; if(ddx*ddx+ddy*ddy>(R-2*sc)*(R-2*sc))return; rect(X-sz/2,Y-sz/2,sz,sz,col); };
    for(auto&v:vh)dot(v.x,v.z,v.dead?0xFF606060:0xFFFFD23B,3*sc);
    for(auto&o:e){ if(!o.alive||&o==&me)continue; dot(o.b.x,o.b.z,o.team?0xFF5080FF:0xFFFF5050,2*sc); }
    rect(rcx-sc,rcy-sc,2*sc,2*sc,0xFFFFD060); rect(rcx-sc/2,rcy-4*sc,std::max(1,sc/2),3*sc,0xFFFFD060);
    // ---- الصحة: شريط مقطّع + صليب ----
    { int bx=8*sc,by=rcy+R+8*sc,bw=62*sc,bh=8*sc; rrect(bx,by,bw+16*sc,bh+4*sc,5*sc,0xFF0B0F14,0.78f); int segs=10,sw2=(bw-4*sc)/segs; float hp=std::max(0.f,me.hp)/100.f;
      for(int k=0;k<segs;k++){ bool on=(k+0.5f)/segs<=hp+0.001f; rect(bx+2*sc+k*sw2,by+2*sc,sw2-sc,bh,on?(hp>0.35f?0xFF3CD070:0xFFE04030):0xFF1E2833); }
      rrect(bx+bw+2*sc,by+sc,12*sc,bh+2*sc,3*sc,0xFFE23B3B); rect(bx+bw+7*sc,by+3*sc,2*sc,bh-sc,0xFFFFFFFF); rect(bx+bw+4*sc,by+bh/2+sc/2,8*sc,2*sc,0xFFFFFFFF);
      snprintf(b,64,"%d",(int)std::max(0.f,me.hp)); text(bx+bw+16*sc,by+bh/2-3*sc,b,sc,0xFFE8EEF6); }
    // ---- أسفل الوسط: الذخيرة أو لوحة الشاص ----
    if(drive>=0&&me.alive){ const VH::Veh&v=vh[drive]; int pw=96*sc,ph=22*sc,px=W/2-pw/2,py=H-ph-6*sc; rrect(px,py,pw,ph,8*sc,0xFF0B0F14,0.8f);
      snprintf(b,64,"%d",(int)(fabsf(v.speed)*3.6f)); textC(W/2-14*sc,py+3*sc,b,sc*2,0xFFFFFFFF); text(W/2+6*sc,py+8*sc,"km/h",sc,0xFF8FA0B4);
      int bw=pw-20*sc; rrect(px+10*sc,py+ph-6*sc,bw,3*sc,sc,0xFF1E2833); rrect(px+10*sc,py+ph-6*sc,std::max(2,(int)(bw*std::max(0.f,v.hp)/VH::HP)),3*sc,sc,v.hp>150?0xFF3CD070:0xFFE04030);
      textC(W/2,py-12*sc,"Triangle: نزول",sm,0xFFB8C4D4); }
    else { int pw=124*sc,ph=19*sc,px=W/2-pw/2,py=H-ph-6*sc; rrect(px,py,pw,ph,8*sc,0xFF0B0F14,0.8f);
      text(px+pw-8*sc-tw(d.name,sm),py+ph/2-3*sc,d.name,sm,me.reloadT>=0?0xFFFFC040:0xFFFFB020);
      snprintf(b,64,"%d",me.ammo); int ax=px+12*sc; text(ax,py+ph/2-7*sc,b,sc*2,me.reloadT>=0?0xFFFFC040:0xFFFFFFFF);
      snprintf(b,64,"/ %d",me.mags*d.mag); text(ax+tw("00",sc*2)+6*sc,py+ph/2-4*sc,b,sc,0xFF8FA0B4);
      if(me.reloadT>=0)textC(W/2,H/2+20*sc,"جارٍ التعبئة…",sc,0xFFFFC040);
      if(nearVeh)textC(W/2,py-12*sc,"Triangle: ركوب الشاص",sm,0xFFFFD060); }
    for(int k=0;k<4;k++)if(feedT[k]>0)text(W-tw(feed[k].c_str(),sc)-6*sc,(24+k*10)*sc,feed[k].c_str(),sc,0xFFFFFFFF);
    if(msgT>0)textC(W/2,H/3,msg,sc*2,0xFFFFFFFF);
    if(fpsShow>0){ snprintf(b,64,"%d fps  %dx%d",(int)fpsShow,W,H); text(4*sc,H-10*sc,b,std::max(1,sc-1),0xFF607080); }
    if(role)text(4*sc,H-20*sc,role==1?"LAN HOST":role==2?"LAN CLIENT":"ANDROID LAN",std::max(1,sc-1),0xFF80E0FF);
    if(over){ rect(0,0,W,H,0xFF000000,0.55f); textC(W/2,H/2-20*sc,winner==0?"RED TEAM WINS":"BLUE TEAM WINS",sc*3,winner==0?0xFFFF6060:0xFF6090FF); textC(W/2,H/2+10*sc,role==2?"WAITING FOR HOST":"X = PLAY AGAIN",sc*2,0xFFFFFFFF); }
    if(paused){ rect(0,0,W,H,0xFF000000,0.6f); textC(W/2,H/2-12*sc,"إيقاف مؤقت",sc*3,0xFFFFFFFF); textC(W/2,H/2+14*sc,"OPTIONS: متابعة    O: القائمة",sc*2,0xFFB0B0B0); char sb[40];snprintf(sb,40,"SENSITIVITY < %.1f >",sens);textC(W/2,H/2+34*sc,sb,sc*2,0xFFFFFFFF); {char sw[40];snprintf(sw,40,"R1: STICKS %s",swapSticks?"LEFT=LOOK RIGHT=MOVE":"LEFT=MOVE RIGHT=LOOK");textC(W/2,H/2+54*sc,sw,sc,0xFF80E0FF);} }
  }
  // ---- عناصر واجهة بنمط نسخة أندرويد ----
  static constexpr uint32_t C_PANEL=0xFF141B24,C_LINE=0xFF243040,C_BTN=0xFF1B2430,C_BTNH=0xFF2A3A50,C_TXT=0xFFE8EEF6,C_MUT=0xFF8FA0B4,C_ACC=0xFFFFB020;
  void rrect(int x,int y,int w2,int h2,int rad,uint32_t c,float a=1){ rad=std::min(rad,std::min(w2,h2)/2);
    for(int j=std::max(0,y);j<std::min(r.H,y+h2);j++)for(int i=std::max(0,x);i<std::min(r.W,x+w2);i++){
      int cx=std::min(std::max(i,x+rad),x+w2-1-rad),cy=std::min(std::max(j,y+rad),y+h2-1-rad); float d=sqrtf((float)((i-cx)*(i-cx)+(j-cy)*(j-cy))); float cov=d<=rad-0.5f?1.f:d<rad+0.5f?(rad+0.5f-d):0.f; if(cov<=0)continue; float al=a*cov;
      uint32_t o=r.fb[j*r.W+i]; auto m=[&](int sh){int A=(o>>sh)&255,B=(c>>sh)&255;return (uint32_t)(A+(B-A)*al)<<sh;}; r.fb[j*r.W+i]=0xFF000000u|m(16)|m(8)|m(0); } }
  void card(int x,int y,int w2,int h2,int sc){ rrect(x,y,w2,h2,8*sc,C_LINE); rrect(x+1,y+1,w2-2,h2-2,8*sc-1,C_PANEL,0.96f); }
  void btn(int x,int y,int w2,int h2,int sc,const char*label,bool sel,bool prim){
    if(prim){ rrect(x,y,w2,h2,6*sc,sel?0xFFFFC85A:C_ACC); for(int j=0;j<h2;j++){ float t=(float)j/h2; rect(x+3*sc,y+j,w2-6*sc,1,0xFFE08A00,t*0.35f); } textC(x+w2/2,y+h2/2-4*sc,label,sc,0xFF1A1200); }
    else { rrect(x,y,w2,h2,6*sc,sel?C_ACC:C_LINE); rrect(x+1,y+1,w2-2,h2-2,6*sc-1,sel?C_BTNH:C_BTN); textC(x+w2/2,y+h2/2-4*sc,label,sc,C_TXT); } }
  void menuBg(){ rect(0,0,r.W,r.H,0xFF0B0F14,0.80f); for(int j=0;j<r.H;j+=2){ float t=fabsf((j-r.H*0.3f)/r.H); rect(0,j,r.W,2,0xFF0B0F14,std::min(0.45f,t*0.6f)); } }
  void titlePanel(int sc,int x,int w2,int y){ int cx=x+w2/2; textC(cx,y,"WARZONE",sc*4,C_TXT); rect(cx-30*sc,y+42*sc,30*sc,2*sc,0xFFE23B3B); rect(cx,y+42*sc,30*sc,2*sc,0xFF3B82F6);
    textC(cx,y+56*sc,"حرب فرق ثلاثية الأبعاد — الأحمر ضد الأزرق",sc,C_MUT); char bb[48]; snprintf(bb,48,"BUILD %d",BUILD_ID); textC(cx,y+72*sc,bb,std::max(1,sc-1),0xFF607080); }
  void renderMenu(){
    static float mt=0; mt+=0.016f; int showMap=menu==1?menuSel:mapIdx;
    Cam c{0,6,0,mt*0.2f,-0.15f,1.2f}; if(w.id!=showMap)w.load(showMap); r.setCam(c); r.world(w); int W=r.W,H=r.H,sc=std::max(1,H/270); menuBg();
    char b[96]; int sm=std::max(1,sc-1);
    if(menu>=5){ aclRenderMenu(); return; }
    if(menu==3){ int cw=W*6/10,ch=H/2,cx=(W-cw)/2,cy=(H-ch)/2; card(cx,cy,cw,ch,sc); textC(W/2,cy+ch/2-18*sc,"جارٍ تشغيل الشبكة…",sc*2,C_TXT); snprintf(b,96,"STEP: %s",netWhat); textC(W/2,cy+ch/2+10*sc,b,sc,C_ACC); textC(W/2,cy+ch-16*sc,"O: إلغاء",sm,C_MUT); return; }
    if(menu==2){
      int cw=W*6/10,ch=H*7/10,cx=(W-cw)/2,cy=(H-ch)/2; card(cx,cy,cw,ch,sc); textC(W/2,cy+10*sc,"انضم إلى لعبة شبكة محلية",sc*2,C_TXT);
      if(joining)textC(W/2,H/2,"جارٍ الاتصال…",sc*2,C_ACC);
      else if(hosts.empty())textC(W/2,H/2-6*sc,"جارٍ البحث عن ألعاب على شبكتك…",sc,C_MUT);
      else for(int i=0;i<(int)hosts.size();i++){ snprintf(b,96,"%s   MAP %d   %d/%d",hosts[i].name,hosts[i].map+1,hosts[i].players,hosts[i].maxp); btn(cx+10*sc,cy+40*sc+i*24*sc,cw-20*sc,20*sc,sc,b,i==hostSel,false); }
      if(msgT>0)textC(W/2,cy+ch-34*sc,msg,sc,0xFFFF6060);
      textC(W/2,cy+ch-16*sc,"D-pad: تحديد    X: انضمام    O: رجوع",sm,C_MUT); return; }
    static const char*MN[5]={"المستودع","أطلال الصحراء","شارع المدينة","القاعدة القطبية","صحراء الشاص"};
    const char*dn[3]={"سهل","عادي","صعب"},*md[4]={"ضد البوتات","استضافة PS4","انضمام PS4","انضمام أندرويد"};
    const char*go[4]={"العب منفردًا ضد البوتات","استضف لعبة شبكة محلية","انضم إلى لعبة PS4","انضم إلى لعبة أندرويد"};
    int rh=20*sc,gap=6*sc,pad=12*sc,cw=W*45/100,ch=pad*2+3*(rh+gap)+rh+8*sc+6*sc+10*sc,cx=W/16,cy=(H-ch)/2+4*sc;
    card(cx,cy,cw,ch,sc);
    auto sel_=[&](int i,const char*label,const char*val,uint32_t col){ int y=cy+pad+i*(rh+gap),x=cx+pad,w2=cw-pad*2; bool sel=menuRow==i; rrect(x,y,w2,rh,6*sc,sel?col:C_LINE); rrect(x+1,y+1,w2-2,rh-2,6*sc-1,sel?C_BTNH:C_BTN);
      int ty=y+rh/2-4*sc; text(x+w2-8*sc-tw(label,sc),ty,label,sc,C_MUT); textC(x+w2*3/10,ty,val,sc,sel?C_TXT:0xFFB8C4D4); if(sel){ text(x+8*sc,ty,"◀",sc,col); text(x+w2*11/20,ty,"▶",sc,col); } };
    sel_(0,"الخريطة",MN[menuSel%5],0xFFE23B3B); sel_(1,"الصعوبة",dn[diff],C_ACC); sel_(2,"الوضع",md[mode],0xFF3B82F6);
    btn(cx+pad,cy+pad+3*(rh+gap)+4*sc,cw-pad*2,rh+4*sc,sc,go[mode],menuRow==3,true);
    if(msgT>0)textC(cx+cw/2,cy+ch-14*sc,msg,sm,0xFFFF6060);
    titlePanel(sc,W*55/100,W*38/100,H/2-60*sc);
    textC(W*55/100+W*19/100,H/2+30*sc,"اللعب المنفرد بدون إنترنت",sc,C_MUT); textC(W*55/100+W*19/100,H/2+44*sc,"والشبكة المحلية بين PS4 وأندرويد",sc,C_MUT);
    textC(W/2,H-14*sc,"D-pad: اختيار    X: تأكيد    Options: إيقاف مؤقت",std::max(1,sc-1),0xFF607080);
  }
};
#include "acl.h"
