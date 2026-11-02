// عميل نسخة أندرويد: ينضم إلى لعبة يستضيفها الهاتف (WebSocket + JSON) بنفس بروتوكول shared/game.js
#pragma once
#include <map>

struct Game::ACl{
  WsClient ws; std::shared_ptr<ScanState> scan; std::vector<ScanHit> hits; int sel=0; int ip[4]={192,168,1,100}; int ipSel=3; bool ipEdit=false; std::string ipTxt; int pk=0;
  int myId=0,hostId=0,epoch=0,cfgMap=0; std::string phase="lobby"; bool welcomed=false; float stT=0,pingT=0,connT=0; std::string err; uint32_t hostIp=0; int hostPort=3000;
  struct LP{int id=0;std::string n;int tm=0,b=0;std::string p,s;}; std::vector<LP> lobby;
  std::map<int,int> idx; std::string loP="carbine",loS="pistol"; int slot=0; int slotAm[2][2]={{-1,0},{-1,0}};
  struct Gr{float x,y,z;}; std::vector<Gr> gr; struct Vh{float x,z,h;int hp,fl;}; std::vector<Vh> vs;
  int att[2][4]={{0,0,0,0},{0,0,0,0}}; bool inMatch=false; int lastEvtCnt=0; int seat=-1; float vT=0,crMax=0; bool pu=false;
};
static int weaponIdx(const std::string&id){ for(int i=0;i<WEAPON_COUNT;i++)if(id==WEAPONS[i].id)return i; return 0; }
static std::string cleanName(const std::string&n,int id,bool bot){ bool ok=!n.empty(); for(unsigned char c:n)if(c<32||c>126)ok=false; if(ok)return n.substr(0,12); char b[16]; snprintf(b,16,"%s%d",bot?"BOT":"P",id); return b; }

static const char*OPT_ID[8][5]={{"iron","red","x2","x4",0},{"iron","red","x2",0,0},{"iron","red",0,0,0},{"x4","x8",0,0,0},{"iron","red","x2","x4",0},{"red","x2","x4","x8",0},{"iron","red",0,0,0},{"iron","red",0,0,0}};
static const char*OPT_AR[5]={"حديدي","نقطة حمراء","2x","4x","8x"}; static const char*MUZ_ID[3]={"none","supp","comp"},*MUZ_AR[3]={"بدون","كاتم","مخمّد"}; static const char*UND_ID[3]={"none","grip","laser"},*UND_AR[3]={"بدون","مقبض","ليزر"}; static const char*MAG_ID[3]={"std","ext","fast"},*MAG_AR[3]={"عادي","ممتد","سريع"};
static int wIdx(const std::string&id){ static const char*N[8]={"carbine","smg","shotgun","sniper","lmg","dmr","pistol","magnum"}; for(int i=0;i<8;i++)if(id==N[i])return i; return 0; }
static int nOpt(int w){ int n=0; while(n<5&&OPT_ID[w][n])n++; return n; }
static std::string optName(int w,int k){ const char*id=OPT_ID[w][k]; return id; }
static const char*optAr(int w,int k){ const char*id=OPT_ID[w][k]; if(!strcmp(id,"iron"))return OPT_AR[0]; if(!strcmp(id,"red"))return OPT_AR[1]; if(!strcmp(id,"x2"))return OPT_AR[2]; if(!strcmp(id,"x4"))return OPT_AR[3]; return OPT_AR[4]; }
static std::string loJson(Game::ACl&A){ std::string o="{\"t\":\"lo\",\"lo\":{\"p\":\""+A.loP+"\",\"s\":\""+A.loS+"\",\"a\":["; for(int q=0;q<2;q++){ int w=wIdx(q?A.loS:A.loP); o+=std::string(q?",":"")+"{\"optic\":\""+optName(w,A.att[q][0])+"\",\"muzzle\":\""+MUZ_ID[A.att[q][1]]+"\",\"under\":\""+UND_ID[A.att[q][2]]+"\",\"mag\":\""+MAG_ID[A.att[q][3]]+"\"}"; } return o+"]}}"; }
static void applyAtt(Game&g,Ent&p,int slot){ { int w=wIdx(slot?g.ac->loS:g.ac->loP); const char*o=OPT_ID[w][g.ac->att[slot][0]]; g.optZ=!strcmp(o,"red")?1.5f:!strcmp(o,"x2")?2.2f:!strcmp(o,"x4")?3.6f:!strcmp(o,"x8")?6.f:0.f; } int m=g.ac->att[slot][3]; p.magM=m==1?1.5f:1.f; p.relM=m==1?1.12f:m==2?0.72f:1.f; p.ammo=g.cap(p); }
void Game::aclStart(uint32_t ip,int port){
  if(ac){ac->ws.close();delete ac;} ac=new ACl(); role=3; meI=0; e.clear(); over=false;
  if(ip){ ac->hostIp=ip; ac->hostPort=port; ac->ws.connectTo(ip,port); menu=7; }
  else { ac->scan=std::make_shared<ScanState>(); scanStart(ac->scan); menu=5; }
}
void Game::aclStop(){ if(ac){ ac->ws.close(); if(ac->scan)ac->scan->stop=true; delete ac; ac=nullptr; } role=0; meI=0; e.assign(1,Ent()); menu=1; paused=false; mapIdx=std::min(mapIdx,MAP_COUNT-1); startMatch(menuSel); }
void Game::aclShoot(const std::string&dj){ if(!ac)return; std::string m="{\"t\":\"shoot\",\"w\":"+std::to_string(ac->slot)+",\"d\":["+dj+"]}"; ac->ws.send(m); }
void Game::aclReload(){ if(ac)ac->ws.send("{\"t\":\"rl\"}"); }
void Game::aclGrenade(){ if(!ac||meI>=(int)e.size())return; Ent&p=e[meI]; float cp=cosf(p.pitch); char b[120]; snprintf(b,120,"{\"t\":\"gren\",\"d\":[%.4f,%.4f,%.4f]}",-sinf(p.yaw)*cp,sinf(p.pitch),-cosf(p.yaw)*cp); ac->ws.send(b); }
void Game::aclSwapSlot(){
  if(!ac||meI>=(int)e.size())return; Ent&p=e[meI]; ac->slotAm[ac->slot][0]=p.ammo; ac->slotAm[ac->slot][1]=p.mags; ac->slot^=1;
  int wi=weaponIdx(ac->slot?ac->loS:ac->loP); giveWeapon(p,wi); applyAtt(*this,p,ac->slot); if(ac->slotAm[ac->slot][0]>=0){p.ammo=ac->slotAm[ac->slot][0];p.mags=ac->slotAm[ac->slot][1];}
}
void Game::aclRender3D(){
  if(!ac)return;
  for(auto&g:ac->gr)r.obb(g.x,g.y,g.z,0.09f,0.09f,0.09f,0,0,0.2f,0.25f,0.15f);
}
// ---------- معالجة الرسائل ----------
static int entFor(Game&g,int id){ auto it=g.ac->idx.find(id); if(it!=g.ac->idx.end())return it->second; Ent n; n.bot=false; n.alive=false; g.e.push_back(n); int k=(int)g.e.size()-1; g.ac->idx[id]=k; snprintf(g.e[k].name,16,"P%d",id); return k; }
static void lobbyApply(Game&g,const J&m){
  auto&A=*g.ac; A.phase=m["ph"].str("lobby"); A.hostId=m["host"].i(0); A.cfgMap=m["cfg"]["map"].i(0); A.lobby.clear();
  for(size_t i=0;i<m["ps"].size();i++){ const J&q=m["ps"][i]; Game::ACl::LP l; l.id=q["id"].i(); l.n=q["n"].str(); l.tm=q["tm"].i(); l.b=q["b"].i(); l.p=q["p"].str("carbine"); l.s=q["s"].str("pistol"); A.lobby.push_back(l);
    if(A.inMatch||g.menu==0){ int k=entFor(g,l.id); g.e[k].team=l.tm?1:0; snprintf(g.e[k].name,16,"%s",l.id==A.myId?"YOU":cleanName(l.n,l.id,l.b).c_str()); g.e[k].gone=false; } }
}
static const Game::ACl::LP* lpOf(Game&g,int id){ for(auto&l:g.ac->lobby)if(l.id==id)return &l; return nullptr; }
void Game::aclUpdate(float dt,const Input&in){
  // ---- قوائم: 5 بحث ، 6 عنوان يدوي ، 7 اتصال ، 8 غرفة ----
  if(menu==5||menu==6){ ACl&A=*ac;
    if(msgT>0)msgT-=dt;
    bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f,rt=in.mx>0.5f&&prev.mx<=0.5f,lf=in.mx<-0.5f&&prev.mx>=-0.5f;
    if(A.scan){ std::lock_guard<std::mutex> g(A.scan->mu); A.hits=A.scan->hits; }
    if(menu==5){
      if(up&&A.sel>0)A.sel--; if(dn&&A.sel+1<(int)A.hits.size())A.sel++;
      if(in.confirm&&!prev.confirm&&!A.hits.empty()){ auto h=A.hits[A.sel]; if(A.scan)A.scan->stop=true; A.hostIp=h.ip; A.hostPort=h.port; A.ws.connectTo(h.ip,h.port); A.connT=0; menu=7; }
      if(in.reload&&!prev.reload){ menu=6; }
      if(in.back&&!prev.back){ aclStop(); }
    } else {
      if(A.ipTxt.empty()){ uint32_t li=localIp(); if(li){ char lb[24]; ipStr(li,lb); std::string t=lb; size_t d=t.rfind('.'); A.ipTxt=t.substr(0,d+1); } else A.ipTxt="192.168."; }
      if(rt&&A.pk<12&&A.pk%3<2)A.pk++; if(lf&&A.pk<12&&A.pk%3>0)A.pk--;
      if(up){ if(A.pk==12)A.pk=10; else if(A.pk>=3)A.pk-=3; } if(dn){ if(A.pk>=9&&A.pk<12)A.pk=12; else if(A.pk<9)A.pk+=3; }
      if(in.reload&&!prev.reload&&!A.ipTxt.empty())A.ipTxt.pop_back();
      if(in.confirm&&!prev.confirm){
        static const char*K[12]={"1","2","3","4","5","6","7","8","9",".","0","<"};
        if(A.pk<12){ const char*k=K[A.pk]; if(k[0]=='<'){ if(!A.ipTxt.empty())A.ipTxt.pop_back(); } else if(A.ipTxt.size()<15)A.ipTxt+=k; }
        else { in_addr_t ip=inet_addr(A.ipTxt.c_str()); if(ip!=INADDR_NONE){ A.hostIp=ip; A.hostPort=3000; if(A.scan)A.scan->stop=true; A.ws.connectTo(A.hostIp,3000); A.connT=0; menu=7; } else { msgT=2; } }
      }
      if(in.back&&!prev.back)menu=5;
    }
    prev=in; return; }
  if(!ac){ menu=1; role=0; return; }
  ACl&A=*ac;
  if(menu==7){ A.connT+=dt; int st=A.ws.state();
    if(in.back&&!prev.back){ aclStop(); prev=in; return; }
    if(st<0||A.connT>12.f){ char m[64]; snprintf(m,64,"COULD NOT CONNECT (%s)",A.ws.what()); say(m); msgT=5; aclStop(); prev=in; return; }
    if(st==1&&A.welcomed){ menu=8; }
  }
  // ---- رسائل الخادم ----
  std::string raw; int guard=0;
  while(ac&&A.ws.poll(raw)&&guard++<64){ bool ok=false; J m=jparse(raw,&ok); if(!ok)continue; std::string t=m["t"].str();
    if(t=="welcome"){ A.myId=m["id"].i(); A.welcomed=true; std::string lo=loJson(A); lo="{\"t\":\"hello\",\"n\":\"PS4\","+lo.substr(lo.find("\"lo\":")); A.ws.send(lo); }
    else if(t=="full"){ say("HOST IS FULL"); msgT=5; aclStop(); return; }
    else if(t=="lobby"){ lobbyApply(*this,m); if(A.phase=="lobby"&&menu==0&&A.inMatch){ /* انتهت المباراة والغرفة عادت */ } }
    else if(t=="start"){ A.inMatch=true; mapIdx=m["map"].i()%MAP_COUNT; w.load(mapIdx); e.clear(); A.idx.clear(); A.vs.clear(); vh.clear(); drive=-1; A.seat=-1; A.gr.clear(); over=false; score[0]=score[1]=0; tr.clear(); evq.clear(); menu=0; A.phase="playing"; g_aclTime=m["tl"].i();
      for(auto&l:A.lobby){ int k=entFor(*this,l.id); e[k].team=l.tm?1:0; snprintf(e[k].name,16,"%s",l.id==A.myId?"YOU":cleanName(l.n,l.id,l.b).c_str()); } meI=entFor(*this,A.myId); }
    else if(t=="tolobby"){ A.inMatch=false; A.phase="lobby"; menu=8; over=false; e.clear(); A.idx.clear(); }
    else if(t=="spawn"&&A.inMatch){ int id=m["id"].i(); int k=entFor(*this,id); Ent&p=e[k]; p.b.x=m["x"].num();p.b.y=m["y"].num();p.b.z=m["z"].num();p.b.vy=0;p.b.onGround=true; p.yaw=m["yaw"].num(); p.tx=p.b.x;p.ty=p.b.y;p.tz=p.b.z;p.tyaw=p.yaw; p.team=m["tm"].i()?1:0; p.alive=true;p.hp=100;p.prot=2;p.gone=false;p.crouch=false;
      if(id==A.myId){ meI=k; A.epoch=m["e"].i(); A.loP=m["p"].str(A.loP.c_str()); A.loS=m["s"].str(A.loS.c_str()); A.slot=0; A.slotAm[0][0]=A.slotAm[1][0]=-1; p.pitch=0; giveWeapon(p,weaponIdx(A.loP)); applyAtt(*this,p,0); p.rkP=p.rkY=0; }
      else { p.wpn=weaponIdx(m["p"].str("carbine")); } }
    else if(t=="snap"&&A.inMatch){
      g_aclTime=m["tl"].i(); score[0]=m["sc"][0].i(); score[1]=m["sc"][1].i();
      for(size_t i=0;i<m["ps"].size();i++){ const J&q=m["ps"][i]; int id=q[0].i(); int k=entFor(*this,id); Ent&p=e[k]; int f=q[6].i(); bool al=f&4; p.gone=false;
        const ACl::LP*lp=lpOf(*this,id); if(lp&&id!=A.myId){ p.team=lp->tm?1:0; p.wpn=weaponIdx(q[8].i()==1?lp->s:lp->p); }
        if(id==A.myId){ int hp=q[7].i(); if(hp<p.hp&&al)dmgFlash=0.5f; p.hp=(float)hp; if(!al&&p.alive){ p.alive=false; p.deaths++; p.respawn=3; } if(al&&!p.alive&&A.epoch>0){ p.alive=true; } meI=k; }
        else { bool first=!p.alive&&al; p.alive=al; p.hp=(float)q[7].i(); p.tx=q[1].num();p.ty=q[2].num();p.tz=q[3].num();p.tyaw=q[4].num(); p.pitch=q[5].num(); p.crouch=f&1; p.moving=f&8; p.sprint=f&32; p.phase+=p.moving?dt*8:0; if(first){p.b.x=p.tx;p.b.y=p.ty;p.b.z=p.tz;p.yaw=p.tyaw;} }
      }
      A.gr.clear(); for(size_t i=0;i<m["gr"].size();i++){ const J&g=m["gr"][i]; A.gr.push_back({(float)g[1].num(),(float)g[2].num(),(float)g[3].num()}); }
      { size_t nv=m["vs"].size(); if(vh.size()!=nv){ vh.clear(); drive=-1; A.seat=-1; for(size_t i=0;i<nv;i++){ const J&v=m["vs"][i]; VH::Veh o; o.x=o.tx=o.sx=(float)v[1].num(); o.z=o.tz=o.sz=(float)v[2].num(); o.h=o.th=o.sh=(float)v[3].num(); vh.push_back(o); } }
        for(size_t i=0;i<nv&&i<vh.size();i++){ const J&v=m["vs"][i]; VH::Veh&o=vh[i]; bool dd=v[8].i()&2; if(dd&&!o.dead)puffsBoom(o.x,o.z); if(!dd&&o.dead){ o.x=o.tx=(float)v[1].num(); o.z=o.tz=(float)v[2].num(); o.h=o.th=(float)v[3].num(); }
          o.dead=dd; o.burn=v[8].i()&1; o.hp=(float)v[7].i(); o.driver=-1; o.tires=v[6].i(); for(int q=0;q<4;q++)o.tireHp[q]=(o.tires>>q&1)?0.f:30.f;
          if((int)i!=drive){ o.tx=(float)v[1].num(); o.tz=(float)v[2].num(); o.th=(float)v[3].num(); o.speed=(float)v[4].num(); o.steer=(float)v[5].num(); if(v[9].i()>0)o.driver=0; } else o.driver=meI; } } }
    else if(t=="seat"){ int vi=m["v"].i(),sn=m["s"].i(); A.epoch=m["e"].i(); if(vi>=0&&vi<(int)vh.size()){ drive=vi; A.seat=sn; vh[vi].driver=meI; if(meI<(int)e.size()){ e[meI].crouch=false; e[meI].reloadT=-1; e[meI].pitch=-0.18f; } lookIdle=0; } }
    else if(t=="exit"){ A.epoch=m["e"].i(); if(drive>=0&&drive<(int)vh.size())vh[drive].driver=-1; drive=-1; A.seat=-1; if(meI<(int)e.size()){ Ent&p=e[meI]; p.b=Body(); p.b.x=(float)m["x"].num(); p.b.y=(float)m["y"].num(); p.b.z=(float)m["z"].num(); p.pitch=0; } }
    else if(t=="vfix"){ if(drive>=0&&drive<(int)vh.size()){ VH::Veh&o=vh[drive]; o.x=(float)m["x"].num(); o.z=(float)m["z"].num(); o.h=(float)m["h"].num(); o.vx=(float)m["vx"].num(); o.vz=(float)m["vz"].num(); } }
    else if(t=="vboom"){ int vi=m["v"].i(); if(vi>=0&&vi<(int)vh.size()){ vh[vi].dead=true; } puffsBoom((float)m["x"].num(),(float)m["z"].num()); if(vi==drive){ drive=-1; A.seat=-1; } }
    else if(t=="hit"){ int v=m["v"].i(),a=m["a"].i(); if(v==A.myId)dmgFlash=0.5f; if(a==A.myId&&v!=A.myId)hitMark=0.25f; }
    else if(t=="kill"&&A.inMatch){ int k=m["k"].i(),v=m["v"].i(); auto ik=A.idx.find(k),iv=A.idx.find(v); if(ik!=A.idx.end()&&iv!=A.idx.end()){ char b[64]; snprintf(b,64,"%s > %s%s",e[ik->second].name,e[iv->second].name,m["h"].i()?" HS":""); addFeed(b); if(k==A.myId)e[ik->second].kills++; } if(iv!=A.idx.end()){ e[iv->second].alive=false; if(v==A.myId){ e[iv->second].respawn=3; } } }
    else if(t=="shot"&&A.inMatch){ int id=m["id"].i(); auto it=A.idx.find(id); if(it!=A.idx.end()){ e[it->second].flash=0.06f; } float ox=m["o"][0].num(),oy=m["o"][1].num(),oz=m["o"][2].num(); for(size_t i=0;i<m["e"].size()&&i<6;i++){ const J&q=m["e"][i]; if(tr.size()<64)tr.push_back({ox,oy-0.1f,oz,(float)q[0].num(),(float)q[1].num(),(float)q[2].num(),0.09f}); } }
    else if(t=="end"){ over=true; winner=m["win"].i(); if(winner<0)winner=0; score[0]=m["sc"][0].i(); score[1]=m["sc"][1].i(); }
    else if(t=="left"){ auto it=A.idx.find(m["id"].i()); if(it!=A.idx.end()){ e[it->second].gone=true; e[it->second].alive=false; } }
  }
  if(ac&&A.ws.state()<0&&menu!=7){ say("DISCONNECTED FROM HOST"); msgT=5; aclStop(); prev=in; return; }
  A.pingT+=dt; if(A.pingT>4.f){ A.pingT=0; A.ws.send("{\"t\":\"ping\",\"c\":0}"); }
  // ---- غرفة الانتظار ----
  if(menu==8){
    bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f,rt=in.mx>0.5f&&prev.mx<=0.5f,lf=in.mx<-0.5f&&prev.mx>=-0.5f;
    auto pickWeapon=[&](bool primary,int d){ int cur=weaponIdx(primary?A.loP:A.loS); int n=WEAPON_COUNT; int k=cur; for(int i=0;i<n;i++){ k=(k+d+n)%n; if(WEAPONS[k].type==(primary?0:1))break; } (primary?A.loP:A.loS)=WEAPONS[k].id; for(int q=0;q<4;q++)A.att[primary?0:1][q]=0;
      std::string lo=loJson(A); A.ws.send(lo); };
    { int sl=in.ads?1:0; int w=wIdx(sl?A.loS:A.loP); bool ch=false;
      if(in.tri&&!prev.tri){ A.att[sl][0]=(A.att[sl][0]+1)%nOpt(w); ch=true; } if(in.swapP&&!prev.swapP){ A.att[sl][1]=(A.att[sl][1]+1)%3; ch=true; }
      if(in.swapN&&!prev.swapN){ A.att[sl][2]=(A.att[sl][2]+1)%3; ch=true; } if(in.fire&&!prev.fire){ A.att[sl][3]=(A.att[sl][3]+1)%3; ch=true; }
      if(ch)A.ws.send(loJson(A)); }
    if(rt)pickWeapon(true,1); if(lf)pickWeapon(true,-1); if(up)pickWeapon(false,1); if(dn)pickWeapon(false,-1);
    if(in.reload&&!prev.reload)A.ws.send("{\"t\":\"team\",\"v\":-1}");
    if(in.confirm&&!prev.confirm&&A.myId==A.hostId)A.ws.send("{\"t\":\"start\"}");
    if(in.back&&!prev.back){ aclStop(); prev=in; return; }
    prev=in; return; }
  if(menu!=0){ prev=in; return; }
  // ---- أثناء المباراة ----
  if(!A.inMatch||meI>=(int)e.size()){ prev=in; return; }
  if(in.start&&!prev.start)paused=!paused; Input use=in;
  if(paused){ if(in.mx>0.5f&&prev.mx<=0.5f)sens=std::min<float>(2.5f,sens+0.2f); if(in.mx<-0.5f&&prev.mx>=-0.5f)sens=std::max<float>(0.3f,sens-0.2f); if(in.back&&!prev.back){ aclStop(); prev=in; return; } use=Input(); }
  t+=dt; if(msgT>0)msgT-=dt; if(hitMark>0)hitMark-=dt; if(dmgFlash>0)dmgFlash-=dt;
  for(auto&q:tr)q.t-=dt; tr.erase(std::remove_if(tr.begin(),tr.end(),[](Tracer&q){return q.t<=0;}),tr.end()); for(int k=0;k<4;k++)if(feedT[k]>0)feedT[k]-=dt;
  Ent&me=e[meI]; if(me.prot>0)me.prot-=dt;
  for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(i==meI){ if(p.alive&&!over)updatePlayer(dt,use); continue; } if(p.gone)continue;
    float k=std::min<float>(1.f,dt*16.f); p.b.x+=(p.tx-p.b.x)*k;p.b.y+=(p.ty-p.b.y)*k;p.b.z+=(p.tz-p.b.z)*k; float dy=p.tyaw-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*k; if(p.flash>0)p.flash-=dt; if(p.moving)p.phase+=dt*7; }
  // ---- الشاص (ملتزم ببروتوكول الخادم: use / v / seat / exit) ----
  { bool u=in.use&&!paused; if(u&&!A.pu&&me.alive&&!over)A.ws.send("{\"t\":\"use\"}"); A.pu=u;
    for(int k=0;k<(int)vh.size();k++){ VH::Veh&v=vh[k]; if(k==drive)continue; float a=std::min<float>(1.f,dt*14.f); v.x+=(v.tx-v.x)*a; v.z+=(v.tz-v.z)*a; float dh=v.th-v.h; while(dh>3.14159f)dh-=6.28318f; while(dh<-3.14159f)dh+=6.28318f; v.h+=dh*a;
      if(v.burn&&!v.dead){ v.smoke-=dt; if(v.smoke<=0){ v.smoke=0.12f; puffs.push_back({v.x+(frand()-0.5f),1.3f,v.z+(frand()-0.5f),1.6f,0.9f,0.5f,frand()<0.3f}); } } }
    for(auto&q:puffs){ q.t-=dt; q.y+=q.vy*dt; q.sz+=dt*0.8f; } puffs.erase(std::remove_if(puffs.begin(),puffs.end(),[](Puff&q){return q.t<=0;}),puffs.end());
    if(drive>=0&&(!me.alive||drive>=(int)vh.size()||vh[drive].dead)){ if(drive<(int)vh.size())vh[drive].driver=-1; drive=-1; A.seat=-1; }
    if(drive>=0){ VH::Veh&v=vh[drive];
      if(A.seat==0){ float thr=std::max<float>(-1.f,std::min<float>(1.f,use.my+(use.fire?1.f:0.f)-(use.ads?1.f:0.f))); float crash=VH::step(w,v,vh,thr,use.mx,use.jump,dt); if(crash>A.crMax)A.crMax=crash; if(crash>VH::CRASHMIN)dmgFlash=std::max<float>(dmgFlash,0.15f); }
      static const float ST[5][3]={{-0.45f,0.35f,-0.25f},{0.45f,0.35f,-0.25f},{-0.55f,0.85f,1.2f},{0.55f,0.85f,1.2f},{0,0.85f,1.9f}}; int sn=std::max(0,std::min(4,A.seat)); float sx,sz; VH::l2w(v,ST[sn][0],ST[sn][2],sx,sz); me.b.x=sx;me.b.z=sz;me.b.y=ST[sn][1];me.b.vy=0;me.moving=false;
      A.vT+=dt; if(A.seat==0&&A.vT>=0.05f){ A.vT=0; char b[200]; snprintf(b,200,"{\"t\":\"v\",\"e\":%d,\"x\":%.2f,\"z\":%.2f,\"h\":%.3f,\"vx\":%.2f,\"vz\":%.2f,\"s\":%.3f,\"sp\":%.2f,\"cr\":%.2f}",A.epoch,v.x,v.z,v.h,v.vx,v.vz,v.steer,v.speed,A.crMax); A.ws.send(b); A.crMax=0; } } }
  A.stT+=dt; if(A.stT>=0.05f){ A.stT=0; if(me.alive){ int f=(me.crouch?1:0)|(use.ads?2:0)|(me.moving?4:0)|(me.sprint?8:0); char b[260]; snprintf(b,260,"{\"t\":\"st\",\"e\":%d,\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"yaw\":%.3f,\"pitch\":%.3f,\"f\":%d,\"w\":%d}",A.epoch,me.b.x,me.b.y,me.b.z,me.yaw,me.pitch,f,A.slot); A.ws.send(b); } }
  // عودة المباراة: المضيف يضغط X ... (الخادم يتولى الإعادة)
  prev=in;
}
void Game::aclRenderMenu(){
  if(!ac)return; ACl&A=*ac; int W=r.W,H=r.H,sc=std::max(1,H/270),sm=std::max(1,sc-1); char b[96];
  int cw=W*66/100,ch=H*84/100,cx=(W-cw)/2,cy=(H-ch)/2; card(cx,cy,cw,ch,sc);
  textC(W/2,cy+8*sc,"انضم إلى لعبة أندرويد",sc*2,C_TXT); rect(W/2-24*sc,cy+30*sc,48*sc,2*sc,C_ACC);
  if(menu==5){
    int done=A.scan?A.scan->done.load():0,total=A.scan?A.scan->total.load():0;
    if(A.hits.empty())textC(W/2,H/2-10*sc,total?"جارٍ البحث عن هواتف على شبكتك…":"جارٍ التجهيز…",sc,C_ACC);
    for(int i=0;i<(int)A.hits.size()&&i<6;i++){ char ib[24]; ipStr(A.hits[i].ip,ib); snprintf(b,96,"%s     %d  لاعب",ib,A.hits[i].players); btn(cx+12*sc,cy+42*sc+i*24*sc,cw-24*sc,20*sc,sc,b,i==A.sel,i==A.sel); }
    if(total){ int bw=cw-24*sc; rrect(cx+12*sc,cy+ch-34*sc,bw,4*sc,2*sc,C_LINE); rrect(cx+12*sc,cy+ch-34*sc,std::max(4*sc,bw*done/std::max(1,total)),4*sc,2*sc,C_ACC); }
    textC(W/2,cy+ch-26*sc,"شغّل «استضف لعبة شبكة محلية» في تطبيق الأندرويد أولاً",sm,C_MUT);
    textC(W/2,cy+ch-14*sc,"X: انضمام    Square: كتابة IP    O: رجوع",sm,C_MUT);
  } else if(menu==6){
    rrect(W/2-70*sc,cy+38*sc,140*sc,22*sc,6*sc,C_ACC); rrect(W/2-70*sc+1,cy+38*sc+1,140*sc-2,22*sc-2,6*sc-1,C_BTN);
    snprintf(b,96,"%s_",A.ipTxt.c_str()); textC(W/2,cy+38*sc+11*sc-5*sc,b,sc*2,C_TXT);
    static const char*K[13]={"1","2","3","4","5","6","7","8","9",".","0","⌫","اتصال"};
    int kw=34*sc,kh=17*sc,gx=W/2-kw*3/2,gy=cy+66*sc;
    for(int i=0;i<13;i++){ int x,y,w2; if(i<12){ x=gx+(i%3)*kw; y=gy+(i/3)*kh; w2=kw-3*sc; } else { x=gx; y=gy+4*kh; w2=kw*3-3*sc; }
      btn(x,y,w2,kh-3*sc,sc,i==11?"DEL":K[i],i==A.pk,i==12); }
    if(msgT>0)textC(W/2,cy+ch-26*sc,"عنوان IP غير صالح",sc,0xFFFF6060);
    textC(W/2,cy+ch-14*sc,"D-pad: تحريك    X: إدخال    Square: حذف    O: رجوع",sm,C_MUT);
  } else if(menu==7){ textC(W/2,H/2-14*sc,"جارٍ الاتصال…",sc*2,C_ACC); snprintf(b,96,"STEP: %s",A.ws.what()); textC(W/2,H/2+10*sc,b,sc,C_MUT); textC(W/2,cy+ch-14*sc,"O: إلغاء",sm,C_MUT);
  } else if(menu==8){
    static const char*MN[5]={"المستودع","أطلال الصحراء","شارع المدينة","القاعدة القطبية","صحراء الشاص"}; snprintf(b,96,"%s",MN[A.cfgMap%5]); textC(W/2,cy+38*sc,b,sc,C_ACC);
    int y=cy+54*sc; for(auto&l:A.lobby){ bool me2=l.id==A.myId; snprintf(b,96,"%s%s%s",l.id==A.hostId?"★ ":"",cleanName(l.n,l.id,l.b).c_str(),me2?"   (أنت)":"");
      rrect(cx+16*sc,y,cw-32*sc,15*sc,5*sc,l.tm?0xFF3B82F6:0xFFE23B3B,me2?0.55f:0.28f); text(cx+24*sc,y+4*sc,b,sc,C_TXT); y+=17*sc; if(y>cy+ch-72*sc)break; }
    for(int q=0;q<2;q++){ int w=wIdx(q?A.loS:A.loP); int wi=weaponIdx(q?A.loS:A.loP); snprintf(b,96,"%s: %s  |  %s  |  %s  |  %s  |  %s",q?"ثانوي":"رئيسي",WEAPONS[wi].name,optAr(w,A.att[q][0]),MUZ_AR[A.att[q][1]],UND_AR[A.att[q][2]],MAG_AR[A.att[q][3]]); textC(W/2,cy+ch-58*sc+q*10*sc,b,sm,C_TXT); }
    bool hostMe=A.myId==A.hostId; btn(cx+cw/4,cy+ch-36*sc,cw/2,18*sc,sc,hostMe?"ابدأ المباراة":"بانتظار المضيف…",hostMe,hostMe);
    textC(W/2,cy+ch-12*sc,"D-pad: سلاح  △ منظار  L1 فوهة  R1 مقبض  R2 مخزن (L2=الثانوي)  □ فريق  O: خروج",sm,C_MUT);
  }
  if(msgT>0&&menu!=6)textC(W/2,cy+ch-48*sc,msg,sc,0xFFFF6060);
}
