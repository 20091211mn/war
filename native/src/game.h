// لعبة Warzone الأصلية: لاعب + بوتات + أسلحة + تسجيل نقاط + واجهة
#pragma once
#include "lang.h"
#include "render.h"
#include "veh.h"
#include "font.h"
#include "ar.h"
#include "net.h"
#include "sfx.h"
#include "reload.h"
#include "gunmodels.h"
#include <string>
#include <cstdlib>


#ifndef BUILD_ID
#define BUILD_ID 0
#endif
struct Input{
  bool tri=false,seat=false,c4=false,det=false;
  float mx=0,my=0;      // حركة: x يمين، y أمام
  float lx=0,ly=0;      // نظر: x يمين، y أعلى (معدّل)
  bool fire=false,ads=false,jump=false,crouch=false,reload=false,sprint=false,swapP=false,swapN=false,grenade=false,drone=false,use=false,start=false,confirm=false,back=false; int pick=-1;
};

static float frand(){return (float)rand()/(float)RAND_MAX;}

struct Ent{
  bool off=false; /* خانة فارغة في غرفة بلا بوتات */ bool mnt=false; int mvi=-1; // رامي الرشاش المثبّت على الشاص (مقعد الصندوق الخلفي)
  Body b; float yaw=0,pitch=0; int team=0; bool bot=true,alive=false,crouch=false,sprint=false,moving=false;
  float magM=1,relM=1,reveal=0,hp=100,prot=0,respawn=0,phase=0,flash=0,cd=0,reloadT=-1,rkP=0,rkY=0;
  int wpn=0,ammo=0,mags=0,kills=0,deaths=0;
  float think=0,tYaw=0,sx=0,sz=0,stuck=0,turnT=0,turnDir=1,burst=0,react=0,strafe=1,strafeT=0; int target=-1; float lastX=0,lastZ=0,lastDmgT=0; int lastHitBy=-1;
  float revealT=0; int c4Left=2; float c4Refill=0; uint8_t rc4p=0,rc4d=0; bool rc4Init=false;
  char name[16]={0}; bool remote=false,gone=false; uint8_t spawnSeq=0; float tx=0,ty=0,tz=0,tyaw=0; int lastAmmo=0; int lastHp=100; NetInput rin{}; float lastPkt=0; int lastKills=0;
};
struct C4s{float x,y,z; int team,owner;};
struct Tracer{float x0,y0,z0,x1,y1,z1,t;};

struct Jet{ float x=0,y=0,z=0,vx=0,vy=0,vz=0,yaw=0,pitch=0,roll=0,th=0.7f,hp=300,fire=0,cdG=0,respT=0,aiT=0,ab=0,wob=0,evade=0,gunF=0; int team=0,pilot=-1,gun=600,tgt=-1,lastBy=-1; bool alive=false,spare=false,gnd=false,stall=false; };
struct Game{
  World w; Renderer r; std::vector<Ent> e; std::vector<Tracer> tr;
  int mapIdx=0,scoreLim=30,score[2]={0,0},nBots=6,diff=0; float sens=1.0f; bool swapTrig=false,swapL=false,swapSticks=false; float t=0,hitMark=0,dmgFlash=0,wsT=0; bool wheelOpen=false; bool over=false; int winner=0; int streak=0,glCnt=2; float hitAng=0,hitAngT=0;
  float fpsShow=0; std::vector<VH::Veh> vh; int drive=-1; float lookIdle=0; bool nearVeh=false; struct Puff{float x,y,z,vy,t,sz;bool fire;}; std::vector<Puff> puffs; struct Drn{bool on=false;float x=0,y=0,z=0,yaw=0,pitch=0,vx=0,vy=0,vz=0,t=0;} dn; float dnCd=0,dnSpot=0; bool pdn=false,pfireD=false;
  struct Shell{float x,y,z,vx,vy,vz,t;}; std::vector<Shell> shells; struct Gr{float x,y,z,vx,vy,vz,t;}; std::vector<Gr> grs; struct Rk{float ox,oy,oz,dx,dy,dz,len,t,by;}; std::vector<Rk> rks; int gren=2; bool pg=false; struct Spark{float x,y,z,vx,vy,vz,t;}; std::vector<Spark> sparks;
  void sparkAt(float x,float y,float z,float nx,float ny,float nz,int n){ for(int k=0;k<n&&sparks.size()<120;k++)sparks.push_back({x,y,z,-nx*1.5f+(frand()-0.5f)*3.f,-ny*1.5f+frand()*2.5f,-nz*1.5f+(frand()-0.5f)*3.f,0.22f+frand()*0.15f}); }
  int prim=0; Input prev; bool paused=false; float msgT=0; char msg[64]={0}; int shotsFeed[4]={-1,-1,-1,-1}; std::string feed[4]; float feedT[4]={};
  int netGoal=0,netSt=0,netPhase=0; const char*netWhat=""; sockaddr_in pendJoin;

  int menu=1,menuSel=0,menuRow=0,mode=0; int qApplied=-1; bool qChanged=false;
  // ---- لمس مباشر للقوائم: مناطق تُملأ أثناء الرسم ثم تُترجم لنبضة إدخال واحدة ----
  struct UiHit{int x,y,w,h,kind,idx;}; std::vector<UiHit> hits; struct Tap{int mx=0; bool confirm=false,back=false,start=false,tri=false,reload=false,swapP=false,fire=false;} tap;
  void hit_(int x,int y,int w,int h,int kind,int idx=0){ hits.push_back({x,y,w,h,kind,idx}); }
  bool uiTap(int px,int py){ for(int i=(int)hits.size()-1;i>=0;i--){ const UiHit&h=hits[i]; if(px<h.x||px>=h.x+h.w||py<h.y||py>=h.y+h.h)continue; int dir=(px<h.x+h.w*45/100)?-1:1;
      switch(h.kind){
        case 1: if(menu==4){ rmRow=h.idx; tap.mx=dir; } else { menuRow=h.idx; tap.mx=dir; } return true;
        case 2: if(menu==1)menuRow=5; tap.confirm=true; return true;
        case 3: hostSel=h.idx; tap.confirm=true; return true;
        case 4: tap.back=true; return true;
        case 5: tap.start=true; return true;
        case 6: tap.mx=dir; return true;
        case 7: swapSticks=!swapSticks; return true;
        case 8: if(h.idx==0)tap.tri=true; else if(h.idx==1)tap.reload=true; else if(h.idx==2)tap.swapP=true; else tap.fire=true; return true; }
    } return false; } static constexpr int MROWS=6; // menu: 1 رئيسية ، 2 ردهة الانضمام
  // ---- شبكة ----
  int role=0,meI=0; uint8_t matchId=0,spawnCtr=0,evtId=0,lastEvt=0; std::vector<NetEvt> evq; uint16_t netSeq=0;
  Udp sock,beaconRx,beaconTx; float snapT=0,inT=0,beaconT=0,srvSilence=0,joinT=0; uint32_t myIp=0; int sweepIdx=0; float ipT=0; char ipTxt[24]={0}; bool joining=false; sockaddr_in hostAddr; Input lastIn;
  std::vector<NetTr> trOut; struct Cli{bool on=false;sockaddr_in a;int ent=-1;float last=0;}; Cli cli[8];
  struct HostEnt{sockaddr_in a;uint8_t map,players,maxp,lim5=6,mins=0,bots=0,style=0;char name[16];float seen;}; std::vector<HostEnt> hosts; int hostSel=0;
  std::string nm(int i)const{ char b[16]; if(nLocal==2&&i>=0&&i<2){ snprintf(b,16,"P%d",i+1); return b; } if(i==meI)return "YOU"; if(i==0)return "HOST"; if(i<(int)e.size()&&e[i].remote)snprintf(b,16,"P%d",i); else snprintf(b,16,"BOT%d",i); return b; }
  void init(int rw,int rh){ r.init(rw,rh); startMatch(mapIdx); }
  void say(const char*s){snprintf(msg,64,"%s",trl(s));msgT=2.5f;}
  void addFeed(const std::string&s){for(int i=3;i>0;i--){feed[i]=feed[i-1];feedT[i]=feedT[i-1];}feed[0]=s;feedT[0]=5;}
  void startMatch(int idx){
    mapIdx=idx%MAP_COUNT; w.load(mapIdx); e.assign(1+nBots*2+ (0),Ent()); // لاعب + فريقان
    int nbA=(mapIdx==8&&role==0&&nLocal==1)?8:nBots; int total=(duel&&nLocal==2&&role==0)?2:1+nbA*2-1; if(roomOn&&role==1)total=rmMaxE(); e.assign(total,Ent()); c4s.clear();
    for(int i=0;i<total;i++){Ent&p=e[i];p.bot=i>0;p.team=(i==0)?0:((i&1)?1:0);snprintf(p.name,16,"%s",nm(i).c_str());p.wpn=i==0?prim:rand()%WEAPON_COUNT;}
    if(role==1)for(auto&c:cli)if(c.on&&c.ent>0&&c.ent<total){ e[c.ent].remote=true; e[c.ent].bot=false; }
    dn.on=false; dnCd=0; grs.clear(); rks.clear(); vh.clear(); drive=-1; puffs.clear(); if(role<=1)for(int k=0;k<w.m->nv;k++)vh.push_back(makeVeh(k));
    matchId++; evq.clear(); score[0]=score[1]=0;over=false;t=0;tr.clear();
    for(auto&p:e)spawn(p);
    if(roomOn&&role==1){ int nb=rmBotsE(); for(int i=1;i<total;i++)if(i>nb&&!e[i].remote){ e[i].bot=false; e[i].off=true; e[i].alive=false; e[i].respawn=1e9f; } }
    air=(mapIdx==8&&role==0&&nLocal==1); if(air)airInit(); else { jets.clear(); entJet.clear(); }
    if(nLocal==2&&role==0&&e.size()>1){ if(act)lsx(); ls2=LS(); ls2.meI=1; ls2.prim=prim; for(int k=0;k<16;k++)ls2.wAtt[k]=wAtt[k]; e[1].bot=false; e[1].remote=false; e[1].team=coop?0:1; snprintf(e[1].name,16,"P2"); snprintf(e[0].name,16,"P1"); lsx(); spawn(e[1]); lsx(); }
  }
  float optZ=0; float zoomOf(const Ent&p)const{ return (optZ>0&&&p==&e[meI])?optZ:WEAPONS[p.wpn].zoom; }
  int cap(const Ent&p)const{ return (int)(WEAPONS[p.wpn].mag*p.magM+0.5f); }
  void giveWeapon(Ent&p,int wi){p.wpn=wi;p.magM=p.relM=1;if(&p==&e[meI]&&role!=3)optZ=0;p.ammo=WEAPONS[wi].mag;p.mags=WEAPONS[wi].mags;p.reloadT=-1;p.cd=0; if(role!=3&&&p==&e[meI])applyWAtt(p); }
  void spawn(Ent&p){
    if(roomOn&&cfgWpn>=0&&role!=2)p.wpn=cfgWpn;
    if(role!=2&&!e.empty()){ int ix=(int)(&p-&e[0]); if(ix>=0&&ix<(int)e.size())c4Clear(ix); }
    const P2*sp=p.team?w.m->blue:w.m->red; int n=p.team?w.m->nbl:w.m->nr; P2 s=sp[rand()%n];
    // اختر نقطة بعيدة عن الأعداء
    float bestD=-1;for(int k=0;k<4;k++){P2 c=sp[rand()%n];float md=1e9;for(auto&o:e)if(o.alive&&o.team!=p.team){md=std::min(md,(o.b.x-c.x)*(o.b.x-c.x)+(o.b.z-c.z)*(o.b.z-c.z));}if(md>bestD){bestD=md;s=c;}}
    p.b=Body();p.b.x=s.x;p.b.z=s.z;p.b.y=20;for(int i=0;i<120;i++){moveBody(w,p.b,0,0,0.04f,false,false);if(p.b.onGround)break;}
    p.yaw=p.team?1.57f:-1.57f; if(w.m->W<w.m->D*0.0)p.yaw=0; p.pitch=0;
    // الاتجاه نحو وسط الخريطة
    p.yaw=atan2f(-(0-p.b.x),-(0-p.b.z)); if(!p.bot&&!p.remote)gren=2; p.alive=true;p.hp=100;p.prot=2;p.crouch=false;p.spawnSeq=++spawnCtr;giveWeapon(p,(p.bot||p.remote)?p.wpn:prim);
  }
  static float dmgAt(const WeaponDef&d,float dist){ if(dist<=d.r0)return d.dmg; if(dist>=d.r1)return d.dmg*d.minMul; return d.dmg*(1-(1-d.minMul)*(dist-d.r0)/(d.r1-d.r0)); }
  bool visible(const Ent&a,const Ent&b2){
    float ax=a.b.x,ay=a.b.y+PK.eye,az=a.b.z,bx=b2.b.x,by=b2.b.y+1.2f,bz=b2.b.z;
    float dx=bx-ax,dy=by-ay,dz=bz-az,d=sqrtf(dx*dx+dy*dy+dz*dz); if(d<0.01f)return true;
    return raycastMap(w,ax,ay,az,dx/d,dy/d,dz/d,d)>=1e29f;
  }
  void hurt(int vi,int ai,float dmg,bool head){
    Ent&v=e[vi]; if(!v.alive||v.prot>0)return; if(roomOn)dmg*=cfgDmg;
    v.hp-=dmg; v.lastHitBy=ai; v.lastDmgT=t; if(vi==meI){dmgFlash=0.5f;SFX::play(SFX::HURT,0.9f); if(ai>=0&&ai!=vi){ hitAng=atan2f(-(e[ai].b.x-v.b.x),-(e[ai].b.z-v.b.z))-v.yaw; hitAngT=1.2f; } }
    if(ai==meI){hitMark=0.25f;SFX::play(SFX::HIT,0.7f);}
    if(nLocal==2){ if(vi==ls2.meI){ ls2.dmgFlash=0.5f; ls2.streak=0; if(ai>=0&&ai!=vi){ ls2.hitAng=atan2f(-(e[ai].b.x-v.b.x),-(e[ai].b.z-v.b.z))-v.yaw; ls2.hitAngT=1.2f; } } if(ai==ls2.meI&&vi!=ai)ls2.hitMark=0.25f; }
    if(v.hp<=0){ v.alive=false;v.deaths++;v.respawn=4; if(role!=2)c4Clear(vi);
      if(vi==meI)streak=0;
      if(ai==meI&&vi!=meI){ SFX::play(SFX::KILL,0.8f); streak++; if(head)say("تصويبة رأس!"); else if(streak==2)say("قتلتان متتاليتان"); else if(streak==3)say("ثلاث قتلات!"); else if(streak==5)say("سلسلة قتل!"); else if(streak==8)say("لا يمكن إيقافك!"); }
      if(ai>=0&&ai!=vi){Ent&k=e[ai];k.kills++;if(!air)score[k.team]++; char b[64];snprintf(b,64,"%s > %s%s",nm(ai).c_str(),nm(vi).c_str(),head?" HS":"");addFeed(b); evq.push_back({++evtId,(uint8_t)ai,(uint8_t)vi,(uint8_t)head}); if(evq.size()>6)evq.erase(evq.begin());} 
      if(score[0]>=scoreLim||score[1]>=scoreLim){over=true;winner=score[0]>=scoreLim?0:1;}
    }
  }
  // ---- الشاص ----
  void explodeVeh(VH::Veh&v,int by){
    SFX::play(SFX::EXPLODE,std::max(0.2f,1.f-sqrtf((v.x-e[meI].b.x)*(v.x-e[meI].b.x)+(v.z-e[meI].b.z)*(v.z-e[meI].b.z))/80.f)); v.dead=true; v.respawnT=30; v.burn=false; if(v.driver>=0){ if(v.driver==meI)drive=-1; v.driver=-1; }
    for(int k=0;k<14;k++)puffs.push_back({v.x+(frand()-0.5f)*3,0.6f+frand()*1.5f,v.z+(frand()-0.5f)*3,1.5f+frand()*2.f,1.2f+frand()*1.2f,1.2f+frand()*1.5f,k<8});
    for(int i=0;i<(int)e.size();i++){ if(!e[i].alive)continue; float dx=e[i].b.x-v.x,dz=e[i].b.z-v.z,dd=sqrtf(dx*dx+dz*dz); if(dd<7.f)hurt(i,by,260*powf(1-dd/7.f,1.1f),false); }
    if(by==meI)hitMark=0.4f; say("دُمِّرت المركبة!");
  }
  void hurtVeh(VH::Veh&v,float amt,int by){ if(v.dead||amt<=0)return; v.hp-=amt; if(v.hp<=0)explodeVeh(v,by); else if(v.hp<150)v.burn=true; }
  void damageVeh(VH::Veh&v,int part,float d,int by){ if(v.dead)return;
    if(part>=10){ int i=part-10; if((v.tires>>i)&1)return; v.tireHp[i]-=d; if(v.tireHp[i]<=0)v.tires|=1<<i; return; }
    hurtVeh(v,(part==1?d*1.3f:d*0.35f)*VH::vp(v).armor,by); if(by==meI)hitMark=0.25f; }
  int vConf=0;
  void puffsBoom(float x,float z){ for(int k=0;k<14;k++)puffs.push_back({x+(frand()-0.5f)*3,0.6f+frand()*1.5f,z+(frand()-0.5f)*3,1.5f+frand()*2.f,1.2f+frand()*1.2f,1.2f+frand()*1.5f,k<8}); SFX::play(SFX::EXPLODE,std::max(0.2f,1.f-hypotf(x-e[meI].b.x,z-e[meI].b.z)/80.f)); }
  VH::Veh makeVeh(int k){ VH::Veh v; if(k<w.m->nv){ v.x=v.sx=v.tx=w.m->veh[k].x; v.z=v.sz=v.tz=w.m->veh[k].z; v.h=v.sh=v.th=w.m->veh[k].h; v.type=w.m->veh[k].t; } v.hp=VH::VPS[v.type==1].hp; v.ty=v.h; return v; }
  float camX=0,camY=3,camZ=0; bool pFireV=false;
  // مدفع الدبابة: قذيفة بخط مستقيم، انفجار شظايا
  void cannonFx(VH::Veh&v,float dx,float dy,float dz,bool dmg,int by){
    float ox=v.x+dx*4.1f,oy=1.65f+dy*4.1f,oz=v.z+dz*4.1f; float best=std::min<float>(raycastMap(w,ox,oy,oz,dx,dy,dz,160.f),160.f);
    for(auto&o:vh){ if(&o==&v||o.dead)continue; int pt; float tv=VH::rayVeh(o,ox,oy,oz,dx,dy,dz,best,pt); if(tv<best)best=tv; }
    float ex=ox+dx*best,ey=std::max(0.1f,oy+dy*best),ez=oz+dz*best; v.cannonT=2.5f; v.kick=1.f;
    if(tr.size()<64)tr.push_back({ox,oy,oz,ex,ey,ez,0.12f}); if(role==1&&trOut.size()<8)trOut.push_back({ox,oy,oz,ex,ey,ez});
    for(int j=0;j<6;j++)puffs.push_back({ox+(frand()-0.5f)*0.6f,oy,oz+(frand()-0.5f)*0.6f,0.8f+frand(),0.5f+frand()*0.4f,0.5f,false});
    for(int j=0;j<12;j++)puffs.push_back({ex+(frand()-0.5f)*3,ey+frand()*1.5f,ez+(frand()-0.5f)*3,1.2f+frand()*1.8f,0.8f+frand()*0.6f,1.f+frand()*1.2f,j<6});
    sparkAt(ex,ey,ez,0,-1,0,40); SFX::play(SFX::EXPLODE,1.f); if(by==meI)hitMark=0.3f;
    if(!dmg)return; const float R=6.5f;
    for(int i=0;i<(int)e.size();i++){ if(!e[i].alive)continue; if(e[i].team==e[by].team&&i!=by)continue; float qx=e[i].b.x-ex,qy=e[i].b.y+1.f-ey,qz=e[i].b.z-ez,d=sqrtf(qx*qx+qy*qy+qz*qz); if(d<R)hurt(i,by,230.f*powf(1-d/R,0.9f),false); }
    for(auto&o:vh){ if(o.dead)continue; float qx=o.x-ex,qz=o.z-ez,d=sqrtf(qx*qx+qz*qz); if(d<R+1.5f)hurtVeh(o,420.f*(1-d/(R+1.5f)),by); }
  }
  void tankAimDir(VH::Veh&v,float yaw,float pitch,float*d){ float cp=cosf(pitch),ddx=-sinf(yaw)*cp,ddy=sinf(pitch),ddz=-cosf(yaw)*cp;
    float t=std::min<float>(raycastMap(w,camX,camY,camZ,ddx,ddy,ddz,160.f),160.f); float px=camX+ddx*t,py=camY+ddy*t,pz=camZ+ddz*t; float ax=px-v.x,ay=py-1.65f,az=pz-v.z,l=sqrtf(ax*ax+ay*ay+az*az)+1e-5f; d[0]=ax/l;d[1]=ay/l;d[2]=az/l; }
  std::vector<C4s> c4s; uint8_t c4pCnt=0,c4dCnt=0; bool pc4=false,pdet=false;
  bool c4Place(int i,float dx,float dy,float dz){
    Ent&p=e[i]; if(!p.alive||p.c4Left<=0)return false; int mine=0; for(auto&c:c4s)if(c.owner==i)mine++; if(mine>=3)return false;
    float l=sqrtf(dx*dx+dy*dy+dz*dz); if(l<0.5f)return false; dx/=l;dy/=l;dz/=l; float ey=p.b.y+(p.crouch?PK.eyec:PK.eye),R=4.5f;
    float t=raycastMap(w,p.b.x,ey,p.b.z,dx,dy,dz,R); if(t>=R){ if(dy>=-0.05f)return false; float tt=(ey-0.05f)/-dy; if(tt>R)return false; t=tt; }
    float x=p.b.x+dx*(t-0.06f),y=std::max<float>(0.05f,ey+dy*(t-0.06f)),z=p.b.z+dz*(t-0.06f);
    p.c4Left--; c4s.push_back({x,y,z,p.team,i}); if(i==meI)SFX::play(SFX::EMPTY,0.8f); return true; }
  void blastAt(float x,float y,float z,int owner,int team,float R,float pd,float vd){
    for(int k=0;k<16;k++)puffs.push_back({x+(frand()-0.5f)*3,y+frand()*1.5f,z+(frand()-0.5f)*3,1.4f+frand()*2.f,1.f+frand()*1.f,1.2f+frand()*1.4f,k<9});
    sparkAt(x,y,z,0,-1,0,40); { float ddx=x-e[meI].b.x,ddz=z-e[meI].b.z; SFX::play(SFX::EXPLODE,std::max(0.25f,1.f-sqrtf(ddx*ddx+ddz*ddz)/70.f)); }
    if(role==2)return; // العميل يرى النتيجة من المضيف
    for(int i=0;i<(int)e.size();i++){ if(!e[i].alive)continue; if(e[i].team==team&&i!=owner)continue; float qx=e[i].b.x-x,qy=e[i].b.y+1.f-y,qz=e[i].b.z-z,d=sqrtf(qx*qx+qy*qy+qz*qz); if(d>R)continue;
      float tw=raycastMap(w,x,y+0.2f,z,qx/(d+1e-5f),(qy-0.2f)/(d+1e-5f),qz/(d+1e-5f),d); if(tw<d-0.3f)continue; hurt(i,owner,pd*powf(1-d/R,1.2f),false); }
    for(auto&o:vh){ if(o.dead)continue; float qx=o.x-x,qz=o.z-z,d=std::max<float>(0.f,sqrtf(qx*qx+(y-0.8f)*(y-0.8f)+qz*qz)-1.6f); if(d<R)hurtVeh(o,vd*powf(1-d/R,1.1f),owner); } }
  void c4Det(int i){ std::vector<C4s> mine; for(size_t k=0;k<c4s.size();){ if(c4s[k].owner==i){ mine.push_back(c4s[k]); c4s.erase(c4s.begin()+k); } else k++; }
    if(mine.empty())return; e[i].c4Refill=20.f; for(auto&c:mine)blastAt(c.x,c.y,c.z,i,c.team,8.f,210.f,380.f); }
  void c4Clear(int i){ for(size_t k=0;k<c4s.size();){ if(c4s[k].owner==i)c4s.erase(c4s.begin()+k); else k++; } e[i].c4Left=2; e[i].c4Refill=0; }
  void c4Tick(float dt){ for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(p.alive&&p.c4Left<2&&p.c4Refill>0){ p.c4Refill-=dt; bool any=false; for(auto&c:c4s)if(c.owner==i)any=true; if(p.c4Refill<=0&&!any){ p.c4Left=2; p.c4Refill=0; } } } }
  void exitVeh(){ gunner=false; if(drive<0)return; VH::Veh&v=vh[drive]; Ent&p=e[meI]; static const float SPS[4][2]={{-1.9f,-0.25f},{1.9f,-0.25f},{0,3.6f},{0,-3.6f}},SPT[4][2]={{2.6f,0},{-2.6f,0},{0,4.2f},{0,-4.2f}}; const float (*SP)[2]=v.type==1?SPT:SPS;
    float bx=v.x,bz=v.z; for(int q=0;q<4;q++){ auto&s=SP[q]; float wx,wz; VH::l2w(v,s[0],s[1],wx,wz); if(!blocked(w,wx,0.05f,wz,PK.h)){ bx=wx;bz=wz;break; } }
    p.b=Body(); p.b.x=bx;p.b.z=bz;p.b.y=0.3f; for(int k=0;k<30;k++){moveBody(w,p.b,0,0,0.04f,false,false);if(p.b.onGround)break;} p.pitch=0; v.driver=-1; drive=-1; }
  void updateVeh(float dt,const Input&in){
    Ent&me=e[meI]; nearVeh=false; bool tri=in.tri&&!prev.tri;
    if(drive>=0&&(!me.alive||vh[drive].dead)){ vh[drive].driver=-1; drive=-1; }
    if(me.alive&&drive<0){ int best=-1; float bd=2.8f; for(int k=0;k<(int)vh.size();k++){ auto&v=vh[k]; if(v.dead)continue; float lx,lz; VH::w2l(v,me.b.x,me.b.z,lx,lz); const VH::VP&P=VH::vp(v); float dd=hypotf(std::max<float>(0.f,fabsf(lx)-P.hw),std::max<float>(0.f,fabsf(lz)-P.hl));
        if(dd<bd&&me.b.y<2.3f&&v.driver<0&&(v.team<0||v.team==me.team)){bd=dd;best=k;} }
      nearVeh=best>=0; if(nearVeh&&tri){ drive=best; vh[best].driver=meI; vh[best].team=me.team; me.pitch=-0.18f; me.crouch=false; me.reloadT=-1; lookIdle=0; } }
    else if(me.alive&&drive>=0&&tri)exitVeh();
    for(int k=0;k<(int)vh.size();k++){ VH::Veh&v=vh[k];
      if(vpass==1&&k!=drive)continue; if(vpass==0&&nLocal==2&&v.driver>=0&&v.driver==ls2.meI)continue;
      if(v.driver>=0&&v.driver!=meI&&role==1&&(v.driver>=(int)e.size()||!e[v.driver].remote||!e[v.driver].alive))v.driver=-1;
      if(role==2&&k!=drive){ float a=std::min<float>(1.f,dt*14.f); v.x+=(v.tx-v.x)*a; v.z+=(v.tz-v.z)*a; float dh=v.th-v.h; while(dh>3.14159f)dh-=6.28318f; while(dh<-3.14159f)dh+=6.28318f; v.h+=dh*a; if(v.burn&&!v.dead){ v.smoke-=dt; if(v.smoke<=0){ v.smoke=0.12f; puffs.push_back({v.x+(frand()-0.5f),1.3f,v.z+(frand()-0.5f),1.6f,0.9f,0.5f,frand()<0.3f}); } } continue; }
      if(role==2&&v.dead)continue;
      if(role==1&&!v.dead&&v.driver>=0&&v.driver!=meI){ // يقوده لاعب عن بُعد: الحالة من مدخلاته
        if(v.burn){ v.hp-=6*dt; if(v.hp<=0){ explodeVeh(v,v.driver); } } continue; }
      if(v.dead){ v.respawnT-=dt; if(v.respawnT<=0){ VH::Veh n; n.type=v.type; n.hp=VH::vp(v).hp; n.x=n.sx=v.sx;n.z=n.sz=v.sz;n.h=n.sh=n.ty=v.sh; v=n; } continue; }
      bool dr=drive==k&&me.alive;
      if(dr&&role!=3){ if(in.seat&&!pSeat)gunner=!gunner; pSeat=in.seat; }
      float thr=0,st=0; bool hb=false; if(dr&&gunner){ hb=true; } else if(dr){ thr=v.type==1?std::max<float>(-1.f,std::min<float>(1.f,in.my-(in.ads?1.f:0.f))):std::max<float>(-1.f,std::min<float>(1.f,in.my+(in.fire?1.f:0.f)-(in.ads?1.f:0.f))); st=in.mx; hb=in.jump; }
      if(v.type!=1){ int gi=-1; for(int j=0;j<(int)e.size();j++)if(e[j].mnt&&e[j].mvi==k&&e[j].alive){gi=j;break;} v.mgFp=(gi==meI&&drive==k); if(gi>=0){ float dy=e[gi].yaw-v.ty; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; v.ty+=dy*std::min<float>(1.f,dt*12.f); v.tp=std::max<float>(-0.5f,std::min<float>(0.9f,e[gi].pitch)); } else { v.ty=v.h; v.tp=0; } }
      if(v.type==1){ if(v.cannonT>0)v.cannonT-=dt; v.kick=std::max<float>(0.f,v.kick-dt*3); int di=dr?meI:v.driver; if(di>=0&&di<(int)e.size()){ v.ty=e[di].yaw; v.tp=std::max<float>(-0.35f,std::min<float>(0.5f,e[di].pitch)); } else v.ty=v.h;
        if(dr&&!gunner&&in.fire&&v.cannonT<=0&&role!=2&&role!=3){ float d[3]; tankAimDir(v,me.yaw,me.pitch,d); cannonFx(v,d[0],d[1],d[2],true,meI); }
        else if(dr&&!gunner&&role==2&&in.fire&&v.cannonT<=0){ float d[3]; tankAimDir(v,me.yaw,me.pitch,d); v.cannonT=2.5f; v.kick=1.f; SFX::play(SFX::EXPLODE,0.8f); } }
      if(!dr)hb=fabsf(v.speed)<2.f; // بلا سائق: فرامل يد عند البطء فقط
      float crash=VH::step(w,v,vh,thr,st,hb,dt);
      if(crash>VH::vp(v).crashMin&&v.crashT<=0){ v.crashT=0.45f; if(role!=2)hurtVeh(v,(crash-VH::vp(v).crashMin)*9,v.driver); if(dr)dmgFlash=std::max<float>(dmgFlash,0.15f); } if(v.crashT>0)v.crashT-=dt;
      if(v.dead)continue;
      if(v.burn){ if(role!=2)v.hp-=6*dt; v.smoke-=dt; if(v.smoke<=0){ v.smoke=0.12f; puffs.push_back({v.x+(frand()-0.5f),1.3f,v.z+(frand()-0.5f),1.6f,0.9f,0.5f,frand()<0.3f}); } if(v.hp<=0&&role!=2){ explodeVeh(v,v.driver); continue; } }
      for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(!p.alive||(dr&&i==meI)||p.gone)continue; VH::pushOut(v,p.b.x,p.b.z,p.b.y);
        if(role!=2&&dr&&fabsf(v.speed)>5.f&&p.team!=me.team){ float lx,lz; VH::w2l(v,p.b.x,p.b.z,lx,lz); if(fabsf(lx)<VH::vp(v).hw+0.45f&&fabsf(lz)<VH::vp(v).hl+0.45f&&p.b.y<1.8f&&p.prot<=0)hurt(i,meI,fabsf(v.speed)*(v.type==1?24:14),false); } }
      if(dr){ float sx,sz; if(gunner){ if(v.type==1)VH::l2w(v,0,2.0f,sx,sz); else VH::l2w(v,0.f,1.9f,sx,sz); me.b.y=v.type==1?1.55f:0.85f; } else if(v.type==1){ VH::l2w(v,0,-0.5f,sx,sz); me.b.y=1.0f; } else { VH::l2w(v,-0.45f,-0.25f,sx,sz); me.b.y=0.35f; } me.b.x=sx;me.b.z=sz;me.b.vy=0; me.moving=false; }
    }
    if(vpass==0){ for(auto&q:puffs){ q.t-=dt; q.y+=q.vy*dt; q.sz+=dt*0.8f; } puffs.erase(std::remove_if(puffs.begin(),puffs.end(),[](Puff&q){return q.t<=0;}),puffs.end()); }
  }
  static bool isExpl(int w){ return !strcmp(WEAPONS[w].id,"rpg"); }
  void explosionAt(float gx,float gy,float gz,int by,float R,float pd,float vd){
    for(int i=0;i<(int)e.size();i++){ if(!e[i].alive)continue; if(i!=by&&e[i].team==e[by].team)continue; float ex=e[i].b.x-gx,ey=e[i].b.y+0.9f-gy,ez=e[i].b.z-gz,dd=sqrtf(ex*ex+ey*ey+ez*ez); if(dd<R)hurt(i,by,pd*powf(1.f-dd/R,1.2f),false); }
    for(auto&v:vh){ if(v.dead)continue; float ex=v.x-gx,ez=v.z-gz,dd=std::max(0.f,sqrtf(ex*ex+ez*ez)-1.6f); if(dd<R)hurtVeh(v,vd*powf(1.f-dd/R,1.1f),by); }
    if(air){ for(int k=0;k<(int)jets.size();k++){ Jet&T=jets[k]; if(!T.alive||T.team==e[by].team)continue; float dx=T.x-gx,dy=T.y-gy,dz=T.z-gz,d=sqrtf(dx*dx+dy*dy+dz*dz)-5.f; if(d<R+4.f)jetHurt(k,pd*1.3f*powf(1.f-std::max(0.f,d)/(R+4.f),0.8f),by,false); } }
    puffsBoom(gx,gz); for(int j=0;j<10;j++)puffs.push_back({gx+(frand()-0.5f)*2,gy+frand()*1.2f,gz+(frand()-0.5f)*2,1.2f+frand()*1.5f,0.7f+frand()*0.5f,0.9f+frand()*0.9f,j<5}); }
  void fire(int i){
    Ent&p=e[i]; const WeaponDef&d=p.mnt?MOUNTDEF:WEAPONS[p.wpn]; const bool ex=!p.mnt&&isExpl(p.wpn); if(!(i==meI&&role!=3&&!p.mnt&&wAtt[p.wpn&15].muz==1))p.revealT=5.f; // يظهر على رادار الخصم 5 ث بعد كل طلقة (إلا بالكاتم)
    float aDm=1.f,aHip=1.f,aRv=1.f,aRh=1.f,aVol=1.f; // تأثير الملحقات (كما في النسخة الأندرويد/الويب): كاتم/معوّض فوهة، مقبض، ليزر
    if(i==meI&&role!=3&&!p.mnt){ const GAtt&at=wAtt[p.wpn&15]; if(at.muz==1){aDm=0.95f;aRv*=0.92f;aVol=0.4f;} else if(at.muz==2){aRh*=0.55f;aRv*=0.88f;} if(at.under==1){aRv*=0.82f;aRh*=0.85f;aHip=0.9f;} else if(at.under==2)aHip=0.6f; }
    { float ddx=p.b.x-e[meI].b.x,ddz=p.b.z-e[meI].b.z; float vv=1.f-sqrtf(ddx*ddx+ddz*ddz)/70.f; SFX::shot(p.mnt?4:WKIND[p.wpn%WEAPON_COUNT],aVol<1.f,(i==meI?1.f:vv*0.8f)*(aVol<1.f?1.f:1.f),0); }
    if(i==meI&&drive<0&&shells.size()<40){ float cy=cosf(p.yaw),sy=sinf(p.yaw); float rx=cy,rz=-sy; // اليمين
      shells.push_back({p.b.x+rx*0.2f-sy*0.3f,p.b.y+(p.crouch?PK.eyec:PK.eye)-0.18f,p.b.z+rz*0.2f-cy*0.3f,rx*(1.8f+frand())+(frand()-0.5f),1.8f+frand()*0.8f,rz*(1.8f+frand())+(frand()-0.5f),1.5f}); }
    if(!p.mnt)p.ammo--; p.cd=60.f/d.rpm; p.flash=0.06f; p.prot=0;
    float moveF=p.moving?1.6f:1.f; bool ads=(i==meI)&&adsHeld;
    float sp=(ads?d.ads:d.hip*aHip)*moveF; if(p.crouch)sp*=0.7f;
    float eyeY=p.b.y+(p.crouch?PK.eyec:PK.eye);
    
    for(int k=0;k<d.pellets;k++){
      float a=sp; if(p.bot){a=(0.022f+0.05f*0.5f)*(diff==0?3.2f:diff==1?1.5f:0.8f);}
      float yw=p.yaw+(frand()-0.5f)*2*a, pt=p.pitch+(frand()-0.5f)*2*a; if(d.pellets>1){yw+=(frand()-0.5f)*0.09f;pt+=(frand()-0.5f)*0.09f;}
      float cp=cosf(pt); float dx=-sinf(yw)*cp,dy=sinf(pt),dz=-cosf(yw)*cp;

      float tw=raycastMap(w,p.b.x,eyeY,p.b.z,dx,dy,dz,d.r1*1.4f); float best=std::min<float>(tw,d.r1*1.4f); int hv=-1; bool hh=false;
      for(int j=0;j<(int)e.size();j++){ if(j==i||!e[j].alive||e[j].team==p.team)continue;
        RayHit h=rayPlayer(e[j].b.x,e[j].b.y,e[j].b.z,e[j].crouch,p.b.x,eyeY,p.b.z,dx,dy,dz,best); if(h.ok&&h.t<best){best=h.t;hv=j;hh=h.head;} }
      { VH::Veh*vhit=nullptr;int vpart=0; for(auto&v:vh){ if(v.dead)continue; if(v.driver==i)continue; /* مركبتي لا تحجب رصاصي */ if(v.driver>=0&&e[v.driver].team==p.team)continue; int pt2; float tv=VH::rayVeh(v,p.b.x,eyeY,p.b.z,dx,dy,dz,best,pt2); if(tv<best){best=tv;vhit=&v;vpart=pt2;hv=-1;} }
        if(vhit&&!ex)damageVeh(*vhit,vpart,dmgAt(d,best)*aDm,i); }
      if(air){ int hj=-1; for(int j=0;j<(int)jets.size();j++){ const Jet&J=jets[j]; if(!J.alive||J.team==p.team)continue; float ox=J.x-p.b.x,oy=J.y+1.f-eyeY,oz=J.z-p.b.z,tc=ox*dx+oy*dy+oz*dz; if(tc<0||tc>=best)continue; float px2=ox-dx*tc,py2=oy-dy*tc,pz2=oz-dz*tc; if(px2*px2+py2*py2+pz2*pz2<30.f){ best=tc; hj=j; hv=-1; } }
        if(hj>=0&&!ex)jetHurt(hj,dmgAt(d,best)*aDm*0.4f,i,false); }
      if(hv>=0&&!ex)hurt(hv,i,dmgAt(d,best)*aDm*(hh?d.head:1.f),hh);
      if(ex&&role!=2&&rks.size()<8){ rks.push_back({p.b.x+dx*0.9f,eyeY-0.15f+dy*0.9f,p.b.z+dz*0.9f,dx,dy,dz,std::max(0.5f,best-0.9f),0.f,(float)i}); } // قذيفة تطير 115 م/ث ثم تنفجر
      if(best<d.r1*1.39f)sparkAt(p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best,dx,dy,dz,hv>=0?0:5); if(tr.size()<64)tr.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best,0.09f}); if(role==1&&trOut.size()<8)trOut.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best});
    }

    if(i==meI&&role!=2){ p.rkP+=d.recoilV*aRv*(ads?0.6f:1.f)*1.6f; p.rkY+=(frand()-0.5f)*d.recoilH*aRh*3.f; }
  }
  bool adsHeld=false;
  void reloadStart(Ent&p){const WeaponDef&d=WEAPONS[p.wpn]; if(p.mnt||p.reloadT>=0||p.ammo>=cap(p)||p.mags<=0)return; p.reloadT=0; if(&p==&e[meI])SFX::reload(WKIND[p.wpn%WEAPON_COUNT],d.reload*p.relM,0.8f); }
  void updateWeapon(int i,float dt,bool trig){
    Ent&p=e[i]; const WeaponDef&d=p.mnt?MOUNTDEF:WEAPONS[p.wpn]; if(p.cd>0)p.cd-=dt; if(p.flash>0)p.flash-=dt;
    if(p.mnt){ if(trig&&p.cd<=0)fire(i); return; } // رشاش مثبّت: بلا تعبئة ولا نفاد ذخيرة
    if(p.reloadT>=0){p.reloadT+=dt; if(p.reloadT>=d.reload*p.relM){p.reloadT=-1;p.mags--;p.ammo=cap(p);} return;}
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
      float dy=want-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*std::min<float>(1.f,dt*(vis?(diff==0?3.5f:diff==1?6.f:10.f):3.f));
      // ارتفاع الهدف
      float pitchW=atan2f((t2.b.y+1.3f)-(p.b.y+PK.eye),std::max<float>(1.f,d)); p.pitch+=(pitchW-p.pitch)*std::min<float>(1.f,dt*8);
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
    updateWeapon(i,dt,trig); if(p.ammo<=cap(p)/6&&!trig)reloadStart(p);
  }
  // ---- اللاعب ----
  bool gunner=false,pSeat=false; bool passenger()const{ return drive>=0&&gunner; }
  void updatePlayer(float dt,const Input&in){
    Ent&p=e[meI]; adsHeld=in.ads;
    p.mnt=drive>=0&&drive<(int)vh.size()&&vh[drive].type==0&&gunner; p.mvi=p.mnt?drive:-1;
    float zoom=in.ads?zoomOf(p):1.f; float sens=1.f/ (in.ads?std::max<float>(1.f,zoom*0.8f):1.f);
    p.yaw-=in.lx*dt*sens*this->sens; p.pitch+=in.ly*dt*sens*this->sens; p.yaw+=p.rkY*std::min<float>(1.f,dt*30); p.pitch+=p.rkP*std::min<float>(1.f,dt*30); p.rkP*=0.8f; p.rkY*=0.8f;
    p.pitch=std::max<float>(-1.45f,std::min<float>(1.45f,p.pitch));
    if(drive>=0){ // أثناء القيادة: العصا اليمنى للكاميرا فقط ، وتعود خلف الشاص تلقائياً
      if(fabsf(in.lx)+fabsf(in.ly)<0.05f){ lookIdle+=dt; if(lookIdle>1.0f){ float dy=vh[drive].h-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*std::min<float>(1.f,dt*1.6f); } } else lookIdle=0;
      if(passenger()){ // راكب في صندوق/مقعد الشاص: حرية النظر والتصويب والإطلاق من مقعده
        p.pitch=std::max<float>(-1.2f,std::min<float>(1.2f,p.pitch)); p.crouch=true; p.sprint=false; p.moving=false; lookIdle=0;
        if(in.reload)reloadStart(p); if((in.swapP&&!prev.swapP)||(in.swapN&&!prev.swapN)){ prim=(prim+(in.swapN&&!prev.swapN?1:WEAPON_COUNT-1))%WEAPON_COUNT; giveWeapon(p,prim); wsT=2.2f; } else if(in.pick>=0&&in.pick<WEAPON_COUNT&&in.pick!=prim){ prim=in.pick; giveWeapon(p,prim); wsT=2.2f; }
        bool trg=in.fire; if(!p.mnt&&!WEAPONS[p.wpn].autof&&prev.fire)trg=false; updateWeapon(meI,dt,trg); return; }
      p.pitch=std::max<float>(-0.7f,std::min<float>(0.5f,p.pitch)); p.crouch=false; p.sprint=false; p.moving=false; adsHeld=false; return; }
    bool wantCrouch=in.crouch; if(!wantCrouch&&p.crouch&&blocked(w,p.b.x,p.b.y,p.b.z,PK.h))wantCrouch=true; p.crouch=wantCrouch;
    float mx=in.mx,mz=in.my; float ml=hypotf(mx,mz); if(ml>1){mx/=ml;mz/=ml;}
    p.sprint=in.sprint&&mz>0.3f&&!in.ads&&!p.crouch;
    float sp=(p.crouch?PK.crouch:(p.sprint?PK.sprint:PK.walk))*WEAPONS[p.wpn].move*(in.ads?0.65f:1.f);
    float fxw=-sinf(p.yaw),fzw=-cosf(p.yaw),rxw=cosf(p.yaw),rzw=-sinf(p.yaw);
    float vx=(mz*fxw+mx*rxw)*sp,vz=(mz*fzw+mx*rzw)*sp; float ox=p.b.x,oz=p.b.z;
    moveBody(w,p.b,vx,vz,dt,in.jump,p.crouch); float mv=hypotf(p.b.x-ox,p.b.z-oz); p.moving=mv>0.001f&&p.b.onGround; p.phase+=mv*3.4f;
    if(in.reload&&role!=2)reloadStart(p);

    else if((in.swapP&&!prev.swapP)||(in.swapN&&!prev.swapN)){ prim=(prim+(in.swapN&&!prev.swapN?1:WEAPON_COUNT-1))%WEAPON_COUNT; giveWeapon(p,prim); wsT=2.2f; } else if(in.pick>=0&&in.pick<WEAPON_COUNT&&in.pick!=prim){ prim=in.pick; giveWeapon(p,prim); wsT=2.2f; }
    bool trig=in.fire&&!p.sprint; if(!WEAPONS[p.wpn].autof&&prev.fire)trig=false;
    if(role!=2)updateWeapon(meI,dt,trig);
    { bool pl=in.c4&&!pc4,dtn=in.det&&!pdet; pc4=in.c4; pdet=in.det; if(pl&&p.alive){ float cpt=std::min<float>(p.pitch-0.45f,-0.45f),cp=cosf(cpt),dx=-sinf(p.yaw)*cp,dy=sinf(cpt),dz=-cosf(p.yaw)*cp;
        if(role<=1)c4Place(meI,dx,dy,dz); else if(role==2)c4pCnt++; }
      if(dtn&&p.alive){ if(role<=1)c4Det(meI); else if(role==2)c4dCnt++; } }
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
    myIp=localIp(); if(netGoal==1){ role=1; meI=0; for(auto&c:cli)c=Cli(); menu=0; startMatch(menuSel); say("HOSTING LAN GAME - WAITING FOR PLAYERS"); msgT=8; snapT=inT=beaconT=0; }
    else if(netGoal==2){ menu=2; hosts.clear(); hostSel=0; joining=false; }
    else { hostAddr=pendJoin; joining=true; joinT=0; menu=2; NetHello h{T_HELLO,NET_VER}; sock.send(hostAddr,&h,sizeof h); }
  }
  void fillBeacon(NetBeacon&bc)const{ memset(&bc,0,sizeof bc); bc.t=T_BEACON;bc.map=mapIdx;bc.players=(uint8_t)humans(); if(roomOn){ bc.maxp=(uint8_t)e.size(); snprintf(bc.name,16,"%s",rmNameOf(rmName)); bc.lim5=(uint8_t)(scoreLim/5); bc.mins=(uint8_t)(timeLim/60); bc.bots=(uint8_t)rmBotsE(); bc.style=(uint8_t)rmStyle; } else { bc.maxp=8; snprintf(bc.name,16,"LAN HOST"); bc.lim5=6; } }
  void hostStart(){ netBegin(1); }
  void lobbyStart(){ netBegin(2); }
  void joinAddr(const sockaddr_in&a){ pendJoin=a; netBegin(3); }
  void joinDirect(const char*ip){ joinAddr(mkAddr(inet_addr(ip),PORT_GAME)); }
  int assignSlot(){
    int hc[2]={0,0}; for(auto&p:e)if(p.remote)hc[p.team]++; hc[0]+=1; // المضيف في الأحمر
    int pref=hc[0]<=hc[1]?0:1;
    for(int pk=0;pk<2;pk++)for(int pass=0;pass<2;pass++)for(int i=1;i<(int)e.size();i++){ Ent&p=e[i]; if(p.remote)continue; if(pk==0?!p.off:!(p.bot&&!p.off))continue; if(p.team==(pass?1-pref:pref))return i; } // الخانات الفارغة أولاً ثم البوتات
    return -1;
  }
  void packSnap(){
    uint8_t buf[1400]; NetSnapHead h; h.t=T_SNAP;h.map=mapIdx;h.matchId=matchId;h.n=(uint8_t)e.size();h.over=over;h.winner=winner;h.s0=score[0];h.s1=score[1];h.seq=++netSeq; h.nVeh=(uint8_t)std::min<size_t>(8,vh.size()); h.nC4=(uint8_t)std::min<size_t>(6,c4s.size());
    int ne=(int)e.size(); h.nEvt=(uint8_t)evq.size(); h.nTr=(uint8_t)trOut.size(); int off=sizeof h; memcpy(buf,&h,off);
    for(int i=0;i<ne;i++){ const Ent&p=e[i]; NetEnt n; n.x=p.b.x;n.y=p.b.y;n.z=p.b.z;n.yaw=p.yaw;n.pitch=p.pitch;n.flash=p.flash;n.phase=p.phase;n.reloadT=p.reloadT;n.hp=p.off?(int16_t)-1:(int16_t)p.hp;
      n.flags=(p.alive?1:0)|(p.crouch?2:0)|(p.moving?4:0)|(p.sprint?8:0)|(p.team?16:0)|((p.remote||i==0)?32:0)|(p.prot>0?64:0)|(p.revealT>0?128:0); n.wpn=p.wpn;n.ammo=p.ammo;n.mags=p.mags;n.spawnSeq=p.spawnSeq;n.kills=p.kills;n.deaths=p.deaths;n.c4=(uint8_t)p.c4Left; memcpy(buf+off,&n,sizeof n);off+=sizeof n; }
    for(auto&v:evq){memcpy(buf+off,&v,sizeof v);off+=sizeof v;}
    for(auto&q:trOut){memcpy(buf+off,&q,sizeof q);off+=sizeof q;} trOut.clear();
    for(int k=0;k<h.nVeh;k++){ const VH::Veh&v=vh[k]; NetVeh n; n.x=v.x;n.z=v.z;n.h=v.h;n.speed=v.speed;n.steer=v.steer;n.hp=(int16_t)v.hp;n.driver=(int8_t)v.driver;n.flags=(v.dead?1:0)|(v.burn?2:0); memcpy(buf+off,&n,sizeof n);off+=sizeof n; }
    for(int k=0;k<h.nC4;k++){ NetC4 q{c4s[k].x,c4s[k].y,c4s[k].z,(uint8_t)c4s[k].team,(uint8_t)c4s[k].owner}; memcpy(buf+off,&q,sizeof q);off+=sizeof q; }
    for(auto&c:cli)if(c.on)sock.send(c.a,buf,off);
  }
  void applySnap(const Pkt&pk){
    if(pk.n<(int)sizeof(NetSnapHead))return; NetSnapHead h; memcpy(&h,pk.d,sizeof h);
    int need=sizeof h+h.n*sizeof(NetEnt)+h.nEvt*sizeof(NetEvt)+h.nTr*sizeof(NetTr)+h.nVeh*sizeof(NetVeh)+h.nC4*sizeof(NetC4); if(pk.n<need||h.n>16||h.n<2)return;
    srvSilence=0; if(h.map!=mapIdx||(int)e.size()!=h.n){ mapIdx=h.map%MAP_COUNT; w.load(mapIdx); e.assign(h.n,Ent()); for(int i=0;i<h.n;i++){e[i].bot=false;e[i].alive=false;} }
    if(h.matchId!=matchId){ matchId=h.matchId; tr.clear(); evq.clear(); lastEvt=h.nEvt?lastEvt:lastEvt; }
    over=h.over;winner=h.winner;score[0]=h.s0;score[1]=h.s1; int off=sizeof h;
    for(int i=0;i<h.n;i++){ NetEnt n; memcpy(&n,pk.d+off,sizeof n); off+=sizeof n; Ent&p=e[i];
      p.team=(n.flags&16)?1:0; if(n.flags&128)p.revealT=5.f; p.remote=(n.flags&32)&&i!=0; bool al=n.flags&1; p.kills=n.kills;p.deaths=n.deaths; p.off=(n.hp==-1&&!al); snprintf(p.name,16,"%s",nm(i).c_str());
      if(i==meI){
        p.c4Left=n.c4; if(n.hp<p.hp&&al)dmgFlash=0.5f; p.hp=n.hp;
        if(n.spawnSeq!=p.spawnSeq||(al&&!p.alive)){ p.spawnSeq=n.spawnSeq; p.b.x=n.x;p.b.y=n.y;p.b.z=n.z;p.b.vy=0;p.yaw=n.yaw;p.pitch=0;p.crouch=false; prim=n.wpn; p.wpn=n.wpn; }
        p.alive=al; p.prot=(n.flags&64)?1.f:0.f; if(n.wpn==p.wpn){ if(n.ammo<p.ammo&&n.ammo>=0&&p.alive)p.rkP+=WEAPONS[p.wpn].recoilV*1.6f; p.ammo=n.ammo; p.mags=n.mags; p.reloadT=n.reloadT; p.flash=n.flash; }
      } else {
        bool first=!p.alive&&al; p.alive=al; p.hp=n.hp; p.tx=n.x;p.ty=n.y;p.tz=n.z;p.tyaw=n.yaw; p.pitch=n.pitch; p.crouch=n.flags&2; p.moving=n.flags&4; p.sprint=n.flags&8; p.wpn=n.wpn; p.flash=n.flash; p.phase=n.phase; p.ammo=n.ammo;
        if(first||n.spawnSeq!=p.spawnSeq){ p.spawnSeq=n.spawnSeq; p.b.x=n.x;p.b.y=n.y;p.b.z=n.z; p.yaw=n.yaw; }
      }
    }
    for(int k=0;k<h.nEvt;k++){ NetEvt v; memcpy(&v,pk.d+off,sizeof v); off+=sizeof v; if((int8_t)(v.id-lastEvt)>0){ lastEvt=v.id; if(v.killer==meI)hitMark=0.25f; char b[64]; snprintf(b,64,"%s > %s%s",nm(v.killer).c_str(),nm(v.victim).c_str(),v.head?" HS":""); addFeed(b); } }
    for(int k=0;k<h.nTr;k++){ NetTr q; memcpy(&q,pk.d+off,sizeof q); off+=sizeof q; tr.push_back({q.x0,q.y0,q.z0,q.x1,q.y1,q.z1,0.09f}); }
    if(h.nVeh>0&&(int)vh.size()!=h.nVeh){ vh.clear(); for(int k=0;k<h.nVeh;k++)vh.push_back(makeVeh(k)); drive=-1; }
    for(int k=0;k<h.nVeh&&k<(int)vh.size();k++){ NetVeh n; memcpy(&n,pk.d+off,sizeof n); off+=sizeof n; VH::Veh&v=vh[k];
      bool wasDead=v.dead; v.dead=n.flags&1; v.burn=n.flags&2; v.hp=n.hp; v.tx=n.x;v.tz=n.z;v.th=n.h;
      if(k!=drive){ v.speed=n.speed; v.steer=n.steer; v.driver=n.driver; if(!wasDead&&v.dead){ puffsBoom(v.x,v.z); } if(wasDead&&!v.dead){ v.x=n.x;v.z=n.z;v.h=n.h; } if(v.dead)v.driver=-1; }
      else { if(v.dead){ puffsBoom(v.x,v.z); drive=-1; v.driver=-1; } else if(n.driver>=0&&n.driver!=meI&&(++vConf>8)){ v.driver=n.driver; drive=-1; vConf=0; } else if(n.driver==meI)vConf=0; } }
    { std::vector<C4s> nc; for(int k=0;k<h.nC4;k++){ NetC4 q; memcpy(&q,pk.d+off,sizeof q); off+=sizeof q; nc.push_back({q.x,q.y,q.z,(int)q.team,(int)q.owner}); }
      for(auto&o:c4s){ bool f=false; for(auto&q:nc)if(fabsf(q.x-o.x)<0.02f&&fabsf(q.z-o.z)<0.02f)f=true; if(!f&&o.owner>=0&&o.owner<(int)e.size()&&e[o.owner].alive)blastAt(o.x,o.y,o.z,o.owner,o.team,8.f,0,0); }
      c4s=nc; }
  }
  void netPump(float dt){
    Pkt pk;
    if(role==1){
      while(sock.recv(pk)){ if(pk.n<2)continue; uint8_t ty=pk.d[0];
        int ci=-1; for(int k=0;k<8;k++)if(cli[k].on&&cli[k].a.sin_addr.s_addr==pk.from.sin_addr.s_addr&&cli[k].a.sin_port==pk.from.sin_port)ci=k;
        if(ty==T_HELLO){ if(ci<0){ for(int k=0;k<8&&ci<0;k++)if(!cli[k].on){ int en=assignSlot(); if(en<0)break; ci=k; cli[k].on=true;cli[k].a=pk.from;cli[k].ent=en; Ent&p=e[en]; p.remote=true;p.bot=false;p.off=false; spawn(p); snprintf(p.name,16,"%s",nm(en).c_str()); char b[40];snprintf(b,40,"%s JOINED",nm(en).c_str());addFeed(b);} }
          if(ci>=0){ cli[ci].last=0; NetWelcome wl{T_WELCOME,(uint8_t)cli[ci].ent,(uint8_t)mapIdx,(uint8_t)e.size()}; sock.send(pk.from,&wl,sizeof wl); }
          else { NetWelcome wl{T_WELCOME,255,0,0}; sock.send(pk.from,&wl,sizeof wl); } }
        else if(ty==T_PROBE){ NetBeacon bc; fillBeacon(bc); sock.send(pk.from,&bc,sizeof bc); }
        else if(ty==T_BYE&&ci>=0){ Ent&p=e[cli[ci].ent]; p.remote=false;p.bot=true; if(roomOn&&rmBotsE()==0){ p.bot=false;p.off=true;p.alive=false;p.respawn=1e9f; } char b[40];snprintf(b,40,"%s LEFT",nm(cli[ci].ent).c_str());addFeed(b); cli[ci]=Cli(); }
        else if(ty==T_INPUT&&ci>=0&&pk.n>=(int)sizeof(NetInput)){ cli[ci].last=0; NetInput in; memcpy(&in,pk.d,sizeof in); if((int16_t)(in.seq-e[cli[ci].ent].rin.seq)>0||e[cli[ci].ent].rin.t==0)e[cli[ci].ent].rin=in; }
      }
      for(auto&c:cli)if(c.on){ c.last+=dt; if(c.last>5.f){ Ent&p=e[c.ent]; p.remote=false;p.bot=true; if(roomOn&&rmBotsE()==0){ p.bot=false;p.off=true;p.alive=false;p.respawn=1e9f; } c=Cli(); } }
      snapT+=dt; if(snapT>=0.05f){snapT=0;packSnap();}
      beaconT+=dt; if(beaconT>=1.f){ beaconT=0; NetBeacon bc; fillBeacon(bc);
        beaconTx.send(mkAddr(htonl(INADDR_BROADCAST),PORT_BEACON),&bc,sizeof bc); uint32_t sub=(myIp&0x00FFFFFFu)|0xFF000000u; if(myIp)beaconTx.send(mkAddr(sub,PORT_BEACON),&bc,sizeof bc); }
    } else if(role==2||joining){
      while(sock.recv(pk)){ if(pk.n<1)continue; uint8_t ty=pk.d[0];
        if(ty==T_WELCOME&&joining&&pk.n>=(int)sizeof(NetWelcome)){ NetWelcome wl; memcpy(&wl,pk.d,sizeof wl); joining=false;
          if(wl.slot==255){ say("HOST FULL"); sock.close_(); menu=2; } else { role=2; meI=wl.slot; mapIdx=wl.map%MAP_COUNT; w.load(mapIdx); e.assign(wl.total,Ent()); for(auto&p:e){p.bot=false;p.alive=false;} menu=0; srvSilence=0; inT=0; over=false; lastEvt=0; evq.clear(); say("CONNECTED"); } }
        else if(ty==T_SNAP&&role==2)applySnap(pk); }
      if(joining){ joinT+=dt; if(joinT>0.4f&&joinT<8.f){ static float ra=0; ra+=dt; if(ra>0.4f){ra=0;NetHello h{T_HELLO,NET_VER};sock.send(hostAddr,&h,sizeof h);} } if(joinT>=8.f){joining=false;sock.close_();say("NO REPLY FROM HOST");} }
      if(role==2){ srvSilence+=dt; if(srvSilence>4.f){ netClose(); menu=1; say("DISCONNECTED"); return; }
        inT+=dt; if(inT>=1.f/30){ inT=0; const Ent&p=e[meI]; NetInput n; n.t=T_INPUT;n.slot=(uint8_t)meI;n.seq=++netSeq;n.x=p.b.x;n.y=p.b.y;n.z=p.b.z;n.yaw=p.yaw;n.pitch=p.pitch;
          n.flags=(gunner?128:0)|(lastIn.fire?1:0)|(p.crouch?2:0)|(lastIn.reload?4:0)|(p.sprint?8:0)|(p.moving?16:0)|(lastIn.ads?32:0); n.wpn=prim; n.veh=0;n.vx=n.vz=n.vh=n.vsp=0;n.c4p=c4pCnt;n.c4d=c4dCnt; if(drive>=0&&drive<(int)vh.size()){ const VH::Veh&v=vh[drive]; n.veh=(uint8_t)(drive+1);n.vx=v.x;n.vz=v.z;n.vh=v.h;n.vsp=v.speed; } sock.send(hostAddr,&n,sizeof n); } }
    }
    if(menu==2&&beaconRx.ready()){ ipT-=dt; if(ipT<=0||!myIp){ ipT=3.f; myIp=localIp(); }
      if(myIp&&!joining){ NetHello pr{T_PROBE,NET_VER}; for(int k=0;k<10;k++){ sweepIdx=sweepIdx%254+1; uint32_t t=(myIp&0x00FFFFFFu)|((uint32_t)sweepIdx<<24); if(t!=myIp)beaconRx.send(mkAddr(t,PORT_GAME),&pr,sizeof pr); } } // مسح مباشر (unicast) للشبكة: يعمل حتى حيث يُمنع البث (iOS)
      while(beaconRx.recv(pk)){ if(pk.n<20||pk.d[0]!=T_BEACON)continue; NetBeacon bc; memset(&bc,0,sizeof bc); memcpy(&bc,pk.d,std::min<size_t>((size_t)pk.n,sizeof bc)); if(!bc.lim5)bc.lim5=6; bool f=false;
        for(auto&h:hosts)if(h.a.sin_addr.s_addr==pk.from.sin_addr.s_addr){h.map=bc.map;h.players=bc.players;h.maxp=bc.maxp;h.lim5=bc.lim5;h.mins=bc.mins;h.bots=bc.bots;h.style=bc.style;h.seen=0;f=true;}
        if(!f){ HostEnt he; he.a=mkAddr(pk.from.sin_addr.s_addr,PORT_GAME); he.map=bc.map;he.players=bc.players;he.maxp=bc.maxp;he.lim5=bc.lim5;he.mins=bc.mins;he.bots=bc.bots;he.style=bc.style;memcpy(he.name,bc.name,16);he.name[15]=0;he.seen=0; hosts.push_back(he);} }
      for(auto&h:hosts)h.seen+=dt; hosts.erase(std::remove_if(hosts.begin(),hosts.end(),[](HostEnt&h){return h.seen>4.f;}),hosts.end()); if(hostSel>=(int)hosts.size())hostSel=std::max(0,(int)hosts.size()-1); }
  }
  // لاعب بعيد (على المضيف): تطبيق آخر حالة وصلت منه
  void updateRemote(int i,float dt){
    Ent&p=e[i]; const NetInput&n=p.rin; if(n.t!=T_INPUT)return;
    for(int k=0;k<(int)vh.size();k++)if(vh[k].driver==i&&n.veh!=k+1)vh[k].driver=-1;
    if(n.veh>0&&n.veh<=(int)vh.size()){ VH::Veh&v=vh[n.veh-1]; if(!v.dead&&(v.driver<0||v.driver==i)&&p.alive){ v.driver=i; v.team=p.team; v.x=n.vx;v.z=n.vz;v.h=n.vh;v.speed=n.vsp; if(v.type==1&&!(n.flags&128)&&(n.flags&1)&&v.cannonT<=0){ float cp=cosf(n.pitch); cannonFx(v,-sinf(n.yaw)*cp,sinf(n.pitch),-cosf(n.yaw)*cp,true,i); } } }
    if(!p.rc4Init){ p.rc4Init=true; p.rc4p=n.c4p; p.rc4d=n.c4d; } else { if(n.c4p!=p.rc4p){ p.rc4p=n.c4p; float cpt=std::min<float>(n.pitch-0.45f,-0.45f),cp=cosf(cpt); c4Place(i,-sinf(n.yaw)*cp,sinf(cpt),-cosf(n.yaw)*cp); } if(n.c4d!=p.rc4d){ p.rc4d=n.c4d; c4Det(i); } }
    p.b.x=std::max<float>(-w.m->W/2.f,std::min<float>(w.m->W/2.f,n.x)); p.b.y=n.y; p.b.z=std::max<float>(-w.m->D/2.f,std::min<float>(w.m->D/2.f,n.z)); p.yaw=n.yaw;p.pitch=n.pitch;p.crouch=n.flags&2;p.sprint=n.flags&8;p.moving=n.flags&16;
    if(p.moving)p.phase+=dt*8; p.mnt=(n.veh>0&&n.veh<=(int)vh.size()&&vh[n.veh-1].type==0&&(n.flags&128)); p.mvi=p.mnt?n.veh-1:-1; if(n.wpn<WEAPON_COUNT&&n.wpn!=p.wpn)giveWeapon(p,n.wpn);
    if(n.flags&4)reloadStart(p); updateWeapon(i,dt,(n.flags&1)&&!p.sprint&&(!(n.veh>0&&n.veh<=(int)vh.size())||(n.flags&128)));
  }
  // ======== التحديث ========
  void drawDroneModel(float x,float y,float z,float yaw,bool mine){
    float hv=sinf(t*3.5f+x)*0.04f; y+=hv; r.obb(x,y,z,0.15f,0.05f,0.17f,yaw,0,0.12f,0.12f,0.14f,true,true); r.obb(x,y+0.06f,z,0.07f,0.025f,0.08f,yaw,0,0.25f,0.27f,0.3f,true,true);
    float bl=0.6f+0.4f*sinf(t*8); r.obb(x,y-0.06f,z,0.03f,0.025f,0.03f,yaw,0,mine?0.2f*bl:1.f*bl,mine?1.f*bl:0.2f*bl,0.2f*bl,false,false);
    for(int sx=-1;sx<=1;sx+=2)for(int sz=-1;sz<=1;sz+=2){ float o[3]; Renderer::rot(yaw,0,sx*0.19f,0,sz*0.19f,o); r.obb(x+o[0],y,z+o[2],0.02f,0.015f,0.19f,yaw+sx*sz*0.785f,0,0.2f,0.2f,0.22f,true,true);
      Renderer::rot(yaw,0,sx*0.3f,0.05f,sz*0.3f,o); r.obb(x+o[0],y+o[1],z+o[2],0.15f,0.006f,0.15f,t*40*sx,0,0.7f,0.75f,0.8f,false,false); } }
  void landDrone(float cd){ dn.on=false; dnCd=cd; }
  void updateDrone(float dt,const Input&in,Input&use,bool world=true){
    Ent&me=e[meI]; if(dnCd>0)dnCd-=dt; if(world)for(auto&o:e)if(o.reveal>0)o.reveal-=dt;
    if(in.drone&&!pdn&&me.alive&&drive<0){ if(dn.on)landDrone(10); else if(dnCd>0){ char b[48]; snprintf(b,48,T("المسيّرة تُشحن %d ث"),(int)ceilf(dnCd)); say(b); }
      else { dn=Drn(); dn.on=true; dn.x=me.b.x; dn.y=me.b.y+2.f; dn.z=me.b.z; dn.yaw=me.yaw; dn.t=25.f; dnSpot=0; SFX::play(SFX::EMPTY,0.7f); } }
    pdn=in.drone; bool fe=in.fire&&!pfireD; pfireD=in.fire;
    if(!dn.on)return;
    if(!me.alive||drive>=0){ landDrone(40); return; }
    dn.t-=dt; if(dn.t<=0){ landDrone(20); say("نفدت بطارية المسيّرة"); return; }
    dn.yaw-=in.lx*dt*sens; dn.pitch=std::max<float>(-1.4f,std::min<float>(1.4f,dn.pitch+in.ly*dt*sens));
    float cp=cosf(dn.pitch),sp=sinf(dn.pitch),fx=-sinf(dn.yaw)*cp,fy=sp,fz=-cosf(dn.yaw)*cp,rx=cosf(dn.yaw),rz=-sinf(dn.yaw);
    float up=(in.jump?1.f:0.f)-(in.crouch?1.f:0.f);
    float tx=(fx*in.my+rx*in.mx)*12.f,ty=fy*in.my*12.f+up*6.f,tz=(fz*in.my+rz*in.mx)*12.f; float k=std::min<float>(1.f,6.f*dt);
    dn.vx+=(tx-dn.vx)*k; dn.vy+=(ty-dn.vy)*k; dn.vz+=(tz-dn.vz)*k;
    float dx=dn.vx*dt,dy=dn.vy*dt,dz=dn.vz*dt,L=sqrtf(dx*dx+dy*dy+dz*dz);
    if(L>1e-5f){ float h=raycastMap(w,dn.x,dn.y,dn.z,dx/L,dy/L,dz/L,L+0.5f); if(h<L+0.5f){ dn.vx=dn.vy=dn.vz=0; } else { float nx=dn.x+dx,ny=dn.y+dy,nz=dn.z+dz;
      float hw=w.m->W*0.5f-0.5f,hd=w.m->D*0.5f-0.5f; if(fabsf(nx)<hw&&fabsf(nz)<hd&&ny>0.5f&&ny<30.f&&hypotf(nx-me.b.x,nz-me.b.z)<=70.f){ dn.x=nx;dn.y=ny;dn.z=nz; } else dn.vx=dn.vy=dn.vz=0; } }
    dnSpot-=dt; if(dnSpot<=0){ dnSpot=0.6f; for(auto&o:e){ if(!o.alive||o.team==me.team)continue; float ex=o.b.x-dn.x,ey=o.b.y+1.2f-dn.y,ez=o.b.z-dn.z,d=sqrtf(ex*ex+ey*ey+ez*ez); if(d>45.f||d<0.1f)continue;
        if(raycastMap(w,dn.x,dn.y,dn.z,ex/d,ey/d,ez/d,d)>=d-0.3f)o.reveal=2.5f; } }
    if(fe){ float gx=dn.x,gy=dn.y,gz=dn.z; landDrone(40);
      for(int i=0;i<(int)e.size();i++){ if(!e[i].alive||e[i].team==me.team)continue; float ex=e[i].b.x-gx,ey=e[i].b.y+1.f-gy,ez=e[i].b.z-gz,d=sqrtf(ex*ex+ey*ey+ez*ez); if(d>7.f)continue;
        float h=raycastMap(w,gx,gy+0.2f,gz,ex/(d+1e-4f),(ey-0.2f)/(d+1e-4f),ez/(d+1e-4f),d); if(h<d-0.3f)continue; hurt(i,meI,140.f*powf(1.f-d/7.f,1.2f),false); }
      for(auto&v:vh){ if(v.dead)continue; float ex=v.x-gx,ez=v.z-gz,d=sqrtf(ex*ex+ez*ez); if(d<7.5f)hurtVeh(v,220.f*(1.f-d/7.5f),meI); }
      for(int j=0;j<10;j++)puffs.push_back({gx+(frand()-0.5f)*2,gy+frand()*1.2f,gz+(frand()-0.5f)*2,1.2f+frand()*1.5f,0.7f+frand()*0.5f,0.9f+frand()*0.9f,j<5});
      sparkAt(gx,gy,gz,0,-1,0,30); SFX::play(SFX::EXPLODE,0.8f); }
    use.mx=use.my=use.lx=use.ly=0; use.fire=use.ads=use.jump=use.crouch=use.reload=use.swapP=use.swapN=use.grenade=use.sprint=use.tri=false;
  }
  void updateGren(float dt,const Input&in,bool world=true){
    Ent&me=e[meI];
    if(in.grenade&&!pg&&gren>0&&me.alive&&drive<0&&role==0){ gren--; float cp=cosf(me.pitch+0.25f);
      grs.push_back({me.b.x,me.b.y+1.5f,me.b.z,-sinf(me.yaw)*cp*17.f,sinf(me.pitch+0.25f)*17.f+2.f,-cosf(me.yaw)*cp*17.f,2.2f}); SFX::play(SFX::EMPTY,0.6f); }
    pg=in.grenade; if(!world)return;
    for(size_t k=0;k<shells.size();){ Shell&c=shells[k]; c.t-=dt; if(c.y>0.03f){ c.vy-=18.f*dt; c.x+=c.vx*dt;c.y+=c.vy*dt;c.z+=c.vz*dt; if(c.y<0.03f){c.y=0.03f;c.vx=c.vz=0;} } if(c.t<=0)shells.erase(shells.begin()+k); else k++; }
    for(size_t k=0;k<rks.size();){ Rk&q=rks[k]; q.t+=dt*115.f; if(q.t>=q.len){ float gx=q.ox+q.dx*q.len,gy=q.oy+q.dy*q.len,gz=q.oz+q.dz*q.len; explosionAt(gx,gy,gz,(int)q.by,6.5f,WEAPONS[8].dmg,340.f); rks.erase(rks.begin()+k); continue; } if(frand()<0.7f)puffs.push_back({q.ox+q.dx*(q.t-0.6f),q.oy+q.dy*(q.t-0.6f),q.oz+q.dz*(q.t-0.6f),0.5f+frand()*0.5f,0.5f+frand()*0.3f,0.5f+frand()*0.4f,false}); k++; }
    for(size_t k=0;k<grs.size();){ Gr&g=grs[k]; g.t-=dt; g.vy-=22.f*dt;
      float dx=g.vx*dt,dy=g.vy*dt,dz=g.vz*dt,L=sqrtf(dx*dx+dy*dy+dz*dz);
      if(L>1e-5f){ float h=raycastMap(w,g.x,g.y,g.z,dx/L,dy/L,dz/L,L+0.15f);
        if(h<L+0.15f){ // ارتداد: أوقف الحركة العمودية على أقرب محور
          float tx=g.x+dx/L*h,ty=g.y+dy/L*h,tz=g.z+dz/L*h; g.x=tx-dx/L*0.12f;g.y=ty-dy/L*0.12f;g.z=tz-dz/L*0.12f;
          if(fabsf(dy)>=fabsf(dx)&&fabsf(dy)>=fabsf(dz)){ g.vy=-g.vy*0.4f; g.vx*=0.7f; g.vz*=0.7f; if(fabsf(g.vy)<1.5f)g.vy=0; }
          else if(fabsf(dx)>=fabsf(dz))g.vx=-g.vx*0.5f; else g.vz=-g.vz*0.5f;
        } else { g.x+=dx;g.y+=dy;g.z+=dz; } }
      if(g.y<0.15f){ g.y=0.15f; if(g.vy<0)g.vy=-g.vy*0.35f; g.vx*=0.9f; g.vz*=0.9f; }
      if(g.t<=0){ float gx=g.x,gy=g.y,gz=g.z;
        for(int i=0;i<(int)e.size();i++){ if(!e[i].alive)continue; float ex=e[i].b.x-gx,ey=e[i].b.y+0.9f-gy,ez=e[i].b.z-gz,dd=sqrtf(ex*ex+ey*ey+ez*ez); if(dd<6.5f)hurt(i,meI,150.f*(1.f-dd/6.5f)+10.f,false); }
        for(auto&v:vh){ if(v.dead)continue; float ex=v.x-gx,ez=v.z-gz,dd=sqrtf(ex*ex+ez*ez); if(dd<7.f)hurtVeh(v,220.f*(1.f-dd/7.f),meI); }
        for(int j=0;j<10;j++)puffs.push_back({gx+(frand()-0.5f)*2,gy+frand()*1.2f,gz+(frand()-0.5f)*2,1.2f+frand()*1.5f,0.7f+frand()*0.5f,0.9f+frand()*0.9f,j<5});
        sparkAt(gx,gy,gz,0,-1,0,30);
        { float ex=gx-me.b.x,ez=gz-me.b.z; SFX::play(SFX::EXPLODE,std::max(0.25f,1.f-sqrtf(ex*ex+ez*ez)/60.f)); }
        grs.erase(grs.begin()+k); continue; }
      k++; }
  }
  void update(float dt,const Input&in0){
    Input in0b=in0; if(tap.mx){in0b.mx=(float)tap.mx;} if(tap.confirm)in0b.confirm=true; if(tap.back)in0b.back=true; if(tap.start)in0b.start=true; if(tap.tri)in0b.tri=true; if(tap.reload)in0b.reload=true; if(tap.swapP)in0b.swapP=true; if(tap.fire)in0b.fire=true; tap=Tap();
    airIn=in0b; Input in=in0b; if(air&&myJet()>=0){ in.drone=false;in.grenade=false;in.c4=false;in.det=false;in.fire=false;in.tri=false;in.use=false;in.reload=false;in.swapP=false;in.swapN=false;in.jump=false;in.crouch=false;in.pick=-1; }
    lastIn=in; if(qApplied!=g_quality){ qApplied=g_quality; static const float FK[5]={0.40f,0.62f,0.82f,1.f,1.3f}; r.fogK=FK[g_quality]; qChanged=true; }

    if(menu==1){
      bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f,rt=in.mx>0.5f&&prev.mx<=0.5f,lf=in.mx<-0.5f&&prev.mx>=-0.5f;
      if(up)menuRow=(menuRow+MROWS-1)%MROWS; if(dn)menuRow=(menuRow+1)%MROWS; int d=rt?1:lf?-1:0;
      if(d){ if(menuRow==0)menuSel=(menuSel+d+MAP_COUNT)%MAP_COUNT; else if(menuRow==1)diff=(diff+d+3)%3; else if(menuRow==2)mode=(mode+d+modeCount)%modeCount; else if(menuRow==3){ g_quality=(g_quality+d+5)%5; saveSettings(); } else if(menuRow==4){ g_lang=(g_lang+d+3)%3; saveSettings(); } }
      if(msgT>0)msgT-=dt;
      if(((in.confirm&&!prev.confirm)||(in.start&&!prev.start))&&menuSel==8&&mode>=1&&mode<=2){ say("خريطة الطيران للاعب الواحد فقط"); prev=in; return; }
      if((in.confirm&&!prev.confirm)||(in.start&&!prev.start)){ if(mode==0||mode>=3){ roomOn=false; scoreLim=30; timeLim=0; nLocal=mode>=3?2:1; coop=(mode!=4&&mode!=5); duel=(mode==5); fovMul=nLocal==2?1.12f:1.f; hudK=nLocal==2?0.8f:1.f; menu=0;role=0;meI=0;startMatch(menuSel);} else { nLocal=1; duel=false; fovMul=hudK=1.f; if(mode==1){ roomOn=false; menu=4; rmRow=0; } else lobbyStart(); } }
      prev=in; return; }
    if(menu==4){ // إعداد الغرفة
      bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f,rt=in.mx>0.5f&&prev.mx<=0.5f,lf=in.mx<-0.5f&&prev.mx>=-0.5f;
      if(up)rmRow=(rmRow+RROWS-1)%RROWS; if(dn)rmRow=(rmRow+1)%RROWS; int d=rt?1:lf?-1:0;
      if(d){ switch(rmRow){ case 0:rmName=(rmName+d+8)%8;break; case 1:rmMaxI=(rmMaxI+d+7)%7;break; case 2:rmLimI=(rmLimI+d+4)%4;break; case 3:rmMinI=(rmMinI+d+4)%4;break; case 4:rmBotI=(rmBotI+d+4)%4;break; case 5:rmStyle=(rmStyle+d+2)%2;break; case 6:rmWpnI=(rmWpnI+d+WEAPON_COUNT+1)%(WEAPON_COUNT+1);break; default:rmDmgI=(rmDmgI+d+3)%3; } }
      if(in.back&&!prev.back){ menu=1; prev=in; return; }
      if((in.confirm&&!prev.confirm)||(in.start&&!prev.start)){ roomOn=true; roomApply(); hostStart(); }
      prev=in; return; }
    if(menu==3){ netStartStep(); if(in.back&&!prev.back){ netClose(); menu=1; } prev=in; return; }
    if(menu==2){
      netPump(dt); if(msgT>0)msgT-=dt;
      bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f; if(up&&hostSel>0)hostSel--; if(dn&&hostSel+1<(int)hosts.size())hostSel++;
      if(in.confirm&&!prev.confirm&&!joining&&!hosts.empty()){ scoreLim=hosts[hostSel].lim5*5; timeLim=hosts[hostSel].mins*60; roomOn=false; joinAddr(hosts[hostSel].a); }
      if(in.back&&!prev.back){ netClose(); menu=1; }
      prev=in; return; }
    if(in.start&&!prev.start)paused=!paused;
    bool net=role!=0; Input use=in;
    if(paused&&role!=3){ if(in.tri&&!prev.tri)cycleAtt(0); if(in.reload&&!prev.reload)cycleAtt(1); if(in.swapP&&!prev.swapP)cycleAtt(2); if(in.fire&&!prev.fire)cycleAtt(3); }
    if(paused){ if(in.mx>0.5f&&prev.mx<=0.5f)sens=std::min<float>(2.5f,sens+0.2f); if(in.mx<-0.5f&&prev.mx>=-0.5f)sens=std::max<float>(0.3f,sens-0.2f); if(in.back&&!prev.back){ if(net)netClose(); menu=1;paused=false;}
      if(!net){prev=in;return;} use=Input(); }
    if(net)netPump(dt);
    if(menu){prev=in;return;} // انقطع الاتصال
    if(over&&role!=2){ if(in.confirm&&!prev.confirm&&!paused){ if(role==1)for(auto&c:cli)if(c.on&&c.ent>0)e[c.ent].remote=true; startMatch(mapIdx);} prev=in; return; }
    if(over&&role==2){ prev=in; return; }
    { Ent&m0=e[meI]; static float lastPh=0; if(m0.alive&&m0.moving&&drive<0&&!dn.on&&(int)(m0.phase/(m0.sprint?2.2f:2.8f))!=(int)(lastPh/(m0.sprint?2.2f:2.8f)))SFX::play(SFX::STEP,m0.crouch?0.12f:0.3f,0); lastPh=m0.phase;
      SFX::engSpeed.store(drive>=0&&drive<(int)vh.size()?fabsf(vh[drive].speed):-1.f); SFX::droneVol.store(dn.on?1.f:0.f); }
    t+=dt; if(role!=2&&timeLim>0&&!over&&t>=timeLim){ over=true; winner=score[0]>=score[1]?0:1; } updateGren(dt,in); if(msgT>0)msgT-=dt; if(hitMark>0)hitMark-=dt; if(dmgFlash>0)dmgFlash-=dt; if(hitAngT>0)hitAngT-=dt; if(wsT>0)wsT-=dt;
    for(auto&q:tr)q.t-=dt; tr.erase(std::remove_if(tr.begin(),tr.end(),[](Tracer&q){return q.t<=0;}),tr.end());
    for(int k=0;k<4;k++)if(feedT[k]>0)feedT[k]-=dt;
    for(auto&k:sparks){ k.t-=dt; k.vy-=9.f*dt; k.x+=k.vx*dt; k.y=std::max<float>(0.02f,k.y+k.vy*dt); k.z+=k.vz*dt; } sparks.erase(std::remove_if(sparks.begin(),sparks.end(),[](Spark&k){return k.t<=0;}),sparks.end());
    if(role==2){ // عميل: حركة محلية للاعب فقط + تنعيم الآخرين
      for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(i==meI){ if(p.alive)updatePlayer(dt,use); continue; } if(p.gone)continue;
        float k=std::min<float>(1.f,dt*16.f); p.b.x+=(p.tx-p.b.x)*k;p.b.y+=(p.ty-p.b.y)*k;p.b.z+=(p.tz-p.b.z)*k; float dy=p.tyaw-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*k; if(p.flash>0)p.flash-=dt; }
      if(role==2)updateVeh(dt,use);
      prev=in; return; }
    Input use2=in2;
    if(role==0)updateDrone(dt,in,use,true);
    if(nLocal==2&&role==0){ lsx(); updateGren(dt,in2,false); updateDrone(dt,in2,use2,false); if(msgT>0)msgT-=dt; if(hitMark>0)hitMark-=dt; if(dmgFlash>0)dmgFlash-=dt; if(hitAngT>0)hitAngT-=dt; lsx(); }
    if(air)airUpdate(dt,airIn); else for(int i=0;i<(int)e.size();i++){ Ent&p=e[i];
      if(nLocal==2&&i==ls2.meI)continue;
      if(p.prot>0)p.prot-=dt;
      if(p.off&&!p.remote)continue;
      if(!p.alive){ p.respawn-=dt; if(p.respawn<=0){spawn(p);} continue; }
      if(i==meI)updatePlayer(dt,use); else if(p.remote)updateRemote(i,dt); else updateBot(i,dt);
    }
    if(nLocal==2&&role==0){ lsx(); Ent&p2=e[meI]; if(p2.prot>0)p2.prot-=dt; if(!p2.alive){ p2.respawn-=dt; if(p2.respawn<=0)spawn(p2); } else updatePlayer(dt,use2); lsx(); }
    for(auto&o:e)if(o.revealT>0)o.revealT-=dt;
    if(role<=1)c4Tick(dt);
    if(role<=2)updateVeh(dt,use);
    if(nLocal==2&&role==0){ lsx(); vpass=1; updateVeh(dt,use2); vpass=0; prev=in2; lsx(); }
    prev=in;
  }
  // ---- الرسم ----
  static void gcol(char c,float*o){ static const float T[7][3]={{.14f,.14f,.16f},{.34f,.35f,.38f},{.05f,.05f,.06f},{.42f,.27f,.13f},{.5f,.45f,.32f},{.9f,.5f,.1f},{.86f,.68f,.55f}}; int k=c=='d'?0:c=='g'?1:c=='b'?2:c=='w'?3:c=='t'?4:c=='o'?5:6; o[0]=T[k][0];o[1]=T[k][1];o[2]=T[k][2]; }
  // يرسم سلاحاً (من بيانات أندرويد) عند أصل عالمي مع دوران yaw/pitch
  struct GAtt{int optic=0,muz=0,under=0,mag=0;}; GAtt myAtt; GAtt wAtt[16];
  // ---- لاعبان على نفس الجهاز (شاشة مقسومة): حالة اللاعب المحلي كلها تُبدَّل بين اللاعبين (lsx) ، والعالم مشترك ----
  struct LS{ int meI=1,drive=-1,gren=2,prim=0,streak=0,glCnt=2; float lookIdle=0,hitMark=0,dmgFlash=0,hitAng=0,hitAngT=0,dnCd=0,dnSpot=0,msgT=0,optZ=0,camX=0,camY=3,camZ=0;
    bool nearVeh=false,pdn=false,pfireD=false,pg=false,gunner=false,pSeat=false,pc4=false,pdet=false,adsHeld=false,pFireV=false; Drn dn; Input prev; char msg[64]={0}; GAtt wAtt[16],myAtt; };
  // ---- حرب الطائرات (خريطة 8) ----
  std::vector<Jet> jets; std::vector<int> entJet; bool air=false; Input airIn,airPrev; float footT=0,jetMsgT=0; int rpgIdx=8; char jetMsg[48]={0}; float jetRc=0,jetPc=0;
  void airInit(); void airUpdate(float dt,const Input&in); void airSpawn(int i); int jetNew(int team,bool airborne,int slot); void jetHurt(int j,float dmg,int by,bool inc); void jetDie(int j,int by);
  bool jetStep(Jet&J,float dt,float rc,float pc,float yc,float thd,bool ab); void jetAI(int j,float dt); void jetGun(int j,int aim=-1); float jetSpeed(const Jet&J)const{ return sqrtf(J.vx*J.vx+J.vy*J.vy+J.vz*J.vz); }
  void airCam(Cam&c); void airDraw(const Cam&c); void airHud(int sc); bool projP(float x,float y,float z,int&X,int&Y,float&zc); void jetSay(const char*s){snprintf(jetMsg,48,"%s",trl(s));jetMsgT=2.5f;} int myJet()const{ return (air&&meI>=0&&meI<(int)entJet.size())?entJet[meI]:-1; }
  int modeCount=6; bool duel=false;
  // ---- إعداد الغرفة (استضافة LAN) ----
  bool roomOn=false; static constexpr int RROWS=8; int rmWpnI=0,rmDmgI=1; /* سلاح البداية (0=عشوائي) وقوة الضرر {0.5,1,2} — يطبّقها المضيف */ float cfgDmg=1.f; int cfgWpn=-1; int rmName=0,rmMaxI=6,rmLimI=2,rmMinI=0,rmBotI=2,rmStyle=0,rmRow=0,timeLim=0; // rmMaxI: 2..8 = 2+rmMaxI؛ البوتات {0,2,4,6}
  static const char*rmNameOf(int i){ static const char*N[8]={"Falcon","Lion","Wolf","Eagle","Tiger","Cobra","Viper","Shark"}; return N[i&7]; }
  static int rmLimV(int i){ static const int L[4]={10,20,30,50}; return L[i&3]; }
  static int rmMinV(int i){ static const int M[4]={0,5,10,15}; return M[i&3]; }
  int rmMaxE()const{ return rmStyle==1?2:2+rmMaxI; }
  int rmBotsE()const{ static const int B[4]={0,2,4,6}; return rmStyle==1?0:std::min(B[rmBotI&3],rmMaxE()-1); }
  void roomApply(){ scoreLim=rmLimV(rmLimI); timeLim=rmMinV(rmMinI)*60; static const float DM[3]={0.5f,1.f,2.f}; cfgDmg=DM[rmDmgI%3]; cfgWpn=rmWpnI>0?rmWpnI-1:-1; } int nLocal=1,act=0; bool coop=true; Input in2; LS ls2; int vpass=0; float fovMul=1.f,hudK=1.f;
  bool skipVM=false; // PS4 شاشة منقسمة: تخطّي توليد مجسّم السلاح في هذا الإطار
  void lsx(){ if(nLocal<2)return; LS&o=ls2; std::swap(meI,o.meI);std::swap(drive,o.drive);std::swap(gren,o.gren);std::swap(prim,o.prim);std::swap(streak,o.streak);std::swap(glCnt,o.glCnt);
    std::swap(lookIdle,o.lookIdle);std::swap(hitMark,o.hitMark);std::swap(dmgFlash,o.dmgFlash);std::swap(hitAng,o.hitAng);std::swap(hitAngT,o.hitAngT);std::swap(dnCd,o.dnCd);std::swap(dnSpot,o.dnSpot);std::swap(msgT,o.msgT);std::swap(optZ,o.optZ);
    std::swap(camX,o.camX);std::swap(camY,o.camY);std::swap(camZ,o.camZ);std::swap(nearVeh,o.nearVeh);std::swap(pdn,o.pdn);std::swap(pfireD,o.pfireD);std::swap(pg,o.pg);std::swap(gunner,o.gunner);std::swap(pSeat,o.pSeat);
    std::swap(pc4,o.pc4);std::swap(pdet,o.pdet);std::swap(adsHeld,o.adsHeld);std::swap(pFireV,o.pFireV);std::swap(dn,o.dn);std::swap(prev,o.prev);std::swap(msg,o.msg);std::swap(wAtt,o.wAtt);std::swap(myAtt,o.myAtt); act^=1; }
  static int optList(int w,int*o){ static const int L[9][4]={{0,1,2,3},{0,1,2,-1},{0,1,-1,-1},{3,4,-1,-1},{0,1,2,3},{1,2,3,4},{0,1,-1,-1},{0,1,-1,-1},{0,-1,-1,-1}}; if(w<0||w>8)w=0; int n=0; for(int i=0;i<4;i++)if(L[w][i]>=0)o[n++]=L[w][i]; return n; }
  void applyWAtt(Ent&p){ if(p.wpn<0||p.wpn>=16)return; GAtt&a=wAtt[p.wpn]; if(&p==&e[meI]){ static const float Z[5]={0,1.5f,2.2f,3.6f,6.f}; optZ=Z[a.optic]; } p.magM=a.mag==1?1.5f:1.f; p.relM=a.mag==1?1.12f:a.mag==2?0.72f:1.f; if(p.ammo>cap(p))p.ammo=cap(p); }
  void cycleAtt(int what){ Ent&p=e[meI]; GAtt&a=wAtt[p.wpn&15]; if(what==0){ int o[4]; int n=optList(p.wpn,o); int k=0; for(int i=0;i<n;i++)if(o[i]==a.optic)k=i; a.optic=o[(k+1)%n]; } else if(what==1)a.muz=(a.muz+1)%3; else if(what==2)a.under=(a.under+1)%3; else a.mag=(a.mag+1)%3; applyWAtt(p); } // optic: 0 حديدي 1 نقطة 2 x2 3 x4 4 x8
  float gsc=1; // مقياس موديل السلاح (منظور الآخرين أكبر قليلًا)
  float vmMinD=0,vmF[3]={0,0,-1}; // المجسّم الأول: نتجاهل مثلثات أقرب من هذه المسافة للكاميرا (تمنع تشوّه القص)
  float vmLight(float nx,float ny,float nz)const{ // إضاءة المجسّم الأول بالنسبة للكاميرا (ضوء من أعلى اليسار خلف اللاعب) ليظهر التفصيل بدل لون مسطّح
    float fx=vmF[0],fy=vmF[1],fz=vmF[2]; float rx=-fz,rz=fx; float rl=sqrtf(rx*rx+rz*rz)+1e-6f; rx/=rl; rz/=rl; // يمين = (−fz,0,fx) مُطبَّع
    float ux=-fy*rz,uy=rz*fx-rx*fz,uz=rx*fy; // up = right × forward
    float Lx=-fx*0.55f-rx*0.45f+ux*0.75f,Ly=-fy*0.55f+uy*0.75f,Lz=-fz*0.55f-rz*0.45f+uz*0.75f; float ll=1.f/sqrtf(Lx*Lx+Ly*Ly+Lz*Lz);
    float d=(nx*Lx+ny*Ly+nz*Lz)*ll; return 0.42f+0.78f*std::max(0.f,d); }
  const RPose*rlp=nullptr; // حركة التعبئة الحالية للمجسّم الأول فقط
  // موديلات الأسلحة الحقيقية (مثلثات بألوان رؤوس مولَّدة من GLB): مجموعات 0 أمامي 1 مخزن 2 متحرك 3 كعب
  struct GMod{int nv;const int*grp;const float*lo;const float*rng;const int16_t*pos;const int8_t*nor;const uint8_t*col;const uint16_t*idx;};
  static const GMod* gmodOf(int wi){
    static const GMod AW={GMD::sniper_NV,GMD::sniper_GRP,GMD::sniper_LO,GMD::sniper_RNG,GMD::sniper_pos,GMD::sniper_nor,GMD::sniper_col,GMD::sniper_idx};
    static const GMod RI={GMD::rifle_NV,GMD::rifle_GRP,GMD::rifle_LO,GMD::rifle_RNG,GMD::rifle_pos,GMD::rifle_nor,GMD::rifle_col,GMD::rifle_idx};
    static const GMod DM={GMD::dmr_NV,GMD::dmr_GRP,GMD::dmr_LO,GMD::dmr_RNG,GMD::dmr_pos,GMD::dmr_nor,GMD::dmr_col,GMD::dmr_idx};
    static const GMod LA={GMD::launcher_NV,GMD::launcher_GRP,GMD::launcher_LO,GMD::launcher_RNG,GMD::launcher_pos,GMD::launcher_nor,GMD::launcher_col,GMD::launcher_idx};
    switch(wi){ case 0:return &RI; case 3:return &AW; case 5:return &DM; case 8:return &LA; default:return nullptr; } }
  struct AwmV{float x,y,z,nx,ny,nz,r,g,b;}; std::vector<AwmV> gmV[9]; std::vector<float> awmW; std::vector<char> awmDone;
  void drawAwmGroup(int wi,int grp,float ox,float oy,float oz,float yaw,float pitch,float roll,float dz,float k,bool lit,const float*tint,float dx=0,float dy=0){
    const GMod*M=gmodOf(wi); if(!M)return; std::vector<AwmV>&awmV=gmV[wi%9];
    if(awmV.empty()){ awmV.resize(M->nv); for(int i=0;i<M->nv;i++){ AwmV&v=awmV[i]; v.x=M->lo[0]+(M->pos[i*3]+32768)/65535.f*M->rng[0]; v.y=M->lo[1]+(M->pos[i*3+1]+32768)/65535.f*M->rng[1]; v.z=M->lo[2]+(M->pos[i*3+2]+32768)/65535.f*M->rng[2]; v.nx=M->nor[i*3]/127.f; v.ny=M->nor[i*3+1]/127.f; v.nz=M->nor[i*3+2]/127.f; v.r=M->col[i*3]/255.f; v.g=M->col[i*3+1]/255.f; v.b=M->col[i*3+2]/255.f; } }
    int off=0; for(int i=0;i<grp;i++)off+=M->grp[i]; int cnt=M->grp[grp]; if(cnt<=0)return;
    std::vector<float>&W=awmW; W.resize((size_t)M->nv*6); std::vector<char>&done=awmDone; done.assign(M->nv,0);
    float rc=cosf(roll),rs=sinf(roll);
    auto xf=[&](int vi){ if(done[vi])return; done[vi]=1; const AwmV&v=awmV[vi]; float lx=(v.x+dx)*gsc,ly=(v.y+dy)*gsc; if(roll!=0){float t=lx*rc-ly*rs;ly=lx*rs+ly*rc;lx=t;} float o[3]; Renderer::rot(yaw,pitch,lx,ly,(v.z+dz)*gsc,o); float nx=v.nx,ny=v.ny; if(roll!=0){float t=nx*rc-ny*rs;ny=nx*rs+ny*rc;nx=t;} float n[3]; Renderer::rot(yaw,pitch,nx,ny,v.nz,n);
      float*w=&W[(size_t)vi*6]; w[0]=ox+o[0];w[1]=oy+o[1];w[2]=oz+o[2];w[3]=n[0];w[4]=n[1];w[5]=n[2]; };
    for(int t=0;t<cnt;t+=3){ int a=M->idx[off+t],b=M->idx[off+t+1],c=M->idx[off+t+2]; xf(a);xf(b);xf(c); const float*A=&W[(size_t)a*6],*B=&W[(size_t)b*6],*C=&W[(size_t)c*6];
      float nx=A[3]+B[3]+C[3],ny=A[4]+B[4]+C[4],nz=A[5]+B[5]+C[5]; float cx=(A[0]+B[0]+C[0])*(1.f/3),cy=(A[1]+B[1]+C[1])*(1.f/3),cz=(A[2]+B[2]+C[2])*(1.f/3);
      { float e1x=B[0]-A[0],e1y=B[1]-A[1],e1z=B[2]-A[2],e2x=C[0]-A[0],e2y=C[1]-A[1],e2z=C[2]-A[2]; float fx=e1y*e2z-e1z*e2y,fy=e1z*e2x-e1x*e2z,fz=e1x*e2y-e1y*e2x; if(fx*nx+fy*ny+fz*nz<0){fx=-fx;fy=-fy;fz=-fz;} // اتجاه المثلث الحقيقي (يتجاهل انقلاب الترتيب بعد التبسيط)
        if(vmMinD<=0&&(r.cam.x-cx)*fx+(r.cam.y-cy)*fy+(r.cam.z-cz)*fz<=0)continue; }
      if(vmMinD>0){ float d0=(A[0]-r.cam.x)*vmF[0]+(A[1]-r.cam.y)*vmF[1]+(A[2]-r.cam.z)*vmF[2],d1=(B[0]-r.cam.x)*vmF[0]+(B[1]-r.cam.y)*vmF[1]+(B[2]-r.cam.z)*vmF[2],d2=(C[0]-r.cam.x)*vmF[0]+(C[1]-r.cam.y)*vmF[1]+(C[2]-r.cam.z)*vmF[2]; if(std::min(d0,std::min(d1,d2))<vmMinD)continue; }
      float il=1.f/std::max(1e-5f,sqrtf(nx*nx+ny*ny+nz*nz)); float l=lit?std::min<float>(1.25f,r.lightOf(nx*il,ny*il,nz*il)):(vmMinD>0?vmLight(nx*il,ny*il,nz*il):1.f); l*=k;
      const AwmV&va=awmV[a],&vb=awmV[b],&vc=awmV[c]; float cr=(va.r+vb.r+vc.r)*(1.f/3)*l*tint[0],cg=(va.g+vb.g+vc.g)*(1.f/3)*l*tint[1],cb=(va.b+vb.b+vc.b)*(1.f/3)*l*tint[2];
      float pts[3][3]={{A[0],A[1],A[2]},{B[0],B[1],B[2]},{C[0],C[1],C[2]}}; r.poly(pts,3,cr,cg,cb,false,nullptr,true); } }
  void drawGunAt(float ox,float oy,float oz,float yaw,float pitch,int wi,float flash,bool lit,float k,const GAtt*at=nullptr,bool hideOptic=false){
    const GunDef&g=GUNDEFS[wi]; GAtt a=at?*at:GAtt(); if(hideOptic)a.optic=0; const RPose*R=rlp; float rl=R?R->rz:0.f; r.obbRoll=rl; // أثناء التصويب عبر المنظار: لا نرسم جسم المنظار كي لا يحجب الرؤية
    auto part=[&](float x,float y,float z,float sx,float sy,float sz,const float*c){float o[3];{float xx=x,yy=y; if(rl!=0){float c_=cosf(rl),s_=sinf(rl); xx=x*c_-y*s_; yy=x*s_+y*c_;} Renderer::rot(yaw,pitch,xx,yy,z,o);}r.obb(ox+o[0],oy+o[1],oz+o[2],sx*.5f,sy*.5f,sz*.5f,yaw,pitch,c[0]*k,c[1]*k,c[2]*k,true,lit);};
    const bool awm=gmodOf(wi)!=nullptr&&(vmMinD>0||[&]{ float dx=ox-r.cam.x,dy=oy-r.cam.y,dz=oz-r.cam.z; return dx*dx+dy*dy+dz*dz<gunLod()*gunLod(); }()); // بعيد: مكعبات بدل الموديل المفصّل (يوفّر الكثير من المثلثات)
    if(awm){ static const float one[3]={1,1,1}, fast[3]={1.5f,0.95f,0.5f}; drawAwmGroup(wi,0,ox,oy,oz,yaw,pitch,rl,0,k,lit,one); if(!(R&&R->hideRear))drawAwmGroup(wi,3,ox,oy,oz,yaw,pitch,rl,0,k,lit,one); drawAwmGroup(wi,2,ox,oy,oz,yaw,pitch,rl,R?R->bolt:0.f,k,lit,one); if(!(R&&R->hideMag))drawAwmGroup(wi,1,ox,oy,oz,yaw,pitch,rl,0,k,lit,a.mag==2?fast:one); if(R&&R->hasRocket)drawAwmGroup(wi,1,ox,oy,oz,yaw,pitch,rl,R->rk[2],k,lit,one,R->rk[0],R->rk[1]); }
    if(!awm)for(int i=0;i<g.np;i++){float c[3];gcol(g.p[i].c,c); if(R&&R->hideRear&&i!=0&&g.p[i].z-g.p[i].sz*0.5f>0.1f)continue; float dz=(R&&(i==R->shiftIdx||i==R->shiftIdx2))?R->shiftV:0.f; part(g.p[i].x,g.p[i].y,g.p[i].z+dz,g.p[i].sx,g.p[i].sy,g.p[i].sz,c);}
    float cb[3],cd[3]; gcol('b',cb); gcol('d',cd);
    if(!awm&&g.hasMag&&!(R&&R->hideMag)){ float kk=a.mag==1?1.4f:1.f; float c[3]; if(a.mag==2){c[0]=0.9f;c[1]=0.5f;c[2]=0.1f;} else gcol('b',c); part(g.mag[0],g.mag[1]-(kk-1)*g.mag[4]*0.5f,g.mag[2],g.mag[3],g.mag[4]*kk,g.mag[5],c); }
    if(!awm&&a.optic>0){ static const float OP[5][4]={{0,0,0,0},{0.03f,0.035f,0.05f,0.075f},{0.034f,0.034f,0.12f,0.08f},{0.04f,0.04f,0.18f,0.085f},{0.045f,0.045f,0.24f,0.09f}}; const float*o=OP[a.optic]; float zc=-0.04f,top=o[3];
      part(0,0.044f,zc,0.024f,0.014f,o[2]*0.6f+0.04f,cb);
      if(a.optic==1){ float gl[3]={0.5f,0.8f,1}, rd[3]={1,0.1f,0.1f}; part(0,top-0.02f,zc,0.034f,0.04f,0.05f,cb); part(0,top-0.02f,zc-0.026f,0.026f,0.028f,0.004f,gl); part(0,top-0.018f,zc-0.028f,0.006f,0.006f,0.003f,rd); }
      else { float R=o[0],L=o[2],cy=top-R/2-0.004f; float gl[3]={0.3f,0.55f,0.9f}; part(0,cy,zc,R,R,L,cb); part(0,cy,zc-L/2-0.012f,R*1.35f,R*1.35f,0.03f,cd); part(0,cy,zc+L/2+0.01f,R*1.2f,R*1.2f,0.025f,cd); part(0,cy,zc-L/2-0.028f,R*1.05f,R*1.05f,0.004f,gl); } }
    float mzOff=0;
    if(a.muz==1){ part(0,0.005f,g.mz-0.07f,0.036f,0.036f,0.2f,cb); for(int q=0;q<4;q++)part(0,0.005f,g.mz-0.01f-q*0.045f,0.04f,0.04f,0.006f,cd); mzOff=-0.16f; }
    else if(a.muz==2){ part(0,0.005f,g.mz-0.01f,0.036f,0.036f,0.06f,cd); part(0.019f,0.005f,g.mz-0.01f,0.004f,0.02f,0.03f,cb); part(-0.019f,0.005f,g.mz-0.01f,0.004f,0.02f,0.03f,cb); part(0,0.021f,g.mz-0.01f,0.02f,0.004f,0.03f,cb); }
    if(a.under==1){ part(0,-0.075f,g.mz*0.5f-0.02f,0.028f,0.09f,0.03f,cd); part(0,-0.032f,g.mz*0.5f-0.02f,0.04f,0.012f,0.05f,cb); part(0,-0.118f,g.mz*0.5f-0.02f,0.032f,0.01f,0.034f,cb); }
    else if(a.under==2){ float rd[3]={1,0.1f,0.1f}; part(0.03f,-0.015f,g.mz*0.5f,0.022f,0.026f,0.07f,cb); part(0.03f,-0.015f,g.mz*0.5f-0.04f,0.012f,0.012f,0.012f,rd); }
    if(flash>0){float c[3]={1,0.85f,0.4f},w2[3]={1,1,0.85f};float sz=(a.muz==1?0.35f:1.f)*(0.06f+0.05f*flash); float zz=g.mz+mzOff-0.03f;
      part(0,0.005f,zz,sz,sz,sz*2.2f,w2); part(0,0.005f,zz-0.01f,sz*3.2f,sz*0.35f,sz*0.5f,c); part(0,0.005f,zz-0.01f,sz*0.35f,sz*3.2f,sz*0.5f,c); part(0,0.005f,zz-0.12f,sz*0.5f,sz*0.5f,sz*3.f,c);}
    if(R){ // عناصر التعبئة المتحركة: مخازن، خرطوشة، مقبض التلقيم
      float mcol[3]; if(a.mag==2){mcol[0]=0.9f;mcol[1]=0.5f;mcol[2]=0.1f;} else gcol('b',mcol);
      for(int q=0;q<R->nm;q++)part(R->mg[q][0],R->mg[q][1],R->mg[q][2],R->ms[0],R->ms[1],R->ms[2],mcol);
      if(R->ns>0){ float br[3]={0.85f,0.62f,0.18f},cp[3]={0.7f,0.1f,0.08f}; part(R->sh[0][0],R->sh[0][1],R->sh[0][2],0.02f,0.02f,0.07f,br); part(R->sh[0][0],R->sh[0][1],R->sh[0][2]+0.03f,0.021f,0.021f,0.018f,cp); }
      if(R->handle&&!awm){ float gg[3]; gcol('g',gg); bool sn=(wi==3); part(sn?0.062f:0.03f,sn?0.f:0.032f,0.05f+R->bolt,0.014f,0.014f,0.04f,gg); } }
    r.obbRoll=0;
  }
  void drawAvatar(const Ent&a,float camDist){
    float tc[2][3]={{0.82f,0.14f,0.14f},{0.15f,0.36f,0.9f}}; const float*c=tc[a.team]; const float td[2][3]={{0.5f,0.08f,0.08f},{0.08f,0.2f,0.55f}}; const float*d=td[a.team];
    float x=a.b.x,y=a.b.y,z=a.b.z,yw=a.yaw;
    static const float pants[3]={.17f,.18f,.15f},boot[3]={.08f,.07f,.07f},vest[3]={.2f,.22f,.2f},plate[3]={.26f,.28f,.25f},pocket[3]={.16f,.17f,.15f},pack[3]={.18f,.2f,.16f},belt[3]={.1f,.1f,.11f},skin[3]={.86f,.68f,.55f},glove[3]={.1f,.1f,.11f},visor[3]={.05f,.06f,.08f},mask[3]={.12f,.12f,.12f};
    // ---- سقوط عند الموت: ينهار للأمام بسلاسة ثم يبقى مستلقياً ----
    if(!a.alive){ float dt_=std::max<float>(0.f,std::min<float>(1.f,(4.f-a.respawn)/0.45f)); if(a.respawn>4.f||a.respawn<=0)dt_=1; float k=1-(1-dt_)*(1-dt_); float ph=-1.5708f*k;
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
      float th=sinf(ph)*amp+(cr?0.85f:0.f); float flex=std::max<float>(0.f,cosf(ph))*0.85f*mv*(cr?0.5f:1.f)+(cr?1.7f:0.f); float sa=th-flex;
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
    float gs[3]; UPp(0.12f,1.3f,0,gs); float cA=cosf(ap),sA=sinf(ap); const bool lau=isExpl(a.wpn); const float gdy=lau?-0.13f:0.05f,gdz=lau?0.14f:0.38f; float wo[3]; Renderer::rot(yw,0,gs[0],gs[1]-gdy*cA+gdz*sA,gs[2]-gdy*sA-gdz*cA,wo);
    gsc=(gmodOf(a.wpn)&&WKIND[a.wpn%WEAPON_COUNT]>1)?1.5f:1.f; drawGunAt(x+wo[0],y+wo[1],z+wo[2],yw,ap,a.wpn,a.flash>0?1.f:0.f,true,1.f); gsc=1.f;
    // ---- ذراعان بمفصل مرفق (IK): اليد اليمنى على قبضة الزناد واليسرى على المقدّمة، مع كتف وجذع يميلان مع الإطلاق ----
    { const float gsK=(gmodOf(a.wpn)&&WKIND[a.wpn%WEAPON_COUNT]>1)?1.5f:1.f; const GunDef&gd=GUNDEFS[a.wpn]; const bool pistol=(a.wpn==6||a.wpn==7);
      float gl[3]={gs[0],gs[1]-gdy*cA+gdz*sA,gs[2]-gdy*sA-gdz*cA}; // أصل السلاح (محلي)
      auto G2L=[&](float hx,float hy,float hz,float*o3){ hx*=gsK; hy*=gsK; hz*=gsK; o3[0]=gl[0]+hx; o3[1]=gl[1]+hy*cA-hz*sA; o3[2]=gl[2]+hy*sA+hz*cA; };
      auto seg=[&](const float*p0,const float*p1,float th,const float*col){ float w0[3],w1[3]; Renderer::rot(yw,0,p0[0],p0[1],p0[2],w0); Renderer::rot(yw,0,p1[0],p1[1],p1[2],w1);
        float dx=w1[0]-w0[0],dy=w1[1]-w0[1],dz=w1[2]-w0[2],l=sqrtf(dx*dx+dy*dy+dz*dz); if(l<0.01f)return; float yw2=atan2f(-dx,-dz),pt=asinf(dy/l);
        r.obb(x+(w0[0]+w1[0])*0.5f,y+(w0[1]+w1[1])*0.5f,z+(w0[2]+w1[2])*0.5f,th*.5f,th*.5f,l*.5f,yw2,pt,col[0],col[1],col[2]); };
      auto limb=[&](float side,const float*hand,bool rightArm){ float S[3]; UPp(side*0.27f,1.36f,0,S); const float L1=0.3f,L2=0.29f; float dv[3]={hand[0]-S[0],hand[1]-S[1],hand[2]-S[2]}; float D=sqrtf(dv[0]*dv[0]+dv[1]*dv[1]+dv[2]*dv[2]); D=std::max(0.12f,D);
        float k2=D>L1+L2-0.02f?(L1+L2-0.02f)/D:1.f; float H[3]={S[0]+dv[0]*k2,S[1]+dv[1]*k2,S[2]+dv[2]*k2}; D*=k2; float ux=(H[0]-S[0])/D,uy=(H[1]-S[1])/D,uz=(H[2]-S[2])/D;
        float pv[3]={side*0.55f,-1.f,0.25f}; float dp=pv[0]*ux+pv[1]*uy+pv[2]*uz; pv[0]-=ux*dp; pv[1]-=uy*dp; pv[2]-=uz*dp; float pl=sqrtf(pv[0]*pv[0]+pv[1]*pv[1]+pv[2]*pv[2]); if(pl<1e-3f){pv[0]=side;pv[1]=0;pv[2]=0;pl=1;} for(int q=0;q<3;q++)pv[q]/=pl;
        float aa=(L1*L1-L2*L2+D*D)/(2*D),hh=sqrtf(std::max(0.f,L1*L1-aa*aa)); float E[3]={S[0]+ux*aa+pv[0]*hh,S[1]+uy*aa+pv[1]*hh,S[2]+uz*aa+pv[2]*hh};
        seg(S,E,0.115f,d); seg(E,H,0.1f,d); float hw[3]; Renderer::rot(yw,0,H[0],H[1],H[2],hw); r.obb(x+hw[0],y+hw[1],z+hw[2],.05f,.05f,.055f,yw,ap,glove[0],glove[1],glove[2]); (void)rightArm; };
      float rh[3],lh[3]; float mzz=gd.mz;
      if(isExpl(a.wpn)){ G2L(0.02f,-0.06f,mzz*0.10f,rh); G2L(-0.01f,0.0f,mzz*0.55f,lh); }
      else if(pistol){ G2L(0.0f,-0.055f,mzz*0.20f,rh); G2L(-0.025f,-0.075f,mzz*0.18f,lh); }
      else { G2L(-0.005f,-0.06f,mzz*0.32f,rh); G2L(-0.03f,-0.07f,mzz*0.72f,lh); }
      limb(1.f,rh,true); limb(-1.f,lh,false); }
  }
  void render(){
    Ent&me=e[meI]; float eyeH=me.alive?(me.b.y+(me.crouch?PK.eyec:PK.eye)):me.b.y+0.6f;
    float fov=1.3f*fovMul; if(adsHeld&&me.alive){fov=1.3f*fovMul/zoomOf(me);}
    Cam c{me.b.x,eyeH,me.b.z,me.yaw,me.pitch,fov}; { }
    if(drive>=0&&me.alive&&!passenger()){ const VH::Veh&v=vh[drive]; float cp=cosf(me.pitch),fxw=-sinf(me.yaw)*cp,fyw=sinf(me.pitch),fzw=-cosf(me.yaw)*cp; float cx0=v.x,cy0=v.type==1?2.3f:2.2f,cz0=v.z; float D=v.type==1?11.f:8.f; float tw2=raycastMap(w,cx0,cy0,cz0,-fxw,-fyw,-fzw,D); D=std::min<float>(D,tw2-0.3f); D=std::max<float>(v.type==1?6.5f:1.5f,D); c.x=cx0-fxw*D; c.y=std::max<float>(0.5f,cy0-fyw*D); c.z=cz0-fzw*D; camX=c.x;camY=c.y;camZ=c.z; }
    if(dn.on&&me.alive)c=Cam{dn.x,dn.y,dn.z,dn.yaw,dn.pitch,1.3f};
    if(air)airCam(c);
    r.setCam(c); r.world(w);
    if(air)airDraw(c);
    for(auto&v:vh){ float dx=v.x-c.x,dz=v.z-c.z,dd=sqrtf(dx*dx+dz*dz); if(dd<r.fogF)VH::draw(r,v,dd); }
    for(auto&c:shells)r.obb(c.x,c.y,c.z,0.012f,0.012f,0.03f,t*20+c.x*9,0,0.85f,0.62f,0.15f,false,false);
    for(auto&g:grs)r.obb(g.x,g.y,g.z,0.09f,0.12f,0.09f,t*6,0,0.2f,0.3f,0.2f,false,false);
    for(auto&q:rks){ static const float one[3]={1,1,1}; float yw=atan2f(-q.dx,-q.dz),pt=asinf(q.dy); float px=q.ox+q.dx*q.t-q.dx*0.475f,py=q.oy+q.dy*q.t-q.dy*0.475f,pz=q.oz+q.dz*q.t-q.dz*0.475f; drawAwmGroup(8,1,px,py,pz,yw,pt,0,0,1.f,true,one); }
    for(auto&c:c4s){ r.obb(c.x,c.y+0.04f,c.z,0.1f,0.04f,0.065f,0,0,0.22f,0.22f,0.2f,true,true); r.obb(c.x,c.y+0.085f,c.z,0.085f,0.008f,0.05f,0,0,0.45f,0.4f,0.3f,true,true); bool on=((int)(t*2.2f))%2==0; float k=on?1.f:0.15f; r.obb(c.x+0.05f,c.y+0.1f,c.z,0.015f,0.015f,0.015f,0,0,c.team?0.1f*k:k,0.1f*k,c.team?k:0.1f*k,false,false); }
    for(auto&q:puffs){ float k=std::min<float>(1.f,q.t); if(q.fire)r.obb(q.x,q.y,q.z,q.sz*.4f,q.sz*.4f,q.sz*.4f,0,0,1.0f,0.55f*k+0.2f,0.1f,false,false); else r.obb(q.x,q.y,q.z,q.sz*.5f,q.sz*.5f,q.sz*.5f,0,0,0.12f+0.1f*k,0.12f+0.1f*k,0.12f+0.1f*k,false,false); }
    for(int i=0;i<(int)e.size();i++){if((i==meI&&!dn.on)||e[i].gone||(air&&i<(int)entJet.size()&&entJet[i]>=0))continue;const Ent&a=e[i];float dx=a.b.x-c.x,dz=a.b.z-c.z; if(dx*dx+dz*dz>r.fogF*r.fogF||(dx*dx+dz*dz<0.5f&&i!=meI))continue; drawAvatar(a,sqrtf(dx*dx+dz*dz));}
    for(auto&q:tr){ float dx=q.x1-q.x0,dy=q.y1-q.y0,dz=q.z1-q.z0,l=sqrtf(dx*dx+dy*dy+dz*dz); if(l<0.1f)continue; float p0=std::min<float>(1.f,std::max<float>(0.f,1.f-q.t/0.09f)); float hd=p0*l,tl=std::max<float>(0.f,hd-std::min<float>(5.f,l*0.7f)); float sg=hd-tl; if(sg<0.05f)continue;
      float yw=atan2f(-dx,-dz),pt=asinf(dy/l); float m=(hd+tl)*0.5f/l; float cx=q.x0+dx*m,cy=q.y0+dy*m,cz=q.z0+dz*m; float fa=1.f-p0*0.5f;
      r.obb(cx,cy,cz,0.022f,0.022f,sg/2,yw,pt,1.0f*fa,0.62f*fa,0.2f*fa,false,false); r.obb(cx,cy,cz,0.01f,0.01f,sg/2+0.01f,yw,pt,1,0.98f,0.85f,false,false);
      float hx=q.x0+dx*hd/l,hy=q.y0+dy*hd/l,hz=q.z0+dz*hd/l; r.obb(hx,hy,hz,0.035f,0.035f,0.06f,yw,pt,1,1,0.9f,false,false); }
    for(auto&k:sparks){ float a=std::min<float>(1.f,k.t*5.f); r.obb(k.x,k.y,k.z,0.025f,0.025f,0.025f,0,0,1.f,0.7f*a+0.2f,0.2f,false,false); }

    // مجسّم السلاح (نفس قطع نسخة أندرويد)
    if(!skipVM&&me.alive&&(drive<0||passenger())&&!dn.on&&myJet()<0&&!(adsHeld&&(zoomOf(me)>=2.f||(gmodOf(me.wpn)&&me.wpn!=8)))){ r.flush(); r.zBias=1000.f; const GunDef&g=GUNDEFS[me.wpn]; float k=adsHeld?1.f:0.f; float top=g.sy;
      static float bob=0; bob+=me.moving?(me.sprint?0.17f:0.11f):0; float bx=sinf(bob)*0.008f*(me.moving?1:0),by=fabsf(cosf(bob))*0.008f*(me.moving?1:0);
      float x=0.17f*(1-k)+0*k+bx,y=-0.17f*(1-k)+(-top-0.016f)*k+by,z=-0.33f*(1-k)-0.36f*k; float kick=me.flash>0?0.045f:0.f; z+=kick; float rx=kick*1.2f,ry=0.07f*(1-k); y+=sinf(t*1.7f)*0.0025f*(1-k*0.7f); x+=sinf(t*0.9f)*0.0015f;
      RPose RP; bool inRl=false; if(me.reloadT>=0){ float tt=std::min<float>(1.f,me.reloadT/(WEAPONS[me.wpn].reload*me.relM)); int wk=WKIND[me.wpn%WEAPON_COUNT]; RL::pose(GUNDEFS[me.wpn],wk<=1,wk==7,tt,RP,isExpl(me.wpn)); inRl=true; x+=RP.x; y+=RP.y; z+=RP.z; rx+=RP.rx; ry+=RP.ry; }
      if(me.sprint){y-=0.06f;rx-=0.35f;ry=0.4f;x-=0.04f;}
      float O[3];Renderer::rot(me.yaw,me.pitch,x,y,z,O); float gy=me.yaw+ry,gp=me.pitch+rx; float ox=c.x+O[0],oy=c.y+O[1],oz=c.z+O[2];
      { float cp=cosf(me.pitch); vmF[0]=-sinf(me.yaw)*cp; vmF[1]=sinf(me.pitch); vmF[2]=-cosf(me.yaw)*cp; vmMinD=0.2f; }
      rlp=inRl?&RP:nullptr; drawGunAt(ox,oy,oz,gy,gp,me.wpn,me.flash>0?std::min<float>(1.f,me.flash*16.f):0.f,false,0.95f,&wAtt[me.wpn&15],adsHeld); rlp=nullptr; vmMinD=0;
      float sl[3]={me.team?0.08f:0.5f,me.team?0.2f:0.08f,me.team?0.55f:0.08f},sk[3]={.86f,.68f,.55f};
      float hrl=inRl?RP.rz:0.f; auto hp=[&](float px,float py,float pz,float sx,float sy,float sz,const float*cc){float o[3];float xx=px,yy=py; if(hrl!=0){float c_=cosf(hrl),s_=sinf(hrl); xx=px*c_-py*s_; yy=px*s_+py*c_;} Renderer::rot(gy,gp,xx,yy,pz,o);r.obbRoll=hrl;r.obb(ox+o[0],oy+o[1],oz+o[2],sx*.5f,sy*.5f,sz*.5f,gy,gp,cc[0]*0.95f,cc[1]*0.95f,cc[2]*0.95f,false,false);r.obbRoll=0;};
      const bool pk=WKIND[me.wpn%WEAPON_COUNT]<=1; float gz=pk?0.04f:0.06f; hp(0,-0.075f,gz,.05f,.05f,.06f,sk); hp(0.015f,-0.12f,gz+0.12f,.065f,.06f,.28f,sl);
      if(inRl){ hp(RP.hand[0],RP.hand[1],RP.hand[2],.055f,.05f,.07f,sk); hp(RP.hand[0]-0.03f,RP.hand[1]-0.05f,RP.hand[2]+0.13f,.06f,.055f,.18f,sl); } else if(!pk){hp(-0.005f,-0.05f,g.mz*0.42f,.055f,.05f,.07f,sk);hp(-0.03f,-0.09f,g.mz*0.42f+0.2f,.06f,.055f,.3f,sl);} else hp(-0.03f,-0.075f,gz+0.01f,.05f,.05f,.06f,sk);
    }
    if(adsHeld&&me.alive&&drive<0&&!dn.on&&myJet()<0&&zoomOf(me)>=3.4f){ // عدسة المنظار (مثل نسخة الأندرويد): أسود خارج دائرة + شعرة تصويب
      r.flush(); int Hh=r.H,Ww=r.W,rad=(int)(Hh*0.33f),cx=Ww/2,cy=Hh/2;
      for(int y=0;y<Hh;y+=1){ int dy=y-cy; int half=std::abs(dy)>=rad?0:(int)sqrtf((float)(rad*rad-dy*dy)); blendSpan(y,0,cx-half,0xFF000000,1.f); blendSpan(y,cx+half,Ww,0xFF000000,1.f); }
      rect(cx-rad,cy,rad*2,1,0xFF000000,0.9f); rect(cx,cy-rad,1,rad*2,0xFF000000,0.9f); }
    if(adsHeld&&me.alive&&drive<0&&!dn.on&&myJet()<0&&zoomOf(me)<3.4f&&(optZ>0||(gmodOf(me.wpn)&&me.wpn!=8))){ // مصوّب بديل (النقطة الحمراء / تقاطع x2) حين يُخفى جسم السلاح
      r.flush(); int cx=r.W/2,cy=r.H/2,s=std::max(1,r.H/600); rect(cx-s,cy-s,s*2+1,s*2+1,0xFFFF2828,0.95f); // نقطة صغيرة لا تحجب الهدف
      if(zoomOf(me)>=2.f){ int rad=r.H/12,gap=s*4; rect(cx-rad,cy,rad-gap,1,0xFF000000,0.85f); rect(cx+gap,cy,rad-gap,1,0xFF000000,0.85f); rect(cx,cy+gap,1,rad-gap,0xFF000000,0.85f); rect(cx,cy-rad,1,rad-gap,0xFF000000,0.85f); } }
    hud();
  }
  // ---- واجهة ----
  void px(int x,int y,uint32_t c){ if(x>=0&&y>=0&&x<r.W&&y<r.H){ r.fb[y*r.W+x]=c; r.rowD[y]=1; } }
  void blendSpan(int y,int x0,int x1,uint32_t c,float a){ if(y<0||y>=r.H)return; x0=std::max(0,x0); x1=std::min(r.W,x1); if(x1<=x0||a<=0)return; uint32_t*row=&r.fb[(size_t)y*r.W]; r.rowD[y]=1;
    if(a>=0.999f){ std::fill(row+x0,row+x1,c|0xFF000000u); return; }
    uint32_t ia=(uint32_t)(a*256.f+0.5f),na=256-ia,crb=(c&0xFF00FFu)*ia,cg=(c&0x00FF00u)*ia,ca=255u*ia;
    for(int i=x0;i<x1;i++){ uint32_t o=row[i]; uint32_t rb=(((o&0xFF00FFu)*na+crb)>>8)&0xFF00FFu; uint32_t g=(((o&0x00FF00u)*na+cg)>>8)&0x00FF00u; uint32_t aa=((((o>>24)*na)+ca)>>8)&0xFFu; row[i]=(aa<<24)|rb|g; } } // ألفا مُتراكمة (تُستعمل في وضع GPU؛ في الـCPU تبقى 255)
  void rect(int x,int y,int w2,int h2,uint32_t c,float a=1){ r.flush(); for(int j=std::max(0,y);j<std::min(r.H,y+h2);j++)blendSpan(j,x,x+w2,c,a); }
  // خط حقيقي: sc*9 = حجم الخط بالبكسل ؛ يدعم العربية
  static int fontEm(int sc){ return std::max(8,sc*9); }
  static const FGlyph* fgFor(int em,uint32_t cp,int&base){ const FGlyph*a;int n; if(em<=22){a=FG_S;n=FN_S;base=16;} else if(em<=44||cp>=0x80){a=FG_M;n=FN_M;base=32;} else {a=FG_L;n=FN_L;base=64;}
    const FGlyph*g=AR::find(a,n,cp); if(!g&&cp!=0x20){ g=AR::find(a,n,'?'); } return g; }
  static float gunLod(){ static const float L[5]={2.f,3.5f,5.f,7.f,10.f}; return L[g_quality%5]; }
  int tw(const char*s,int sc){ s=trl(s); int em=fontEm(sc),best=0,cur=0; for(auto cp:AR::visual(s)){ if(cp=='\n'){best=std::max(best,cur);cur=0;continue;} int base; const FGlyph*g=fgFor(em,cp,base); cur+=g?(int)(g->adv16/16.0f*em/base+0.5f):em/3; } return std::max(best,cur); }
  void text(int x,int y,const char*s,int sc,uint32_t col){ s=trl(s); r.flush(); int em=fontEm(sc); std::string line; const char*p=s; int ly=y;
    auto flush=[&](){ int cx=x,bl=ly+7*sc; for(auto cp:AR::visual(line.c_str())){ int base; const FGlyph*g=fgFor(em,cp,base); if(!g){cx+=em/3;continue;} float k=(float)em/base;
        if(g->w){ int dw=std::max<float>(1,(int)(g->w*k+0.5f)),dh=std::max<float>(1,(int)(g->h*k+0.5f)); int dx=cx+(int)(g->bx*k+(g->bx<0?-0.5f:0.5f)),dy=bl-(int)(g->by*k+0.5f); const uint8_t*src=FDATA+g->off;
          for(int j=0;j<dh;j++){ int py=dy+j; if(py<0||py>=r.H)continue; r.rowD[py]=1; float fy=(j+0.5f)/k-0.5f; int y0=(int)floorf(fy); float ty=fy-y0; int y1=std::min(g->h-1,y0+1); y0=std::max(0,y0);
            for(int i=0;i<dw;i++){ int px=dx+i; if(px<0||px>=r.W)continue; float fx=(i+0.5f)/k-0.5f; int x0=(int)floorf(fx); float tx=fx-x0; int x1=std::min(g->w-1,x0+1); x0=std::max(0,x0);
              float a=(src[y0*g->w+x0]*(1-tx)+src[y0*g->w+x1]*tx)*(1-ty)+(src[y1*g->w+x0]*(1-tx)+src[y1*g->w+x1]*tx)*ty; a/=255.f; if(a<0.04f)continue;
              uint32_t o=r.fb[py*r.W+px]; auto m=[&](int sh){int A=(o>>sh)&255,B=(col>>sh)&255;return (uint32_t)(A+(B-A)*a)<<sh;}; { int oa=(o>>24)&255; r.fb[py*r.W+px]=((uint32_t)(oa+(255-oa)*a)<<24)|m(16)|m(8)|m(0); } } } }
        cx+=(int)(g->adv16/16.0f*k+0.5f); } line.clear(); };
    for(;*p;p++){ if(*p=='\n'){ flush(); ly+=em*3/2; } else line+=*p; } flush(); }
  void textC(int cx,int y,const char*s,int sc,uint32_t col){text(cx-tw(s,sc)/2,y,s,sc,col);}
  // ---------- عجلة الأسلحة / شريط التبديل (PS4 + أندرويد + آيفون) ----------
  void wheelRect(int i,int sc,int&x,int&y,int&w2,int&h2)const{ int W=r.W,H=r.H; w2=98*sc; h2=29*sc; int gp=3*sc; int cx=W/2-(3*w2+2*gp)/2,cy=H/2-(3*h2+2*gp)/2-6*sc; x=cx+(i%3)*(w2+gp); y=cy+(i/3)*(h2+gp); }
  int hudSc()const{ return std::max(1,(int)((r.H*hudK*1.2f+135)/270)); } // 1.2 = واجهة أكبر
  void weaponPanel(int sc){
    float a=wheelOpen?1.f:std::min<float>(1.f,wsT*2.5f); if(a<=0.01f)return; int sm2=std::max(1,sc-1); Ent&me=e[meI]; char b[48];
    float mxD=1,mxR=1,mxG=1; for(int i=0;i<WEAPON_COUNT;i++){ mxD=std::max<float>(mxD,WEAPONS[i].dmg*std::min(4,std::max(1,WEAPONS[i].pellets))); mxR=std::max<float>(mxR,WEAPONS[i].rpm); mxG=std::max<float>(mxG,WEAPONS[i].r1); }
    int x0,y0,w0,h0; wheelRect(0,sc,x0,y0,w0,h0); int gp=3*sc; if(wheelOpen){ rect(0,0,r.W,r.H,0xFF05080C,0.55f); } rrect(x0-6*sc,y0-14*sc,3*w0+2*gp+12*sc,3*h0+2*gp+20*sc,6*sc,0xFF05080C,0.72f*a);
    textC(r.W/2,y0-11*sc,wheelOpen?"اختر سلاحك":"السلاح",sc,C_ACC);
    for(int i=0;i<WEAPON_COUNT;i++){ int x,y,w2,h2; wheelRect(i,sc,x,y,w2,h2); const WeaponDef&d=WEAPONS[i]; bool sel=i==me.wpn;
      rrect(x,y,w2,h2,3*sc,sel?0xFF4A3A14:0xFF121A24,0.92f*a); if(sel){ rect(x,y,w2,sc,C_ACC,a); rect(x,y+h2-sc,w2,sc,C_ACC,a); rect(x,y,sc,h2,C_ACC,a); rect(x+w2-sc,y,sc,h2,C_ACC,a); }
      text(x+4*sc,y+3*sc,d.name,sc,sel?C_ACC:0xFFFFFFFF); snprintf(b,48,T("%d طلقة"),d.mag); text(x+4*sc,y+h2-9*sc,b,sm2,0xFF8FA0B4);
      int bw=46*sc,bx=x+w2-bw-5*sc; float v[3]={d.dmg*std::min(4,std::max(1,d.pellets))/mxD,d.rpm/mxR,d.r1/mxG}; uint32_t cl[3]={0xFFE2573B,0xFFFFB02E,0xFF42D66B};
      for(int k=0;k<3;k++){ int by=y+h2-18*sc+k*5*sc; rect(bx,by,bw,3*sc,0xFF2A3644,a); rect(bx,by,std::max(sc,(int)(bw*std::min(1.f,v[k]))),3*sc,cl[k],a); } }
  }
  void hud(){
    int W=r.W,H=r.H,sc=std::max(1,(int)((H*hudK+135)/270)); Ent&me=e[meI]; const WeaponDef&d=WEAPONS[me.wpn];
    { r.flush(); for(auto&o:e){ if(!o.alive||o.reveal<=0||&o==&me||o.team==me.team)continue; float ddx=o.b.x-r.cam.x,ddy=o.b.y+2.1f-r.cam.y,ddz=o.b.z-r.cam.z; float zc=ddx*r.fx+ddy*r.fy+ddz*r.fz; if(zc<0.5f)continue;
        float xc=ddx*r.rx+ddz*r.rz,yc=ddx*r.ux+ddy*r.uy+ddz*r.uz; int X=W/2+(int)(xc/zc*r.focal),Y=H/2-(int)(yc/zc*r.focal); if(X<8||X>W-8||Y<8||Y>H-8)continue; float a=std::min<float>(1.f,o.reveal*2.f);
        int s2=3*sc; for(int k=0;k<=s2;k++){ int wdt=s2-k; blendSpan(Y-k,X-wdt,X+wdt+1,0xFFFF3030,a); blendSpan(Y+k,X-wdt,X+wdt+1,0xFFFF3030,a); }
        char db[16]; snprintf(db,16,"%dm",(int)sqrtf(ddx*ddx+ddy*ddy+ddz*ddz)); textC(X,Y+s2+3*sc,db,sc,0xFFFFB0B0); } }
    if(dn.on){ rect(0,0,W,6*sc,0xFF000000,0.5f); rect(0,H-6*sc,W,6*sc,0xFF000000,0.5f); char db[96]; snprintf(db,96,T("مسيّرة  %d ث   R2: تفجير   D-pad ↓: هبوط"),(int)ceilf(dn.t)); textC(W/2,H-14*sc,db,sc,0xFF9CC8FF);
      float rg=hypotf(dn.x-e[meI].b.x,dn.z-e[meI].b.z); if(rg>58.f)textC(W/2,H/2+40*sc,"ابتعدت كثيراً عن الجندي",sc,0xFFFFC040); }
    if(dmgFlash>0)rect(0,0,W,H,0xFFB01010,dmgFlash*0.9f);
    if(hitAngT>0&&!menu){ float a=std::min<float>(1.f,hitAngT*1.5f); for(int k=-3;k<=3;k++){ float an=hitAng+k*0.09f; int hx=W/2-(int)(sinf(an)*H*0.22f),hy=H/2-(int)(cosf(an)*H*0.22f); int s2=std::max<int>(3,H/150); rect(hx-s2,hy-s2,s2*2,s2*2,0xFFFF3030,a*(k==0?1.f:0.6f)); } }
    if(!me.alive){ rect(0,0,W,H,0xFF300000,0.5f); char b[32];snprintf(b,32,"RESPAWN %d",(int)ceilf(me.respawn));textC(W/2,H/2-4*sc,b,sc*2,0xFFFFFFFF); }
    else { int cx=W/2,cy=H/2; uint32_t cc=hitMark>0?0xFFFF4040:0xFFFFFFFF; int g=3*sc+(me.moving?2*sc:0)+(me.flash>0?2*sc:0); if(!(adsHeld&&zoomOf(me)>=2.f)){ rect(cx-g-4*sc,cy,4*sc,sc,cc);rect(cx+g,cy,4*sc,sc,cc);rect(cx,cy-g-4*sc,sc,4*sc,cc);rect(cx,cy+g,sc,4*sc,cc); } /* المنظار له شعرته الخاصة: الشعرة الديناميكية كانت تغطي العدو البعيد */ if(hitMark>0){rect(cx-6*sc,cy-6*sc,2*sc,2*sc,0xFFFF4040);rect(cx+5*sc,cy+5*sc,2*sc,2*sc,0xFFFF4040);rect(cx-6*sc,cy+5*sc,2*sc,2*sc,0xFFFF4040);rect(cx+5*sc,cy-6*sc,2*sc,2*sc,0xFFFF4040);} }
    char b[64]; int sm=std::max(1,sc-1);
    // ---- شريط النتيجة والوقت (أعلى الوسط) ----
    { int pw=70*sc,ph=15*sc,px=W/2-pw/2,py=3*sc; rrect(px,py,pw,ph,ph/2,0xFF0B0F14,0.78f);
      snprintf(b,64,"%d",score[0]); textC(px+14*sc,py+ph/2-4*sc,b,sc,0xFFE23B3B); snprintf(b,64,"%d",score[1]); textC(px+pw-14*sc,py+ph/2-4*sc,b,sc,0xFF3B82F6);
      int secs=std::max(0,(timeLim>0?timeLim:600)-(int)t); snprintf(b,64,"%d:%02d",secs/60,secs%60); textC(W/2,py+ph/2-4*sc,b,sc,0xFFE8EEF6); }
    // ---- رادار دائري (أعلى اليسار) ----
    int R=30*sc,rcx=8*sc+R,rcy=8*sc+R; float fxw=-sinf(me.yaw),fzw=-cosf(me.yaw),rxw=cosf(me.yaw),rzw=-sinf(me.yaw);
    r.flush(); for(int j=-R;j<=R;j++){ int xo=(int)sqrtf((float)(R*R-j*j)+0.5f); int in2=(R-sc)*(R-sc)-j*j; int xi=in2>0?(int)sqrtf((float)in2):-1; int Y=rcy+j;
      if(xi<0){ blendSpan(Y,rcx-xo,rcx+xo+1,0xFF8FA0B4,0.85f); } else { blendSpan(Y,rcx-xo,rcx-xi,0xFF8FA0B4,0.85f); blendSpan(Y,rcx+xi+1,rcx+xo+1,0xFF8FA0B4,0.85f); blendSpan(Y,rcx-xi,rcx+xi+1,0xFF0B0F14,0.55f); } }
    auto dot=[&](float wx,float wz,uint32_t col,int sz){ float dx=wx-me.b.x,dz=wz-me.b.z; float lr=dx*rxw+dz*rzw,lf=dx*fxw+dz*fzw; float k=(R-2*sc)/(air?700.f:40.f); int X=rcx+(int)(lr*k),Y=rcy-(int)(lf*k); int ddx=X-rcx,ddy=Y-rcy; if(ddx*ddx+ddy*ddy>(R-2*sc)*(R-2*sc))return; rect(X-sz/2,Y-sz/2,sz,sz,col); };
    for(auto&v:vh)dot(v.x,v.z,v.dead?0xFF606060:0xFFFFD23B,3*sc);
    for(auto&o:e){ if(!o.alive||&o==&me)continue; if(o.team!=me.team&&o.revealT<=0&&!air)continue; dot(o.b.x,o.b.z,o.team?0xFF5080FF:0xFFFF5050,2*sc); } // الأعداء لا يظهرون إلا عند إطلاقهم
    rect(rcx-sc,rcy-sc,2*sc,2*sc,0xFFFFD060); rect(rcx-sc/2,rcy-4*sc,std::max(1,sc/2),3*sc,0xFFFFD060);
    // ---- الصحة: شريط مقطّع + صليب ----
    { int bx=8*sc,by=rcy+R+8*sc,bw=62*sc,bh=8*sc; rrect(bx,by,bw+16*sc,bh+4*sc,5*sc,0xFF0B0F14,0.78f); int segs=10,sw2=(bw-4*sc)/segs; float hp=std::max<float>(0.f,me.hp)/100.f;
      for(int k=0;k<segs;k++){ bool on=(k+0.5f)/segs<=hp+0.001f; rect(bx+2*sc+k*sw2,by+2*sc,sw2-sc,bh,on?(hp>0.35f?0xFF3CD070:0xFFE04030):0xFF1E2833); }
      rrect(bx+bw+2*sc,by+sc,12*sc,bh+2*sc,3*sc,0xFFE23B3B); rect(bx+bw+7*sc,by+3*sc,2*sc,bh-sc,0xFFFFFFFF); rect(bx+bw+4*sc,by+bh/2+sc/2,8*sc,2*sc,0xFFFFFFFF);
      snprintf(b,64,"%d",(int)std::max<float>(0.f,me.hp)); text(bx+bw+16*sc,by+bh/2-3*sc,b,sc,0xFFE8EEF6); }
    // ---- أسفل الوسط: الذخيرة أو لوحة الشاص ----
    if(drive>=0&&me.alive){ const VH::Veh&v=vh[drive]; int pw=96*sc,ph=22*sc,px=W/2-pw/2,py=H-ph-6*sc; rrect(px,py,pw,ph,8*sc,0xFF0B0F14,0.8f);
      snprintf(b,64,"%d",(int)(fabsf(v.speed)*3.6f)); textC(W/2-14*sc,py+3*sc,b,sc*2,0xFFFFFFFF); text(W/2+6*sc,py+8*sc,"km/h",sc,0xFF8FA0B4);
      int bw=pw-20*sc; rrect(px+10*sc,py+ph-6*sc,bw,3*sc,sc,0xFF1E2833); rrect(px+10*sc,py+ph-6*sc,std::max<float>(2,(int)(bw*std::max<float>(0.f,v.hp)/VH::vp(v).hp)),3*sc,sc,v.hp>150?0xFF3CD070:0xFFE04030);
      textC(W/2,py-12*sc,"Triangle: نزول",sm,0xFFB8C4D4); if(me.mnt)textC(W/2,py-24*sc,"رشاش الشاص المثبّت - ذخيرة لا نهائية",sm,C_ACC); }
    else if(!dn.on&&myJet()<0){ int pw=124*sc,ph=19*sc,px=W/2-pw/2,py=H-ph-6*sc; rrect(px,py,pw,ph,8*sc,0xFF0B0F14,0.8f);
      text(px+pw-8*sc-tw(d.name,sm),py+ph/2-3*sc,d.name,sm,me.reloadT>=0?0xFFFFC040:C_ACC);
      snprintf(b,64,"%d",me.ammo); int ax=px+12*sc; text(ax,py+ph/2-7*sc,b,sc*2,me.reloadT>=0?0xFFFFC040:0xFFFFFFFF);
      snprintf(b,64,"/ %d",me.mags*cap(me)); text(ax+tw("00",sc*2)+6*sc,py+ph/2-4*sc,b,sc,0xFF8FA0B4);
      { bool mine=false; for(auto&c:c4s)if(c.owner==meI)mine=true; char c1[16],c2[16],c3[16]; snprintf(c1,16,"G x%d",gren); snprintf(c2,16,"C4 x%d",me.c4Left); if(dnCd>0)snprintf(c3,16,"UAV %d",(int)ceilf(dnCd)); else snprintf(c3,16,"UAV");
        const char*cs[3]={c1,c2,c3}; uint32_t cl[3]={gren>0?0xFFB8E0A0:0xFF707070,(me.c4Left>0||mine)?0xFFFFB070:0xFF707070,dnCd>0?0xFF707070:0xFF9CC8FF}; int tot=0; for(int i=0;i<3;i++)tot+=tw(cs[i],sm)+12*sc+3*sc; int x=W/2-tot/2; int y=py-12*sc;
        for(int i=0;i<3;i++){ int w2=tw(cs[i],sm)+12*sc; rrect(x,y,w2,10*sc,5*sc,0xFF0A0E14,0.72f); rect(x+3*sc,y+2*sc,sc,6*sc,cl[i]); text(x+7*sc,y+sc,cs[i],sm,cl[i]); x+=w2+3*sc; } }
      if(me.reloadT>=0)textC(W/2,H/2+20*sc,"جارٍ التعبئة…",sc,0xFFFFC040);
      if(nearVeh)textC(W/2,py-26*sc,"Triangle: ركوب الشاص",sm,0xFFFFD060); }
    for(int k=0;k<4;k++)if(feedT[k]>0){ float fa=std::min(1.f,feedT[k]); int fw=tw(feed[k].c_str(),sm)+12*sc,fx=W-fw-6*sc,fy=(24+k*12)*sc-sc; bool mineK=feed[k].rfind("YOU >",0)==0; rrect(fx,fy,fw,10*sc,5*sc,0xFF0A0E14,0.72f*fa); rect(fx+3*sc,fy+2*sc,sc,6*sc,mineK?C_ACC:0xFF607080,fa); text(fx+7*sc,fy+sc,feed[k].c_str(),sm,mineK?C_ACC:0xFFE8EEF6); }
    // ---- بوصلة أعلى الوسط ----
    { float bd=-me.yaw*57.29578f; float pxd=0.9f*sc; int cw2=(int)(120*pxd),cx2=W/2,cy2=21*sc; rrect(cx2-cw2/2-4*sc,cy2-2*sc,cw2+8*sc,10*sc,5*sc,0xFF0A0E14,0.55f);
      for(int k=(int)floorf((bd-60)/15);k<=(int)ceilf((bd+60)/15);k++){ float a=k*15.f,off=a-bd; int X=cx2+(int)(off*pxd); float fa=1.f-fabsf(off)/62.f; if(fa<=0)continue; int m=((k*15)%360+360)%360; bool big=m%45==0; rect(X,cy2+(big?0:2*sc),sc,big?4*sc:2*sc,0xFFE8EEF6,fa*0.8f);
        if(m%90==0){ const char*L=m==0?"N":m==90?"E":m==180?"S":"W"; textC(X,cy2+3*sc,L,sm,m==0?0xFFE23B3B:0xFFE8EEF6); } }
      rect(cx2,cy2-3*sc,sc,3*sc,C_ACC); }
    // ---- وميض نبض عند انخفاض الصحة ----
    if(me.alive&&me.hp<35.f){ float pa=0.22f+0.14f*sinf(t*6.f); for(int k=0;k<7;k++){ float fa=pa*(1.f-k/7.f); rect(0,k*2*sc,W,2*sc,0xFFB01010,fa); rect(0,H-(k+1)*2*sc,W,2*sc,0xFFB01010,fa); rect(k*2*sc,0,2*sc,H,0xFFB01010,fa*0.8f); rect(W-(k+1)*2*sc,0,2*sc,H,0xFFB01010,fa*0.8f); } }
    if(!menu&&!paused&&me.alive&&myJet()<0){ // علامة الحلفاء (الاسم + سهم) كي ترى صاحبك من بعيد
      for(int i=0;i<(int)e.size();i++){ const Ent&q=e[i]; if(i==meI||!q.alive||q.off||q.team!=me.team)continue; float dx=q.b.x-me.b.x,dz=q.b.z-me.b.z,dd=sqrtf(dx*dx+dz*dz); if(dd>140.f||dd<2.f)continue;
        int X,Y; float zc; if(!projP(q.b.x,q.b.y+2.1f,q.b.z,X,Y,zc)||zc<=0||X<8||X>W-8||Y<8||Y>H-8)continue; uint32_t col=q.bot?0xFF6CB4FF:0xFF5CFF8C; int s2=std::max(1,sc-1); int w2=tw(q.name,s2);
        for(int k=0;k<3;k++)rect(X-(3-k)*s2,Y+k*s2,(3-k)*2*s2,s2,col,0.9f); text(X-w2/2,Y-8*s2,q.name,s2,col); } }
    if(!menu&&!paused&&me.alive&&drive<0&&myJet()<0)weaponPanel(sc);
    if(air&&!menu)airHud(sc);
    if(msgT>0)textC(W/2,H/3,msg,sc*2,0xFFFFFFFF);
    if(fpsShow>0){ snprintf(b,64,"%d fps  %dx%d",(int)fpsShow,W,H); text(4*sc,H-10*sc,b,std::max(1,sc-1),0xFF607080); }
    if(role)text(4*sc,H-20*sc,role==1?"LAN HOST":role==2?"LAN CLIENT":"ANDROID LAN",std::max(1,sc-1),0xFF80E0FF);
    if(over){ rect(0,0,W,H,0xFF000000,0.62f); int cw=W*70/100,ch=H*78/100,cx=(W-cw)/2,cy=(H-ch)/2; card(cx,cy,cw,ch,sc); rect(cx+8*sc,cy,cw/2-8*sc,2*sc,0xFFE23B3B); rect(cx+cw/2,cy,cw/2-8*sc,2*sc,0xFF3B82F6);
      textC(W/2,cy+6*sc,winner==0?"فاز الفريق الأحمر":"فاز الفريق الأزرق",sc*3,winner==0?0xFFFF6060:0xFF6090FF); snprintf(b,64,"%d  -  %d",score[0],score[1]); textC(W/2,cy+30*sc,b,sc*2,C_TXT);
      int bestI=-1,bestK=-1; for(int i=0;i<(int)e.size();i++)if(e[i].kills>bestK){bestK=e[i].kills;bestI=i;}
      for(int tm=0;tm<2;tm++){ int colx=cx+10*sc+tm*(cw/2),colw=cw/2-20*sc,y=cy+52*sc; uint32_t tc=tm?0xFF3B82F6:0xFFE23B3B; text(colx,y,"اللاعب",sm,C_MUT); text(colx+colw*60/100,y,"K",sm,C_MUT); text(colx+colw*80/100,y,"D",sm,C_MUT); y+=11*sc;
        std::vector<int> ix; for(int i=0;i<(int)e.size();i++)if(e[i].team==tm)ix.push_back(i); std::sort(ix.begin(),ix.end(),[&](int a,int c){return e[a].kills>e[c].kills;});
        for(int i:ix){ if(y>cy+ch-26*sc)break; bool me2=i==meI; rrect(colx-2*sc,y-sc,colw,9*sc,3*sc,me2?0xFF1F3340:0xFF121A24,0.9f); rect(colx,y,sc,7*sc,tc); std::string n=nm(i); text(colx+4*sc,y,n.c_str(),sm,me2?C_ACC:C_TXT); if(i==bestI)text(colx+4*sc+tw(n.c_str(),sm)+3*sc,y,"MVP",sm,0xFFFFD23B);
          snprintf(b,64,"%d",e[i].kills); text(colx+colw*60/100,y,b,sm,C_TXT); snprintf(b,64,"%d",e[i].deaths); text(colx+colw*80/100,y,b,sm,C_MUT); y+=10*sc; } }
      textC(W/2,cy+ch-14*sc,role==2?"بانتظار المضيف…":"X = لعب مرة أخرى",sc,C_ACC); }
    hits.clear(); if(paused){ rect(0,0,W,H,0xFF000000,0.62f);
      int cw=W*52/100,ch=H*72/100,cx=(W-cw)/2,cy=(H-ch)/2; card(cx,cy,cw,ch,sc); rect(cx+8*sc,cy,cw/2-8*sc,2*sc,0xFFE23B3B); rect(cx+cw/2,cy,cw/2-8*sc,2*sc,0xFF3B82F6);
      textC(W/2,cy+8*sc,"إيقاف مؤقت",sc*3,C_TXT); { int bw=(cw-40*sc)/2,by2=cy+32*sc; btn(cx+16*sc,by2,bw,15*sc,sc,"متابعة",false,true); hit_(cx+16*sc,by2,bw,15*sc,5); btn(cx+24*sc+bw,by2,bw,15*sc,sc,"القائمة الرئيسية",false,false); hit_(cx+24*sc+bw,by2,bw,15*sc,4); }
      char sb[48]; snprintf(sb,48,T("الحساسية   < %.1f >"),sens); rrect(cx+16*sc,cy+52*sc,cw-32*sc,16*sc,6*sc,C_BTN); textC(W/2,cy+56*sc,sb,sc,C_TXT); hit_(cx+16*sc,cy+52*sc,cw-32*sc,16*sc,6);
      snprintf(sb,48,T("R1: العصي — %s"),swapSticks?T("يسار = نظر"):T("يسار = حركة")); rrect(cx+16*sc,cy+70*sc,cw-32*sc,16*sc,6*sc,C_BTN); textC(W/2,cy+74*sc,sb,sc,0xFF80E0FF); hit_(cx+16*sc,cy+70*sc,cw-32*sc,16*sc,7);
      if(role!=3){ static const char*OA[5]={"حديدي","نقطة حمراء","2x","4x","8x"},*MA[3]={"بدون","كاتم","مخمّد"},*UA[3]={"بدون","مقبض","ليزر"},*GA[3]={"عادي","ممتد","سريع"}; GAtt&a=wAtt[me.wpn&15];
        int by=cy+94*sc; rrect(cx+16*sc,by,cw-32*sc,ch-94*sc-12*sc,8*sc,0xFF0B0F14,0.75f); textC(W/2,by+4*sc,WEAPONS[me.wpn].name,sc*2,C_ACC);
        const char*lb[4]={"مثلث: منظار","مربع: فوهة","L1: مقبض","R2: مخزن"},*vl[4]={OA[a.optic],MA[a.muz],UA[a.under],GA[a.mag]}; int chw=(cw-32*sc-30*sc)/4;
        for(int i=0;i<4;i++){ int x=cx+24*sc+i*(chw+6*sc),y=by+26*sc; hit_(x,y,chw,26*sc,8,i); rrect(x,y,chw,26*sc,6*sc,C_BTN); rrect(x,y,3*sc,26*sc,2*sc,i%2?0xFF3B82F6:0xFFE23B3B); textC(x+chw/2,y+3*sc,lb[i],std::max(1,sc-1),C_MUT); textC(x+chw/2,y+14*sc,vl[i],sc,C_TXT); }
        const WeaponDef&d=WEAPONS[me.wpn]; struct B{const char*n;float v;} bars[3]={{"الضرر",std::min(1.f,d.dmg/110.f)},{"معدل النار",std::min(1.f,d.rpm/1000.f)},{"المدى",std::min(1.f,d.r1/120.f)}};
        for(int i=0;i<3;i++){ int y=by+60*sc+i*9*sc; text(cx+24*sc,y,bars[i].n,std::max(1,sc-1),C_MUT); int bx=cx+cw/3,bw=cw/2; rect(bx,y+2*sc,bw,3*sc,0xFF243040); rect(bx,y+2*sc,(int)(bw*bars[i].v),3*sc,i==0?0xFFE23B3B:i==1?C_ACC:0xFF3B82F6); } } }
  }
  // ---- عناصر واجهة بنمط نسخة أندرويد ----
  static constexpr uint32_t C_PANEL=0xFF141B24,C_LINE=0xFF243040,C_BTN=0xFF1B2430,C_BTNH=0xFF2A3A50,C_TXT=0xFFE8EEF6,C_MUT=0xFF8FA0B4,C_ACC=0xFF35D6FF;
  void rrect(int x,int y,int w2,int h2,int rad,uint32_t c,float a=1){ r.flush(); rad=std::min(rad,std::min(w2,h2)/2);
    for(int j=std::max(0,y);j<std::min(r.H,y+h2);j++){ int cy=std::min(std::max(j,y+rad),y+h2-1-rad); int dy=j-cy;
      if(dy==0){ blendSpan(j,x,x+w2,c,a); continue; }
      blendSpan(j,x+rad,x+w2-rad,c,a);
      for(int side=0;side<2;side++){ int i0=side?x+w2-rad:x,i1=side?x+w2:x+rad,cx=side?x+w2-1-rad:x+rad;
        for(int i=i0;i<i1;i++){ int dx=i-cx; float d=sqrtf((float)(dx*dx+dy*dy)); float cov=d<=rad-0.5f?1.f:d<rad+0.5f?(rad+0.5f-d):0.f; if(cov>0)blendSpan(j,i,i+1,c,a*cov); } } } }
  void card(int x,int y,int w2,int h2,int sc){ rrect(x,y,w2,h2,8*sc,C_LINE); rrect(x+1,y+1,w2-2,h2-2,8*sc-1,C_PANEL,0.96f); }
  void btn(int x,int y,int w2,int h2,int sc,const char*label,bool sel,bool prim){
    if(prim){ rrect(x,y,w2,h2,6*sc,sel?0xFF8CE9FF:C_ACC); for(int j=0;j<h2;j++){ float t=(float)j/h2; rect(x+3*sc,y+j,w2-6*sc,1,0xFF1593B8,t*0.35f); } textC(x+w2/2,y+h2/2-4*sc,label,sc,0xFF04141A); }
    else { rrect(x,y,w2,h2,6*sc,sel?C_ACC:C_LINE); rrect(x+1,y+1,w2-2,h2-2,6*sc-1,sel?C_BTNH:C_BTN); textC(x+w2/2,y+h2/2-4*sc,label,sc,C_TXT); } }
  void menuBg(){ rect(0,0,r.W,r.H,0xFF0B0F14,0.80f); { int W=r.W,H=r.H; for(int j=0;j<H;j+=2){ float ty=(float)j/H; rect(0,j,W/2,2,0xFFE23B3B,0.16f*(1-ty)*(1-ty)); rect(W/2,j,W-W/2,2,0xFF3B82F6,0.16f*ty*ty); } } for(int j=0;j<r.H;j+=2){ float t=fabsf((j-r.H*0.3f)/r.H); rect(0,j,r.W,2,0xFF0B0F14,std::min<float>(0.45f,t*0.6f)); } }
  void titlePanel(int sc,int x,int w2,int y){ int cx=x+w2/2; textC(cx-2*sc,y,"WARZONE",sc*3,0xFFE23B3B); textC(cx+2*sc,y,"WARZONE",sc*3,0xFF3B82F6); textC(cx,y,"WARZONE",sc*3,C_TXT);
    rect(cx-60*sc,y+34*sc,60*sc,3*sc,0xFFE23B3B); rect(cx,y+34*sc,60*sc,3*sc,0xFF3B82F6);
    textC(cx,y+48*sc,"حرب فرق ثلاثية الأبعاد — الأحمر ضد الأزرق",sc,C_MUT); char bb[48]; snprintf(bb,48,"BUILD %d",BUILD_ID); textC(cx,y+64*sc,bb,std::max(1,sc-1),0xFF607080);
    const char*feat[4]={"دبابة ومركبات","ملحقات الأسلحة","مسيّرة استطلاع","لعب شبكي مشترك"}; for(int i=0;i<4;i++){ int pw=tw(feat[i],std::max(1,sc-1))+12*sc,px=cx-pw/2,py=y+80*sc+i*15*sc; rrect(px,py,pw,12*sc,6*sc,0xFF1B2430,0.9f); textC(cx,py+3*sc,feat[i],std::max(1,sc-1),i%2?0xFF8FB8FF:0xFFFF9A8A); } }
  void backBtn(int sc){ int x=4*sc,y=4*sc,w2=40*sc,h2=16*sc; rrect(x,y,w2,h2,6*sc,C_BTN); rrect(x,y,w2,h2,6*sc,C_LINE,0.0f); textC(x+w2/2,y+h2/2-4*sc,"<",sc*2,C_TXT); hit_(x,y,w2,h2,4); }
  void renderMenu(){ SFX::engSpeed.store(-1.f); SFX::droneVol.store(0.f);
    static float mt=0; mt+=0.016f; int showMap=menu==1?menuSel:mapIdx;
    Cam c{0,6,0,mt*0.2f,-0.15f,1.2f}; if(w.id!=showMap)w.load(showMap); r.setCam(c); r.world(w); int W=r.W,H=r.H,sc=std::max(1,(H+135)/270); menuBg();
    char b[96]; int sm=std::max(1,sc-1); hits.clear();

    if(menu==3){ int cw=W*6/10,ch=H/2,cx=(W-cw)/2,cy=(H-ch)/2; card(cx,cy,cw,ch,sc); textC(W/2,cy+ch/2-18*sc,"جارٍ تشغيل الشبكة…",sc*2,C_TXT); snprintf(b,96,"STEP: %s",netWhat); textC(W/2,cy+ch/2+10*sc,b,sc,C_ACC); textC(W/2,cy+ch-16*sc,"O: إلغاء",sm,C_MUT); return; }
    if(menu==4){ // شاشة إعداد الغرفة
      const char*L[8]={"اسم الغرفة","أقصى لاعبين","حد النقاط","الوقت","البوتات","النمط","سلاح البداية","قوة الضرر"}; char v[8][40];
      snprintf(v[0],40,"%s",rmNameOf(rmName)); snprintf(v[1],40,"%d",rmMaxE()); snprintf(v[2],40,"%d",rmLimV(rmLimI)); if(rmMinV(rmMinI))snprintf(v[3],40,"%d min",rmMinV(rmMinI)); else snprintf(v[3],40,"بلا حد");
      if(rmBotsE()==0)snprintf(v[4],40,"بدون بوتات"); else snprintf(v[4],40,"%d",rmBotsE()); snprintf(v[5],40,"%s",rmStyle?"مبارزة 1 ضد 1":"فرق"); snprintf(v[6],40,"%s",rmWpnI>0?WEAPONS[rmWpnI-1].name:"عشوائي"); { static const char*DV[3]={"50%","100%","200%"}; snprintf(v[7],40,"%s",DV[rmDmgI%3]); }
      int rh=17*sc,gap=4*sc,pad=9*sc,cw=W*52/100,ch=pad*2+8*(rh+gap)+rh+12*sc,cx=(W-cw)/2,cy=(H-ch)/2;
      card(cx,cy,cw,ch,sc); rect(cx+8*sc,cy,cw/2-8*sc,2*sc,0xFFE23B3B); rect(cx+cw/2,cy,cw/2-8*sc,2*sc,0xFF3B82F6);
      for(int k=0;k<8;k++){ int y=cy+pad+k*(rh+gap),x=cx+pad,w2=cw-pad*2; bool sel=rmRow==k; hit_(x,y,w2,rh,1,k); uint32_t col=k&1?0xFF3B82F6:0xFFE23B3B; rrect(x,y,w2,rh,6*sc,sel?col:C_LINE); rrect(x+1,y+1,w2-2,rh-2,6*sc-1,sel?C_BTNH:C_BTN);
        int ty=y+rh/2-4*sc; text(x+w2-8*sc-tw(L[k],sc),ty,L[k],sc,C_MUT); textC(x+w2*3/10,ty,v[k],sc,sel?C_TXT:0xFFB8C4D4); if(sel){ text(x+8*sc,ty,"◀",sc,col); text(x+w2*11/20,ty,"▶",sc,col); } }
      btn(cx+pad,cy+pad+8*(rh+gap)+2*sc,cw-pad*2,rh+4*sc,sc,"أنشئ الغرفة وابدأ",false,true); hit_(cx+pad,cy+pad+8*(rh+gap)+2*sc,cw-pad*2,rh+4*sc,2); backBtn(sc);
      textC(W/2,cy+ch+8*sc,"D-pad: اختيار    X: إنشاء    O: رجوع",std::max(1,sc-1),0xFF607080); return; }
    if(menu==2){
      int cw=W*6/10,ch=H*7/10,cx=(W-cw)/2,cy=(H-ch)/2; card(cx,cy,cw,ch,sc); textC(W/2,cy+10*sc,"انضم إلى لعبة شبكة محلية",sc*2,C_TXT);
      if(joining)textC(W/2,H/2,"جارٍ الاتصال…",sc*2,C_ACC);
      else if(hosts.empty())textC(W/2,H/2-6*sc,"جارٍ البحث عن ألعاب على شبكتك…",sc,C_MUT);
      else for(int i=0;i<(int)hosts.size();i++){ snprintf(b,96,"%s  MAP %d  %d/%d  %dK%s%s",hosts[i].name,hosts[i].map+1,hosts[i].players,hosts[i].maxp,hosts[i].lim5*5,hosts[i].mins?"  TIME":"",hosts[i].style?"  DUEL":(hosts[i].bots?"  BOTS":"")); btn(cx+10*sc,cy+40*sc+i*24*sc,cw-20*sc,20*sc,sc,b,i==hostSel,false); hit_(cx+10*sc,cy+40*sc+i*24*sc,cw-20*sc,20*sc,3,i); } backBtn(sc);
      if(msgT>0)textC(W/2,cy+ch-34*sc,msg,sc,0xFFFF6060);
      textC(W/2,cy+ch-16*sc,"D-pad: تحديد    X: انضمام    O: رجوع",sm,C_MUT); return; }
    static const char*MN[9]={"المستودع","أطلال الصحراء","شارع المدينة","القاعدة القطبية","صحراء الشاص","أهرامات الجيزة","المدينة القديمة","ميناء الإسكندرية","قاعدة جوية F-35"};
    const char*dn[3]={"سهل","عادي","صعب"},*md[6]={"ضد البوتات","استضافة LAN","انضمام LAN","لاعبان: تعاون ضد البوتات","لاعبان: ضد بعض","لاعبان: مبارزة (بدون بوتات)"};
    const char*go[6]={"العب منفردًا ضد البوتات","استضف لعبة شبكة محلية","انضم إلى لعبة شبكة محلية","ابدأ لاعبين على نفس الجهاز (تعاون)","ابدأ لاعبين على نفس الجهاز (ضد بعض)","ابدأ مبارزة: أنت وصاحبك فقط بدون بوتات"};
    int rh=18*sc,gap=4*sc,pad=10*sc,cw=W*45/100,ch=pad*2+5*(rh+gap)+rh+8*sc+6*sc+10*sc,cx=W/16,cy=(H-ch)/2+4*sc;
    card(cx,cy,cw,ch,sc); rect(cx+8*sc,cy,cw/2-8*sc,2*sc,0xFFE23B3B); rect(cx+cw/2,cy,cw/2-8*sc,2*sc,0xFF3B82F6);
    auto sel_=[&](int i,const char*label,const char*val,uint32_t col){ int y=cy+pad+i*(rh+gap),x=cx+pad,w2=cw-pad*2; hit_(x,y,w2,rh,1,i); bool sel=menuRow==i; rrect(x,y,w2,rh,6*sc,sel?col:C_LINE); rrect(x+1,y+1,w2-2,rh-2,6*sc-1,sel?C_BTNH:C_BTN);
      int ty=y+rh/2-4*sc; text(x+w2-8*sc-tw(label,sc),ty,label,sc,C_MUT); textC(x+w2*3/10,ty,val,sc,sel?C_TXT:0xFFB8C4D4); if(sel){ text(x+8*sc,ty,"◀",sc,col); text(x+w2*11/20,ty,"▶",sc,col); } };
    sel_(0,"الخريطة",MN[menuSel%9],0xFFE23B3B); sel_(1,"الصعوبة",dn[diff],C_ACC); sel_(2,"الوضع",md[mode],0xFF3B82F6);
    static const char*QN[5]={"ضعيفة جداً","ضعيفة","متوسطة","عالية","فائقة"},*LN[3]={"العربية","English","Español"};
    sel_(3,"الجودة",QN[g_quality%5],0xFF3DDC84); sel_(4,"اللغة",LN[g_lang%3],0xFFB07CFF);
    btn(cx+pad,cy+pad+5*(rh+gap)+4*sc,cw-pad*2,rh+4*sc,sc,go[mode],menuRow==5,true); hit_(cx+pad,cy+pad+5*(rh+gap)+4*sc,cw-pad*2,rh+4*sc,2);
    if(msgT>0)textC(cx+cw/2,cy+ch-14*sc,msg,sm,0xFFFF6060);
    titlePanel(sc,W*55/100,W*38/100,H/2-72*sc);
    textC(W/2,H-14*sc,"D-pad: اختيار    X: تأكيد    Options: إيقاف مؤقت",std::max(1,sc-1),0xFF607080);
  }
};
#include "air.h"
