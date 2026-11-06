// الشاص (تويوتا لاند كروزر بيك أب): فيزياء قيادة ونموذج — منقول من shared/vehicle.js و vehview.js
#pragma once
#include "core.h"
#include "render.h"
namespace VH{
static const float HP=450,L=3.0f,HW=1.0f,MAXFWD=26,MAXREV=7,ACCEL=7.5f,BRAKE=16,MAXSTEER=0.52f,TIREHP=30,CRASHMIN=4;
struct VP{ float hp,L,hw,hl,maxFwd,maxRev,accel,brake,maxSteer,crashMin,armor; int nseats; bool tracked; };
static const VP VPS[2]={ {450,3.0f,1.0f,2.5f,26,7,7.5f,16,0.52f,4,1.f,5,false}, {1100,3.6f,1.5f,3.1f,11,5,3.8f,12,1.f,6,0.14f,2,true} };
struct Veh{ int occ=0; int type=0; float kick=0,ty=0,tp=0,cannonT=0; bool mgFp=false; float x=0,z=0,h=0,vx=0,vz=0,steer=0,speed=0,hp=HP,tireHp[4]={30,30,30,30}; int tires=0,driver=-1,team=-1; bool dead=false,burn=false; float respawnT=0,sx=0,sz=0,sh=0,smoke=0,crashT=0,tx=0,tz=0,th=0; };
static inline void l2w(const Veh&v,float lx,float lz,float&ox,float&oz){ float c=cosf(v.h),s=sinf(v.h); ox=v.x+c*lx+s*lz; oz=v.z-s*lx+c*lz; }
static inline void w2l(const Veh&v,float wx,float wz,float&lx,float&lz){ float c=cosf(v.h),s=sinf(v.h),dx=wx-v.x,dz=wz-v.z; lx=c*dx-s*dz; lz=s*dx+c*dz; }
static inline const VP&vp(const Veh&v){ return VPS[v.type==1?1:0]; }
static inline int tiresBlown(int m){ return (m&1)+((m>>1)&1)+((m>>2)&1)+((m>>3)&1); }
struct BB{float x0,y0,z0,x1,y1,z1;int part;}; // part: 0 جسم ، 1 خزان ، 10+i عجلة
static const BB BODY[]={{-1,0.3f,-2.5f,1,1.1f,-0.9f,0},{-1,0.3f,-0.9f,1,1.05f,0.6f,0},{-0.95f,1.8f,-0.7f,0.95f,1.9f,0.4f,0},{-1,0.3f,0.6f,1,0.85f,2.5f,1},
 {-1,0.85f,0.6f,-0.9f,1.25f,2.5f,0},{0.9f,0.85f,0.6f,1,1.25f,2.5f,0},{-1,0.85f,2.4f,1,1.25f,2.5f,0},{-1,0.85f,0.6f,1,1.25f,0.7f,0}};
static const BB BODY_T[]={{-1.45f,0.35f,-3.1f,1.45f,1.3f,3.1f,0},{-1.0f,1.3f,-1.5f,1.0f,1.95f,1.3f,0},{-0.12f,1.5f,-4.8f,0.12f,1.8f,-1.5f,0},{-1.2f,1.0f,1.4f,1.2f,1.35f,3.0f,1},{-1.62f,0.1f,-3.0f,-1.2f,0.9f,3.0f,0},{1.2f,0.1f,-3.0f,1.62f,0.9f,3.0f,0}};
static const float WH[4][2]={{-0.95f,-1.55f},{0.95f,-1.55f},{-0.95f,1.5f},{0.95f,1.5f}};
// تقاطع شعاع (عالمي) مع الشاص: يرجع t أو 1e30 ، ويملأ part
static inline float rayVeh(const Veh&v,float ox,float oy,float oz,float dx,float dy,float dz,float maxT,int&part){
  float c=cosf(v.h),s=sinf(v.h); float px=ox-v.x,pz=oz-v.z; float lox=c*px-s*pz,loz=s*px+c*pz,ldx=c*dx-s*dz,ldz=s*dx+c*dz; float best=maxT; part=-1;
  if(v.type==1){ for(auto&b:BODY_T){ float t=rayBox(lox,oy,loz,ldx,dy,ldz,b.x0,b.y0,b.z0,b.x1,b.y1,b.z1); if(t<best){best=t;part=b.part;} } return part>=0?best:1e30f; }
  for(auto&b:BODY){ float t=rayBox(lox,oy,loz,ldx,dy,ldz,b.x0,b.y0,b.z0,b.x1,b.y1,b.z1); if(t<best){best=t;part=b.part;} }
  for(int i=0;i<4;i++){ float t=rayBox(lox,oy,loz,ldx,dy,ldz,WH[i][0]-0.2f,0,WH[i][1]-0.4f,WH[i][0]+0.2f,0.76f,WH[i][1]+0.4f); if(t<best+0.02f&&t<maxT){best=std::min(best,t);part=10+i;} }
  return part>=0?best:1e30f;
}
static bool circleHit(const World&w,float cx,float cz,float r,float minH=0.55f){
  float lx=w.m->W/2-r,lz=w.m->D/2-r; if(cx>lx||cx<-lx||cz>lz||cz<-lz)return true;
  for(const Box&b:w.boxes){ if(b.y1<=minH||b.y0>=1.6f)continue; if(cx<b.x0-r||cx>b.x1+r||cz<b.z0-r||cz>b.z1+r)continue;
    float nx=cx<b.x0?b.x0:cx>b.x1?b.x1:cx,nz=cz<b.z0?b.z0:cz>b.z1?b.z1:cz; float dx=cx-nx,dz=cz-nz; if(dx*dx+dz*dz<r*r)return true; }
  return false;
}
static bool collides(const World&w,float x,float z,float h,const std::vector<Veh>&all,const Veh*self){
  const VP&P=self?vp(*self):VPS[0]; float bx=sinf(h),bz=cosf(h); float e=P.hl-0.3f; float S[5]={-e,-e/2,0,e/2,e};
  for(float k:S){ float cx=x+bx*k,cz=z+bz*k; if(circleHit(w,cx,cz,P.hw,P.tracked?1.15f:0.55f))return true;
    for(auto&o:all){ if(&o==self||o.dead)continue; const VP&Q=vp(o); float ob=sinf(o.h),oc=cosf(o.h),lim=(P.hw+Q.hw)*(P.hw+Q.hw)*0.82f; for(float j:{-Q.hl*0.64f,0.f,Q.hl*0.64f}){ float dx=cx-(o.x+ob*j),dz=cz-(o.z+oc*j); if(dx*dx+dz*dz<lim)return true; } } }
  return false;
}
// يدفع كيان خارج جسم الشاص
static bool pushOut(const Veh&v,float&px,float&pz,float py){
  if(py>1.85f||py<-1)return false; float lx,lz; w2l(v,px,pz,lx,lz); const VP&P=vp(v); float hx=P.hw+0.38f,hz=P.hl+0.38f; if(fabsf(lx)>=hx||fabsf(lz)>=hz)return false;
  float a=hx-fabsf(lx),b=hz-fabsf(lz); float nx=lx,nz=lz; if(a<b)nx=(lx<0?-1.f:1.f)*hx; else nz=(lz<0?-1.f:1.f)*hz; l2w(v,nx,nz,px,pz); return true;
}
// فيزياء القيادة: thr أمام/خلف ، steer يمين +، hb فرامل يد. يرجع سرعة الاصطدام
static float step(const World&w,Veh&v,std::vector<Veh>&all,float thr,float steer,bool hb,float dt){
  float fx=-sinf(v.h),fz=-cosf(v.h),rx=cosf(v.h),rz=-sinf(v.h);
  float vf=v.vx*fx+v.vz*fz,vl=v.vx*rx+v.vz*rz; const VP&P=vp(v); bool T=P.tracked; int nb=T?0:tiresBlown(v.tires);
  float maxFwd=std::max<float>(5.f,P.maxFwd*(1-0.17f*nb)),accel=P.accel*(1-0.1f*nb);
  if(thr>0){ if(vf<-0.5f)vf+=P.brake*dt; else vf+=accel*thr*dt*std::max<float>(0.f,1-vf/maxFwd); }
  else if(thr<0){ if(vf>0.5f)vf-=P.brake*dt; else vf-=accel*0.6f*-thr*dt*std::max<float>(0.f,1-(-vf)/MAXREV); }
  else { float roll=(1.5f+0.012f*vf*vf)*dt; vf-=(vf>0?1.f:-1.f)*std::min<float>(fabsf(vf),roll); }
  if(vf>maxFwd)vf=std::max(maxFwd,vf-12*dt);
  if(hb)vf-=(vf>0?1.f:-1.f)*std::min<float>(fabsf(vf),9*dt);
  float grip=T?14.f:(hb?1.4f:7.f)*(1-0.12f*nb); vl*=expf(-grip*dt); float yaw;
  if(T){ v.steer+=(steer*0.5f-v.steer)*std::min<float>(1.f,dt*8); yaw=steer*0.85f*(1-std::min<float>(0.5f,fabsf(vf)/24)); }
  else { float maxS=P.maxSteer*(1-std::min<float>(0.72f,fabsf(vf)/38)); v.steer+=(steer*maxS-v.steer)*std::min<float>(1.f,dt*6);
    yaw=vf/P.L*tanf(v.steer)*(hb?1.35f:1.f);
    float pull=(float)(((v.tires>>1&1)+(v.tires>>3&1))-((v.tires&1)+(v.tires>>2&1))); yaw+=pull*0.09f*std::min<float>(1.f,fabsf(vf)/10)*(vf<0?-1.f:1.f); }
  float nh=v.h-yaw*dt; float crash=0;
  if(!collides(w,v.x,v.z,nh,all,&v))v.h=nh; else if(fabsf(vf)>1)crash=std::max<float>(crash,fabsf(vf)*0.4f);
  float nfx=-sinf(v.h),nfz=-cosf(v.h),nrx=cosf(v.h),nrz=-sinf(v.h); float nvx=nfx*vf+nrx*vl,nvz=nfz*vf+nrz*vl;
  float nx=v.x+nvx*dt,nz=v.z+nvz*dt; bool hit=false;
  if(!collides(w,nx,v.z,v.h,all,&v))v.x=nx; else {hit=true;nvx*=-0.2f;}
  if(!collides(w,v.x,nz,v.h,all,&v))v.z=nz; else {hit=true;nvz*=-0.2f;}
  if(hit){crash=std::max<float>(crash,fabsf(vf));nvx*=0.6f;nvz*=0.6f;}
  v.vx=nvx;v.vz=nvz;v.speed=nvx*nfx+nvz*nfz;
  if(collides(w,v.x,v.z,v.h,all,&v)){ bool done=false; for(int k=1;k<=12&&!done;k++){ static const int D[4][2]={{1,0},{-1,0},{0,1},{0,-1}}; for(auto&d:D){ if(!collides(w,v.x+d[0]*k*0.25f,v.z+d[1]*k*0.25f,v.h,all,&v)){v.x+=d[0]*k*0.25f;v.z+=d[1]*k*0.25f;done=true;break;} } } }
  return crash;
}
struct Part{float x,y,z,sx,sy,sz,r,g,b;bool unlit;};
static const float BC[3]={0.78f,0.72f,0.58f};
#define BD BC[0],BC[1],BC[2]
static const Part PARTS[]={
 {0,0.5f,0,1.7f,0.2f,4.9f,.1f,.1f,.11f,0},
 {0,0.86f,-1.7f,1.96f,0.46f,1.6f,BD,0},{0,1.12f,-1.55f,1.2f,0.05f,0.9f,.13f,.13f,.14f,0},
 {-0.97f,0.98f,-1.55f,0.14f,0.34f,1.1f,BD,0},{0.97f,0.98f,-1.55f,0.14f,0.34f,1.1f,BD,0},
 {0,0.74f,-2.52f,1.9f,0.38f,0.1f,.13f,.13f,.14f,0},{0,0.5f,-2.58f,2.0f,0.16f,0.14f,.45f,.46f,.48f,0},
 {-0.68f,0.94f,-2.53f,0.26f,0.2f,0.06f,1,.95f,.75f,1},{0.68f,0.94f,-2.53f,0.26f,0.2f,0.06f,1,.95f,.75f,1},
 {0,0.72f,-0.15f,2.0f,0.66f,1.5f,BD,0},{0,1.86f,-0.15f,1.96f,0.1f,1.2f,BD,0},
 {-0.93f,1.45f,-0.74f,0.08f,0.8f,0.08f,BD,0},{0.93f,1.45f,-0.74f,0.08f,0.8f,0.08f,BD,0},{-0.93f,1.45f,0.4f,0.08f,0.8f,0.1f,BD,0},{0.93f,1.45f,0.4f,0.08f,0.8f,0.1f,BD,0},
 {0,1.46f,-0.76f,1.8f,0.78f,0.04f,.2f,.3f,.38f,0},{0,1.46f,0.45f,1.8f,0.6f,0.04f,.2f,.3f,.38f,0},
 {-0.45f,0.62f,-0.25f,0.7f,0.14f,0.6f,.2f,.2f,.19f,0},{0.45f,0.62f,-0.25f,0.7f,0.14f,0.6f,.2f,.2f,.19f,0},{-0.45f,0.95f,0.12f,0.7f,0.55f,0.12f,.22f,.22f,.21f,0},{0.45f,0.95f,0.12f,0.7f,0.55f,0.12f,.22f,.22f,.21f,0},
 {0,0.98f,-0.72f,1.7f,0.12f,0.2f,.13f,.13f,.14f,0},
 {0,0.88f,1.55f,1.9f,0.08f,1.9f,.13f,.13f,.14f,0},{-0.96f,1.08f,1.55f,0.08f,0.42f,1.9f,BD,0},{0.96f,1.08f,1.55f,0.08f,0.42f,1.9f,BD,0},
 {0,1.08f,2.46f,1.9f,0.42f,0.08f,BD,0},{0,1.08f,0.66f,1.9f,0.42f,0.08f,BD,0},{0,0.62f,1.6f,1.6f,0.28f,1.6f,.1f,.1f,.1f,0},
 {0,0.7f,-2.575f,1.1f,0.03f,0.03f,.62f,.64f,.68f,0},{0,0.77f,-2.575f,1.1f,0.03f,0.03f,.62f,.64f,.68f,0},{0,0.84f,-2.575f,1.1f,0.03f,0.03f,.62f,.64f,.68f,0},
 {0,0.86f,-2.58f,0.2f,0.1f,0.03f,.7f,.1f,.1f,0},{0,0.37f,2.56f,2.0f,0.14f,0.12f,.4f,.41f,.43f,0},
 {-0.55f,1.1f,-1.4f,0.6f,0.012f,0.45f,.62f,.56f,.42f,0},{0.35f,1.1f,-1.9f,0.5f,0.012f,0.3f,.62f,.56f,.42f,0},{0.1f,1.45f,-0.3f,0.7f,0.012f,0.5f,.62f,.56f,.42f,0},
 {0.55f,1.35f,0.9f,0.8f,0.8f,0.3f,.06f,.06f,.07f,0},{0,1.28f,1.7f,0.1f,0.8f,0.1f,.07f,.07f,.07f,0},
 {-0.55f,1.0f,2.0f,0.3f,0.2f,0.2f,.3f,.3f,.24f,0},
 {-0.7f,0.62f,2.52f,0.3f,0.12f,0.05f,.9f,.1f,.1f,1},{0.7f,0.62f,2.52f,0.3f,0.12f,0.05f,.9f,.1f,.1f,1},
 {-1.005f,0.96f,-0.62f,0.012f,0.72f,0.025f,.05f,.05f,.05f,0},{1.005f,0.96f,-0.62f,0.012f,0.72f,0.025f,.05f,.05f,.05f,0},{-1.005f,0.96f,0.32f,0.012f,0.72f,0.025f,.05f,.05f,.05f,0},{1.005f,0.96f,0.32f,0.012f,0.72f,0.025f,.05f,.05f,.05f,0},
 {-1.12f,1.36f,-0.82f,0.09f,0.15f,0.1f,.06f,.06f,.06f,0},{1.12f,1.36f,-0.82f,0.09f,0.15f,0.1f,.06f,.06f,.06f,0},
 {-1.05f,0.5f,-0.15f,0.12f,0.05f,1.5f,.08f,.08f,.08f,0},{1.05f,0.5f,-0.15f,0.12f,0.05f,1.5f,.08f,.08f,.08f,0},
};
static void drawTank(Renderer&r,const Veh&v,float dist,float t){
  float tint=v.dead?0.16f:0.55f+0.45f*std::max<float>(0.f,v.hp/VPS[1].hp);
  auto P=[&](float lx,float y,float lz,float sx,float sy,float sz,float cr,float cg,float cb,bool unlit,float pitch=0){ float wx,wz; l2w(v,lx,lz,wx,wz); r.obb(wx,y,wz,sx*.5f,sy*.5f,sz*.5f,v.h,pitch,cr*(unlit?1:tint),cg*(unlit?1:tint),cb*(unlit?1:tint),true,!unlit); };
  const float B[3]={.34f,.39f,.25f}; float rel=v.ty-v.h;
  auto TU=[&](float lx,float y,float lz,float sx,float sy,float sz,float cr,float cg,float cb,bool unlit,float pitch=0){ float o[3]; Renderer::rot(rel,0,lx,y,lz,o); float wx,wz; l2w(v,o[0],-0.1f+o[2],wx,wz); r.obb(wx,1.3f+o[1],wz,sx*.5f,sy*.5f,sz*.5f,v.h+rel,pitch,cr*(unlit?1:tint),cg*(unlit?1:tint),cb*(unlit?1:tint),true,!unlit); };
  auto GN=[&](float y,float z,float sx,float sy,float sz,float cr,float cg,float cb,bool unlit){ float pv[3],g[3]; Renderer::rot(rel,0,0,0.35f,-1.7f,pv); Renderer::rot(rel,v.tp,0,y,z,g); float wx,wz; l2w(v,pv[0]+g[0],-0.1f+pv[2]+g[2],wx,wz); r.obb(wx,1.3f+pv[1]+g[1],wz,sx*.5f,sy*.5f,sz*.5f,v.h+rel,v.tp,cr*(unlit?1:tint),cg*(unlit?1:tint),cb*(unlit?1:tint),true,!unlit); };
  P(0,0.78f,0,2.55f,0.62f,6.0f,B[0],B[1],B[2],false);
  if(dist>60){ TU(0,0.35f,0.1f,2.1f,0.7f,2.6f,B[0],B[1],B[2],false); GN(0,-1.7f,0.22f,0.22f,3.6f,.11f,.12f,.1f,false); return; }
  for(int sx=-1;sx<=1;sx+=2){ P(sx*1.45f,0.5f,0,0.62f,0.85f,6.5f,.07f,.07f,.07f,false); P(sx*1.45f,0.96f,0,0.7f,0.06f,6.4f,.11f,.12f,.1f,false);
    for(int i=0;i<6;i++){ float z=-2.5f+i*1.0f; P(sx*1.5f,0.4f,z,0.42f,0.74f,0.74f,.2f,.21f,.19f,false); } }
  P(0,1.0f,-2.55f,2.5f,0.3f,1.5f,B[0],B[1],B[2],false,-0.5f); P(0,1.12f,2.7f,2.4f,0.26f,1.2f,B[0],B[1],B[2],false);
  for(int i=0;i<4;i++)P(-0.7f+i*0.47f,1.28f,2.7f,0.3f,0.03f,0.9f,.11f,.12f,.1f,false);
  P(0.75f,1.2f,-2.2f,0.55f,0.24f,0.55f,.11f,.12f,.1f,false); P(0,0.7f,-3.08f,2.4f,0.4f,0.08f,.11f,.12f,.1f,false);
  P(-0.95f,0.88f,-3.1f,0.32f,0.18f,0.06f,1,.95f,.75f,!v.dead); P(0.95f,0.88f,-3.1f,0.32f,0.18f,0.06f,1,.95f,.75f,!v.dead);
  P(-1.0f,0.72f,3.04f,0.3f,0.14f,0.05f,.9f,.1f,.1f,!v.dead); P(1.0f,0.72f,3.04f,0.3f,0.14f,0.05f,.9f,.1f,.1f,!v.dead);
  TU(0,0.32f,0.15f,2.3f,0.62f,2.9f,B[0],B[1],B[2],false); TU(0,0.3f,-1.4f,1.7f,0.54f,0.7f,B[0],B[1],B[2],false,-0.3f); TU(0,0.22f,1.6f,1.8f,0.4f,0.5f,B[0],B[1],B[2],false);
  TU(0.55f,0.72f,0.5f,0.75f,0.12f,0.75f,.11f,.12f,.1f,false); TU(-1.18f,0.35f,0.2f,0.08f,0.3f,1.4f,.45f,.47f,.44f,false); TU(1.18f,0.35f,0.2f,0.08f,0.3f,1.4f,.45f,.47f,.44f,false);
  float kk=v.kick*0.45f;
  TU(0,0.35f,-1.65f,0.8f,0.7f,0.3f,.11f,.12f,.1f,false);
  GN(0,-0.0f-1.4f+kk,0.24f,0.24f,3.2f,.11f,.12f,.1f,false);
  GN(0,-3.15f+kk,0.36f,0.36f,0.34f,.07f,.07f,.07f,false);
  if(v.kick>0.6f)GN(0,-3.6f,0.7f,0.7f,0.8f,1,.8f,.3f,true);
}
static void draw(Renderer&r,const Veh&v,float dist){
  if(v.type==1){ drawTank(r,v,dist,0); return; }
  float tint=v.dead?0.16f:0.55f+0.45f*std::max<float>(0.f,v.hp/HP);
  auto P=[&](float lx,float y,float lz,float sx,float sy,float sz,float cr,float cg,float cb,bool unlit){ float wx,wz; l2w(v,lx,lz,wx,wz); r.obb(wx,y,wz,sx*.5f,sy*.5f,sz*.5f,v.h,0,cr*(unlit?1:tint),cg*(unlit?1:tint),cb*(unlit?1:tint),true,!unlit); };
  if(dist>60){ P(0,0.6f,0,1.9f,0.5f,4.9f,BC[0],BC[1],BC[2],false); P(0,1.3f,-0.15f,1.9f,0.8f,1.5f,BC[0],BC[1],BC[2],false); }
  else for(auto&p:PARTS)P(p.x,p.y,p.z,p.sx,p.sy,p.sz,p.r,p.g,p.b,p.unlit&&!v.dead);
  for(int i=0;i<4;i++){ bool bl=(v.tires>>i)&1; P(WH[i][0],bl?0.3f:0.38f,WH[i][1],0.34f,bl?0.55f:0.76f,bl?0.55f:0.76f,0.06f,0.06f,0.07f,false); }
  if(dist<=60){ // الرشاش المثبّت: يدور مع نظر الرامي (v.ty/v.tp) ؛ للرامي المحلي نخفي الجسم والدرع كي لا يحجبا الرؤية
    float rel=v.ty-v.h,pt=v.tp; auto G=[&](float lx,float ly,float lz,float sx,float sy,float sz,float cr,float cg,float cb){ float o[3]; Renderer::rot(rel,pt,lx,ly,lz,o); float wx,wz; l2w(v,o[0],1.6f+o[2],wx,wz); r.obb(wx,1.72f+o[1],wz,sx*.5f,sy*.5f,sz*.5f,v.h+rel,pt,cr*tint,cg*tint,cb*tint,true,true); };
    if(!v.mgFp){ G(0,0,-0.15f,0.16f,0.16f,0.7f,.1f,.1f,.11f); G(0,0.13f,-0.2f,0.8f,0.45f,0.04f,BD); } G(0,-0.04f,-0.75f,0.05f,0.05f,0.55f,.06f,.06f,.06f); }
}
}
