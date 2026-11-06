// مرسم برمجي (CPU): مضلعات + z-buffer + ضباب + نقش الشبكة كنسخة أندرويد + سماء بشمس
#pragma once
#include "core.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#if defined(__SSE__)
#include <xmmintrin.h>
#include <emmintrin.h>
#endif
#include <algorithm>

#ifdef WZ_PROF
#include <chrono>
static double g_flushMs=0,g_skyMs=0;
#endif
struct Cam { float x,y,z,yaw,pitch,fov; };

struct Pool{
  std::vector<std::thread> th; std::mutex m; std::condition_variable cv,cd; std::function<void()> fn; int gen=0,run=0; bool quit=false;
  ~Pool(){ { std::lock_guard<std::mutex> l(m); quit=true; } cv.notify_all(); for(auto&t:th)t.join(); }
  void start(int n){ for(int i=0;i<n;i++)th.emplace_back([this]{ int seen=0; for(;;){ std::unique_lock<std::mutex> l(m); cv.wait(l,[&]{return quit||gen!=seen;}); if(quit)return; seen=gen; auto f=fn; l.unlock(); f(); l.lock(); if(--run==0)cd.notify_one(); } }); }
  void exec(const std::function<void()>&f){ if(th.empty()){f();return;} { std::lock_guard<std::mutex> l(m); fn=f; run=(int)th.size(); gen++; } cv.notify_all(); f(); std::unique_lock<std::mutex> l(m); cd.wait(l,[&]{return run==0;}); }
};
struct TJ{ float x0,y0,z0,u0,v0,x1,y1,z1,u1,v1,x2,y2,z2,u2,v2,r,g,b; bool det,fog; float ylo,yhi,key; };
struct Renderer {
  Pool pool; int nthreads=0; std::vector<TJ> jobs; bool skyPend=false;
  void startThreads(int n){ if(nthreads||n<=1)return; nthreads=n; pool.start(n-1); }
  int W=0,H=0; std::vector<uint32_t> fb; std::vector<float> zb;
  Cam cam{}; float fx=0,fy=0,fz=0,rx=0,rz=0,ux=0,uy=0,uz=0,focal=1;
  const MapDef* m=nullptr; float fog[3]={0,0,0}, fogN=30, fogF=100; float sunN[3]={0,1,0};
  float powLut[257]; float lineLut[32*32]; float hashTab[256]; int LL[1024],HH[256];
  void init(int w,int h){flush();W=w;H=h;fb.assign(w*h,0);zb.assign(w*h,0);for(int i=0;i<=256;i++)powLut[i]=powf(i/256.f,0.55f); for(int j=0;j<32;j++)for(int i=0;i<32;i++){float fu=(i+0.5f)/32-0.5f,fv=(j+0.5f)/32-0.5f;float gm=std::max<float>(fabsf(fu),fabsf(fv));float ln=gm>0.43f?std::min<float>(1.f,(gm-0.43f)/0.07f):0.f;lineLut[j*32+i]=1.f-0.2f*ln;} for(int i=0;i<256;i++)hashTab[i]=0.9f+0.2f*hash2(i&15,i>>4); for(int i=0;i<1024;i++)LL[i]=(int)(lineLut[i]*256.f+0.5f); for(int i=0;i<256;i++)HH[i]=(int)(hashTab[i]*256.f+0.5f);}
  void setCam(const Cam&c){
    cam=c; float cy=cosf(c.yaw),sy=sinf(c.yaw),cp=cosf(c.pitch),sp=sinf(c.pitch);
    fx=-sy*cp; fy=sp; fz=-cy*cp; rx=cy; rz=-sy;
    ux=-rz*fy; uy=rz*fx-rx*fz; uz=rx*fy;
    focal=(H*0.5f)/tanf(c.fov*0.5f);
  }
  static uint32_t pack(float r,float g,float b){
    auto q=[](float v){int i=(int)(v*255.f+0.5f);return (uint32_t)(i<0?0:i>255?255:i);};
    return 0xFF000000u|(q(r)<<16)|(q(g)<<8)|q(b);
  }
#if defined(__SSE2__)
  static inline uint32_t packv(__m128 v){ v=_mm_add_ps(_mm_mul_ps(v,_mm_set1_ps(255.f)),_mm_set1_ps(0.5f)); __m128i i=_mm_cvttps_epi32(v); i=_mm_packs_epi32(i,i); i=_mm_packus_epi16(i,i); return 0xFF000000u|(uint32_t)_mm_cvtsi128_si32(i); }
#endif
#if defined(__SSE__)
  static inline float frcp(float x){ __m128 v=_mm_set_ss(x); __m128 r=_mm_rcp_ss(v); r=_mm_sub_ss(_mm_add_ss(r,r),_mm_mul_ss(_mm_mul_ss(r,r),v)); return _mm_cvtss_f32(r); }
#else
  static inline float frcp(float x){ return 1.f/x; }
#endif
  void clear_black(){std::fill(fb.begin(),fb.end(),0xFF000000u);}
  void clear(){
    const MapDef&d=*m; float sl=sqrtf(d.sun[0]*d.sun[0]+d.sun[1]*d.sun[1]+d.sun[2]*d.sun[2]); for(int i=0;i<3;i++)sunN[i]=d.sun[i]/sl;
    flush(); clearPend=true; zBias=0;
    for(int i=0;i<3;i++)fog[i]=d.fog[i]; fogN=d.fogN; fogF=d.fogF;
  }
  // السماء تُرسم بعد الهندسة فقط في البكسلات الفارغة (أرخص بكثير)
  void skyAt(float vx,float vy,float&R,float&G,float&B,float&sOut)const{
    const MapDef&d=*m; float dx=fx+vx*rx+vy*ux,dy=fy+vy*uy,dz=fz+vx*rz+vy*uz;
    float q=dx*dx+dy*dy+dz*dz;
#if defined(__SSE__)
    float inv=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(q))); inv=inv*(1.5f-0.5f*q*inv*inv);
#else
    float inv=1.f/sqrtf(q);
#endif
    dx*=inv;dy*=inv;dz*=inv;
    if(dy>=0){ float t=powLut[(int)(std::min<float>(1.f,dy)*256)]; R=d.hor[0]+(d.sky[0]-d.hor[0])*t;G=d.hor[1]+(d.sky[1]-d.hor[1])*t;B=d.hor[2]+(d.sky[2]-d.hor[2])*t; }
    else { float k=std::min<float>(1.f,-dy*4.f)*0.2f; R=d.hor[0]*(1-k);G=d.hor[1]*(1-k);B=d.hor[2]*(1-k); }
    float s=dx*sunN[0]+dy*sunN[1]+dz*sunN[2]; sOut=s;
    if(s>0.55f){ float s2=s*s,s4=s2*s2,s8=s4*s4,s12=s8*s4; float g1=s12*0.12f; float g2=s>0.9985f?1.2f:0; R+=g1+g2;G+=0.92f*g1+g2*0.92f;B+=0.75f*g1+g2*0.75f; }
  }
  // السماء تُرسم أولاً (تعبئة خطية سريعة) ثم تُغطّيها الهندسة
  void fillSky(int by0,int by1){
    float th=tanf(cam.fov*0.5f),asp=(float)W/H; const int BK=8; int nb=(W+BK-1)/BK; std::vector<uint32_t> bc(nb); std::vector<uint8_t> hot(nb);
    for(int y=by0;y<by1;y++){
      uint32_t*row=&fb[y*W];
      if(y%BK==0||y==by0){ int yy=y-y%BK; for(int bx=0;bx<nb;bx++){ float R,G,B,s; skyAt((2.f*(bx*BK+BK*0.5f)/W-1.f)*th*asp,(1.f-2.f*(yy+BK*0.5f)/H)*th,R,G,B,s); bc[bx]=pack(R,G,B); hot[bx]=s>0.996f; } }
      const float vyy=(1.f-2.f*(y+0.5f)/H)*th, vxs=(2.f/W)*th*asp, vx0=(0.5f*2.f/W-1.f)*th*asp;
      for(int bx=0;bx<nb;bx++){ int xa=bx*BK,xb=std::min(W,xa+BK);
        if(!hot[bx]){ uint32_t c=bc[bx]; for(int x=xa;x<xb;x++) row[x]=c; }
        else for(int x=xa;x<xb;x++){ float R,G,B,s; skyAt(vx0+vxs*x,vyy,R,G,B,s); row[x]=pack(R,G,B); }
      }
    }
  }
  // تنفيذ كل المضلعات المسجّلة على شرائح أفقية بالتوازي
  bool clearPend=false; std::vector<int> ord,cnt; 
  void flush(){
    if(gpu){ jobs.clear(); skyPend=false; clearPend=false; return; }
    if(jobs.empty()&&!skyPend&&!clearPend)return; bool sky=skyPend,clr=clearPend; skyPend=false; clearPend=false;
    int bh=std::max(8,((H/(nthreads>1?34:1))+7)/8*8); int nb=(H+bh-1)/bh; std::atomic<int> nxt(0);
    // ترتيب الأقرب أولاً بفرز عدّي O(n) على مفتاح مكمّم (للأداء فقط؛ الصحة يضمنها z-buffer)
    int n=(int)jobs.size(); ord.resize(n); cnt.assign(4097,0);
    for(int i=0;i<n;i++){ int q=(int)(sqrtf(jobs[i].key)*1100.f); if(q>4095)q=4095; q=4095-q; jobs[i].key=(float)q; cnt[q+1]++; }
    for(int i=0;i<4096;i++)cnt[i+1]+=cnt[i];
    for(int i=0;i<n;i++)ord[cnt[(int)jobs[i].key]++]=i;
    auto work=[&]{ for(;;){ int bi=nxt++; if(bi>=nb)break; int y0=bi*bh,y1=std::min(H,y0+bh);
      if(clr)std::fill(zb.begin()+(size_t)y0*W,zb.begin()+(size_t)y1*W,0.f);
      if(sky)fillSky(y0,y1);
      for(int oi=0;oi<n;oi++){ const TJ&j=jobs[ord[oi]]; if(j.yhi<y0||j.ylo>=y1)continue; rast(j,y0,y1); } } };
#ifdef WZ_PROF
    auto t0=std::chrono::steady_clock::now();
#endif
    pool.exec(work);
#ifdef WZ_PROF
    g_flushMs+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
#endif
    jobs.clear();
  }
  struct PV{float x,y,z,u,v;};
  struct Tex{bool on=false; float lr,lg,lb;};  // نقش: اللون مضاء مسبقاً
  // إضاءة وجه (كما في شيدر أندرويد): amb*(0.55+0.45*hemi)+sun*dot
  float lightOf(float nx,float ny,float nz)const{
    const MapDef&d=*m; float dt=nx*sunN[0]+ny*sunN[1]+nz*sunN[2]; if(dt<0)dt=0; float hemi=0.5f+0.5f*ny; float l=d.amb*(0.55f+0.45f*hemi)+d.sunI*dt; return l;
  }
  // مضلع محدب بإحداثيات نقش (u,v بالعالم) اختيارية
  void poly(const float (*p)[3],int n,float r,float g,float b,bool detail=false,const float (*uv)[2]=nullptr,bool useFog=true){
    if(gpu){ gpuPoly(p,n,r,g,b,useFog,detail,uv); return; }
    PV v[8]; int cnt=0; const float NEAR=0.08f; PV c[8];
    for(int i=0;i<n;i++){ float dx=p[i][0]-cam.x,dy=p[i][1]-cam.y,dz=p[i][2]-cam.z;
      c[i]={dx*rx+dz*rz, dx*ux+dy*uy+dz*uz, dx*fx+dy*fy+dz*fz, uv?uv[i][0]:0, uv?uv[i][1]:0}; }
    for(int i=0;i<n;i++){
      const PV&a=c[i],&bb=c[(i+1)%n]; bool ia=a.z>=NEAR,ib=bb.z>=NEAR;
      if(ia&&cnt<8)v[cnt++]=a;
      if(ia!=ib&&cnt<8){float t=(NEAR-a.z)/(bb.z-a.z);v[cnt++]={a.x+(bb.x-a.x)*t,a.y+(bb.y-a.y)*t,NEAR,a.u+(bb.u-a.u)*t,a.v+(bb.v-a.v)*t};}
    }
    if(cnt<3)return;
    float sx[8],sy[8],iz[8],uz[8],vz[8]; float cxs=W*0.5f,cys=H*0.5f;
    for(int i=0;i<cnt;i++){iz[i]=1.f/v[i].z;sx[i]=cxs+v[i].x*iz[i]*focal;sy[i]=cys-v[i].y*iz[i]*focal;uz[i]=v[i].u*iz[i];vz[i]=v[i].v*iz[i];}
    bool det=detail&&uv;
    for(int i=1;i+1<cnt;i++)tri(sx[0],sy[0],iz[0],uz[0],vz[0],sx[i],sy[i],iz[i],uz[i],vz[i],sx[i+1],sy[i+1],iz[i+1],uz[i+1],vz[i+1],r,g,b,det,useFog);
  }
  static float hash2(int a,int b){ uint32_t h=(uint32_t)a*374761393u+(uint32_t)b*668265263u; h=(h^(h>>13))*1274126177u; h^=h>>16; return (h&1023)/1023.f; }
  struct E{float x,z,u,v;};
  static std::vector<TJ>*& tgt(){ static thread_local std::vector<TJ>* t=nullptr; return t; }
  // ===== وضع GPU التجريبي: المضلعات تُجمع كمثلثات (x,y,z,u,v) وتُرسم عبر Piglet؛ اللون = إحداثي في نسيج لوحة ألوان 6-6-6 =====
  bool gpu=false; std::vector<float> gv[2]; std::vector<float> gd[3]; // [0]=العالم ، [1]=مجسّم السلاح ؛ gd = طبقات نقش الشبكة (بحسب الضباب)
  // رأس = {x,y,z,u,v} (u,v إحداثيات النقش التفصيلي بوحدة العالم)
  void gpuTri(const float*a,const float*b,const float*c,float r,float g,float bl,bool useFog,bool det,int dep){
    float cx=(a[0]+b[0]+c[0])*(1.f/3)-cam.x,cy=(a[1]+b[1]+c[1])*(1.f/3)-cam.y,cz=(a[2]+b[2]+c[2])*(1.f/3)-cam.z; float d=sqrtf(cx*cx+cy*cy+cz*cz);
    float f=0; if(useFog){
      const float*P[3]={a,b,c}; float rad=0; for(int i=0;i<3;i++){ float ex=P[i][0]-cam.x-cx,ey=P[i][1]-cam.y-cy,ez=P[i][2]-cam.z-cz; rad=std::max(rad,sqrtf(ex*ex+ey*ey+ez*ez)); }
      float e2=0; for(int i=0;i<3;i++){ const float*p0=P[i],*p1=P[(i+1)%3]; float ex=p0[0]-p1[0],ey=p0[1]-p1[1],ez=p0[2]-p1[2]; e2=std::max(e2,ex*ex+ey*ey+ez*ez); }
      bool grad=(d-rad<fogF)&&(d+rad>fogN);
      if(grad&&dep<7&&e2>14.f*14.f){ float m0[5],m1[5],m2[5]; for(int i=0;i<5;i++){ m0[i]=(a[i]+b[i])*.5f; m1[i]=(b[i]+c[i])*.5f; m2[i]=(c[i]+a[i])*.5f; }
        gpuTri(a,m0,m2,r,g,bl,useFog,det,dep+1); gpuTri(m0,b,m1,r,g,bl,useFog,det,dep+1); gpuTri(m2,m1,c,r,g,bl,useFog,det,dep+1); gpuTri(m0,m1,m2,r,g,bl,useFog,det,dep+1); return; }
      f=(d-fogN)/(fogF-fogN+1e-9f); f=f<0?0:f>1?1:f; }
    float R=r+(fog[0]-r)*f,G=g+(fog[1]-g)*f,B=bl+(fog[2]-bl)*f;
    auto q=[](float v,float m){ int i=(int)(v*m+0.5f); return i<0?0:i>(int)m?(int)m:i; }; int idx=(q(R,127.f)<<13)|(q(G,127.f)<<6)|q(B,63.f); float u=((idx&1023)+0.5f)*(1.f/1024),vv=((idx>>10)+0.5f)*(1.f/1024);
    std::vector<float>&o=gv[zBias>0?1:0]; const float*P[3]={a,b,c}; for(int i=0;i<3;i++){ o.push_back(P[i][0]); o.push_back(P[i][1]); o.push_back(P[i][2]); o.push_back(u); o.push_back(vv); }
    if(det&&f<0.8f){ std::vector<float>&dq=gd[f<0.27f?0:f<0.54f?1:2]; for(int i=0;i<3;i++){ dq.push_back(P[i][0]); dq.push_back(P[i][1]); dq.push_back(P[i][2]); dq.push_back(P[i][3]*(1.f/8)); dq.push_back(P[i][4]*(1.f/8)); } }
  }
  void gpuPoly(const float (*p)[3],int n,float r,float g,float b,bool useFog,bool det,const float (*uv)[2]){ float V[8][5]; if(n>8)n=8; for(int i=0;i<n;i++){ V[i][0]=p[i][0];V[i][1]=p[i][1];V[i][2]=p[i][2]; V[i][3]=uv?uv[i][0]:0.f; V[i][4]=uv?uv[i][1]:0.f; } for(int i=1;i+1<n;i++)gpuTri(V[0],V[i],V[i+1],r,g,b,useFog,det&&uv,0); }
  float zBias=0; // مجسّم السلاح: يُرسم فوق كل شيء بدون مسح مخزن العمق (إزاحة ثابتة في 1/z)
  void tri(float x0,float y0,float z0,float u0,float v0,float x1,float y1,float z1,float u1,float v1,float x2,float y2,float z2,float u2,float v2,float r,float g,float b,bool det,bool useFog){
    float key=std::max(z0,std::max(z1,z2)); z0+=zBias;z1+=zBias;z2+=zBias;
    TJ j={x0,y0,z0,u0,v0,x1,y1,z1,u1,v1,x2,y2,z2,u2,v2,r,g,b,det,useFog,std::min(y0,std::min(y1,y2)),std::max(y0,std::max(y1,y2)),key};
    if(j.yhi<0||j.ylo>=H)return; (tgt()?*tgt():jobs).push_back(j);
  }
  void rast(const TJ&J,int by0,int by1){
    float x0=J.x0,y0=J.y0,z0=J.z0,u0=J.u0,v0=J.v0,x1=J.x1,y1=J.y1,z1=J.z1,u1=J.u1,v1=J.v1,x2=J.x2,y2=J.y2,z2=J.z2,u2=J.u2,v2=J.v2,r=J.r,g=J.g,b=J.b; bool det=J.det,useFog=J.fog;
    auto sw=[&](float&xa,float&ya,float&za,float&ua,float&va,float&xb,float&yb,float&zb2,float&ub,float&vb){std::swap(xa,xb);std::swap(ya,yb);std::swap(za,zb2);std::swap(ua,ub);std::swap(va,vb);};
    if(y1<y0)sw(x0,y0,z0,u0,v0,x1,y1,z1,u1,v1);
    if(y2<y0)sw(x0,y0,z0,u0,v0,x2,y2,z2,u2,v2);
    if(y2<y1)sw(x1,y1,z1,u1,v1,x2,y2,z2,u2,v2);
    int ya=std::max(by0,(int)ceilf(y0)),yb=std::min(by1-1,(int)ceilf(y2)-1);
    if(yb<ya||y2-y0<1e-6f)return;
    float inv02=1.f/(y2-y0);
    const float zDet=det?(1.f/34.f):1e30f, zFog=useFog?(1.f/fogN):0.f; const uint32_t basePk=pack(r,g,b); auto q8=[](float v){int i=(int)(v*255.f+0.5f);return i<0?0:i>255?255:i;}; const int R8=q8(r),G8=q8(g),B8=q8(b);
    const float invFR=1.f/(fogF-fogN+1e-9f), zFar=useFog?(1.f/fogF):-1.f; const uint32_t fogPk=pack(fog[0],fog[1],fog[2]);
#if defined(__SSE2__)
    const __m128 C4=_mm_set_ps(0.f,r,g,b), F4=_mm_set_ps(0.f,fog[0],fog[1],fog[2]);
#endif
    // حواف تزايدية (بدون قسمة لكل سطر): تُهيَّأ مرة لكل (مضلع، شريحة)
    auto c01=[](float t){return t<0.f?0.f:t>1.f?1.f:t;};
    const float fy0=(float)ya+0.5f; const float t0_=c01((fy0-y0)*inv02);
    float exa=x0+(x2-x0)*t0_,eza=z0+(z2-z0)*t0_,eua=u0+(u2-u0)*t0_,eva=v0+(v2-v0)*t0_; const float sxa=(x2-x0)*inv02,sza=(z2-z0)*inv02,sua=(u2-u0)*inv02,sva=(v2-v0)*inv02;
    const float d01=y1-y0,d12=y2-y1,i01=d01>1e-6f?1.f/d01:0.f,i12=d12>1e-6f?1.f/d12:0.f;
    float exb=0,ezb=0,eub=0,evb=0,sxb=0,szb=0,sub=0,svb=0; bool inTop=fy0<y1;
    auto setTop=[&](float fy){ float t=c01((fy-y0)*i01); exb=x0+(x1-x0)*t;ezb=z0+(z1-z0)*t;eub=u0+(u1-u0)*t;evb=v0+(v1-v0)*t; sxb=(x1-x0)*i01;szb=(z1-z0)*i01;sub=(u1-u0)*i01;svb=(v1-v0)*i01; };
    auto setBot=[&](float fy){ float t=c01((fy-y1)*i12); exb=x1+(x2-x1)*t;ezb=z1+(z2-z1)*t;eub=u1+(u2-u1)*t;evb=v1+(v2-v1)*t; sxb=(x2-x1)*i12;szb=(z2-z1)*i12;sub=(u2-u1)*i12;svb=(v2-v1)*i12; };
    if(inTop)setTop(fy0); else setBot(fy0);
    for(int y=ya;y<=yb;y++,exa+=sxa,eza+=sza,eua+=sua,eva+=sva,exb+=sxb,ezb+=szb,eub+=sub,evb+=svb){
      if(inTop){ float fy_=(float)y+0.5f; if(fy_>=y1){ inTop=false; setBot(fy_); } }
      float xa=exa,za=eza,ua=eua,va=eva,xb=exb,zb_=ezb,ub=eub,vb=evb;
      if(xa>xb){std::swap(xa,xb);std::swap(za,zb_);std::swap(ua,ub);std::swap(va,vb);}
      int xs=std::max(0,(int)ceilf(xa)),xe=std::min(W-1,(int)ceilf(xb)-1);
      if(xe<xs)continue;
      float inv=1.f/(xb-xa+1e-6f); float dz=(zb_-za)*inv,du=(ub-ua)*inv,dv=(vb-va)*inv; float o=(float)xs+0.5f-xa; float zi=za+dz*o,ui=ua+du*o,vi=va+dv*o;
      uint32_t*row=&fb[y*W]; float*zr=&zb[y*W];
      // بكسلات بلا ضباب ولا نقش (الأغلب): لون ثابت بلا أي قسمة. الباقي: قسمة دقيقة كل 8 بكسلات ثم استيفاء خطي
      for(int xc=xs;xc<=xe;xc+=8){
        int xn=std::min(xe,xc+7); int n=xn-xc+1; float zc=zi,uc=ui,vc=vi;
        bool have=false; float d0=0,dd=0,u0_=0,du_=0,v0_=0,dv_=0;
        for(int x=xc;x<=xn;x++,zi+=dz,ui+=du,vi+=dv){
          if(zi>zr[x]){ zr[x]=zi;
            if(zi<=zFar){ row[x]=fogPk; continue; }
            if(zi>zDet){ // نقش الشبكة (قريب): فهارس صحيحة بلا فروع ولا تحويل لعوم
              if(!have){ have=true; static const float INVN[9]={0.f,1.f,1.f/2,1.f/3,1.f/4,1.f/5,1.f/6,1.f/7,1.f/8}; float in_=INVN[n]; float rc0=frcp(zc); float zE=zc+dz*n; if(zE<1e-7f)zE=zc; float rc1=frcp(zE); float uE=uc+du*n,vE=vc+dv*n;
                d0=rc0; dd=(rc1-rc0)*in_; u0_=uc*rc0; du_=(uE*rc1-u0_)*in_; v0_=vc*rc0; dv_=(vE*rc1-v0_)*in_; }
              int k=x-xc; int U=(int)((u0_+du_*k)*32.f+8192.f),V=(int)((v0_+dv_*k)*32.f+8192.f);
              int K=(LL[((V&31)<<5)|(U&31)]*HH[(((U>>4)&15)<<4)|((V>>4)&15)])>>8;
              if(zi>=zFog){ int rr=(R8*K)>>8,gg=(G8*K)>>8,bb=(B8*K)>>8; row[x]=0xFF000000u|((uint32_t)(rr>255?255:rr)<<16)|((uint32_t)(gg>255?255:gg)<<8)|(uint32_t)(bb>255?255:bb); }
              else { float d=d0+dd*k; float f=(d-fogN)*invFR; f=f<0?0:f>1?1:f; float kk=(float)K*(1.f/256.f);
#if defined(__SSE2__)
                row[x]=packv(_mm_add_ps(_mm_mul_ps(C4,_mm_set1_ps(kk*(1.f-f))),_mm_mul_ps(F4,_mm_set1_ps(f))));
#else
                row[x]=pack(r*kk+(fog[0]-r*kk)*f,g*kk+(fog[1]-g*kk)*f,b*kk+(fog[2]-b*kk)*f);
#endif
              }
            } else if(zi<zFog){ // بلا نقش لكن فيه ضباب
              if(!have){ have=true; static const float INVN[9]={0.f,1.f,1.f/2,1.f/3,1.f/4,1.f/5,1.f/6,1.f/7,1.f/8}; float in_=INVN[n]; float rc0=frcp(zc); float zE=zc+dz*n; if(zE<1e-7f)zE=zc; float rc1=frcp(zE); float uE=uc+du*n,vE=vc+dv*n;
                d0=rc0; dd=(rc1-rc0)*in_; u0_=uc*rc0; du_=(uE*rc1-u0_)*in_; v0_=vc*rc0; dv_=(vE*rc1-v0_)*in_; }
              int k=x-xc; float d=d0+dd*k; float f=(d-fogN)*invFR; f=f<0?0:f>1?1:f;
#if defined(__SSE2__)
              row[x]=packv(_mm_add_ps(_mm_mul_ps(C4,_mm_set1_ps(1.f-f)),_mm_mul_ps(F4,_mm_set1_ps(f))));
#else
              row[x]=pack(r+(fog[0]-r)*f,g+(fog[1]-g)*f,b+(fog[2]-b)*f);
#endif
            } else row[x]=basePk;
          }
        }
      }
    }
  }
  static void rot(float yaw,float pitch,float x,float y,float z,float*o){
    float cp=cosf(pitch),sp=sinf(pitch); float y1=y*cp-z*sp,z1=y*sp+z*cp;
    float c=cosf(yaw),s=sinf(yaw); o[0]=x*c+z1*s; o[1]=y1; o[2]=-x*s+z1*c;
  }
  float obbRoll=0;
  void obb(float cx,float cy,float cz,float hx,float hy,float hz,float yaw,float pitch,float r,float g,float b,bool fogOn=true,bool lit=true){
    float P[8][3];
    float rc=1,rs=0; if(obbRoll!=0){rc=cosf(obbRoll);rs=sinf(obbRoll);}
    for(int i=0;i<8;i++){float o[3];float lx=(i&1?hx:-hx),ly=(i&2?hy:-hy);if(obbRoll!=0){float nx=lx*rc-ly*rs;ly=lx*rs+ly*rc;lx=nx;}rot(yaw,pitch,lx,ly,(i&4?hz:-hz),o);P[i][0]=cx+o[0];P[i][1]=cy+o[1];P[i][2]=cz+o[2];}
    static const int F[6][4]={{0,2,3,1},{4,5,7,6},{0,4,6,2},{1,3,7,5},{0,1,5,4},{2,6,7,3}};
    static const float N[6][3]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
    for(int f=0;f<6;f++){
      float n[3];{float nx0=N[f][0],ny0=N[f][1];if(obbRoll!=0){float nx=nx0*rc-ny0*rs;ny0=nx0*rs+ny0*rc;nx0=nx;}rot(yaw,pitch,nx0,ny0,N[f][2],n);}
      const float*q=P[F[f][0]]; if((cam.x-q[0])*n[0]+(cam.y-q[1])*n[1]+(cam.z-q[2])*n[2]<=0)continue;
      float l=lit?std::min<float>(1.25f,lightOf(n[0],n[1],n[2])):1.f; float pts[4][3]; for(int k=0;k<4;k++)memcpy(pts[k],P[F[f][k]],12);
      poly(pts,4,r*l,g*l,b*l,false,nullptr,fogOn);
    }
  }
  void aabb(const Box&b,float r,float g,float b2){
    float x0=b.x0,x1=b.x1,y0=b.y0,y1=b.y1,z0=b.z0,z1=b.z1;
    auto face=[&](float nx,float ny,float nz,const float a[3],const float bq[3],const float c[3],const float d[3],int ax){
      float p[4][3]; memcpy(p[0],a,12);memcpy(p[1],bq,12);memcpy(p[2],c,12);memcpy(p[3],d,12);
      float uv[4][2]; for(int k=0;k<4;k++){ if(ax==0){uv[k][0]=p[k][2];uv[k][1]=p[k][1];} else if(ax==2){uv[k][0]=p[k][0];uv[k][1]=p[k][1];} else {uv[k][0]=p[k][0];uv[k][1]=p[k][2];} }
      float l=lightOf(nx,ny,nz); poly(p,4,r*l,g*l,b2*l,true,uv);};
    if(cam.x<x0){float a[3]={x0,y0,z0},bq[3]={x0,y0,z1},c[3]={x0,y1,z1},d[3]={x0,y1,z0};face(-1,0,0,a,bq,c,d,0);}
    if(cam.x>x1){float a[3]={x1,y0,z1},bq[3]={x1,y0,z0},c[3]={x1,y1,z0},d[3]={x1,y1,z1};face(1,0,0,a,bq,c,d,0);}
    if(cam.z<z0){float a[3]={x1,y0,z0},bq[3]={x0,y0,z0},c[3]={x0,y1,z0},d[3]={x1,y1,z0};face(0,0,-1,a,bq,c,d,2);}
    if(cam.z>z1){float a[3]={x0,y0,z1},bq[3]={x1,y0,z1},c[3]={x1,y1,z1},d[3]={x0,y1,z1};face(0,0,1,a,bq,c,d,2);}
    if(cam.y>y1){float a[3]={x0,y1,z1},bq[3]={x1,y1,z1},c[3]={x1,y1,z0},d[3]={x0,y1,z0};face(0,1,0,a,bq,c,d,1);}
    if(cam.y<y0){float a[3]={x0,y0,z0},bq[3]={x1,y0,z0},c[3]={x1,y0,z1},d[3]={x0,y0,z1};face(0,-1,0,a,bq,c,d,1);}
  }
  float cullHalf=1.f;
  bool cull(const Box&b){
    float cx=(b.x0+b.x1)*.5f,cy=(b.y0+b.y1)*.5f,cz=(b.z0+b.z1)*.5f;
    float rr=0.5f*sqrtf((b.x1-b.x0)*(b.x1-b.x0)+(b.y1-b.y0)*(b.y1-b.y0)+(b.z1-b.z0)*(b.z1-b.z0));
    float dx=cx-cam.x,dy=cy-cam.y,dz=cz-cam.z; float d=sqrtf(dx*dx+dy*dy+dz*dz);
    if(d-rr>fogF)return true;
    float zf=dx*fx+dy*fy+dz*fz; if(zf+rr<0)return true;
    float half=cullHalf; float xr=dx*rx+dz*rz;
    if(zf>0&&fabsf(xr)-rr>zf*half+rr)return true;
    return false;
  }
  void world(const World&w){
    m=w.m; clear(); skyPend=true; cullHalf=tanf(cam.fov*0.5f)*(float)W/H*1.05f+0.2f;
    auto col=[&](const Box&b,float*o){int k=b.k%m->ncol;for(int i=0;i<3;i++)o[i]=m->cols[k][i];};
    // الأقرب أولاً: يرفض اختبار العمق البكسلات المحجوبة بسرعة
    order.clear(); auto dist2=[&](const Box&b){float cx=(b.x0+b.x1)*.5f-cam.x,cz=(b.z0+b.z1)*.5f-cam.z;return cx*cx+cz*cz;};
    for(int i=0;i<m->nd;i++){const Box&b=m->deco[i];if(b.x1==b.x0||cull(b))continue;order.push_back({dist2(b),&b});}
    for(auto&b:w.boxes){if(cull(b))continue;order.push_back({dist2(b),&b});}
    std::sort(order.begin(),order.end(),[](const OB&a,const OB&b){return a.d<b.d;});
    { int N=(int)order.size(); int nch=nthreads>1?24:1; if((int)chunkJobs.size()<nch)chunkJobs.resize(nch); std::atomic<int> nx(0);
      auto gen=[&]{ for(;;){ int c=nx++; if(c>=nch)break; int i0=N*c/nch,i1=N*(c+1)/nch; auto&v=chunkJobs[c]; v.clear(); tgt()=&v; for(int i=i0;i<i1;i++){ const OB&o=order[i]; float cc[3]; col(*o.b,cc); aabb(*o.b,cc[0],cc[1],cc[2]); } tgt()=nullptr; } };
      pool.exec(gen); size_t tot=jobs.size(); for(int c=0;c<nch;c++)tot+=chunkJobs[c].size(); jobs.reserve(tot); for(int c=0;c<nch;c++){ jobs.insert(jobs.end(),chunkJobs[c].begin(),chunkJobs[c].end()); } }
    float W2=m->W/2+60,D2=m->D/2+60; float g[4][3]={{-W2,0,-D2},{-W2,0,D2},{W2,0,D2},{W2,0,-D2}}; float uv[4][2]={{-W2,-D2},{-W2,D2},{W2,D2},{W2,-D2}};
    float l=lightOf(0,1,0); poly(g,4,m->ground[0]*l,m->ground[1]*l,m->ground[2]*l,true,uv);
  }
  struct OB{float d;const Box*b;}; std::vector<OB> order; std::vector<std::vector<TJ>> chunkJobs;
};
