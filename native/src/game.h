// لعبة Warzone الأصلية: لاعب + بوتات + أسلحة + تسجيل نقاط + واجهة
#pragma once
#include "render.h"
#include "font.h"
#include "net.h"
#include <string>
#include <cstdlib>

#ifndef BUILD_ID
#define BUILD_ID 0
#endif
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
  char name[16]={0}; bool remote=false; uint8_t spawnSeq=0; float tx=0,ty=0,tz=0,tyaw=0; int lastAmmo=0; int lastHp=100; NetInput rin{}; float lastPkt=0; int lastKills=0;
};
struct Tracer{float x0,y0,z0,x1,y1,z1,t;};

struct Game{
  World w; Renderer r; std::vector<Ent> e; std::vector<Tracer> tr;
  int mapIdx=0,scoreLim=30,score[2]={0,0},nBots=6,diff=0; float sens=1.0f; bool swapTrig=false,swapL=false,swapSticks=false; float t=0,hitMark=0,dmgFlash=0; bool over=false; int winner=0;
  int prim=0; Input prev; bool paused=false; float msgT=0; char msg[64]={0}; int shotsFeed[4]={-1,-1,-1,-1}; std::string feed[4]; float feedT[4]={};
  int netGoal=0,netSt=0,netPhase=0; const char*netWhat=""; sockaddr_in pendJoin;
  int menu=1,menuSel=0,menuRow=0,mode=0; // menu: 1 رئيسية ، 2 ردهة الانضمام
  // ---- شبكة ----
  int role=0,meI=0; uint8_t matchId=0,spawnCtr=0,evtId=0,lastEvt=0; std::vector<NetEvt> evq; uint16_t netSeq=0;
  Udp sock,beaconRx,beaconTx; float snapT=0,inT=0,beaconT=0,srvSilence=0,joinT=0; uint32_t myIp=0; char ipTxt[24]={0}; bool joining=false; sockaddr_in hostAddr; Input lastIn;
  std::vector<NetTr> trOut; struct Cli{bool on=false;sockaddr_in a;int ent=-1;float last=0;}; Cli cli[8];
  struct HostEnt{sockaddr_in a;uint8_t map,players,maxp;char name[16];float seen;}; std::vector<HostEnt> hosts; int hostSel=0;
  std::string nm(int i)const{ char b[16]; if(i==meI)return "YOU"; if(i==0)return "HOST"; if(i<(int)e.size()&&e[i].remote)snprintf(b,16,"P%d",i); else snprintf(b,16,"BOT%d",i); return b; }
  void init(int rw,int rh){ r.init(rw,rh); startMatch(mapIdx); }
  void say(const char*s){snprintf(msg,64,"%s",s);msgT=2.5f;}
  void addFeed(const std::string&s){for(int i=3;i>0;i--){feed[i]=feed[i-1];feedT[i]=feedT[i-1];}feed[0]=s;feedT[0]=5;}
  void startMatch(int idx){
    mapIdx=idx%MAP_COUNT; w.load(mapIdx); e.assign(1+nBots*2+ (0),Ent()); // لاعب + فريقان
    int total=1+nBots*2-1; e.assign(total,Ent());
    for(int i=0;i<total;i++){Ent&p=e[i];p.bot=i>0;p.team=(i==0)?0:((i&1)?1:0);snprintf(p.name,16,"%s",nm(i).c_str());p.wpn=i==0?prim:rand()%WEAPON_COUNT;}
    if(role==1)for(auto&c:cli)if(c.on&&c.ent>0&&c.ent<total){ e[c.ent].remote=true; e[c.ent].bot=false; }
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
    p.yaw=std::atan2(-(0-p.b.x),-(0-p.b.z)); p.alive=true;p.hp=100;p.prot=2;p.crouch=false;p.spawnSeq=++spawnCtr;giveWeapon(p,(p.bot||p.remote)?p.wpn:prim);
  }
  static float dmgAt(const WeaponDef&d,float dist){ if(dist<=d.r0)return d.dmg; if(dist>=d.r1)return d.dmg*d.minMul; return d.dmg*(1-(1-d.minMul)*(dist-d.r0)/(d.r1-d.r0)); }
  bool visible(const Ent&a,const Ent&b2){
    float ax=a.b.x,ay=a.b.y+PK.eye,az=a.b.z,bx=b2.b.x,by=b2.b.y+1.2f,bz=b2.b.z;
    float dx=bx-ax,dy=by-ay,dz=bz-az,d=std::sqrt(dx*dx+dy*dy+dz*dz); if(d<0.01f)return true;
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
  void fire(int i){
    Ent&p=e[i]; const WeaponDef&d=WEAPONS[p.wpn];
    p.ammo--; p.cd=60.f/d.rpm; p.flash=0.06f; p.prot=0;
    float moveF=p.moving?1.6f:1.f; bool ads=(i==meI)&&adsHeld;
    float sp=(ads?d.ads:d.hip)*moveF; if(p.crouch)sp*=0.7f;
    float eyeY=p.b.y+(p.crouch?PK.eyec:PK.eye);
    for(int k=0;k<d.pellets;k++){
      float a=sp; if(p.bot){a=(0.022f+0.05f*0.5f)*(diff==0?3.2f:diff==1?1.5f:0.8f);}
      float yw=p.yaw+(frand()-0.5f)*2*a, pt=p.pitch+(frand()-0.5f)*2*a; if(d.pellets>1){yw+=(frand()-0.5f)*0.09f;pt+=(frand()-0.5f)*0.09f;}
      float cp=std::cos(pt); float dx=-std::sin(yw)*cp,dy=std::sin(pt),dz=-std::cos(yw)*cp;
      float tw=raycastMap(w,p.b.x,eyeY,p.b.z,dx,dy,dz,d.r1*1.4f); float best=std::min(tw,d.r1*1.4f); int hv=-1; bool hh=false;
      for(int j=0;j<(int)e.size();j++){ if(j==i||!e[j].alive||e[j].team==p.team)continue;
        RayHit h=rayPlayer(e[j].b.x,e[j].b.y,e[j].b.z,e[j].crouch,p.b.x,eyeY,p.b.z,dx,dy,dz,best); if(h.ok&&h.t<best){best=h.t;hv=j;hh=h.head;} }
      if(hv>=0)hurt(hv,i,dmgAt(d,best)*(hh?d.head:1.f),hh);
      if(tr.size()<64)tr.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best,0.07f}); if(role==1&&trOut.size()<10)trOut.push_back({p.b.x+dx*0.5f,eyeY-0.12f,p.b.z+dz*0.5f,p.b.x+dx*best,eyeY+dy*best,p.b.z+dz*best});
    }
    if(i==meI&&role!=2){ p.rkP+=d.recoilV*(ads?0.6f:1.f)*1.6f; p.rkY+=(frand()-0.5f)*d.recoilH*3.f; }
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
      if(best!=p.target){p.react=(diff==0?1.1f:diff==1?0.6f:0.3f);} p.target=best; p.sx=vis?1.f:0.f; p.sz=std::sqrt(bd);
    }
  }
  void updateBot(int i,float dt){
    Ent&p=e[i]; botThink(i,dt); p.react-=dt; p.strafeT-=dt; if(p.strafeT<=0){p.strafeT=0.8f+frand()*1.5f;p.strafe=frand()<0.5f?-1.f:1.f;}
    float mx=0,mz=0; bool trig=false; p.sprint=false;
    if(p.target>=0&&e[p.target].alive){ Ent&t2=e[p.target];
      float dx=t2.b.x-p.b.x,dz=t2.b.z-p.b.z,d=std::sqrt(dx*dx+dz*dz); float want=std::atan2(-dx,-dz);
      float er=WEAPONS[p.wpn].r0>1?(p.wpn==2?9.f:p.wpn==1?14.f:22.f):22.f; bool vis=p.sx>0.5f;
      float dy=want-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*std::min(1.f,dt*(vis?(diff==0?3.5f:diff==1?6.f:10.f):3.f));
      // ارتفاع الهدف
      float pitchW=std::atan2((t2.b.y+1.3f)-(p.b.y+PK.eye),std::max(1.f,d)); p.pitch+=(pitchW-p.pitch)*std::min(1.f,dt*8);
      if(vis&&d<er){ // اشتباك: تحرّك جانبياً
        mx=p.strafe*0.6f; mz=d<er*0.4f?0.4f:(d>er*0.8f?-0.4f:0);
        if(p.react<=0&&std::fabs(dy)<0.25f&&d<WEAPONS[p.wpn].r1*1.1f){ p.burst-=dt; if(p.burst<=-0.35f)p.burst=0.35f+frand()*0.5f; trig=p.burst>0; }
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
    Ent&p=e[meI]; adsHeld=in.ads;
    float zoom=in.ads?WEAPONS[p.wpn].zoom:1.f; float sens=1.f/ (in.ads?std::max(1.f,zoom*0.8f):1.f);
    p.yaw-=in.lx*dt*sens*this->sens; p.pitch+=in.ly*dt*sens*this->sens; p.yaw+=p.rkY*std::min(1.f,dt*30); p.pitch+=p.rkP*std::min(1.f,dt*30); p.rkP*=0.8f; p.rkY*=0.8f;
    p.pitch=std::max(-1.45f,std::min(1.45f,p.pitch));
    bool wantCrouch=in.crouch; if(!wantCrouch&&p.crouch&&blocked(w,p.b.x,p.b.y,p.b.z,PK.h))wantCrouch=true; p.crouch=wantCrouch;
    float mx=in.mx,mz=in.my; float ml=std::hypot(mx,mz); if(ml>1){mx/=ml;mz/=ml;}
    p.sprint=in.sprint&&mz>0.3f&&!in.ads&&!p.crouch;
    float sp=(p.crouch?PK.crouch:(p.sprint?PK.sprint:PK.walk))*WEAPONS[p.wpn].move*(in.ads?0.65f:1.f);
    float fxw=-std::sin(p.yaw),fzw=-std::cos(p.yaw),rxw=std::cos(p.yaw),rzw=-std::sin(p.yaw);
    float vx=(mz*fxw+mx*rxw)*sp,vz=(mz*fzw+mx*rzw)*sp; float ox=p.b.x,oz=p.b.z;
    moveBody(w,p.b,vx,vz,dt,in.jump,p.crouch); float mv=std::hypot(p.b.x-ox,p.b.z-oz); p.moving=mv>0.001f&&p.b.onGround; p.phase+=mv*3.4f;
    if(in.reload&&role!=2)reloadStart(p);
    if((in.swapP&&!prev.swapP)||(in.swapN&&!prev.swapN)){ prim=(prim+(in.swapN&&!prev.swapN?1:WEAPON_COUNT-1))%WEAPON_COUNT; giveWeapon(p,prim); }
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
    for(int k=0;k<h.nTr;k++){ NetTr q; memcpy(&q,pk.d+off,sizeof q); off+=sizeof q; tr.push_back({q.x0,q.y0,q.z0,q.x1,q.y1,q.z1,0.07f}); }
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
    if(menu==1){
      bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f,rt=in.mx>0.5f&&prev.mx<=0.5f,lf=in.mx<-0.5f&&prev.mx>=-0.5f;
      if(up)menuRow=(menuRow+2)%3; if(dn)menuRow=(menuRow+1)%3; int d=rt?1:lf?-1:0;
      if(d){ if(menuRow==0)menuSel=(menuSel+d+MAP_COUNT)%MAP_COUNT; else if(menuRow==1)diff=(diff+d+3)%3; else mode=(mode+d+3)%3; }
      if(msgT>0)msgT-=dt;
      if((in.confirm&&!prev.confirm)||(in.start&&!prev.start)){ if(mode==0){menu=0;role=0;meI=0;startMatch(menuSel);} else if(mode==1)hostStart(); else lobbyStart(); }
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
    if(role==2){ // عميل: حركة محلية للاعب فقط + تنعيم الآخرين
      for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(i==meI){ if(p.alive)updatePlayer(dt,use); continue; }
        float k=std::min(1.f,dt*16.f); p.b.x+=(p.tx-p.b.x)*k;p.b.y+=(p.ty-p.b.y)*k;p.b.z+=(p.tz-p.b.z)*k; float dy=p.tyaw-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*k; if(p.flash>0)p.flash-=dt; }
      prev=in; return; }
    for(int i=0;i<(int)e.size();i++){ Ent&p=e[i];
      if(p.prot>0)p.prot-=dt;
      if(!p.alive){ p.respawn-=dt; if(p.respawn<=0){spawn(p);} continue; }
      if(i==meI)updatePlayer(dt,use); else if(p.remote)updateRemote(i,dt); else updateBot(i,dt);
    }
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
    float x=a.b.x,y=a.b.y,z=a.b.z,yw=a.yaw; float cs=a.crouch?0.69f:1.f;
    if(!a.alive){ float tk=0.5f; r.obb(x,y+0.18f,z,0.25f,0.15f,0.9f,yw,0,c[0]*0.5f*tk*2,c[1]*0.5f*tk*2,c[2]*0.5f*tk*2); return; }
    auto B=[&](float lx,float ly,float lz,float hx,float hy,float hz,float rx_,const float*col){float o[3];Renderer::rot(yw,0,lx,ly*cs,lz,o);r.obb(x+o[0],y+o[1],z+o[2],hx*.5f,hy*.5f*cs,hz*.5f,yw,rx_,col[0],col[1],col[2]);};
    static const float pants[3]={.17f,.18f,.15f},boot[3]={.08f,.07f,.07f},vest[3]={.2f,.22f,.2f},plate[3]={.26f,.28f,.25f},pocket[3]={.16f,.17f,.15f},pack[3]={.18f,.2f,.16f},belt[3]={.1f,.1f,.11f},skin[3]={.86f,.68f,.55f},glove[3]={.1f,.1f,.11f},visor[3]={.05f,.06f,.08f},mask[3]={.12f,.12f,.12f};
    if(camDist>38.f){ B(0,.42f,0,.4f,.84f,.24f,0,pants);B(0,1.1f,0,.5f,.6f,.3f,0,c);B(0,1.58f,0,.24f,.26f,.24f,0,d); return; }
    float sw=std::sin(a.phase)*0.75f*(a.moving?1:0);
    auto pv=[&](float px,float py,float pz,float rxr,float ox,float oy,float oz,float*o3){float cx=std::cos(rxr),sx=std::sin(rxr);o3[0]=px+ox;o3[1]=py+oy*cx-oz*sx;o3[2]=pz+oy*sx+oz*cx;};
    for(int s2=0;s2<2;s2++){ float lx=s2?0.12f:-0.12f,rot=s2?-sw:sw; float o[3];
      pv(lx,0.8f,0,rot,0,-0.3f,0,o);B(o[0],o[1],o[2],0.2f,0.62f,0.22f,rot,pants);
      pv(lx,0.8f,0,rot,0,-0.74f,-0.02f,o);B(o[0],o[1],o[2],0.21f,0.16f,0.3f,rot,boot); }
    B(0,1.1f,0,.44f,.58f,.25f,0,c); B(0,1.13f,0.015f,.5f,.46f,.29f,0,vest); B(0,1.22f,-0.15f,.36f,.2f,.05f,0,plate);
    for(float px:{-0.14f,0.f,0.14f})B(px,0.98f,-0.16f,.1f,.12f,.06f,0,pocket);
    B(0,1.0f,0.2f,.36f,.42f,.14f,0,pack); B(0,0.92f,0,.5f,.1f,.3f,0,belt); B(0,1.44f,0,.11f,.1f,.11f,0,skin);
    // الرأس (يميل مع النظر)
    { float hp=a.pitch*0.5f; auto H=[&](float ox,float oy,float oz,float sx,float sy,float sz,const float*col){float o[3];pv(0,1.58f,0,hp,ox,oy,oz,o);B(o[0],o[1],o[2],sx,sy,sz,hp,col);};
      H(0,0,0,.21f,.23f,.22f,skin); float hel[3]={d[0]*.9f,d[1]*.9f,d[2]*.9f}; H(0,0.08f,0.01f,.34f,.2f,.35f,hel); H(0,0.02f,-0.115f,.23f,.07f,.03f,visor); H(0,-0.1f,-0.08f,.18f,.05f,.08f,mask); }
    // الذراعان والسلاح
    float ap=a.sprint?0.2f:a.pitch; float base=a.sprint?0.9f:1.5708f;
    auto arm=[&](float ax,float rot){ float len=0.54f,o[3]; pv(ax,1.36f,0,rot,0,-len/2,0,o); B(o[0],o[1],o[2],.12f,len,.12f,rot,d); pv(ax,1.36f,0,rot,0,-len-0.02f,0,o); B(o[0],o[1],o[2],.1f,.1f,.1f,rot,glove); };
    arm(0.27f,base+ap-0.12f); arm(-0.27f,base+ap+0.12f);
    float go[3]; pv(0.12f,1.3f,0,ap,0,-0.05f,-0.38f,go); float wo[3]; Renderer::rot(yw,0,go[0],go[1]*cs,go[2],wo);
    drawGunAt(x+wo[0],y+wo[1],z+wo[2],yw,ap,a.wpn,a.flash>0?1.f:0.f,true,1.f);
  }
  void render(){
    Ent&me=e[meI]; float eyeH=me.alive?(me.b.y+(me.crouch?PK.eyec:PK.eye)):me.b.y+0.6f;
    float fov=1.3f; if(adsHeld&&me.alive){fov=1.3f/WEAPONS[me.wpn].zoom;}
    Cam c{me.b.x,eyeH,me.b.z,me.yaw,me.pitch,fov}; r.setCam(c); r.world(w);
    for(int i=0;i<(int)e.size();i++){if(i==meI)continue;const Ent&a=e[i];float dx=a.b.x-c.x,dz=a.b.z-c.z; if(dx*dx+dz*dz>r.fogF*r.fogF||dx*dx+dz*dz<0.5f)continue; drawAvatar(a,std::sqrt(dx*dx+dz*dz));}
    for(auto&q:tr){ float dx=q.x1-q.x0,dy=q.y1-q.y0,dz=q.z1-q.z0,l=std::sqrt(dx*dx+dy*dy+dz*dz); if(l<0.1f)continue;
      float yw=std::atan2(-dx,-dz),pt=std::asin(dy/l); float cx=(q.x0+q.x1)/2,cy=(q.y0+q.y1)/2,cz=(q.z0+q.z1)/2; r.obb(cx,cy,cz,0.012f,0.012f,std::min(l,6.f)/2,yw,pt,1,0.9f,0.5f,false); }
    // مجسّم السلاح (نفس قطع نسخة أندرويد)
    if(me.alive){ std::fill(r.zb.begin(),r.zb.end(),0.f); const GunDef&g=GUNDEFS[me.wpn]; float k=adsHeld?1.f:0.f; float top=g.sy;
      static float bob=0; bob+=me.moving?(me.sprint?0.17f:0.11f):0; float bx=std::sin(bob)*0.008f*(me.moving?1:0),by=std::fabs(std::cos(bob))*0.008f*(me.moving?1:0);
      float x=0.17f*(1-k)+0*k+bx,y=-0.17f*(1-k)+(-top-0.002f)*k+by,z=-0.33f*(1-k)-0.36f*k; float kick=me.flash>0?0.045f:0.f; z+=kick; float rx=kick*1.2f,ry=0.f;
      if(me.reloadT>=0){float sn=std::sin(me.reloadT/WEAPONS[me.wpn].reload*3.14159f);y-=sn*0.13f;rx-=sn*0.5f;x+=sn*0.03f;}
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
  void rect(int x,int y,int w2,int h2,uint32_t c,float a=1){ for(int j=std::max(0,y);j<std::min(r.H,y+h2);j++)for(int i=std::max(0,x);i<std::min(r.W,x+w2);i++){ if(a>=1)r.fb[j*r.W+i]=c; else { uint32_t o=r.fb[j*r.W+i]; auto m=[&](int sh){int A=(o>>sh)&255,B=(c>>sh)&255;return (uint32_t)(A+(B-A)*a)<<sh;}; r.fb[j*r.W+i]=0xFF000000u|m(16)|m(8)|m(0);} } }
  void text(int x,int y,const char*s,int sc,uint32_t col){ int cx=x; for(;*s;s++){ if(*s=='\n'){cx=x;y+=8*sc;continue;} const uint8_t*g=glyphOf(*s); if(g)for(int j=0;j<7;j++)for(int i=0;i<5;i++)if(g[j]&(16>>i))rect(cx+i*sc,y+j*sc,sc,sc,col); cx+=6*sc; } }
  int tw(const char*s,int sc){return (int)strlen(s)*6*sc;}
  void textC(int cx,int y,const char*s,int sc,uint32_t col){text(cx-tw(s,sc)/2,y,s,sc,col);}
  void hud(){
    int W=r.W,H=r.H,sc=std::max(1,H/270); Ent&me=e[meI]; const WeaponDef&d=WEAPONS[me.wpn];
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
    if(role)text(6*sc,H-30*sc,role==1?"LAN HOST":"LAN CLIENT",sc,0xFF80E0FF);
    if(over){ rect(0,0,W,H,0xFF000000,0.55f); textC(W/2,H/2-20*sc,winner==0?"RED TEAM WINS":"BLUE TEAM WINS",sc*3,winner==0?0xFFFF6060:0xFF6090FF); textC(W/2,H/2+10*sc,role==2?"WAITING FOR HOST":"X = PLAY AGAIN",sc*2,0xFFFFFFFF); }
    if(paused){ rect(0,0,W,H,0xFF000000,0.6f); textC(W/2,H/2-12*sc,"PAUSED",sc*3,0xFFFFFFFF); textC(W/2,H/2+14*sc,"OPTIONS = RESUME   O = MENU",sc*2,0xFFB0B0B0); char sb[40];snprintf(sb,40,"SENSITIVITY < %.1f >",sens);textC(W/2,H/2+34*sc,sb,sc*2,0xFFFFFFFF); {char sw[40];snprintf(sw,40,"R1: STICKS %s",swapSticks?"LEFT=LOOK RIGHT=MOVE":"LEFT=MOVE RIGHT=LOOK");textC(W/2,H/2+54*sc,sw,sc,0xFF80E0FF);} }
  }
  void renderMenu(){
    static float mt=0; mt+=0.016f; int showMap=menu==1?menuSel:mapIdx;
    Cam c{0,6,0,mt*0.2f,-0.15f,1.2f}; if(w.id!=showMap)w.load(showMap); r.setCam(c); r.world(w); int W=r.W,H=r.H,sc=std::max(1,H/270); rect(0,0,W,H,0xFF000000,0.5f); rect(0,0,W,H/6,0xFF000000,0.35f); rect(0,H-H/6,W,H/6,0xFF000000,0.35f);
    textC(W/2+sc*2,H/9+sc*2,"WARZONE",sc*6,0xFF300800); textC(W/2,H/9,"WARZONE",sc*6,0xFFFF5040); rect(W/2-tw("WARZONE",sc*6)/2,H/9+46*sc,tw("WARZONE",sc*6),2*sc,0xFFFF5040); {char bb[40];snprintf(bb,40,"NATIVE PS4 EDITION   BUILD %d",BUILD_ID);textC(W/2,H/9+52*sc,bb,sc*2,0xFFC0C8D0);}
    char b[96];
    if(menu==3){ textC(W/2,H/2-10*sc,"STARTING NETWORK...",sc*3,0xFFFFFFFF); snprintf(b,96,"STEP: %s",netWhat); textC(W/2,H/2+20*sc,b,sc*2,0xFFFFD060); textC(W/2,H/2+44*sc,"O = CANCEL",sc*2,0xFFB0B0B0); return; }
    if(menu==2){
      textC(W/2,H/3,"JOIN LAN GAME",sc*3,0xFFFFFFFF);
      if(joining)textC(W/2,H/2,"CONNECTING...",sc*3,0xFFFFD060);
      else if(hosts.empty())textC(W/2,H/2,"SEARCHING FOR HOSTS...",sc*2,0xFFB0B0B0);
      else for(int i=0;i<(int)hosts.size();i++){ snprintf(b,96,"%s %s  MAP %d  PLAYERS %d/%d",i==hostSel?">":" ",hosts[i].name,hosts[i].map+1,hosts[i].players,hosts[i].maxp); textC(W/2,H/2+i*12*sc,b,sc*2,i==hostSel?0xFFFFFFFF:0xFF909090); }
      if(msgT>0)textC(W/2,H*3/4,msg,sc*2,0xFFFF6060);
      textC(W/2,H-20*sc,"UP/DOWN = SELECT   X = JOIN   O = BACK",sc*2,0xFFB0B0B0); return; }
    static const char*MN[5]={"WAREHOUSE","DESERT RUINS","CITY STREET","POLAR BASE","SHAS DESERT"};
    const char*dn[3]={"EASY","NORMAL","HARD"},*md[3]={"SOLO VS BOTS","HOST LAN GAME","JOIN LAN GAME"};
    int bw=W*6/10,bx=(W-bw)/2,rh=22*sc,gap=6*sc,y0=H*4/10; uint32_t acc[3]={0xFFFF6040,0xFFFFC040,0xFF50E080};
    auto row=[&](int i,const char*label,const char*val,uint32_t col){ int y=y0+i*(rh+gap); bool sel=menuRow==i; rect(bx,y,bw,rh,sel?0xFF202830:0xFF101418,sel?0.85f:0.55f); rect(bx,y,4*sc,rh,sel?col:0xFF404850);
      text(bx+12*sc,y+rh/2-7*sc,label,sc*2,sel?0xFFFFFFFF:0xFF909090); char v[48]; snprintf(v,48,"%s %s %s",sel?"<":" ",val,sel?">":" "); text(bx+bw-tw(v,sc*2)-10*sc,y+rh/2-7*sc,v,sc*2,sel?col:0xFF808890); };
    char v[48]; snprintf(v,48,"%d  %s",menuSel+1,MN[menuSel%5]);
    row(0,"MAP",v,acc[0]); row(1,"DIFFICULTY",dn[diff],acc[1]); row(2,"MODE",md[mode],acc[2]);
    { int y=y0+3*(rh+gap)+4*sc; rect(bx,y,bw,rh,0xFF1C6030,0.95f); rect(bx,y,4*sc,rh,0xFF50E080); textC(W/2,y+rh/2-7*sc,mode==0?"X  START GAME":mode==1?"X  HOST GAME":"X  FIND GAMES",sc*2,0xFFFFFFFF); }
    if(msgT>0)textC(W/2,y0-16*sc,msg,sc*2,0xFFFF6060);
    textC(W/2,H-30*sc,"DPAD: CHOOSE    X: CONFIRM    OPTIONS: PAUSE    R1: SWAP STICKS",sc,0xFFB0B8C0);
    textC(W/2,H-20*sc,"R2 FIRE   L2 AIM   X JUMP   O CROUCH   SQ RELOAD   L1 R1 WEAPON",sc,0xFF808890);
  }
};
