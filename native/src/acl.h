// عميل نسخة أندرويد: ينضم إلى لعبة يستضيفها الهاتف (WebSocket + JSON) بنفس بروتوكول shared/game.js
#pragma once
#include <map>

struct Game::ACl{
  WsClient ws; std::shared_ptr<ScanState> scan; std::vector<ScanHit> hits; int sel=0; int ip[4]={192,168,1,100}; int ipSel=3; bool ipEdit=false; std::string ipTxt; int pk=0;
  int myId=0,hostId=0,epoch=0,cfgMap=0; std::string phase="lobby"; bool welcomed=false; float stT=0,pingT=0,connT=0; std::string err; uint32_t hostIp=0; int hostPort=3000;
  struct LP{int id=0;std::string n;int tm=0,b=0;std::string p,s;}; std::vector<LP> lobby;
  std::map<int,int> idx; std::string loP="carbine",loS="pistol"; int slot=0; int slotAm[2][2]={{-1,0},{-1,0}};
  struct Gr{float x,y,z;}; std::vector<Gr> gr; struct Vh{float x,z,h;int hp,fl;}; std::vector<Vh> vs;
  bool inMatch=false; int lastEvtCnt=0;
};
static int weaponIdx(const std::string&id){ for(int i=0;i<WEAPON_COUNT;i++)if(id==WEAPONS[i].id)return i; return 0; }
static std::string cleanName(const std::string&n,int id,bool bot){ bool ok=!n.empty(); for(unsigned char c:n)if(c<32||c>126)ok=false; if(ok)return n.substr(0,12); char b[16]; snprintf(b,16,"%s%d",bot?"BOT":"P",id); return b; }

void Game::aclStart(uint32_t ip,int port){
  if(ac){ac->ws.close();delete ac;} ac=new ACl(); role=3; meI=0; e.clear(); over=false;
  if(ip){ ac->hostIp=ip; ac->hostPort=port; ac->ws.connectTo(ip,port); menu=7; }
  else { ac->scan=std::make_shared<ScanState>(); scanStart(ac->scan); menu=5; }
}
void Game::aclStop(){ if(ac){ ac->ws.close(); if(ac->scan)ac->scan->stop=true; delete ac; ac=nullptr; } role=0; meI=0; e.assign(1,Ent()); menu=1; paused=false; mapIdx=std::min(mapIdx,MAP_COUNT-1); startMatch(menuSel); }
void Game::aclShoot(const std::string&dj){ if(!ac)return; std::string m="{\"t\":\"shoot\",\"w\":"+std::to_string(ac->slot)+",\"d\":["+dj+"]}"; ac->ws.send(m); }
void Game::aclReload(){ if(ac)ac->ws.send("{\"t\":\"rl\"}"); }
void Game::aclGrenade(){ if(!ac||meI>=(int)e.size())return; Ent&p=e[meI]; float cp=std::cos(p.pitch); char b[120]; snprintf(b,120,"{\"t\":\"gren\",\"d\":[%.4f,%.4f,%.4f]}",-std::sin(p.yaw)*cp,std::sin(p.pitch),-std::cos(p.yaw)*cp); ac->ws.send(b); }
void Game::aclSwapSlot(){
  if(!ac||meI>=(int)e.size())return; Ent&p=e[meI]; ac->slotAm[ac->slot][0]=p.ammo; ac->slotAm[ac->slot][1]=p.mags; ac->slot^=1;
  int wi=weaponIdx(ac->slot?ac->loS:ac->loP); giveWeapon(p,wi); if(ac->slotAm[ac->slot][0]>=0){p.ammo=ac->slotAm[ac->slot][0];p.mags=ac->slotAm[ac->slot][1];}
}
void Game::aclRender3D(){
  if(!ac)return;
  for(auto&v:ac->vs){ if(v.fl&2){ r.obb(v.x,0.4f,v.z,1.0f,0.4f,2.5f,v.h,0,0.1f,0.1f,0.1f); continue; }
    r.obb(v.x,0.7f,v.z,1.0f,0.4f,2.5f,v.h,0,0.35f,0.4f,0.25f); float o[3]; Renderer::rot(v.h,0,0,1.5f,-0.1f,o); r.obb(v.x+o[0],o[1],v.z+o[2],0.95f,0.45f,0.55f,v.h,0,0.3f,0.35f,0.22f);
    for(int sx=-1;sx<=1;sx+=2)for(int sz=-1;sz<=1;sz+=2){ Renderer::rot(v.h,0,sx*1.0f,0.35f,sz*1.55f,o); r.obb(v.x+o[0],o[1],v.z+o[2],0.12f,0.35f,0.35f,v.h,0,0.05f,0.05f,0.05f); } }
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
    if(t=="welcome"){ A.myId=m["id"].i(); A.welcomed=true; std::string lo="{\"t\":\"hello\",\"n\":\"PS4\",\"lo\":{\"p\":\""+A.loP+"\",\"s\":\""+A.loS+"\",\"a\":[{\"optic\":\"iron\",\"muzzle\":\"none\",\"under\":\"none\",\"mag\":\"std\"},{\"optic\":\"iron\",\"muzzle\":\"none\",\"under\":\"none\",\"mag\":\"std\"}]}}"; A.ws.send(lo); }
    else if(t=="full"){ say("HOST IS FULL"); msgT=5; aclStop(); return; }
    else if(t=="lobby"){ lobbyApply(*this,m); if(A.phase=="lobby"&&menu==0&&A.inMatch){ /* انتهت المباراة والغرفة عادت */ } }
    else if(t=="start"){ A.inMatch=true; mapIdx=m["map"].i()%MAP_COUNT; w.load(mapIdx); e.clear(); A.idx.clear(); A.vs.clear(); A.gr.clear(); over=false; score[0]=score[1]=0; tr.clear(); evq.clear(); menu=0; A.phase="playing"; g_aclTime=m["tl"].i();
      for(auto&l:A.lobby){ int k=entFor(*this,l.id); e[k].team=l.tm?1:0; snprintf(e[k].name,16,"%s",l.id==A.myId?"YOU":cleanName(l.n,l.id,l.b).c_str()); } meI=entFor(*this,A.myId); }
    else if(t=="tolobby"){ A.inMatch=false; A.phase="lobby"; menu=8; over=false; e.clear(); A.idx.clear(); }
    else if(t=="spawn"&&A.inMatch){ int id=m["id"].i(); int k=entFor(*this,id); Ent&p=e[k]; p.b.x=m["x"].num();p.b.y=m["y"].num();p.b.z=m["z"].num();p.b.vy=0;p.b.onGround=true; p.yaw=m["yaw"].num(); p.tx=p.b.x;p.ty=p.b.y;p.tz=p.b.z;p.tyaw=p.yaw; p.team=m["tm"].i()?1:0; p.alive=true;p.hp=100;p.prot=2;p.gone=false;p.crouch=false;
      if(id==A.myId){ meI=k; A.epoch=m["e"].i(); A.loP=m["p"].str(A.loP.c_str()); A.loS=m["s"].str(A.loS.c_str()); A.slot=0; A.slotAm[0][0]=A.slotAm[1][0]=-1; p.pitch=0; giveWeapon(p,weaponIdx(A.loP)); p.rkP=p.rkY=0; }
      else { p.wpn=weaponIdx(m["p"].str("carbine")); } }
    else if(t=="snap"&&A.inMatch){
      g_aclTime=m["tl"].i(); score[0]=m["sc"][0].i(); score[1]=m["sc"][1].i();
      for(size_t i=0;i<m["ps"].size();i++){ const J&q=m["ps"][i]; int id=q[0].i(); int k=entFor(*this,id); Ent&p=e[k]; int f=q[6].i(); bool al=f&4; p.gone=false;
        const ACl::LP*lp=lpOf(*this,id); if(lp&&id!=A.myId){ p.team=lp->tm?1:0; p.wpn=weaponIdx(q[8].i()==1?lp->s:lp->p); }
        if(id==A.myId){ int hp=q[7].i(); if(hp<p.hp&&al)dmgFlash=0.5f; p.hp=(float)hp; if(!al&&p.alive){ p.alive=false; p.deaths++; p.respawn=3; } if(al&&!p.alive&&A.epoch>0){ p.alive=true; } meI=k; }
        else { bool first=!p.alive&&al; p.alive=al; p.hp=(float)q[7].i(); p.tx=q[1].num();p.ty=q[2].num();p.tz=q[3].num();p.tyaw=q[4].num(); p.pitch=q[5].num(); p.crouch=f&1; p.moving=f&8; p.sprint=f&32; p.phase+=p.moving?dt*8:0; if(first){p.b.x=p.tx;p.b.y=p.ty;p.b.z=p.tz;p.yaw=p.tyaw;} }
      }
      A.gr.clear(); for(size_t i=0;i<m["gr"].size();i++){ const J&g=m["gr"][i]; A.gr.push_back({(float)g[1].num(),(float)g[2].num(),(float)g[3].num()}); }
      A.vs.clear(); for(size_t i=0;i<m["vs"].size();i++){ const J&v=m["vs"][i]; A.vs.push_back({(float)v[1].num(),(float)v[2].num(),(float)v[3].num(),v[7].i(),v[8].i()}); } }
    else if(t=="hit"){ int v=m["v"].i(),a=m["a"].i(); if(v==A.myId)dmgFlash=0.5f; if(a==A.myId&&v!=A.myId)hitMark=0.25f; }
    else if(t=="kill"&&A.inMatch){ int k=m["k"].i(),v=m["v"].i(); auto ik=A.idx.find(k),iv=A.idx.find(v); if(ik!=A.idx.end()&&iv!=A.idx.end()){ char b[64]; snprintf(b,64,"%s > %s%s",e[ik->second].name,e[iv->second].name,m["h"].i()?" HS":""); addFeed(b); if(k==A.myId)e[ik->second].kills++; } if(iv!=A.idx.end()){ e[iv->second].alive=false; if(v==A.myId){ e[iv->second].respawn=3; } } }
    else if(t=="shot"&&A.inMatch){ int id=m["id"].i(); auto it=A.idx.find(id); if(it!=A.idx.end()){ e[it->second].flash=0.06f; } float ox=m["o"][0].num(),oy=m["o"][1].num(),oz=m["o"][2].num(); for(size_t i=0;i<m["e"].size()&&i<6;i++){ const J&q=m["e"][i]; if(tr.size()<64)tr.push_back({ox,oy-0.1f,oz,(float)q[0].num(),(float)q[1].num(),(float)q[2].num(),0.07f}); } }
    else if(t=="end"){ over=true; winner=m["win"].i(); if(winner<0)winner=0; score[0]=m["sc"][0].i(); score[1]=m["sc"][1].i(); }
    else if(t=="left"){ auto it=A.idx.find(m["id"].i()); if(it!=A.idx.end()){ e[it->second].gone=true; e[it->second].alive=false; } }
  }
  if(ac&&A.ws.state()<0&&menu!=7){ say("DISCONNECTED FROM HOST"); msgT=5; aclStop(); prev=in; return; }
  A.pingT+=dt; if(A.pingT>4.f){ A.pingT=0; A.ws.send("{\"t\":\"ping\",\"c\":0}"); }
  // ---- غرفة الانتظار ----
  if(menu==8){
    bool up=in.my>0.5f&&prev.my<=0.5f,dn=in.my<-0.5f&&prev.my>=-0.5f,rt=in.mx>0.5f&&prev.mx<=0.5f,lf=in.mx<-0.5f&&prev.mx>=-0.5f;
    auto pickWeapon=[&](bool primary,int d){ int cur=weaponIdx(primary?A.loP:A.loS); int n=WEAPON_COUNT; int k=cur; for(int i=0;i<n;i++){ k=(k+d+n)%n; if(WEAPONS[k].type==(primary?0:1))break; } (primary?A.loP:A.loS)=WEAPONS[k].id;
      std::string lo="{\"t\":\"lo\",\"lo\":{\"p\":\""+A.loP+"\",\"s\":\""+A.loS+"\",\"a\":[{\"optic\":\"iron\",\"muzzle\":\"none\",\"under\":\"none\",\"mag\":\"std\"},{\"optic\":\"iron\",\"muzzle\":\"none\",\"under\":\"none\",\"mag\":\"std\"}]}}"; A.ws.send(lo); };
    if(rt)pickWeapon(true,1); if(lf)pickWeapon(true,-1); if(up)pickWeapon(false,1); if(dn)pickWeapon(false,-1);
    if(in.reload&&!prev.reload)A.ws.send("{\"t\":\"team\",\"v\":-1}");
    if(in.confirm&&!prev.confirm&&A.myId==A.hostId)A.ws.send("{\"t\":\"start\"}");
    if(in.back&&!prev.back){ aclStop(); prev=in; return; }
    prev=in; return; }
  if(menu!=0){ prev=in; return; }
  // ---- أثناء المباراة ----
  if(!A.inMatch||meI>=(int)e.size()){ prev=in; return; }
  if(in.start&&!prev.start)paused=!paused; Input use=in;
  if(paused){ if(in.mx>0.5f&&prev.mx<=0.5f)sens=std::min(2.5f,sens+0.2f); if(in.mx<-0.5f&&prev.mx>=-0.5f)sens=std::max(0.3f,sens-0.2f); if(in.back&&!prev.back){ aclStop(); prev=in; return; } use=Input(); }
  t+=dt; if(msgT>0)msgT-=dt; if(hitMark>0)hitMark-=dt; if(dmgFlash>0)dmgFlash-=dt;
  for(auto&q:tr)q.t-=dt; tr.erase(std::remove_if(tr.begin(),tr.end(),[](Tracer&q){return q.t<=0;}),tr.end()); for(int k=0;k<4;k++)if(feedT[k]>0)feedT[k]-=dt;
  Ent&me=e[meI]; if(me.prot>0)me.prot-=dt;
  for(int i=0;i<(int)e.size();i++){ Ent&p=e[i]; if(i==meI){ if(p.alive&&!over)updatePlayer(dt,use); continue; } if(p.gone)continue;
    float k=std::min(1.f,dt*16.f); p.b.x+=(p.tx-p.b.x)*k;p.b.y+=(p.ty-p.b.y)*k;p.b.z+=(p.tz-p.b.z)*k; float dy=p.tyaw-p.yaw; while(dy>3.14159f)dy-=6.28318f; while(dy<-3.14159f)dy+=6.28318f; p.yaw+=dy*k; if(p.flash>0)p.flash-=dt; if(p.moving)p.phase+=dt*7; }
  A.stT+=dt; if(A.stT>=0.05f){ A.stT=0; if(me.alive){ int f=(me.crouch?1:0)|(use.ads?2:0)|(me.moving?4:0)|(me.sprint?8:0); char b[260]; snprintf(b,260,"{\"t\":\"st\",\"e\":%d,\"x\":%.2f,\"y\":%.2f,\"z\":%.2f,\"yaw\":%.3f,\"pitch\":%.3f,\"f\":%d,\"w\":%d}",A.epoch,me.b.x,me.b.y,me.b.z,me.yaw,me.pitch,f,A.slot); A.ws.send(b); } }
  // عودة المباراة: المضيف يضغط X ... (الخادم يتولى الإعادة)
  prev=in;
}
void Game::aclRenderMenu(){
  if(!ac)return; ACl&A=*ac; int W=r.W,H=r.H,sc=std::max(1,H/270); static float mt=0; mt+=0.016f;
  if(w.id!=menuSel)w.load(menuSel); Cam c{0,6,0,mt*0.2f,-0.15f,1.2f}; r.setCam(c); r.world(w); rect(0,0,W,H,0xFF000000,0.55f);
  textC(W/2,H/10,"JOIN ANDROID GAME",sc*4,0xFFFFFFFF); char b[96]; uint32_t dim=0xFF909090;
  if(menu==5){
    int done=A.scan?A.scan->done.load():0,total=A.scan?A.scan->total.load():0;
    if(A.hits.empty())textC(W/2,H/2-8*sc,total?"SEARCHING FOR PHONES ON YOUR NETWORK...":"PREPARING SCAN...",sc*2,0xFFFFD060);
    for(int i=0;i<(int)A.hits.size();i++){ char ib[24]; ipStr(A.hits[i].ip,ib); snprintf(b,96,"%s %s   PLAYERS %d   %s",i==A.sel?">":" ",ib,A.hits[i].players,A.hits[i].phase.c_str()); textC(W/2,H/3+i*14*sc,b,sc*2,i==A.sel?0xFFFFFFFF:dim); }
    if(total){ int bw=W/2; rect(W/4,H*3/4,bw,4*sc,0xFF202830); rect(W/4,H*3/4,bw*done/std::max(1,total),4*sc,0xFF50E080); }
    textC(W/2,H-34*sc,"X: JOIN    SQUARE: TYPE IP    O: BACK",sc*2,0xFFB0B8C0);
    textC(W/2,H-20*sc,"START LAN HOSTING IN THE PHONE APP FIRST",sc,dim);
  } else if(menu==6){
    textC(W/2,H/6,"TYPE THE PHONE IP",sc*2,0xFFFFD060);
    snprintf(b,96,"%s_",A.ipTxt.c_str()); textC(W/2,H/6+22*sc,b,sc*4,0xFFFFFFFF);
    static const char*K[13]={"1","2","3","4","5","6","7","8","9",".","0","DEL","CONNECT"};
    int kw=34*sc,kh=20*sc,gx=W/2-kw*3/2,gy=H/6+60*sc;
    for(int i=0;i<13;i++){ int cx,cy,w2; if(i<12){ cx=gx+(i%3)*kw; cy=gy+(i/3)*kh; w2=kw-4*sc; } else { cx=gx; cy=gy+4*kh; w2=kw*3-4*sc; }
      rect(cx,cy,w2,kh-4*sc,i==A.pk?0xFF50E080:0xFF202830); textC(cx+w2/2,cy+(kh-4*sc)/2-3*sc,K[i],sc*2,i==A.pk?0xFF000000:0xFFFFFFFF); }
    if(msgT>0)textC(W/2,H-48*sc,"INVALID IP",sc*2,0xFFFF6060);
    textC(W/2,H-30*sc,"D-PAD: MOVE    X: PRESS    SQUARE: DELETE    O: BACK",sc,0xFFB0B8C0);
  } else if(menu==7){ textC(W/2,H/2-8*sc,"CONNECTING...",sc*3,0xFFFFD060); snprintf(b,96,"STEP: %s",A.ws.what()); textC(W/2,H/2+20*sc,b,sc*2,dim); textC(W/2,H/2+44*sc,"O = CANCEL",sc*2,dim);
  } else if(menu==8){
    static const char*MN[5]={"WAREHOUSE","DESERT RUINS","CITY STREET","POLAR BASE","SHAS DESERT"}; snprintf(b,96,"ROOM  -  MAP %d %s",A.cfgMap+1,MN[A.cfgMap%5]); textC(W/2,H/5,b,sc*2,0xFFFFD060);
    int y=H/4+10*sc; for(auto&l:A.lobby){ bool me2=l.id==A.myId; snprintf(b,96,"%s%s%s",l.id==A.hostId?"[HOST] ":"",cleanName(l.n,l.id,l.b).c_str(),me2?"  (YOU)":""); text(W/4,y,b,sc*2,l.tm?0xFF6090FF:0xFFFF6060); y+=11*sc; if(y>H*3/4)break; }
    snprintf(b,96,"PRIMARY %s   SECONDARY %s",A.loP.c_str(),A.loS.c_str()); textC(W/2,H-52*sc,b,sc*2,0xFFFFFFFF);
    textC(W/2,H-38*sc,A.myId==A.hostId?"X: START MATCH":"WAITING FOR HOST TO START",sc*2,A.myId==A.hostId?0xFF50E080:dim);
    textC(W/2,H-24*sc,"LEFT/RIGHT: PRIMARY   UP/DOWN: SECONDARY   SQUARE: SWITCH TEAM   O: LEAVE",sc,0xFFB0B8C0);
  }
  if(msgT>0)textC(W/2,H*2/3,msg,sc*2,0xFFFF6060);
}
void Game::aclMenuNav(const Input&){}
