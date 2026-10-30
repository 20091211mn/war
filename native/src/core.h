// منطق مشترك: رياضيات + فيزياء + أشعة (منقول من shared/physics.js)
#pragma once
#include <cmath>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include "../gen/data.h"

struct PlayerK { float r=0.35f,h=1.75f,hc=1.2f,eye=1.62f,eyec=1.1f,step=0.6f,headH=0.3f,walk=4.6f,sprint=7.0f,crouch=2.4f,jump=6.2f,grav=18; };
static const PlayerK PK;

struct Body { float x=0,y=0,z=0,vy=0; bool onGround=false; };

struct World {
  const MapDef* m=nullptr; int id=0;
  std::vector<Box> boxes; // الصلبة
  static const int CS=8; float ox=0,oz=0; int nx=0,nz=0;
  std::vector<std::vector<int>> cells; std::vector<int> mark; int stamp=0;
  void load(int idx){
    id=idx; m=&MAPS[idx]; boxes.assign(m->boxes,m->boxes+m->nb);
    ox=-m->W/2-3; oz=-m->D/2-3; nx=(int)std::ceil((m->W+6)/CS)+1; nz=(int)std::ceil((m->D+6)/CS)+1;
    cells.assign(nx*nz,{}); mark.assign(boxes.size(),0); stamp=0;
    auto ci=[&](float v,float o,int n){return std::max(0,std::min(n-1,(int)std::floor((v-o)/CS)));};
    for(int k=0;k<(int)boxes.size();k++){const Box&b=boxes[k];
      int i0=ci(b.x0-0.6f,ox,nx),i1=ci(b.x1+0.6f,ox,nx),j0=ci(b.z0-0.6f,oz,nz),j1=ci(b.z1+0.6f,oz,nz);
      for(int j=j0;j<=j1;j++)for(int i=i0;i<=i1;i++)cells[j*nx+i].push_back(k);}
  }
  const std::vector<int>& cellAt(float x,float z)const{
    int i=std::max(0,std::min(nx-1,(int)std::floor((x-ox)/CS))),j=std::max(0,std::min(nz-1,(int)std::floor((z-oz)/CS)));
    return cells[j*nx+i];
  }
};

static inline float rayBox(float ox,float oy,float oz,float dx,float dy,float dz,float x0,float y0,float z0,float x1,float y1,float z1){
  float tmin=0,tmax=1e30f;
  const float o[3]={ox,oy,oz},d[3]={dx,dy,dz},lo[3]={x0,y0,z0},hi[3]={x1,y1,z1};
  for(int a=0;a<3;a++){
    if(std::fabs(d[a])<1e-9f){ if(o[a]<lo[a]||o[a]>hi[a]) return 1e30f; }
    else{ float t1=(lo[a]-o[a])/d[a],t2=(hi[a]-o[a])/d[a]; if(t1>t2)std::swap(t1,t2);
      if(t1>tmin)tmin=t1; if(t2<tmax)tmax=t2; if(tmin>tmax) return 1e30f; }
  }
  return tmin;
}

static float raycastMap(World&w,float ox,float oy,float oz,float dx,float dy,float dz,float maxT){
  float best=maxT; const int CS=World::CS;
  int cx=std::max(0,std::min(w.nx-1,(int)std::floor((ox-w.ox)/CS))),cz=std::max(0,std::min(w.nz-1,(int)std::floor((oz-w.oz)/CS)));
  int sx=dx>0?1:-1,sz=dz>0?1:-1;
  float tdx=std::fabs(dx)>1e-9f?CS/std::fabs(dx):1e30f,tdz=std::fabs(dz)>1e-9f?CS/std::fabs(dz):1e30f;
  float tmx=std::fabs(dx)>1e-9f?(((dx>0?(cx+1)*CS+w.ox:cx*CS+w.ox))-ox)/dx:1e30f;
  float tmz=std::fabs(dz)>1e-9f?(((dz>0?(cz+1)*CS+w.oz:cz*CS+w.oz))-oz)/dz:1e30f;
  int id=++w.stamp;
  for(int g=0;g<400;g++){
    for(int k:w.cells[cz*w.nx+cx]){ if(w.mark[k]==id)continue; w.mark[k]=id; const Box&b=w.boxes[k];
      float t=rayBox(ox,oy,oz,dx,dy,dz,b.x0,b.y0,b.z0,b.x1,b.y1,b.z1); if(t<best)best=t; }
    float tn=std::min(tmx,tmz);
    if(best<=tn||tn>=maxT)break;
    if(tmx<tmz){cx+=sx;tmx+=tdx;if(cx<0||cx>=w.nx)break;}else{cz+=sz;tmz+=tdz;if(cz<0||cz>=w.nz)break;}
  }
  return best>=maxT?1e30f:best;
}

struct RayHit{float t;bool head;bool ok;};
static RayHit rayPlayer(float px,float py,float pz,bool crouch,float ox,float oy,float oz,float dx,float dy,float dz,float maxT){
  float hh=crouch?PK.hc:PK.h, yb=py+hh-PK.headH;
  float tb=rayBox(ox,oy,oz,dx,dy,dz,px-0.34f,py,pz-0.34f,px+0.34f,yb,pz+0.34f);
  float th=rayBox(ox,oy,oz,dx,dy,dz,px-0.2f,yb,pz-0.2f,px+0.2f,py+hh,pz+0.2f);
  if(th<=tb&&th<maxT) return {th,true,true};
  if(tb<maxT) return {tb,false,true};
  return {0,false,false};
}

static bool blocked(const World&w,float x,float y,float z,float hh){
  float r=PK.r,st=PK.step;
  for(int k:w.cellAt(x,z)){const Box&b=w.boxes[k];
    if(x+r>b.x0&&x-r<b.x1&&z+r>b.z0&&z-r<b.z1&&y+hh>b.y0&&y+st<b.y1) return true;}
  return false;
}
static float groundAt(const World&w,float x,float z,float y){
  float r=PK.r,lim=y+PK.step+1e-4f,g=0;
  for(int k:w.cellAt(x,z)){const Box&b=w.boxes[k];
    if(b.y1<=lim&&b.y1>g&&x+r>b.x0&&x-r<b.x1&&z+r>b.z0&&z-r<b.z1) g=b.y1;}
  return g;
}
static void moveBody(const World&w,Body&p,float vx,float vz,float dt,bool jump,bool crouched){
  float hh=crouched?PK.hc:PK.h;
  float limx=w.m->W/2-PK.r-0.05f,limz=w.m->D/2-PK.r-0.05f;
  float nx=p.x+vx*dt; nx=std::max(-limx,std::min(limx,nx)); if(!blocked(w,nx,p.y,p.z,hh))p.x=nx;
  float nz=p.z+vz*dt; nz=std::max(-limz,std::min(limz,nz)); if(!blocked(w,p.x,p.y,nz,hh))p.z=nz;
  if(jump&&p.onGround){p.vy=PK.jump;p.onGround=false;}
  p.vy-=PK.grav*dt; if(p.vy<-40)p.vy=-40;
  float ny=p.y+p.vy*dt;
  if(p.vy>0){ float r=PK.r;
    for(int k:w.cellAt(p.x,p.z)){const Box&b=w.boxes[k];
      if(b.y0>=p.y+hh-0.01f&&b.y0<ny+hh&&p.x+r>b.x0&&p.x-r<b.x1&&p.z+r>b.z0&&p.z-r<b.z1){ny=b.y0-hh;p.vy=0;}}
    p.onGround=false;
  } else { float g=groundAt(w,p.x,p.z,p.y); if(ny<=g){ny=g;p.vy=0;p.onGround=true;}else p.onGround=false; }
  p.y=ny;
}
