// محرك صوت برمجي بسيط: مؤثرات مولَّدة (بلا ملفات) تُمزج إلى S16 ستيريو 48kHz
#pragma once
#include <cstdint>
#include <cmath>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstring>
namespace SFX{
  enum{SHOT=0,SHOTGUN,EXPLODE,HIT,HURT,KILL,EMPTY,RELOAD,STEP,SNIPER,PISTOL,N};
  static std::atomic<float> engSpeed{-1.f},droneVol{0.f}; static float engPh=0,drPh=0,engN=0;
  struct Voice{ int id; int pos; int len; float vol,pan; float fp,rate; };
  static const int MAXV=24;
  static Voice vs[MAXV]; static std::atomic_flag lk=ATOMIC_FLAG_INIT; static unsigned rs=12345;
  static inline float rnd(){ rs=rs*1664525u+1013904223u; return ((rs>>9)&0xFFFF)/32768.f-1.f; }
  static inline void lock(){ while(lk.test_and_set(std::memory_order_acquire)){} }
  static inline void unlock(){ lk.clear(std::memory_order_release); }

  // ===== عينات مولَّدة بالمعالجة الرقمية (نفس وصفة public/js/synth.js) =====
  // الأرقام: 0 مسدس 1 ماغنوم 2 بندقية 3 رشاش 4 رشاش ثقيل 5 قناص-شبه 6 قناصة 7 شوتغن | 8 كاتم | 9.. ميكانيكا
  static const int BUF_BASE=100, B_SUPP=8, B_MAGOUT=9, B_MAGIN=10, B_BOLT=11, B_SLAM=12, B_SHELL=13, B_PUMP=14, B_SLIDE=15, NBUF=16;
  static std::vector<float> BUFS[NBUF];
  struct SP{ float dur,bt,hi,tf,tt,ta,tail,tl,ck; int ne; float ec[5][2]; float mech,bolt,pump; };
  static const SP SPAR[8]={
    {0.75f,0.028f,7000,190,0.05f,0.62f,0.3f,0.18f,1.0f,3,{{0.055f,0.3f},{0.13f,0.18f},{0.26f,0.09f}},0.5f,0,0},
    {1.5f,0.06f,6000,120,0.12f,0.95f,0.55f,0.3f,1.2f,4,{{0.07f,0.35f},{0.16f,0.25f},{0.32f,0.15f},{0.6f,0.08f}},0.2f,0,0},
    {1.1f,0.04f,7500,150,0.07f,0.8f,0.45f,0.24f,1.1f,4,{{0.06f,0.3f},{0.14f,0.22f},{0.3f,0.12f},{0.55f,0.06f}},0.8f,0,0},
    {0.75f,0.026f,8000,170,0.05f,0.6f,0.28f,0.2f,1.0f,3,{{0.05f,0.3f},{0.12f,0.18f},{0.24f,0.08f}},0.8f,0,0},
    {1.3f,0.055f,6500,120,0.1f,0.95f,0.5f,0.26f,1.1f,4,{{0.06f,0.32f},{0.15f,0.24f},{0.32f,0.14f},{0.6f,0.07f}},0.9f,0,0},
    {1.5f,0.07f,6500,105,0.12f,1.0f,0.65f,0.3f,1.2f,4,{{0.07f,0.34f},{0.16f,0.25f},{0.34f,0.14f},{0.65f,0.07f}},0.6f,0,0},
    {2.2f,0.1f,5500,80,0.18f,1.2f,0.9f,0.36f,1.3f,5,{{0.09f,0.4f},{0.2f,0.3f},{0.4f,0.2f},{0.8f,0.12f},{1.3f,0.06f}},0.0f,0.62f,0},
    {1.5f,0.09f,4500,90,0.14f,1.1f,0.5f,0.26f,0.9f,4,{{0.07f,0.34f},{0.16f,0.24f},{0.34f,0.13f},{0.6f,0.07f}},0.0f,0,0.45f}};
  static const float TWO_PI=6.2831853f;
  struct Rn{ uint32_t s; float next(){ s^=s<<13; s^=s>>17; s^=s<<5; return (s/4294967296.f)*2.f-1.f; } };
  static void gRing(std::vector<float>&o,float sr,float t0,float f,float tau,float amp,const float*parts,const float*gains,int np){
    int s0=(int)(t0*sr); int n=std::min((int)o.size()-s0,(int)(tau*7*sr)); if(n<=0)return;
    for(int k=0;k<np;k++){ float w=TWO_PI*f*parts[k]/sr,g=amp*gains[k],d=expf(-1.f/sr/(tau/(1+k*0.6f))),e=g; for(int i=0;i<n;i++){ o[s0+i]+=sinf(w*i)*e; e*=d; } } }
  static void ring2(std::vector<float>&o,float sr,float t0,float f,float tau,float amp){ static const float P[3]={1,2.76f,5.4f},G[3]={1,0.5f,0.25f}; gRing(o,sr,t0,f,tau,amp,P,G,3); }
  static void ring2b(std::vector<float>&o,float sr,float t0,float f,float tau,float amp,float p2,float g2){ const float P[2]={1,p2},G[2]={1,g2}; gRing(o,sr,t0,f,tau,amp,P,G,2); }
  static void gTick(std::vector<float>&o,float sr,float t0,float tau,float amp,Rn&r,float hp=0.5f){
    int s0=(int)(t0*sr); int n=std::min((int)o.size()-s0,(int)(tau*8*sr)); if(n<=0)return; float p=0,pw=0,e=amp,d=expf(-1.f/sr/tau);
    for(int i=0;i<n;i++){ float w=r.next(); p=hp*(p+w-pw); pw=w; o[s0+i]+=p*e; e*=d; } }
  static void gThump(std::vector<float>&o,float sr,float t0,float f0,float tau,float amp){
    int s0=(int)(t0*sr); int n=std::min((int)o.size()-s0,(int)(tau*6*sr)); if(n<=0)return; float ph=0,fe=f0,e=amp,at=1,df=expf(-1.f/sr/(tau*0.8f)),de=expf(-1.f/sr/tau),da=expf(-1.f/sr/0.002f);
    for(int i=0;i<n;i++){ ph+=TWO_PI*(fe+40)/sr; o[s0+i]+=sinf(ph)*e*(1-at); fe*=df; e*=de; at*=da; } }
  static void gGrate(std::vector<float>&o,float sr,float t0,float dur,float amp,Rn&r,float rate,float fc){
    int s0=(int)(t0*sr); int n=std::min((int)o.size()-s0,(int)(dur*sr)); if(n<=0)return; float lp=0,k=std::min(0.95f,TWO_PI*fc/sr);
    for(int i=0;i<n;i++){ float t=i/sr,w=r.next(); lp+=k*(w-lp); float am=0.35f+0.65f*std::max(0.f,sinf(TWO_PI*rate*t)),env=sinf(3.14159265f*i/n); o[s0+i]+=(w-lp)*0.6f*am*env*amp+lp*0.1f*env*amp; } }
  static void gFinish(std::vector<float>&o,float sr,float drive,float peak){
    float mx=0; for(auto&v:o){ v=tanhf(v*drive); mx=std::max(mx,fabsf(v)); } float g=peak/(mx>0?mx:1); int fo=(int)(0.03f*sr),N=(int)o.size();
    for(int i=0;i<N;i++){ o[i]*=g; if(i>N-fo)o[i]*=(float)(N-i)/fo; } }
  static void renderShot(int kind,uint32_t seed,bool supp,std::vector<float>&out){
    const float sr=48000.f; const SP&P=SPAR[kind]; Rn rn{seed*7919u+(uint32_t)(kind*104729+13)}; if(!rn.s)rn.s=1;
    int N=(int)(P.dur*sr); out.assign(N,0.f); std::vector<float> core(N,0.f);
    bool heavy=!(kind==0||kind==3||kind==1);
    float lo=0,bp=0,hpP=0,hpW=0,ph=0; float bt=supp?P.bt*1.25f:P.bt,hi=supp?1800.f:P.hi;
    float dCk=expf(-1/sr/0.0012f),dFc=expf(-1/sr/(bt*1.3f)),dBl=expf(-1/sr/bt),dBa=expf(-1/sr/0.0004f),dTf=expf(-1/sr/0.05f),dTh=expf(-1/sr/P.tt),dTa=expf(-1/sr/0.0015f);
    float eCk=1,eFc=1,eBl=1,eBa=1,eTf=1,eTh=1,eTa=1,bAmp=supp?0.9f:1.7f,tAmp=P.ta*(supp?0.55f:1.f);
    for(int i=0;i<N;i++){ float w=rn.next(); hpP=0.55f*(hpP+w-hpW); hpW=w; float crack=supp?0.f:hpP*eCk*P.ck*1.6f;
      float fc=450+(hi-450)*eFc,f=std::min(1.2f,TWO_PI*fc/sr); lo+=f*bp; float hp2=w-lo-0.8f*bp; bp+=f*hp2; float blast=lo*eBl*bAmp*(1-eBa);
      ph+=TWO_PI*(P.tf*eTf+42)/sr; float th=sinf(ph)*eTh*tAmp*(1-eTa); core[i]=crack+blast+th;
      eCk*=dCk; eFc*=dFc; eBl*=dBl; eBa*=dBa; eTf*=dTf; eTh*=dTh; eTa*=dTa; }
    float tl=0,tailTau=supp?P.tail*0.5f:P.tail,tailAmp=supp?P.tl*0.6f:P.tl,dF=expf(-1/sr/0.25f),dT=expf(-1/sr/tailTau),dA=expf(-1/sr/0.01f),eF=1,eT=1,eA=1;
    for(int i=0;i<N;i++){ float fc=300+900*eF; tl+=std::min(0.9f,TWO_PI*fc/sr)*(rn.next()-tl); out[i]=core[i]+tl*eT*tailAmp*2.2f*(1-eA); eF*=dF; eT*=dT; eA*=dA; }
    for(int e=0;e<P.ne;e++){ int d=(int)(P.ec[e][0]*sr*(0.95f+0.1f*fabsf(rn.next()))); float g=P.ec[e][1]*(supp?0.5f:1.f),k=std::max(0.06f,0.45f-e*0.09f),lp=0;
      for(int i=d;i<N;i++){ lp+=k*(core[i-d]-lp); out[i]+=lp*g*(e%2?-1.f:1.f); } }
    float mech=supp?std::max(P.mech,0.5f)*1.6f:P.mech;
    if(mech>0){ gTick(out,sr,0.045f,0.004f,0.25f*mech,rn,0.7f); ring2(out,sr,0.048f,2300,0.02f,0.1f*mech); }
    if(P.bolt>0){ float b=P.bolt; gGrate(out,sr,b,0.1f,0.22f,rn,80,2000); ring2(out,sr,b+0.1f,1300,0.05f,0.16f); gTick(out,sr,b+0.15f,0.004f,0.3f,rn); gThump(out,sr,b+0.15f,130,0.04f,0.3f); }
    if(P.pump>0){ float b=P.pump; gGrate(out,sr,b,0.09f,0.22f,rn,70,1800); gTick(out,sr,b+0.16f,0.004f,0.35f,rn); gThump(out,sr,b+0.16f,110,0.05f,0.4f); ring2(out,sr,b+0.16f,1500,0.04f,0.16f); }
    float shp[3]={0.34f,0.47f,0.55f}; if(kind==0||kind==1){shp[0]=0.28f;shp[1]=0.4f;}
    for(int q=0;q<(heavy?2:3);q++){ ring2b(out,sr,shp[q]+0.02f*rn.next(),4800+600*rn.next(),0.018f,(0.055f-q*0.015f)*(kind==7?1.4f:1.f),1.9f,0.4f); }
    gFinish(out,sr,supp?1.2f:1.5f,0.9f); }
  static void renderMech(int id,std::vector<float>&o){
    const float sr=48000.f; static const float dur[7]={0.35f,0.4f,0.3f,0.25f,0.22f,0.5f,0.45f}; Rn r{(uint32_t)(id*6007u+31337u)}; int N=(int)(dur[id]*sr); o.assign(N,0.f);
    switch(id){
      case 0: gTick(o,sr,0,0.003f,0.7f,r,0.6f); ring2(o,sr,0,1900,0.025f,0.35f); gGrate(o,sr,0.015f,0.11f,0.3f,r,60,1800); ring2b(o,sr,0.14f,820,0.02f,0.2f,2.2f,0.4f); gTick(o,sr,0.15f,0.005f,0.25f,r,0.3f); break;
      case 1: gGrate(o,sr,0,0.1f,0.22f,r,55,1600); gTick(o,sr,0.1f,0.005f,0.8f,r,0.4f); gThump(o,sr,0.1f,170,0.045f,0.7f); ring2(o,sr,0.1f,2400,0.022f,0.3f); gTick(o,sr,0.145f,0.002f,0.5f,r,0.7f); ring2(o,sr,0.145f,3100,0.012f,0.25f); break;
      case 2: gGrate(o,sr,0,0.14f,0.34f,r,90,2200); ring2(o,sr,0,1200,0.06f,0.12f); gTick(o,sr,0.15f,0.004f,0.35f,r,0.6f); gThump(o,sr,0.15f,140,0.04f,0.35f); break;
      case 3: gTick(o,sr,0,0.004f,0.9f,r,0.5f); ring2(o,sr,0,2800,0.04f,0.5f); gThump(o,sr,0,120,0.06f,0.65f); ring2(o,sr,0,1400,0.05f,0.25f); break;
      case 4: gTick(o,sr,0,0.003f,0.5f,r,0.5f); ring2(o,sr,0,3600,0.03f,0.3f); ring2b(o,sr,0,700,0.025f,0.28f,2.4f,0.4f); gTick(o,sr,0.07f,0.002f,0.25f,r,0.6f); break;
      case 5: gGrate(o,sr,0,0.1f,0.3f,r,70,1800); ring2(o,sr,0,1000,0.06f,0.1f); gTick(o,sr,0.17f,0.004f,0.95f,r,0.5f); gThump(o,sr,0.17f,110,0.06f,0.7f); ring2(o,sr,0.17f,1600,0.05f,0.3f); break;
      default: gGrate(o,sr,0,0.09f,0.3f,r,80,2200); gTick(o,sr,0.14f,0.004f,0.85f,r,0.5f); ring2(o,sr,0.14f,2500,0.035f,0.4f); gThump(o,sr,0.14f,150,0.04f,0.5f); break; }
    gFinish(o,sr,1.1f,0.85f); }
  static bool inited=false;
  static inline void initBuffers(){ if(inited)return; inited=true; for(int k=0;k<8;k++)renderShot(k,1,false,BUFS[k]); renderShot(2,1,true,BUFS[B_SUPP]); for(int m=0;m<7;m++)renderMech(m,BUFS[B_MAGOUT+m]); }
  static inline int lenOf(int id){ switch(id){case SHOT:return 9600;case SHOTGUN:return 16000;case EXPLODE:return 48000;case HIT:return 4800;case RELOAD:return 17000;case STEP:return 3000;case SNIPER:return 24000;case PISTOL:return 7200;case HURT:return 8000;case KILL:return 14400;default:return 2400;} }
  // pan: -1..1، vol: 0..1
  static inline void play(int id,float vol=1.f,float pan=0.f){
    if(vol<0.02f)return; lock();
    int k=-1; for(int i=0;i<MAXV;i++)if(vs[i].pos>=vs[i].len){k=i;break;}
    if(k<0){ int b=0; for(int i=1;i<MAXV;i++)if(vs[i].pos>vs[b].pos)b=i; k=b; }
    vs[k]={id,0,lenOf(id),vol>1?1:vol,pan,0.f,1.f}; unlock();
  }

  static inline void startBuf(int bid,float vol,float pan,float rate){ // يُستدعى والقفل ممسوك
    if(bid<0||bid>=NBUF||BUFS[bid].size()<4)return;
    int k=-1; for(int i=0;i<MAXV;i++)if(vs[i].pos>=vs[i].len){k=i;break;}
    if(k<0){ int b=0; for(int i=1;i<MAXV;i++)if(vs[i].pos>vs[b].pos)b=i; k=b; }
    vs[k]={BUF_BASE+bid,0,(int)BUFS[bid].size(),vol>1?1:vol,pan,0.f,rate}; }
  static inline void playBuf(int bid,float vol,float pan=0.f,float rate=1.f){ if(vol<0.02f)return; lock(); startBuf(bid,vol,pan,rate); unlock(); }
  struct Sch{ float t; int bid; float vol,pan; bool on; }; static Sch sch[24];
  static inline void later(int bid,float delay,float vol,float pan=0.f){ lock(); for(auto&q:sch)if(!q.on){ q={delay,bid,vol,pan,true}; break; } unlock(); }
  // طلقة واقعية: kind 0..7 (مسدس،ماغنوم،بندقية،رشاش،ثقيل،شبه قناص،قناصة،شوتغن)
  static inline void shot(int kind,bool supp,float vol,float pan=0.f){ playBuf(supp?B_SUPP:(kind&7),supp?vol*0.6f:vol,pan,0.96f+0.08f*(rnd()*0.5f+0.5f)); }
  // تعبئة بأصوات ميكانيكية متزامنة مع حركة التعبئة (dur بالثواني)
  static inline void reload(int kind,float dur,float vol){
    float v=vol; auto E=[&](int bid,float at){ later(bid,std::max(0.01f,at*dur),v); };
    if(kind==7){ for(int k=0;k<4;k++)E(B_SHELL,0.14f+0.16f*k+0.1f); E(B_PUMP,0.8f); }
    else if(kind==0||kind==1){ E(B_MAGOUT,0.17f); E(B_MAGIN,0.5f); E(B_SLIDE,0.7f); }
    else { E(B_MAGOUT,0.17f); E(B_MAGIN,0.5f); E(B_BOLT,0.69f); E(B_SLAM,0.83f); } }
  static inline float sample(int id,int n,float&st){
    float t=n/48000.f,x=0;
    switch(id){
      case SHOT:{ float e=expf(-t*38.f); st=0.55f*st+0.45f*rnd(); x=(st*0.9f+sinf(t*2*3.14159f*(150-t*400))*0.7f)*e; }break;
      case SHOTGUN:{ float e=expf(-t*20.f); st=0.7f*st+0.3f*rnd(); x=(st*1.1f+sinf(t*2*3.14159f*(90-t*150))*0.8f)*e; }break;
      case EXPLODE:{ float e=expf(-t*4.5f); st=0.97f*st+0.03f*rnd(); x=(st*14.f+sinf(t*2*3.14159f*(55-t*25))*0.9f)*e; if(x>1.2f)x=1.2f; }break;
      case HIT:{ x=sinf(t*2*3.14159f*1500.f)*expf(-t*50.f)*0.7f; }break;
      case HURT:{ float e=expf(-t*18.f); st=0.85f*st+0.15f*rnd(); x=(st*2.f+sinf(t*2*3.14159f*80.f))*e*0.7f; }break;
      case KILL:{ float f=t<0.1f?880.f:1320.f; x=sinf(t*2*3.14159f*f)*expf(-t*9.f)*0.6f; }break;
      case RELOAD:{ float a=expf(-t*60.f)*sinf(t*2*3.14159f*1800.f), b=t>0.18f?expf(-(t-0.18f)*50.f)*sinf((t-0.18f)*2*3.14159f*1100.f):0.f, c=t>0.27f?expf(-(t-0.27f)*70.f)*sinf((t-0.27f)*2*3.14159f*2400.f)*0.8f:0.f; st=0.6f*st+0.4f*rnd(); x=(a+b)*0.6f+c*0.5f+st*0.05f*(t<0.01f); }break;
      case STEP:{ float e=expf(-t*45.f); st=0.8f*st+0.2f*rnd(); x=st*e*0.8f+sinf(t*2*3.14159f*70.f)*e*0.3f; }break;
      case SNIPER:{ float e=expf(-t*9.f); st=0.4f*st+0.6f*rnd(); x=(st*1.2f+sinf(t*2*3.14159f*(70-t*60))*1.0f)*e+(t>0.25f?expf(-(t-0.25f)*40.f)*sinf((t-0.25f)*2*3.14159f*1400.f)*0.3f:0.f); }break;
      case PISTOL:{ float e=expf(-t*55.f); st=0.4f*st+0.6f*rnd(); x=(st*0.9f+sinf(t*2*3.14159f*(260-t*500))*0.5f)*e; }break;
      default:{ x=sinf(t*2*3.14159f*600.f)*expf(-t*80.f)*0.4f; }
    } return x; }
  static float stt[MAXV];
  // يملأ frames إطار ستيريو
  static inline void mix(int16_t*out,int frames){
    float L[512],R[512]; if(frames>512)frames=512;
    for(int i=0;i<frames;i++)L[i]=R[i]=0;
    lock();
    { float dt=frames/48000.f; for(auto&q:sch){ if(!q.on)continue; q.t-=dt; if(q.t<=0){ q.on=false; startBuf(q.bid,q.vol,q.pan,0.97f+0.06f*(rnd()*0.5f+0.5f)); } } }
    for(int v=0;v<MAXV;v++){ Voice&a=vs[v]; if(a.pos>=a.len)continue;
      if(a.id>=BUF_BASE){ const std::vector<float>&B=BUFS[a.id-BUF_BASE]; float gl=a.vol*(a.pan>0?1-a.pan*0.6f:1),gr=a.vol*(a.pan<0?1+a.pan*0.6f:1);
        for(int i=0;i<frames;i++){ int i0=(int)a.fp; if(i0>=a.len-1){ a.pos=a.len; break; } float fr=a.fp-i0,sv=B[i0]*(1-fr)+B[i0+1]*fr; L[i]+=sv*gl; R[i]+=sv*gr; a.fp+=a.rate; }
        continue; }
      float st=stt[v]; if(a.pos==0)st=0;
      float gl=a.vol*(a.pan>0?1-a.pan*0.6f:1),gr=a.vol*(a.pan<0?1+a.pan*0.6f:1);
      for(int i=0;i<frames&&a.pos<a.len;i++,a.pos++){ float s=sample(a.id,a.pos,st); L[i]+=s*gl; R[i]+=s*gr; }
      stt[v]=st; }
    unlock();
    float es=engSpeed.load(),dv=droneVol.load();
    if(es>=0.f){ float f=38.f+es*3.2f; for(int i=0;i<frames;i++){ engPh+=f/48000.f; if(engPh>1)engPh-=1; engN=0.9f*engN+0.1f*rnd(); float sq=(engPh<0.5f?1.f:-1.f)*0.5f+sinf(engPh*6.28318f*2)*0.5f; float v=(sq*0.35f+engN*0.25f)*(0.15f+std::min(1.f,es/26.f)*0.25f); L[i]+=v; R[i]+=v; } }
    if(dv>0.f){ for(int i=0;i<frames;i++){ drPh+=(190.f+sinf(drPh*0.05f)*6.f)/48000.f; if(drPh>1)drPh-=1; float v=((drPh<0.5f?1.f:-1.f)*0.5f+rnd()*0.3f)*0.07f*dv; L[i]+=v; R[i]+=v; } }
    for(int i=0;i<frames;i++){ float l=L[i]*0.5f,r=R[i]*0.5f; l=l>1?1:l<-1?-1:l; r=r>1?1:r<-1?-1:r; out[2*i]=(int16_t)(l*30000); out[2*i+1]=(int16_t)(r*30000); }
  }
}
