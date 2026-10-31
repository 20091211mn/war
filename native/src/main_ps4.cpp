// منصة PS4 (OpenOrbis + SDL2): يرسم الإطار البرمجي إلى سطح النافذة ويقرأ يد التحكم
#include <SDL2/SDL.h>
#include "game.h"

static SDL_Joystick* pad=nullptr;
static float dz(float v,float d=0.14f){float a=std::fabs(v);if(a<d)return 0;a=(a-d)/(1-d);return v<0?-a:a;}
static float axisF(int i){return pad?SDL_JoystickGetAxis(pad,i)/32767.f:0.f;}
static bool btn(int i){return pad&&SDL_JoystickGetButton(pad,i);}

int main(int,char**){
  setvbuf(stdout,NULL,_IONBF,0);
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK)!=0)for(;;);
  SDL_Window*win=SDL_CreateWindow("wz",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1920,1080,0);
  if(!win)for(;;);
  SDL_Surface*surf=SDL_GetWindowSurface(win);
  if(SDL_NumJoysticks()>0)pad=SDL_JoystickOpen(0);
  const int RW=960,RH=540; static Game g; g.menu=1; g.init(RW,RH);
  uint32_t last=SDL_GetTicks(); bool dbg=false; bool dprev=false; float acc=0,fpsT=0; int fpsN=0; float fps=60; int curW=RW;
  float smx=0,smy=0; float rest[12]={0}; int calib=0; int stickA[2]={-1,-1}, trigA[2]={-1,-1}; int nTrig=0;
  for(;;){
    SDL_Event ev; while(SDL_PollEvent(&ev)){}
    SDL_JoystickUpdate();
    if(!pad&&SDL_NumJoysticks()>0)pad=SDL_JoystickOpen(0);
    uint32_t now=SDL_GetTicks(); float dt=(now-last)/1000.f; last=now; if(dt>0.1f)dt=0.1f; if(dt<=0)dt=0.001f;
    fpsT+=dt; fpsN++; if(fpsT>=1.f){fps=fpsN/fpsT;fpsT=0;fpsN=0;
      int want=fps<38?640:(fps>58&&curW==640?960:curW); if(want!=curW){curW=want;g.r.init(curW,curW*9/16);} }
    // معايرة ذاتية: أول 40 إطاراً نقرأ قيم الراحة لكل محور. محور راحته قرب 0 = عصا، وقرب -1 = زناد
    if(calib<40){ calib++; for(int i=0;i<12;i++)rest[i]+=axisF(i)/40.f;
      if(calib==40){ int ns=0; nTrig=0; for(int i=2;i<12;i++){ if(!pad||i>=SDL_JoystickNumAxes(pad))break; if(std::fabs(rest[i])<0.3f){ if(ns<2)stickA[ns++]=i; } else if(nTrig<2)trigA[nTrig++]=i; } } }
    Input in;
    // مدخل أعمى: دائرة ميتة شعاعية + تنعيم الحركة
    auto stick=[&](float x,float y,float&ox,float&oy){float m=std::sqrt(x*x+y*y);const float D=0.16f;if(m<D){ox=oy=0;return;}float k=(m-D)/(1-D);if(k>1)k=1;ox=x/m*k;oy=y/m*k;};
    float lx,ly,rx,ry; stick(axisF(0)-rest[0],axisF(1)-rest[1],lx,ly); if(calib>=40&&stickA[1]>=0)stick(axisF(stickA[0])-rest[stickA[0]],axisF(stickA[1])-rest[stickA[1]],rx,ry); else rx=ry=0;
    if(calib<40){lx=ly=rx=ry=0;}
    float a=std::min(1.f,dt*14.f); smx+=(lx-smx)*a; smy+=(-ly-smy)*a; if(std::fabs(lx)<0.01f&&std::fabs(smx)<0.05f)smx=0; if(std::fabs(ly)<0.01f&&std::fabs(smy)<0.05f)smy=0;
    in.mx=smx; in.my=smy;
    float rm=std::sqrt(rx*rx+ry*ry); float cv=rm*rm*1.9f+rm*0.45f; float rk=rm>0.001f?cv/rm:0;
    in.lx=rx*rk; in.ly=-ry*rk*0.75f;
    bool tl=trigA[0]>=0&&(axisF(trigA[0])-rest[trigA[0]])>0.6f, tr2=trigA[1]>=0&&(axisF(trigA[1])-rest[trigA[1]])>0.6f;
    in.fire=btn(19)||tr2; in.ads=btn(18)||tl;
    in.jump=btn(0); in.crouch=btn(1); in.reload=btn(2); in.swapP=btn(4); in.swapN=btn(5); in.sprint=btn(11); in.start=btn(9); in.confirm=btn(0); in.back=btn(1);
    if(g.menu||g.paused){ if(btn(16))in.mx=1; else if(btn(15))in.mx=-1; if(g.menu){ in.my=btn(13)?1.f:btn(14)?-1.f:0.f; } }
    bool d4=btn(3); if(d4&&!dprev)dbg=!dbg; dprev=d4;
    // خطوات ثابتة 60Hz حتى لا تتأثر الحركة بانخفاض الإطارات
    acc+=dt; int steps=0; while(acc>=1.f/60.f&&steps<4){ g.update(1.f/60.f,in); acc-=1.f/60.f; steps++; } if(steps==4)acc=0;
    if(g.menu)g.renderMenu(); else g.render();
    if(dbg){ char b[96]; snprintf(b,96,"FPS %d RES %d",(int)fps,g.r.W); g.text(40,40,b,2,0xFFFFFF00); snprintf(b,96,"AX %d %d %d %d %d %d",(int)(axisF(0)*100),(int)(axisF(1)*100),(int)(axisF(2)*100),(int)(axisF(3)*100),(int)(axisF(4)*100),(int)(axisF(5)*100)); g.text(40,60,b,2,0xFFFFFF00); snprintf(b,96,"RS %d %d TRG %d %d N%d",stickA[0],stickA[1],trigA[0],trigA[1],pad?SDL_JoystickNumAxes(pad):-1); g.text(40,120,b,2,0xFFFFFF00);
      std::string s; for(int i=0;i<24;i++)if(btn(i)){char c[8];snprintf(c,8,"%d ",i);s+=c;} g.text(40,80,("BTN "+s).c_str(),2,0xFFFFFF00); }
    SDL_LockSurface(surf); uint32_t*dst=(uint32_t*)surf->pixels; int pitch=surf->pitch/4,sw=surf->w,sh=surf->h; int RWc=g.r.W,RHc=g.r.H;
    static std::vector<int> xm; if((int)xm.size()!=sw||xm.empty()||xm[0]!=-RWc){xm.assign(sw,0);for(int x=0;x<sw;x++)xm[x]=x*RWc/sw;}
    for(int y=0;y<sh;y++){ const uint32_t*sr=&g.r.fb[(y*RHc/sh)*RWc]; uint32_t*d=dst+y*pitch; for(int x=0;x<sw;x++)d[x]=sr[xm[x]]; }
    SDL_UnlockSurface(surf); SDL_UpdateWindowSurface(win);
  }
}
