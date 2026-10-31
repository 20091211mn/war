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
  float smx=0,smy=0; float rest[12]={0}; int calib=0; int wiz=pad?0:13; int tbtn[2]={18,19}; int bmap[7]={0,1,2,4,5,11,9}; int skipGuard=0; int wax[6]={1,0,2,3,4,5}; float wsg[6]={-1,1,1,-1,1,1}; int held=0; bool waitRel=false;
  for(;;){
    SDL_Event ev; while(SDL_PollEvent(&ev)){}
    SDL_JoystickUpdate();
    if(!pad&&SDL_NumJoysticks()>0)pad=SDL_JoystickOpen(0);
    uint32_t now=SDL_GetTicks(); float dt=(now-last)/1000.f; last=now; if(dt>0.1f)dt=0.1f; if(dt<=0)dt=0.001f;
    fpsT+=dt; fpsN++; if(fpsT>=1.f){fps=fpsN/fpsT;fpsT=0;fpsN=0;
      int want=fps<38?640:(fps>58&&curW==640?960:curW); if(want!=curW){curW=want;g.r.init(curW,curW*9/16);} }
    // معايرة: أول 40 إطاراً لقراءة قيم الراحة، ثم معالج يطلب تحريك كل عصا/زناد ليعرف المحاور والاتجاهات
    if(calib<40){ calib++; for(int i=0;i<12;i++)rest[i]+=axisF(i)/40.f; }
    if(calib>=40&&wiz<13&&pad){
      static const char*what[13]={"LEFT STICK UP (MOVE)","LEFT STICK RIGHT (MOVE)","RIGHT STICK RIGHT (LOOK)","RIGHT STICK UP (LOOK)","L2 (AIM)","R2 (FIRE)","X (JUMP)","CIRCLE (CROUCH)","SQUARE (RELOAD)","L1 (PREV WEAPON)","R1 (NEXT WEAPON)","CLICK LEFT STICK L3 (SPRINT)","OPTIONS (PAUSE)"};
      int na=std::min(12,SDL_JoystickNumAxes(pad)); int best=-1; float bv=0;
      if(wiz<6) for(int i=0;i<na;i++){ bool used=false; for(int k=0;k<wiz;k++)if(wax[k]==i)used=true; if(used)continue; float v=axisF(i)-rest[i]; if(std::fabs(v)>std::fabs(bv)){bv=v;best=i;} }
      int pb=-1; for(int i=0;i<24;i++){ if(!btn(i))continue; bool used=false; for(int k=0;k<7;k++)if(bmap[k]==i&&k+6<wiz)used=true; for(int k=4;k<6;k++)if(tbtn[k-4]==i&&k<wiz)used=true; if(!used){pb=i;break;} }
      if(waitRel){ bool anyB=false; for(int i=0;i<24;i++)if(btn(i))anyB=true; if(std::fabs(bv)<0.25f&&!anyB)waitRel=false; }
      else if(wiz<4){ if(best>=0&&std::fabs(bv)>0.6f){ if(++held>=4){ wax[wiz]=best; wsg[wiz]=bv>0?1.f:-1.f; wiz++; held=0; waitRel=true; } } else held=0; }
      else if(wiz<6){ if(best>=0&&std::fabs(bv)>0.6f){ if(++held>=3){ wax[wiz]=best; wsg[wiz]=bv>0?1.f:-1.f; tbtn[wiz-4]=-1; wiz++; held=0; waitRel=true; } } else if(pb>=0){ wax[wiz]=-1; tbtn[wiz-4]=pb; wiz++; held=0; waitRel=true; } else held=0; }
      else { if(pb>=0){ bmap[wiz-6]=pb; wiz++; waitRel=true; } }
      if(btn(9)&&wiz<13&&skipGuard++>20){ int ax0[6]={1,0,2,3,4,5}; float sg0[6]={-1,1,1,-1,1,1}; for(int k=0;k<6;k++){wax[k]=ax0[k];wsg[k]=sg0[k];} tbtn[0]=18;tbtn[1]=19; int b0[7]={0,1,2,4,5,11,9}; for(int k=0;k<7;k++)bmap[k]=b0[k]; wiz=13; }
      g.r.clear_black(); char b2[80]; snprintf(b2,80,"STEP %d/13",wiz+1); g.text(120,100,"CONTROLLER SETUP",4,0xFFFFFFFF); g.text(120,170,b2,3,0xFFFFFF00);
      if(wiz<13)g.text(120,220,what[wiz],4,0xFF40FF40); g.text(120,300,"DO IT, THEN RELEASE",2,0xFFB0B0B0); g.text(120,330,"HOLD OPTIONS = SKIP (DEFAULTS)",2,0xFFB0B0B0);
      SDL_LockSurface(surf); uint32_t*dst=(uint32_t*)surf->pixels; int pitch=surf->pitch/4,sw=surf->w,sh=surf->h; int RWc=g.r.W,RHc=g.r.H;
      for(int y=0;y<sh;y++){ const uint32_t*sr=&g.r.fb[(y*RHc/sh)*RWc]; uint32_t*d=dst+y*pitch; for(int x=0;x<sw;x++)d[x]=sr[x*RWc/sw]; }
      SDL_UnlockSurface(surf); SDL_UpdateWindowSurface(win); continue; }
    Input in;
    // مدخل أعمى: دائرة ميتة شعاعية + تنعيم الحركة
    auto stick=[&](float x,float y,float&ox,float&oy){float m=std::sqrt(x*x+y*y);const float D=0.16f;if(m<D){ox=oy=0;return;}float k=(m-D)/(1-D);if(k>1)k=1;ox=x/m*k;oy=y/m*k;};
    auto ax=[&](int k){return wsg[k]*(axisF(wax[k])-rest[wax[k]]);}; // موجب = (أعلى/يمين/ضغط)
    float lx,ly,rx,ry; stick(ax(1),ax(0),lx,ly); // lx=يمين, ly=أعلى
    stick(ax(2),ax(3),rx,ry);
    if(calib<40){lx=ly=rx=ry=0;}
    float a=std::min(1.f,dt*14.f); smx+=(lx-smx)*a; smy+=(ly-smy)*a; if(std::fabs(lx)<0.01f&&std::fabs(smx)<0.05f)smx=0; if(std::fabs(ly)<0.01f&&std::fabs(smy)<0.05f)smy=0;
    in.mx=smx; in.my=smy;
    float rm=std::sqrt(rx*rx+ry*ry); float cv=rm*rm*1.9f+rm*0.45f; float rk=rm>0.001f?cv/rm:0;
    in.lx=rx*rk; in.ly=ry*rk*0.75f;
    bool tl=(wax[4]>=0&&ax(4)>0.6f)||(tbtn[0]>=0&&btn(tbtn[0])), tr2=(wax[5]>=0&&ax(5)>0.6f)||(tbtn[1]>=0&&btn(tbtn[1]));
    if(g.swapTrig)std::swap(tl,tr2); in.fire=tr2; in.ads=tl;
    in.jump=btn(bmap[0]); in.crouch=btn(bmap[1]); in.reload=btn(bmap[2]); in.swapP=btn(bmap[3]); in.swapN=btn(bmap[4]); in.sprint=btn(bmap[5]); in.start=btn(bmap[6]); in.confirm=btn(bmap[0]); in.back=btn(bmap[1]);
    if(g.menu||g.paused){ if(btn(16))in.mx=1; else if(btn(15))in.mx=-1; if(g.menu){ in.my=btn(13)?1.f:btn(14)?-1.f:0.f; } }
    bool d4=btn(3); if(d4&&!dprev){ if(g.menu||g.paused)g.swapTrig=!g.swapTrig; else dbg=!dbg; } dprev=d4;
    // خطوات ثابتة 60Hz حتى لا تتأثر الحركة بانخفاض الإطارات
    acc+=dt; int steps=0; while(acc>=1.f/60.f&&steps<4){ g.update(1.f/60.f,in); acc-=1.f/60.f; steps++; } if(steps==4)acc=0;
    if(g.menu)g.renderMenu(); else g.render();
    if(dbg){ char b[96]; snprintf(b,96,"FPS %d RES %d",(int)fps,g.r.W); g.text(40,40,b,2,0xFFFFFF00); snprintf(b,96,"AX %d %d %d %d %d %d",(int)(axisF(0)*100),(int)(axisF(1)*100),(int)(axisF(2)*100),(int)(axisF(3)*100),(int)(axisF(4)*100),(int)(axisF(5)*100)); g.text(40,60,b,2,0xFFFFFF00); snprintf(b,96,"MAP %d %d %d %d %d %d",wax[0],wax[1],wax[2],wax[3],wax[4],wax[5]); g.text(40,120,b,2,0xFFFFFF00);
      std::string s; for(int i=0;i<24;i++)if(btn(i)){char c[8];snprintf(c,8,"%d ",i);s+=c;} g.text(40,80,("BTN "+s).c_str(),2,0xFFFFFF00); }
    SDL_LockSurface(surf); uint32_t*dst=(uint32_t*)surf->pixels; int pitch=surf->pitch/4,sw=surf->w,sh=surf->h; int RWc=g.r.W,RHc=g.r.H;
    static std::vector<int> xm; if((int)xm.size()!=sw||xm.empty()||xm[0]!=-RWc){xm.assign(sw,0);for(int x=0;x<sw;x++)xm[x]=x*RWc/sw;}
    for(int y=0;y<sh;y++){ const uint32_t*sr=&g.r.fb[(y*RHc/sh)*RWc]; uint32_t*d=dst+y*pitch; for(int x=0;x<sw;x++)d[x]=sr[xm[x]]; }
    SDL_UnlockSurface(surf); SDL_UpdateWindowSurface(win);
  }
}
