// مرسم برمجي (CPU): مضلعات مسطحة الظلال + z-buffer + ضباب
#pragma once
#include "core.h"

struct Cam { float x,y,z,yaw,pitch,fov; };

struct Renderer {
  int W=0,H=0; std::vector<uint32_t> fb; std::vector<float> zb;
  Cam cam{}; float fx=0,fy=0,fz=0,rx=0,rz=0,ux=0,uy=0,uz=0,focal=1;
  const MapDef* m=nullptr; float fog[3]={0,0,0}, fogN=30, fogF=100;
  void init(int w,int h){W=w;H=h;fb.assign(w*h,0);zb.assign(w*h,0);}
  void setCam(const Cam&c){
    cam=c; float cy=std::cos(c.yaw),sy=std::sin(c.yaw),cp=std::cos(c.pitch),sp=std::sin(c.pitch);
    fx=-sy*cp; fy=sp; fz=-cy*cp; rx=cy; rz=-sy;
    // up = right × fwd
    ux=-fy*rz*0+ (0*fz-rz*fy); uy=rz*fx-rx*fz; uz=rx*fy-0*fx;
    focal=(H*0.5f)/std::tan(c.fov*0.5f);
  }
  void clear(){
    const MapDef&d=*m; // سماء متدرجة
    for(int y=0;y<H;y++){
      float t=std::max(0.f,std::min(1.f,(float)y/(H*0.5f+cam.pitch*focal)));
      float r=d.sky[0]+(d.hor[0]-d.sky[0])*t,g=d.sky[1]+(d.hor[1]-d.sky[1])*t,b=d.sky[2]+(d.hor[2]-d.sky[2])*t;
      uint32_t c=pack(r,g,b); for(int x=0;x<W;x++)fb[y*W+x]=c;
    }
    std::fill(zb.begin(),zb.end(),0.f);
    for(int i=0;i<3;i++)fog[i]=d.fog[i]; fogN=d.fogN; fogF=d.fogF;
  }
  static uint32_t pack(float r,float g,float b){
    auto q=[](float v){int i=(int)(v*255.f+0.5f);return (uint32_t)(i<0?0:i>255?255:i);};
    return 0xFF000000u|(q(r)<<16)|(q(g)<<8)|q(b);
  }
  struct PV{float x,y,z;};
  // مضلع محدب في فضاء العالم -> قصّ على المستوى القريب -> إسقاط -> تعبئة
  void poly(const float (*p)[3],int n,float r,float g,float b,bool useFog=true){
    PV v[8]; int cnt=0; const float NEAR=0.08f;
    PV c[8]; for(int i=0;i<n;i++){
      float dx=p[i][0]-cam.x,dy=p[i][1]-cam.y,dz=p[i][2]-cam.z;
      c[i]={dx*rx+dz*rz, dx*ux+dy*uy+dz*uz, dx*fx+dy*fy+dz*fz};
    }
    for(int i=0;i<n;i++){
      const PV&a=c[i],&bb=c[(i+1)%n]; bool ia=a.z>=NEAR,ib=bb.z>=NEAR;
      if(ia&&cnt<8)v[cnt++]=a;
      if(ia!=ib&&cnt<8){float t=(NEAR-a.z)/(bb.z-a.z);v[cnt++]={a.x+(bb.x-a.x)*t,a.y+(bb.y-a.y)*t,NEAR};}
    }
    if(cnt<3)return;
    float sx[8],sy[8],iz[8]; float cxs=W*0.5f,cys=H*0.5f;
    for(int i=0;i<cnt;i++){iz[i]=1.f/v[i].z;sx[i]=cxs+v[i].x*iz[i]*focal;sy[i]=cys-v[i].y*iz[i]*focal;}
    for(int i=1;i+1<cnt;i++)tri(sx[0],sy[0],iz[0],sx[i],sy[i],iz[i],sx[i+1],sy[i+1],iz[i+1],r,g,b,useFog);
  }
  void tri(float x0,float y0,float z0,float x1,float y1,float z1,float x2,float y2,float z2,float r,float g,float b,bool useFog){
    if(y1<y0){std::swap(x0,x1);std::swap(y0,y1);std::swap(z0,z1);}
    if(y2<y0){std::swap(x0,x2);std::swap(y0,y2);std::swap(z0,z2);}
    if(y2<y1){std::swap(x1,x2);std::swap(y1,y2);std::swap(z1,z2);}
    int ya=std::max(0,(int)std::ceil(y0)),yb=std::min(H-1,(int)std::ceil(y2)-1);
    if(yb<ya||y2-y0<1e-6f)return;
    float inv02=1.f/(y2-y0);
    for(int y=ya;y<=yb;y++){
      float fy=(float)y+0.5f; if(fy<y0)fy=y0;
      float t02=std::min(1.f,std::max(0.f,(fy-y0)*inv02)); float xa=x0+(x2-x0)*t02,za=z0+(z2-z0)*t02,xb,zb_;
      if(fy<y1){float d=y1-y0; float t=d>1e-6f?std::min(1.f,std::max(0.f,(fy-y0)/d)):0; xb=x0+(x1-x0)*t;zb_=z0+(z1-z0)*t;}
      else{float d=y2-y1; float t=d>1e-6f?std::min(1.f,std::max(0.f,(fy-y1)/d)):0; xb=x1+(x2-x1)*t;zb_=z1+(z2-z1)*t;}
      if(xa>xb){std::swap(xa,xb);std::swap(za,zb_);}
      int xs=std::max(0,(int)std::ceil(xa)),xe=std::min(W-1,(int)std::ceil(xb)-1);
      if(xe<xs)continue;
      float dz=(xb-xa)>1e-6f?(zb_-za)/(xb-xa):0; float zi=za+dz*((float)xs+0.5f-xa);
      uint32_t*row=&fb[y*W]; float*zr=&zb[y*W];
      for(int x=xs;x<=xe;x++,zi+=dz){
        if(zi>zr[x]){ zr[x]=zi;
          if(useFog){ float d=1.f/zi; float f=(d-fogN)/(fogF-fogN); f=f<0?0:f>1?1:f;
            row[x]=pack(r+(fog[0]-r)*f,g+(fog[1]-g)*f,b+(fog[2]-b)*f); }
          else row[x]=pack(r,g,b);
        }
      }
    }
  }
  // تدوير متجه محلي: Rx(pitch) ثم Ry(yaw)
  static void rot(float yaw,float pitch,float x,float y,float z,float*o){
    float cp=std::cos(pitch),sp=std::sin(pitch); float y1=y*cp-z*sp,z1=y*sp+z*cp;
    float c=std::cos(yaw),s=std::sin(yaw); o[0]=x*c+z1*s; o[1]=y1; o[2]=-x*s+z1*c;
  }
  // صندوق موجّه بمركز عالمي ودوران yaw/pitch
  void obb(float cx,float cy,float cz,float hx,float hy,float hz,float yaw,float pitch,float r,float g,float b,bool fogOn=true){
    float P[8][3];
    for(int i=0;i<8;i++){float o[3];rot(yaw,pitch,(i&1?hx:-hx),(i&2?hy:-hy),(i&4?hz:-hz),o);P[i][0]=cx+o[0];P[i][1]=cy+o[1];P[i][2]=cz+o[2];}
    static const int F[6][4]={{0,2,3,1},{4,5,7,6},{0,4,6,2},{1,3,7,5},{0,1,5,4},{2,6,7,3}};
    static const float N[6][3]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
    for(int f=0;f<6;f++){
      float n[3];rot(yaw,pitch,N[f][0],N[f][1],N[f][2],n);
      const float*q=P[F[f][0]]; if((cam.x-q[0])*n[0]+(cam.y-q[1])*n[1]+(cam.z-q[2])*n[2]<=0)continue;
      float l=shade(n[0],n[1],n[2]); float pts[4][3]; for(int k=0;k<4;k++)memcpy(pts[k],P[F[f][k]],12);
      poly(pts,4,r*l,g*l,b*l,fogOn);
    }
  }
  float shade(float nx,float ny,float nz){
    const MapDef&d=*m; float sl=std::sqrt(d.sun[0]*d.sun[0]+d.sun[1]*d.sun[1]+d.sun[2]*d.sun[2]);
    float dt=(nx*d.sun[0]+ny*d.sun[1]+nz*d.sun[2])/sl; if(dt<0)dt=0; float l=d.amb*0.62f+d.sunI*dt*0.9f; return l>1.15f?1.15f:l;
  }
  void aabb(const Box&b,float r,float g,float b2){
    // الوجوه المواجهة للكاميرا فقط
    float x0=b.x0,x1=b.x1,y0=b.y0,y1=b.y1,z0=b.z0,z1=b.z1;
    auto face=[&](float nx,float ny,float nz,float a[3],float bq[3],float c[3],float d[3]){
      float p[4][3]; memcpy(p[0],a,12);memcpy(p[1],bq,12);memcpy(p[2],c,12);memcpy(p[3],d,12);
      float l=shade(nx,ny,nz); poly(p,4,r*l,g*l,b2*l);};
    if(cam.x<x0){float a[3]={x0,y0,z0},bq[3]={x0,y0,z1},c[3]={x0,y1,z1},d[3]={x0,y1,z0};face(-1,0,0,a,bq,c,d);}
    if(cam.x>x1){float a[3]={x1,y0,z1},bq[3]={x1,y0,z0},c[3]={x1,y1,z0},d[3]={x1,y1,z1};face(1,0,0,a,bq,c,d);}
    if(cam.z<z0){float a[3]={x1,y0,z0},bq[3]={x0,y0,z0},c[3]={x0,y1,z0},d[3]={x1,y1,z0};face(0,0,-1,a,bq,c,d);}
    if(cam.z>z1){float a[3]={x0,y0,z1},bq[3]={x1,y0,z1},c[3]={x1,y1,z1},d[3]={x0,y1,z1};face(0,0,1,a,bq,c,d);}
    if(cam.y>y1){float a[3]={x0,y1,z1},bq[3]={x1,y1,z1},c[3]={x1,y1,z0},d[3]={x0,y1,z0};face(0,1,0,a,bq,c,d);}
    if(cam.y<y0){float a[3]={x0,y0,z0},bq[3]={x1,y0,z0},c[3]={x1,y0,z1},d[3]={x0,y0,z1};face(0,-1,0,a,bq,c,d);}
  }
  bool cull(const Box&b){
    // تجاهل ما وراء الكاميرا أو أبعد من الضباب (تقريب بالمركز + نصف قطر)
    float cx=(b.x0+b.x1)*.5f,cy=(b.y0+b.y1)*.5f,cz=(b.z0+b.z1)*.5f;
    float rr=0.5f*std::sqrt((b.x1-b.x0)*(b.x1-b.x0)+(b.y1-b.y0)*(b.y1-b.y0)+(b.z1-b.z0)*(b.z1-b.z0));
    float dx=cx-cam.x,dy=cy-cam.y,dz=cz-cam.z; float d=std::sqrt(dx*dx+dy*dy+dz*dz);
    if(d-rr>fogF)return true;
    float zf=dx*fx+dy*fy+dz*fz; if(zf+rr<0)return true;
    float half=std::tan(cam.fov*0.5f)*(float)W/H*1.05f+0.2f; float xr=dx*rx+dz*rz;
    if(zf>0&&std::fabs(xr)-rr>zf*half+rr)return true;
    return false;
  }
  void world(const World&w){
    m=w.m; clear();
    // الأرض
    float W2=m->W/2+60,D2=m->D/2+60; float g[4][3]={{-W2,0,-D2},{-W2,0,D2},{W2,0,D2},{W2,0,-D2}};
    poly(g,4,m->ground[0]*(m->amb*0.62f+m->sunI*0.9f*0.8f),m->ground[1]*(m->amb*0.62f+m->sunI*0.9f*0.8f),m->ground[2]*(m->amb*0.62f+m->sunI*0.9f*0.8f));
    auto col=[&](const Box&b,float*o){int k=b.k%m->ncol;float v=1+0.06f*(((int)(b.x0*7+b.z0*13+b.y1*3))%5-2)*0.5f;for(int i=0;i<3;i++)o[i]=m->cols[k][i]*v;};
    for(int i=0;i<m->nd;i++){const Box&b=m->deco[i];if(b.x1==b.x0)continue;if(cull(b))continue;float c[3];col(b,c);aabb(b,c[0],c[1],c[2]);}
    for(auto&b:w.boxes){if(cull(b))continue;float c[3];col(b,c);aabb(b,c[0],c[1],c[2]);}
  }
};
