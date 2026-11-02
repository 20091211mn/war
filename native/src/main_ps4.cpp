// منصة PS4 (OpenOrbis + SDL2 للعرض + libScePad لليد مباشرة: أزرار وعصي مضبوطة بدون تخمين)
#include <SDL2/SDL.h>
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/AudioOut.h>
#include <pthread.h>
#include "game.h"

static int padH=-1;
static OrbisPadData pd;
static bool B(uint32_t m){return (pd.buttons&m)!=0;}
static float stickN(uint8_t v){return ((int)v-128)/127.f;}

static void* audioThread(void*){
  sceAudioOutInit(); int h=sceAudioOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM,ORBIS_AUDIO_OUT_PORT_TYPE_MAIN,0,256,48000,ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);
  if(h<=0)return NULL; static int16_t buf[256*2];
  for(;;){ SFX::mix(buf,256); sceAudioOutOutput(h,buf); }
  return NULL; }

int main(int,char**){
  setvbuf(stdout,NULL,_IONBF,0);
  if(SDL_Init(SDL_INIT_VIDEO)!=0)for(;;);
  SDL_Window*win=SDL_CreateWindow("wz",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,1920,1080,0);
  if(!win)for(;;);
  SDL_Surface*surf=SDL_GetWindowSurface(win);
  scePadInit(); { pthread_t at; pthread_create(&at,NULL,audioThread,NULL); }
  OrbisUserServiceInitializeParams up; up.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST; sceUserServiceInitialize(&up);
  int uid=0; sceUserServiceGetInitialUser(&uid);
  padH=scePadOpen(uid,0,0,NULL);
  static const int RESW=1920,RESH=1080; static Game g; g.r.startThreads(6); g.menu=1; g.init(RESW,RESH); // دقة ثابتة 1080p
  uint32_t last=SDL_GetTicks(); bool dbg=false; uint32_t prevB=0; float acc=0,fpsT=0,fps=30; int fpsN=0; float smx=0,smy=0; int frame=0;
  for(;;){
    SDL_Event ev; while(SDL_PollEvent(&ev)){}
    memset(&pd,0,sizeof(pd)); if(padH>=0)scePadReadState(padH,&pd); else padH=scePadOpen(uid,0,0,NULL);
    uint32_t now=SDL_GetTicks(); float dt=(now-last)/1000.f; last=now; if(dt>0.1f)dt=0.1f; if(dt<=0)dt=0.001f; frame++;
    fpsT+=dt; fpsN++; if(fpsT>=1.f){fps=fpsN/fpsT;g.fpsShow=fps;fpsT=0;fpsN=0;}
    uint32_t edge=pd.buttons&~prevB; prevB=pd.buttons;
    auto stick=[&](float x,float y,float&ox,float&oy){float m=sqrtf(x*x+y*y);const float D=0.18f;if(m<D){ox=oy=0;return;}float k=(m-D)/(1-D);if(k>1)k=1;ox=x/m*k;oy=y/m*k;};
    float ax,ay,bx,by; stick(stickN(pd.leftStick.x),-stickN(pd.leftStick.y),ax,ay); stick(stickN(pd.rightStick.x),-stickN(pd.rightStick.y),bx,by); // y موجب = أعلى
    float lx_,ly_,rx_,ry_; if(g.swapSticks){rx_=ax;ry_=ay;lx_=bx;ly_=by;} else {lx_=ax;ly_=ay;rx_=bx;ry_=by;} // l = حركة ، r = نظر
    Input in;
    float a=std::min<float>(1.f,dt*14.f); smx+=(lx_-smx)*a; smy+=(ly_-smy)*a; if(lx_==0&&fabsf(smx)<0.05f)smx=0; if(ly_==0&&fabsf(smy)<0.05f)smy=0;
    in.mx=smx; in.my=smy;
    float rm=sqrtf(rx_*rx_+ry_*ry_); float cv=rm*rm*1.9f+rm*0.45f; float rk=rm>0.001f?cv/rm:0; in.lx=rx_*rk; in.ly=ry_*rk*0.75f;
    in.fire=B(ORBIS_PAD_BUTTON_R2)||pd.analogButtons.r2>110; in.ads=B(ORBIS_PAD_BUTTON_L2)||pd.analogButtons.l2>110;
    in.jump=B(ORBIS_PAD_BUTTON_CROSS); in.crouch=B(ORBIS_PAD_BUTTON_CIRCLE); in.reload=B(ORBIS_PAD_BUTTON_SQUARE);
    in.swapP=B(ORBIS_PAD_BUTTON_L1); in.swapN=B(ORBIS_PAD_BUTTON_R1);
    in.sprint=g.swapSticks?B(ORBIS_PAD_BUTTON_R3):B(ORBIS_PAD_BUTTON_L3); // الضغط على عصا الحركة = ركض
    in.use=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_LEFT); in.drone=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_DOWN); in.grenade=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_UP); in.tri=B(ORBIS_PAD_BUTTON_TRIANGLE); in.start=B(ORBIS_PAD_BUTTON_OPTIONS); in.confirm=B(ORBIS_PAD_BUTTON_CROSS); in.back=B(ORBIS_PAD_BUTTON_CIRCLE);
    if(g.menu||g.paused){ if(B(ORBIS_PAD_BUTTON_RIGHT))in.mx=1; else if(B(ORBIS_PAD_BUTTON_LEFT))in.mx=-1; if(g.menu)in.my=B(ORBIS_PAD_BUTTON_UP)?1.f:B(ORBIS_PAD_BUTTON_DOWN)?-1.f:0.f;
      if((edge&ORBIS_PAD_BUTTON_R1)&&(g.menu==1||g.paused))g.swapSticks=!g.swapSticks; }
    if(edge&ORBIS_PAD_BUTTON_TOUCH_PAD)dbg=!dbg;
    acc+=dt; int steps=0; while(acc>=1.f/60.f&&steps<4){ g.update(1.f/60.f,in); acc-=1.f/60.f; steps++; } if(steps==4)acc=0;
    if(g.menu)g.renderMenu(); else g.render();
    if(dbg){ char b[96]; snprintf(b,96,"FPS %d RES %d",(int)fps,g.r.W); g.text(40,40,b,2,0xFFFFFF00);
      snprintf(b,96,"LS %d %d RS %d %d L2 %d R2 %d",pd.leftStick.x,pd.leftStick.y,pd.rightStick.x,pd.rightStick.y,pd.analogButtons.l2,pd.analogButtons.r2); g.text(40,60,b,2,0xFFFFFF00);
      snprintf(b,96,"BTN %05X PAD %d",(unsigned)pd.buttons,padH); g.text(40,80,b,2,0xFFFFFF00); }
    g.r.flush(); SDL_LockSurface(surf); uint32_t*dst=(uint32_t*)surf->pixels; int pitch=surf->pitch/4,sw=surf->w,sh=surf->h; int RWc=g.r.W,RHc=g.r.H;
    static std::vector<int> xm; if((int)xm.size()!=sw||xm.back()!=(sw-1)*RWc/sw){xm.assign(sw,0);for(int x=0;x<sw;x++)xm[x]=x*RWc/sw;}
    if(RWc==sw&&RHc==sh){ std::atomic<int> nx(0); const uint32_t*fbp=g.r.fb.data(); g.r.pool.exec([&]{ for(;;){ int y0=nx.fetch_add(16); if(y0>=sh)break; int y1=std::min(sh,y0+16); for(int y=y0;y<y1;y++)memcpy(dst+y*pitch,fbp+y*RWc,sw*4); } }); } else
    { std::atomic<int> nx(0); const uint32_t*fbp=g.r.fb.data(); g.r.pool.exec([&]{ for(;;){ int y0=nx.fetch_add(32); if(y0>=sh)break; for(int y=y0;y<std::min(sh,y0+32);y++){ const uint32_t*sr=fbp+(y*RHc/sh)*RWc; uint32_t*d=dst+y*pitch; for(int x=0;x<sw;x++)d[x]=sr[xm[x]]; } } }); }
    SDL_UnlockSurface(surf); SDL_UpdateWindowSurface(win);
    { uint32_t el=SDL_GetTicks()-now; if(el<16)sceKernelUsleep((16-el)*1000); } // سقف 60fps (الحدّ الأعلى فقط، بلا إبطاء)
  }
}
