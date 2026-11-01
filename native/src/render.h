// مرسم برمجي (CPU): مضلعات + z-buffer + ضباب + نقش الشبكة كنسخة أندرويد + سماء بشمس
#pragma once
#include "core.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

struct Cam { float x,y,z,yaw,pitch,fov; };

struct Pool{
  std::vector<std::thread> th; std::mutex m; std::condition_variable cv,cd; std::function<void()> fn; int gen=0,run=0; bool quit=false;
  ~Pool(){ { std::lock_guard<std::mutex> l(m); quit=true; } cv.notify_all(); for(auto&t:th)t.join(); }
  void start(int n){ for(int i=0;i<n;i++)th.emplace_back([this]{ int seen=0; for(;;){ std::unique_lock<std::mutex> l(m); cv.wait(l,[&]{return quit||gen!=seen;}); if(quit)return; seen=gen; auto f=fn; l.unlock(); f(); l.lock(); if(--run==0)cd.notify_one(); } }); }
  void exec(const std::function<void()>&f){ if(th.empty()){f();return;} { std::lock_guard<std::mutex> l(m); fn=f; run=(int)th.size(); gen++; } cv.notify_all(); f(); std::unique_lock<std::mutex> l(m); cd.wait(l,[&]{return run==0;}); }
};
struct TJ{ float x0,y0,z0,u0,v0,x1,y1,z1,u1,v1,x2,y2,z2,u2,v2,r,g,b; bool det,fog; };
struct Renderer {
  Pool pool; int nthreads=0; std::vector<TJ> jobs; bool skyPend=false;
  void startThreads(int n){ if(nthreads||n<=1)return; nthreads=n; pool.start(n-1); }
  int W=0,H=0; std::vector<uint32_t> fb; std::vector<float> zb;
  Cam cam{}; float fx=0,fy=0,fz=0,rx=0,rz=0,ux=0,uy=0,uz=0,focal=1;
  const MapDef* m=nullptr; float fog[3]={0,0,0}, fogN=30, fogF=100; float sunN[3]={0,1,0};
  float powLut[257]; float lineLut[32*32]; float hashTab[256];
  void init(int w,int h){flush();W=w;H=h;fb.assign(w*h,0);zb.assign(w*h,0);for(int i=0;i<=256;i++)powLut[i]=powf(i/256.f,0.55f); for(int j=0;j<32;j++)for(int i=0;i<32;i++){float fu=(i+0.5f)/32-0.5f,fv=(j+0.5f)/32-0.5f;float gm=std::max<float>(fabsf(fu),fabsf(fv));float ln=gm>0.43f?std::min<float>(1.f,(gm-0.43f)/0.07f):0.f;lineLut[j*32+i]=1.f-0.2f*ln;} for(int i=0;i<256;i++)hashTab[i]=0.9f+0.2f*hash2(i&15,i>>4);}
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
  void clear_black(){std::fill(fb.begin(),fb.end(),0xFF000000u);}
  void clear(){
    const MapDef&d=*m; float sl=sqrtf(d.sun[0]*d.sun[0]+d.sun[1]*d.sun[1]+d.sun[2]*d.sun[2]); for(int i=0;i<3;i++)sunN[i]=d.sun[i]/sl;
    flush(); std::fill(zb.begin(),zb.end(),0.f);
    for(int i=0;i<3;i++)fog[i]=d.fog[i]; fogN=d.fogN; fogF=d.fogF;
  }
  // السماء تُرسم بعد الهندسة فقط في البكسلات الفارغة (أرخص بكثير)
  void skyAt(float vx,float vy,float&R,float&G,float&B,float&sOut)const{
    const MapDef&d=*m; float dx=fx+vx*rx+vy*ux,dy=fy+vy*uy,dz=fz+vx*rz+vy*uz;
    float inv=1.f/sqrtf(dx*dx+dy*dy+dz*dz); dx*=inv;dy*=inv;dz*=inv;
    if(dy>=0){ float t=powLut[(int)(std::min<float>(1.f,dy)*256)]; R=d.hor[0]+(d.sky[0]-d.hor[0])*t;G=d.hor[1]+(d.sky[1]-d.hor[1])*t;B=d.hor[2]+(d.sky[2]-d.hor[2])*t; }
    else { float k=std::min<float>(1.f,-dy*4.f)*0.2f; R=d.hor[0]*(1-k);G=d.hor[1]*(1-k);B=d.hor[2]*(1-k); }
    float s=dx*sunN[0]+dy*sunN[1]+dz*sunN[2]; sOut=s;
    if(s>0.55f){ float s2=s*s,s4=s2*s2,s8=s4*s4,s12=s8*s4; float g1=s12*0.12f; float g2=s>0.9985f?1.2f:0; R+=g1+g2;G+=0.92f*g1+g2*0.92f;B+=0.75f*g1+g2*0.75f; }
  }
  void fillSky(int by0,int by1){
    float th=tanf(cam.fov*0.5f),asp=(float)W/H; const int BK=4; int nb=(W+BK-1)/BK; std::vector<uint32_t> bc(nb); std::vector<uint8_t> hot(nb);
    for(int y=by0;y<by1;y++){
      uint32_t*row=&fb[y*W]; const float*zr=&zb[y*W];
      if(y%BK==0||y==by0){ int yy=y-y%BK; for(int bx=0;bx<nb;bx++){ float R,G,B,s; skyAt((2.f*(bx*BK+BK*0.5f)/W-1.f)*th*asp,(1.f-2.f*(yy+BK*0.5f)/H)*th,R,G,B,s); bc[bx]=pack(R,G,B); hot[bx]=s>0.94f; } }
      for(int x=0;x<W;x++){ if(zr[x]>0)continue; int bx=x/BK;
        if(hot[bx]){ float R,G,B,s; skyAt((2.f*(x+0.5f)/W-1.f)*th*asp,(1.f-2.f*(y+0.5f)/H)*th,R,G,B,s); row[x]=pack(R,G,B); } else row[x]=bc[bx]; }
    }
  }
  // تنفيذ كل المضلعات المسجّلة على شرائح أفقية بالتوازي
  void flush(){
    if(jobs.empty()&&!skyPend)return; bool sky=skyPend; skyPend=false;
    int bh=std::max(4,((H/(nthreads>1?20:1))+3)/4*4); int nb=(H+bh-1)/bh; std::atomic<int> nxt(0);
    auto work=[&]{ for(;;){ int bi=nxt++; if(bi>=nb)break; int y0=bi*bh,y1=std::min(H,y0+bh);
      for(const TJ&j:jobs){ float lo=std::min(j.y0,std::min(j.y1,j.y2)),hi=std::max(j.y0,std::max(j.y1,j.y2)); if(hi<y0||lo>=y1)continue; rast(j,y0,y1); }
      if(sky)fillSky(y0,y1); } };
    pool.exec(work); jobs.clear();
  }
  struct PV{float x,y,z,u,v;};
  struct Tex{bool on=false; float lr,lg,lb;};  // نقش: اللون مضاء مسبقاً
  // إضاءة وجه (كما في شيدر أندرويد): amb*(0.55+0.45*hemi)+sun*dot
  float lightOf(float nx,float ny,float nz)const{
    const MapDef&d=*m; float dt=nx*sunN[0]+ny*sunN[1]+nz*sunN[2]; if(dt<0)dt=0; float hemi=0.5f+0.5f*ny; float l=d.amb*(0.55f+0.45f*hemi)+d.sunI*dt; return l;
  }
  // مضلع محدب بإحداثيات نقش (u,v بالعالم) اختيارية
  void poly(const float (*p)[3],int n,float r,float g,float b,bool detail=false,const float (*uv)[2]=nullptr,bool useFog=true){
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
  void tri(float x0,float y0,float z0,float u0,float v0,float x1,float y1,float z1,float u1,float v1,float x2,float y2,float z2,float u2,float v2,float r,float g,float b,bool det,bool useFog){
    float t0=y0; (void)t0; jobs.push_back({x0,y0,z0,u0,v0,x1,y1,z1,u1,v1,x2,y2,z2,u2,v2,r,g,b,det,useFog});
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
    for(int y=ya;y<=yb;y++){
      float fy_=(float)y+0.5f; if(fy_<y0)fy_=y0;
      float t02=std::min<float>(1.f,std::max<float>(0.f,(fy_-y0)*inv02));
      float xa=x0+(x2-x0)*t02,za=z0+(z2-z0)*t02,ua=u0+(u2-u0)*t02,va=v0+(v2-v0)*t02,xb,zb_,ub,vb;
      float t; if(fy_<y1){float d=y1-y0; t=d>1e-6f?std::min<float>(1.f,std::max<float>(0.f,(fy_-y0)/d)):0; xb=x0+(x1-x0)*t;zb_=z0+(z1-z0)*t;ub=u0+(u1-u0)*t;vb=v0+(v1-v0)*t;}
      else{float d=y2-y1; t=d>1e-6f?std::min<float>(1.f,std::max<float>(0.f,(fy_-y1)/d)):0; xb=x1+(x2-x1)*t;zb_=z1+(z2-z1)*t;ub=u1+(u2-u1)*t;vb=v1+(v2-v1)*t;}
      if(xa>xb){std::swap(xa,xb);std::swap(za,zb_);std::swap(ua,ub);std::swap(va,vb);}
      int xs=std::max(0,(int)ceilf(xa)),xe=std::min(W-1,(int)ceilf(xb)-1);
      if(xe<xs)continue;
      float inv=1.f/(xb-xa+1e-6f); float dz=(zb_-za)*inv,du=(ub-ua)*inv,dv=(vb-va)*inv; float o=(float)xs+0.5f-xa; float zi=za+dz*o,ui=ua+du*o,vi=va+dv*o;
      uint32_t*row=&fb[y*W]; float*zr=&zb[y*W];
      for(int x=xs;x<=xe;x++,zi+=dz,ui+=du,vi+=dv){
        if(zi>zr[x]){ zr[x]=zi; float d=1.f/zi; float R=r,G=g,B=b;
          if(det&&d<34.f){ float u=ui*d,v=vi*d; float fl=floorf(u),fm=floorf(v); int a=(int)((u-fl)*32.f),bq=(int)((v-fm)*32.f); int iu=(int)floorf(u*2),iv=(int)floorf(v*2);
            float k=lineLut[(bq&31)*32+(a&31)]*hashTab[((iu&15)<<4)|(iv&15)]; R*=k;G*=k;B*=k; }
          if(useFog){ float f=(d-fogN)/(fogF-fogN); f=f<0?0:f>1?1:f; R+=(fog[0]-R)*f;G+=(fog[1]-G)*f;B+=(fog[2]-B)*f; }
          row[x]=pack(R,G,B);
        }
      }
    }
  }
  static void rot(float yaw,float pitch,float x,float y,float z,float*o){
    float cp=cosf(pitch),sp=sinf(pitch); float y1=y*cp-z*sp,z1=y*sp+z*cp;
    float c=cosf(yaw),s=sinf(yaw); o[0]=x*c+z1*s; o[1]=y1; o[2]=-x*s+z1*c;
  }
  void obb(float cx,float cy,float cz,float hx,float hy,float hz,float yaw,float pitch,float r,float g,float b,bool fogOn=true,bool lit=true){
    float P[8][3];
    for(int i=0;i<8;i++){float o[3];rot(yaw,pitch,(i&1?hx:-hx),(i&2?hy:-hy),(i&4?hz:-hz),o);P[i][0]=cx+o[0];P[i][1]=cy+o[1];P[i][2]=cz+o[2];}
    static const int F[6][4]={{0,2,3,1},{4,5,7,6},{0,4,6,2},{1,3,7,5},{0,1,5,4},{2,6,7,3}};
    static const float N[6][3]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
    for(int f=0;f<6;f++){
      float n[3];rot(yaw,pitch,N[f][0],N[f][1],N[f][2],n);
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
  bool cull(const Box&b){
    float cx=(b.x0+b.x1)*.5f,cy=(b.y0+b.y1)*.5f,cz=(b.z0+b.z1)*.5f;
    float rr=0.5f*sqrtf((b.x1-b.x0)*(b.x1-b.x0)+(b.y1-b.y0)*(b.y1-b.y0)+(b.z1-b.z0)*(b.z1-b.z0));
    float dx=cx-cam.x,dy=cy-cam.y,dz=cz-cam.z; float d=sqrtf(dx*dx+dy*dy+dz*dz);
    if(d-rr>fogF)return true;
    float zf=dx*fx+dy*fy+dz*fz; if(zf+rr<0)return true;
    float half=tanf(cam.fov*0.5f)*(float)W/H*1.05f+0.2f; float xr=dx*rx+dz*rz;
    if(zf>0&&fabsf(xr)-rr>zf*half+rr)return true;
    return false;
  }
  void world(const World&w){
    m=w.m; clear(); skyPend=true;
    auto col=[&](const Box&b,float*o){int k=b.k%m->ncol;for(int i=0;i<3;i++)o[i]=m->cols[k][i];};
    // الأقرب أولاً: يرفض اختبار العمق البكسلات المحجوبة بسرعة
    order.clear(); auto dist2=[&](const Box&b){float cx=(b.x0+b.x1)*.5f-cam.x,cz=(b.z0+b.z1)*.5f-cam.z;return cx*cx+cz*cz;};
    for(int i=0;i<m->nd;i++){const Box&b=m->deco[i];if(b.x1==b.x0||cull(b))continue;order.push_back({dist2(b),&b});}
    for(auto&b:w.boxes){if(cull(b))continue;order.push_back({dist2(b),&b});}
    std::sort(order.begin(),order.end(),[](const OB&a,const OB&b){return a.d<b.d;});
    for(auto&o:order){float c[3];col(*o.b,c);aabb(*o.b,c[0],c[1],c[2]);}
    float W2=m->W/2+60,D2=m->D/2+60; float g[4][3]={{-W2,0,-D2},{-W2,0,D2},{W2,0,D2},{W2,0,-D2}}; float uv[4][2]={{-W2,-D2},{-W2,D2},{W2,D2},{W2,-D2}};
    float l=lightOf(0,1,0); poly(g,4,m->ground[0]*l,m->ground[1]*l,m->ground[2]*l,true,uv);
  }
  struct OB{float d;const Box*b;}; std::vector<OB> order;
};
