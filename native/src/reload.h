// حركة التعبئة (منفَّذة مطابقةً لـ reloadPose في public/js/models.js)
#pragma once
#include <cmath>
#include <algorithm>
struct RPose{
  float x=0,y=0,z=0,rx=0,ry=0,rz=0,tilt=0,bolt=0; float hand[3]={0,0,0};
  bool hideMag=false,hideRear=true,handle=false;
  int nm=0; float mg[3][3]; float ms[3]={0,0,0};          // مخازن متحركة (مواضع) + حجمها
  int ns=0; float sh[1][3];                               // خرطوشة شوتغن
  int shiftIdx=-1; float shiftV=0;                        // جزء يتحرك (شريحة/غطاء)
  int shiftIdx2=-1;
  bool hasRocket=false; float rk[3]={0,0,0};              // RPG: القذيفة المتحركة (إزاحة عن موضعها داخل الأنبوب)
};
namespace RL{
  static inline float ez(float x){ return x<=0?0:x>=1?1:x*x*(3-2*x); }
  static inline float sg(float t,float a,float b){ return ez((t-a)/(b-a)); }
  static inline void mix3(const float*a,const float*b,float k,float*o){ for(int i=0;i<3;i++)o[i]=a[i]+(b[i]-a[i])*k; }
  static inline void set3(float*o,float a,float b,float c){ o[0]=a;o[1]=b;o[2]=c; }
  // pistolClass: مسدس/ماغنوم ، shotgun
  static void launcherPose(float t,RPose&P){
    static const float PT[5][4]={{0.00f,-0.26f,-0.32f,0.80f},{0.17f,-0.24f,-0.16f,0.45f},{0.32f,-0.30f,-0.06f,-0.10f},{0.45f,-0.16f,0.0f,-0.42f},{0.55f,0.0f,0.0f,-0.36f}};
    float tilt=sg(t,0,0.14f)-sg(t,0.9f,1.f); P.tilt=tilt; P.hideRear=false; P.y=0.05f*tilt; P.x=-0.03f*tilt; P.z=-0.06f*tilt; P.rx=0.42f*tilt; P.ry=0; P.rz=-0.32f*tilt; P.hideMag=t<0.74f;
    float H0[3]={-0.03f,-0.13f,-0.08f},off[3]={0,0,0}; bool have=false;
    if(t>=0.07f&&t<0.55f){ int i=0; while(i<3&&t>=PT[i+1][0])i++; float k=i?sg(t,PT[i][0],PT[i+1][0]):sg(t,0.07f,PT[1][0]); float a[3]={PT[i][1],PT[i][2],PT[i][3]},b[3]={PT[i+1][1],PT[i+1][2],PT[i+1][3]}; mix3(a,b,k,off); have=true; }
    else if(t>=0.55f&&t<0.74f){ float k=sg(t,0.6f,0.72f); off[0]=0;off[1]=0;off[2]=-0.36f*(1-k); have=true; }
    P.hasRocket=have; for(int i=0;i<3;i++)P.rk[i]=off[i];
    float hand[3]={H0[0],H0[1],H0[2]};
    if(t<0.07f){ float a[3]={-0.2f,-0.3f,0.6f}; mix3(H0,a,sg(t,0,0.07f),hand); }
    else if(have&&t<0.74f){ set3(hand,off[0]-0.02f,off[1]-0.06f,-0.46f+off[2]+0.1f); }
    else if(t<0.9f){ float a[3]={0,-0.06f,-0.26f}; mix3(a,H0,sg(t,0.78f,0.9f),hand); }
    for(int i=0;i<3;i++)P.hand[i]=hand[i];
    P.z+=0.03f*std::max(0.f,sinf(std::min(1.f,std::max(0.f,(t-0.7f)/0.08f))*3.14159265f)); }
  template<class GD> static void pose(const GD&g,bool pistol,bool shotgun,float t,RPose&P,bool launcher=false){
    if(launcher){ launcherPose(t,P); return; }
    float M[3]={0,-0.08f,0.05f},MS[3]={0.028f,0.09f,0.04f}; if(g.hasMag){ for(int i=0;i<3;i++){M[i]=g.mag[i];MS[i]=g.mag[3+i];} }
    for(int i=0;i<3;i++)P.ms[i]=MS[i];
    float H0[3]; if(pistol)set3(H0,-0.03f,-0.075f,0.05f); else set3(H0,-0.005f,-0.05f,g.mz*0.42f);
    float tilt=sg(t,0,0.13f)-sg(t,0.88f,1.f); P.tilt=tilt;
    P.y=0.09f*tilt; P.x=-0.1f*tilt; P.z=-0.12f*tilt; P.rx=-0.2f*tilt; P.ry=0; P.rz=-0.5f*tilt; P.hideRear=true;
    for(int i=0;i<3;i++)P.hand[i]=H0[i];
    float pouch[3]={-0.14f,-0.17f,0.06f};
    if(shotgun){
      const float W[4]={0.14f,0.30f,0.46f,0.62f}; float hand[3]={H0[0],H0[1],H0[2]}; float port[3]={0,-0.035f,-0.02f},mid[3]={-0.05f,-0.2f,0.02f}; bool cur=false; float cp[3]={0,0,0};
      for(int k=0;k<4;k++){ float a=W[k],b=a+0.14f; if(t>=a&&t<b+0.02f){ float u=(t-a)/(b-a); float pos[3]; if(u<0.45f)mix3(pouch,mid,sg(u,0,0.45f),pos); else mix3(mid,port,sg(u,0.45f,0.9f),pos);
          if(u<0.92f){cur=true;for(int i=0;i<3;i++)cp[i]=pos[i];} set3(hand,pos[0]-0.01f,pos[1]-0.045f,pos[2]); } }
      if(cur){ P.ns=1; for(int i=0;i<3;i++)P.sh[0][i]=cp[i]; }
      float pump=sg(t,0.78f,0.84f)-sg(t,0.84f,0.9f);
      if(t>=0.76f&&t<0.92f){ P.shiftIdx=2; P.shiftIdx2=3; P.shiftV=0.12f*pump; set3(hand,-0.005f,-0.04f,g.mz*0.42f+0.12f*pump); }
      else if(t>=0.9f){ float a[3]={-0.005f,-0.04f,g.mz*0.42f}; mix3(a,H0,sg(t,0.9f,0.97f),hand); }
      else if(t<0.14f){ mix3(H0,pouch,sg(t,0.04f,0.14f),hand); }
      for(int i=0;i<3;i++)P.hand[i]=hand[i]; return; }
    float Mb[3]={M[0],M[1]-MS[1]*0.5f-0.02f,M[2]};
    float hOld=sg(t,0.14f,0.30f);
    P.hideMag=t<0.58f; P.nm=0;
    if(t<0.52f){
      if(t<0.30f){ set3(P.mg[P.nm++],M[0]-0.01f*hOld,M[1]-0.17f*hOld,M[2]); }
      else { float q=t-0.30f; if(q<0.17f)set3(P.mg[P.nm++],M[0]-0.01f-q*0.5f,M[1]-0.17f-12*q*q,M[2]+q*0.3f); } }
    float hand[3]={H0[0],H0[1],H0[2]}; bool haveNew=false; float np[3]={0,0,0};
    float hp1[3]={0,0.025f,0.06f},hr1[3]={0,0.05f,0.05f}; const float*HC=pistol?hp1:hr1;
    if(t<0.14f){ mix3(H0,Mb,sg(t,0,0.14f),hand); }
    else if(t<0.30f){ set3(hand,Mb[0]-0.01f*hOld,Mb[1]-0.17f*hOld,Mb[2]); }
    else if(t<0.40f){ float a[3]={Mb[0]-0.01f,Mb[1]-0.17f,Mb[2]}; mix3(a,pouch,sg(t,0.30f,0.40f),hand); }
    else if(t<0.58f){ float k=t<0.52f?sg(t,0.40f,0.52f):1.f,slam=sg(t,0.52f,0.575f); float tgt[3]={Mb[0],Mb[1]-0.04f,Mb[2]},a[3]; mix3(pouch,tgt,k,a);
      if(t<0.52f){ for(int i=0;i<3;i++)np[i]=a[i]; } else mix3(tgt,M,slam,np);
      haveNew=true; set3(hand,np[0]-0.015f,np[1]-MS[1]*0.5f-0.03f,np[2]); }
    else if(t<0.69f){ float a[3]={M[0]-0.015f,M[1]-0.07f,M[2]}; mix3(a,HC,sg(t,0.585f,0.69f),hand); }
    else if(t<0.84f){ for(int i=0;i<3;i++)hand[i]=HC[i]; }
    else { mix3(HC,H0,sg(t,0.84f,0.95f),hand); }
    if(haveNew){ for(int i=0;i<3;i++)P.mg[P.nm][i]=np[i]; P.nm++; }
    auto c01=[](float v){ return std::max(0.f,std::min(1.f,v)); };
    P.z+=0.022f*std::max(0.f,sinf(c01((t-0.57f)/0.06f)*3.14159265f))-0.012f*std::max(0.f,sinf(c01((t-0.82f)/0.05f)*3.14159265f));
    float b=sg(t,0.69f,0.78f)-sg(t,0.8f,0.83f); P.bolt=0.06f*b;
    if(pistol){ P.shiftIdx=0; P.shiftV=P.bolt; if(t>=0.69f&&t<0.84f)set3(hand,0.f,0.025f,0.06f+P.bolt); }
    else { P.handle=(t>=0.58f&&t<0.9f); if(t>=0.69f&&t<0.84f)set3(hand,0.f,0.05f,0.05f+P.bolt); }
    for(int i=0;i<3;i++)P.hand[i]=hand[i]; }
}
