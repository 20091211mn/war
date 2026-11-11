// طبقة الهاتف المشتركة (أندرويد + آيفون): تحوّل اللمس إلى مدخلات اللعبة، وتُشغّل التحديث والرسم، وتُخرج صورة RGBX جاهزة للعرض.
// المحرك نفسه محرك PS4 (C++، رسم برمجي على المعالج) — لا HTML ولا WebView.
#pragma once
#ifndef THREADS
#define THREADS 2
#endif
#include "../src/game.h"
#include <chrono>
#include <thread>

struct Mobile{
  Game g; int W=0,H=0,sw=1,sh=1,baseH=540,resizeSeq=0; std::vector<uint32_t> out;
  enum{ B_FIRE=1,B_ADS,B_JUMP,B_CROUCH,B_RELOAD,B_GREN,B_USE,B_DRONE,B_C4,B_DET,B_SEAT,B_PREV,B_NEXT,B_PAUSE,B_WEAPON, M_UP,M_DOWN,M_LEFT,M_RIGHT,M_OK,M_BACK, P_OPT,P_MUZ,P_GRIP,P_MAG, NB };
  struct Btn{ int id; float x,y,r; const char*lab; };
  struct Tc{ int id=-1; float x=0,y=0,ox=0,oy=0; int role=0,btn=0; }; // role: 1 حركة ، 2 نظر ، 3 زر
  Tc tc[12]; bool held[NB]={}; bool adsL=false,crL=false; float pdx=0,pdy=0,stX=0,stY=0; float acc=0,costAvg=16,fpsAvg=60,slowT=0,fastT=0; bool started=false; int lastMode=-1;
  int pickReq=-1;
  std::vector<Btn> btns; // أزرار الحالة الحالية (إحداثيات بكسلات الصورة الداخلية)
  float U()const{ return H/540.f; }

  // ---------- تهيئة ----------
  void start(int screenW,int screenH,int cores){
    sw=std::max(1,screenW); sh=std::max(1,screenH); baseH=std::min(540,std::max(360,sh)); H=baseH; W=calcW(H);
    g.modeCount=3; g.menu=1; g.nBots=6; g.r.startThreads(std::max(1,std::min(4,cores-1)));
    g.init(W,H); out.assign((size_t)W*H,0); started=true;
  }
  int calcW(int h)const{ int w=(int)(h*(float)sw/sh+0.5f); w=std::max(h,std::min((int)(h*2.4f),w)); return (w+1)&~1; }
  void setScreen(int screenW,int screenH){ if(screenW==sw&&screenH==sh)return; sw=std::max(1,screenW); sh=std::max(1,screenH); resize(H); }
  void applyQuality(){ static const float QS[5]={0.55f,0.7f,0.85f,1.0f,1.0f}; int cap=g_quality>=4?720:540; int bh=std::min(cap,std::max(360,sh)); bh=std::max(288,(int)(bh*QS[g_quality%5])); baseH=bh; g.qChanged=false; int h2=std::min(H,baseH); if(g_quality>=4)h2=baseH; if(g_quality<3&&H>baseH)h2=baseH; resize(h2); }
  void resize(int newH){ newH=std::max(288,std::min(baseH,newH)); int nw=calcW(newH); if(nw==W&&newH==H)return; g.r.flush(); H=newH; W=nw; g.r.init(W,H); out.assign((size_t)W*H,0); resizeSeq++; }

  // ---------- اللمس ----------
  void layout(){
    btns.clear(); float u=U(),w=(float)W,h=(float)H; auto A=[&](int id,float x,float y,float r,const char*l){ btns.push_back({id,x,y,r*u,l}); };
    if(g.menu||g.paused){
      A(M_UP,130*u,h-205*u,34,"UP"); A(M_DOWN,130*u,h-60*u,34,"DN"); A(M_LEFT,55*u,h-132*u,34,"<"); A(M_RIGHT,205*u,h-132*u,34,">");
      A(M_OK,w-110*u,h-110*u,48,"OK"); A(M_BACK,w-235*u,h-72*u,34,"BACK");
      if(g.paused&&!g.menu){ A(P_OPT,w-60*u,150*u,30,"OPTIC"); A(P_MUZ,w-60*u,215*u,30,"MUZZLE"); A(P_GRIP,w-60*u,280*u,30,"GRIP"); A(P_MAG,w-60*u,345*u,30,"MAG"); }
      return; }
    if(g.air&&g.myJet()>=0){ A(B_FIRE,w-120*u,h-130*u,64,"GUN"); A(B_ADS,w-236*u,h-70*u,40,"BURN"); A(B_JUMP,w-62*u,h-250*u,36,"THR+"); A(B_CROUCH,w-196*u,h-200*u,34,"THR-"); A(B_USE,w-46*u,150*u,25,"EXIT");
      A(B_PREV,w-190*u,32*u,22,"<"); A(B_NEXT,w-140*u,32*u,22,">"); A(B_PAUSE,w/2+125*u,28*u,20,"II"); if(g.over)A(M_OK,w/2,h-120*u,46,"OK"); return; }
    A(B_FIRE,w-120*u,h-130*u,64,"FIRE"); A(B_ADS,w-236*u,h-70*u,40,"ADS"); A(B_JUMP,w-62*u,h-250*u,36,"JUMP"); A(B_CROUCH,w-196*u,h-200*u,34,"CROUCH"); A(B_RELOAD,w-290*u,h-140*u,32,"R");
    A(B_GREN,w-46*u,150*u,25,"G"); A(B_USE,w-46*u,208*u,25,"CAR");
    A(B_C4,44*u,200*u,25,"C4"); A(B_DET,44*u,258*u,25,"DET"); A(B_SEAT,44*u,316*u,25,"SEAT"); A(B_DRONE,44*u,374*u,25,"UAV");
    A(B_PREV,w-190*u,32*u,22,"<"); A(B_NEXT,w-140*u,32*u,22,">"); A(B_WEAPON,w/2+190*u,h-40*u,30,"WPN"); A(B_PAUSE,w/2+125*u,28*u,20,"II");
    if(g.over)A(M_OK,w/2,h-120*u,46,"OK");
  }
  int hit(float x,float y)const{ for(int i=(int)btns.size()-1;i>=0;i--){ const Btn&b=btns[i]; float dx=x-b.x,dy=y-b.y,rr=b.r*1.25f; if(dx*dx+dy*dy<=rr*rr)return b.id; } return 0; }
  Tc* find(int id){ for(auto&t:tc)if(t.id==id)return &t; return nullptr; }
  void touchDown(int id,float px,float py){
    if(!started||find(id))return; Tc*t=nullptr; for(auto&q:tc)if(q.id<0){t=&q;break;} if(!t)return;
    float x=px*W/sw,y=py*H/sh; layout(); int b=hit(x,y);
    if(g.wheelOpen){ int sc2=g.hudSc(); for(int i=0;i<WEAPON_COUNT;i++){ int rx,ry,rw,rh; g.wheelRect(i,sc2,rx,ry,rw,rh); if(x>=rx&&x<=rx+rw&&y>=ry&&y<=ry+rh){ pickReq=i; break; } } g.wheelOpen=false; t->id=id; t->role=4; t->btn=0; return; }
    t->id=id; t->x=x; t->y=y; t->ox=x; t->oy=y; t->btn=0; t->role=0;
    if(b==B_WEAPON){ g.wheelOpen=true; t->role=4; return; }
    if(b){ t->role=3; t->btn=b; held[b]=true; if(b==B_ADS)adsL=!adsL; else if(b==B_CROUCH&&!(g.air&&g.myJet()>=0))crL=!crL; return; }
    if(g.menu||g.paused)return;
    bool haveMove=false; for(auto&q:tc)if(q.id>=0&&q.role==1)haveMove=true;
    if(x<W*0.38f&&!haveMove){ t->role=1; stX=stY=0; } else t->role=2;
  }
  void touchMove(int id,float px,float py){
    Tc*t=find(id); if(!t)return; float x=px*W/sw,y=py*H/sh;
    if(t->role==2||(t->role==3&&t->btn==B_FIRE)){ pdx+=x-t->x; pdy+=y-t->y; }
    else if(t->role==1){ float R=75.f*U(),dx=(x-t->ox)/R,dy=(y-t->oy)/R; float m=sqrtf(dx*dx+dy*dy); if(m>1){dx/=m;dy/=m;} stX=dx; stY=-dy; }
    t->x=x; t->y=y; }
  void touchUp(int id){
    Tc*t=find(id); if(!t)return;
    if(t->role==3&&t->btn){ bool other=false; for(auto&q:tc)if(&q!=t&&q.id>=0&&q.role==3&&q.btn==t->btn)other=true; if(!other)held[t->btn]=false; }
    if(t->role==1){ stX=stY=0; }
    t->id=-1; t->role=0; t->btn=0; }
  void releaseAll(){ for(auto&t:tc){ t.id=-1; t.role=0; t.btn=0; } for(auto&b:held)b=false; stX=stY=0; }

  // ---------- إطار واحد ----------
  Input build(){
    Input in; in.pick=pickReq; pickReq=-1; bool menuMode=g.menu||g.paused; if(menuMode)g.wheelOpen=false;
    if(menuMode){
      in.my=held[M_UP]?1.f:held[M_DOWN]?-1.f:0.f; in.mx=held[M_RIGHT]?1.f:held[M_LEFT]?-1.f:0.f; in.confirm=held[M_OK]; in.start=held[M_OK]&&!g.menu; in.back=held[M_BACK];
      if(g.paused&&!g.menu){ in.tri=held[P_OPT]; in.reload=held[P_MUZ]; in.swapP=held[P_GRIP]; in.fire=held[P_MAG]; }
      return in; }
    float sx=stX,sy=stY,m=sqrtf(sx*sx+sy*sy); const float D=0.14f; if(m<D){sx=sy=0;} else { float k=std::min(1.f,(m-D)/(1-D)); sx=sx/m*k; sy=sy/m*k; }
    in.mx=sx; in.my=sy; in.sprint=sy>0.85f&&m>0.95f;
    in.fire=held[B_FIRE]; in.ads=adsL; in.jump=held[B_JUMP]; in.crouch=(g.air&&g.myJet()>=0)?held[B_CROUCH]:crL; in.reload=held[B_RELOAD];
    in.grenade=held[B_GREN]; in.tri=held[B_USE]; in.use=held[B_USE]; in.drone=held[B_DRONE]; in.c4=held[B_C4]; in.det=held[B_DET]; in.seat=held[B_SEAT];
    in.swapP=held[B_PREV]; in.swapN=held[B_NEXT]; in.start=held[B_PAUSE]; in.confirm=g.over&&held[M_OK];
    return in; }
  void drawCircle(int cx,int cy,int r,uint32_t c,float a,int ring=0){
    for(int dy=-r;dy<=r;dy++){ int half=(int)sqrtf((float)(r*r-dy*dy)); if(ring>0&&std::abs(dy)<r-ring){ int in2=r-ring; int h2=(int)sqrtf((float)(in2*in2-dy*dy)); g.blendSpan(cy+dy,cx-half,cx-h2,c,a); g.blendSpan(cy+dy,cx+h2+1,cx+half+1,c,a); } else g.blendSpan(cy+dy,cx-half,cx+half+1,c,a); } }
  void drawControls(){
    layout(); float u=U(); int sc=std::max(1,(int)(u*1.0f+0.5f));
    for(auto&b:btns){ bool on=held[b.id]||(b.id==B_ADS&&adsL)||(b.id==B_CROUCH&&crL); uint32_t col=(b.id==B_FIRE)?0xFFE23B3B:(b.id>=M_UP&&b.id<=M_BACK)?0xFF3B82F6:0xFFDDE6F2;
      drawCircle((int)b.x,(int)b.y,(int)b.r,col,on?0.55f:0.22f); drawCircle((int)b.x,(int)b.y,(int)b.r,0xFFFFFFFF,0.55f,std::max(1,(int)(2*u)));
      int s2=b.r>=40*u?sc:std::max(1,sc-1+(b.r>=28*u?0:0)); g.textC((int)b.x,(int)b.y-4*s2,b.lab,s2,0xFFFFFFFF); }
    if(!g.menu&&!g.paused){ for(auto&t:tc)if(t.id>=0&&t.role==1){ drawCircle((int)t.ox,(int)t.oy,(int)(75*u),0xFFFFFFFF,0.14f,std::max(1,(int)(2*u))); drawCircle((int)(t.ox+stX*75*u),(int)(t.oy-stY*75*u),(int)(26*u),0xFFFFFFFF,0.4f); } } }
  void frame(float dt){
    if(!started)return; auto t0=std::chrono::steady_clock::now(); if(dt>0.1f)dt=0.1f; if(dt<=0)dt=0.001f;
    { int md=(g.menu?1:0)|(g.paused?2:0)|(g.over?4:0); if(md!=lastMode){ lastMode=md; releaseAll(); } }
    Input in=build(); acc+=dt; int n=std::min(4,(int)(acc*60.f)); float lookX=0,lookY=0;
    if(n>0&&!g.menu&&!g.paused){ float K=0.0036f*(540.f/H); lookX=pdx*K*60.f/n; lookY=-pdy*K*0.85f*60.f/n; pdx=pdy=0; } else if(g.menu||g.paused){ pdx=pdy=0; }
    if(g.qChanged)applyQuality(); int steps=0; while(acc>=1.f/60.f&&steps<4){ Input s=in; s.lx=lookX; s.ly=lookY; g.update(1.f/60.f,s); acc-=1.f/60.f; steps++; } if(steps==4)acc=0;
    if(g.menu)g.renderMenu(); else g.render();
    drawControls(); g.r.flush();
    const uint32_t*fb=g.r.fb.data(); uint32_t*o=out.data(); size_t N=(size_t)W*H; for(size_t i=0;i<N;i++){ uint32_t p=fb[i]; o[i]=0xFF000000u|(p&0x0000FF00u)|((p>>16)&0xFFu)|((p&0xFFu)<<16); }
    float ms=std::chrono::duration<float,std::milli>(std::chrono::steady_clock::now()-t0).count(); costAvg+=(ms-costAvg)*0.08f; fpsAvg+=(1.f/dt-fpsAvg)*0.05f; g.fpsShow=fpsAvg;
    // دقة تلقائية: إن بطؤ الإطار نخفّض الدقة قليلاً، وإن صار سريعاً نرفعها
    if(costAvg>27.f){ slowT+=dt; fastT=0; } else if(costAvg<12.f){ fastT+=dt; slowT=0; } else { slowT=fastT=0; }
    if(slowT>1.5f){ slowT=0; resize((int)(H*0.86f)); costAvg=20; } else if(fastT>4.f&&H<baseH){ fastT=0; resize((int)(H*1.15f)+1); costAvg=16; } }
};
