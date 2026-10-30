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
  uint32_t last=SDL_GetTicks(); bool dbg=false; bool dprev=false;
  for(;;){
    SDL_Event ev; while(SDL_PollEvent(&ev)){}
    SDL_JoystickUpdate();
    if(!pad&&SDL_NumJoysticks()>0)pad=SDL_JoystickOpen(0);
    uint32_t now=SDL_GetTicks(); float dt=(now-last)/1000.f; last=now; if(dt>0.05f)dt=0.05f; if(dt<=0)dt=0.001f;
    Input in;
    float lx=dz(axisF(0)),ly=dz(axisF(1)),rx=dz(axisF(2)),ry=dz(axisF(3));
    in.mx=lx; in.my=-ly;
    in.lx=rx*std::fabs(rx)*3.6f+rx*0.8f; in.ly=-ry*std::fabs(ry)*2.4f-ry*0.6f;
    // أزرار: 0 X,1 O,2 □,3 △,4 L1,5 R1,9 OPTIONS,11 L3,13-16 dpad,18 L2,19 R2
    in.fire=btn(19)||axisF(5)>0.3f; in.ads=btn(18)||axisF(4)>0.3f;
    in.jump=btn(0); in.crouch=btn(1); in.reload=btn(2); in.swapP=btn(4); in.swapN=btn(5); in.sprint=btn(11); in.start=btn(9); in.confirm=btn(0); in.back=btn(1);
    bool d4=btn(3); if(d4&&!dprev)dbg=!dbg; dprev=d4;
    if(g.menu){ if(btn(16))in.mx=1; else if(btn(15))in.mx=-1; }
    g.update(dt,in);
    if(g.menu)g.renderMenu(); else g.render();
    if(dbg){ char b[96]; snprintf(b,96,"AX %d %d %d %d %d %d",(int)(axisF(0)*100),(int)(axisF(1)*100),(int)(axisF(2)*100),(int)(axisF(3)*100),(int)(axisF(4)*100),(int)(axisF(5)*100)); g.text(40,60,b,2,0xFFFFFF00);
      std::string s; for(int i=0;i<24;i++)if(btn(i)){char c[8];snprintf(c,8,"%d ",i);s+=c;} g.text(40,80,("BTN "+s).c_str(),2,0xFFFFFF00); snprintf(b,96,"DT %d MS",(int)(dt*1000)); g.text(40,100,b,2,0xFFFFFF00); }
    // رفع الإطار 2x إلى سطح النافذة
    SDL_LockSurface(surf); uint32_t*dst=(uint32_t*)surf->pixels; int pitch=surf->pitch/4,sw=surf->w,sh=surf->h;
    int sx=sw/RW>1?sw/RW:1,sy=sh/RH>1?sh/RH:1; int ox=(sw-RW*sx)/2,oy=(sh-RH*sy)/2;
    for(int y=0;y<RH;y++){ const uint32_t*s=&g.r.fb[y*RW]; for(int k=0;k<sy;k++){ uint32_t*d=dst+(oy+y*sy+k)*pitch+ox; for(int x=0;x<RW;x++){uint32_t c=s[x];for(int q=0;q<sx;q++)*d++=c;} } }
    SDL_UnlockSurface(surf); SDL_UpdateWindowSurface(win);
  }
}
