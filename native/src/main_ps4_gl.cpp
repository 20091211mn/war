// منصة PS4 (OpenOrbis + SDL2 للعرض + libScePad لليد مباشرة: أزرار وعصي مضبوطة بدون تخمين)
#include <orbis/Pigletv2VSH.h>
#include <orbis/SystemService.h>
#include <orbis/Sysmodule.h>
#include <string>
#include <stdarg.h>
#include <orbis/libkernel.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include "game.h"

static int padH=-1;
static OrbisPadData pd;
static bool B(uint32_t m){return (pd.buttons&m)!=0;}
static float stickN(uint8_t v){return ((int)v-128)/127.f;}


// ===== نسخة تجريبية: الـ GPU (Piglet/GLES2) يكبّر الصورة إلى 1080p؛ المعالج يرسم المشهد بدقة أقل =====
#ifndef WZ_IW
#define WZ_IW 1280
#endif
#ifndef WZ_IH
#define WZ_IH 720
#endif
#define OUT_W 1920
#define OUT_H 1080
static void wzlog(const char*f,...){ FILE*fp=fopen("/data/wz_gl_log.txt","a"); if(!fp)return; va_list a; va_start(a,f); vfprintf(fp,f,a); va_end(a); fputc('\n',fp); fclose(fp); }
static uint32_t ticks(){ return (uint32_t)(sceKernelGetProcessTime()/1000); }
struct NikShader{ char*ident; unsigned char hash[16]; uint64_t len; unsigned char*code; };
extern "C" NikShader scePrecompiledShaderEntries[];
static EGLDisplay gD=EGL_NO_DISPLAY; static EGLSurface gS=EGL_NO_SURFACE; static EGLContext gC=EGL_NO_CONTEXT;
static GLuint gTex=0,gProg=0; static GLint uMV=-1,uPR=-1,uTS=-1,uSam=-1,uOp=-1,aV=-1;
static bool fetchSh(const char*n,unsigned char*&p,uint64_t&l){ for(NikShader*e=scePrecompiledShaderEntries;e->ident;++e) if(!strcmp(n,e->ident)){p=e->code;l=e->len;return true;} return false; }
static bool glInit(){
  int mret=0; std::string pre=std::string("/")+sceKernelGetFsSandboxRandomWord()+"/common/lib/";
  int pg=(int)sceKernelLoadStartModule((pre+"libScePigletv2VSH.sprx").c_str(),0,NULL,0,NULL,&mret); if(pg<0){wzlog("piglet load fail 0x%x",pg);return false;}
  int pc=(int)sceKernelLoadStartModule((pre+"libScePrecompiledShaders.sprx").c_str(),0,NULL,0,NULL,&mret); if(pc<0){wzlog("shaders load fail 0x%x",pc);return false;}
  OrbisPglConfig c; memset(&c,0,sizeof(c)); c.size=sizeof(c); c.flags=ORBIS_PGL_FLAGS_USE_COMPOSITE_EXT|ORBIS_PGL_FLAGS_USE_FLEXIBLE_MEMORY|0x60; c.processOrder=1;
  c.systemSharedMemorySize=250*1024*1024; c.videoSharedMemorySize=512*1024*1024; c.maxMappedFlexibleMemory=170*1024*1024; c.drawCommandBufferSize=1*1024*1024; c.lcueResourceBufferSize=1*1024*1024;
  c.dbgPosCmd_0x40=OUT_W; c.dbgPosCmd_0x44=OUT_H; c.dbgPosCmd_0x48=0; c.dbgPosCmd_0x4C=0; c.unk_0x5C=2;
  if(!scePigletSetConfigurationVSH(&c)){wzlog("piglet config fail");return false;}
  OrbisPglWindow w={0,(khronos_uint32_t)OUT_W,(khronos_uint32_t)OUT_H};
  const EGLint at[]={EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,0,EGL_STENCIL_SIZE,0,EGL_SAMPLE_BUFFERS,0,EGL_SAMPLES,0,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_NONE};
  const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE}; const EGLint wa[]={EGL_RENDER_BUFFER,EGL_BACK_BUFFER,EGL_NONE};
  EGLConfig cfg; EGLint nc=0,maj=0,mi=0;
  gD=eglGetDisplay(EGL_DEFAULT_DISPLAY); if(gD==EGL_NO_DISPLAY){wzlog("no display");return false;}
  if(!eglInitialize(gD,&maj,&mi)){wzlog("egl init fail 0x%x",eglGetError());return false;}
  eglBindAPI(EGL_OPENGL_ES_API); eglSwapInterval(gD,1);
  if(!eglChooseConfig(gD,at,&cfg,1,&nc)||nc<1){wzlog("choose cfg fail");return false;}
  gS=eglCreateWindowSurface(gD,cfg,&w,wa); if(gS==EGL_NO_SURFACE){wzlog("surface fail 0x%x",eglGetError());return false;}
  gC=eglCreateContext(gD,cfg,EGL_NO_CONTEXT,ca); if(gC==EGL_NO_CONTEXT){wzlog("ctx fail 0x%x",eglGetError());return false;}
  if(!eglMakeCurrent(gD,gS,gS,gC)){wzlog("makecurrent fail 0x%x",eglGetError());return false;}
  wzlog("GL %s | %s",(const char*)glGetString(GL_VERSION),(const char*)glGetString(GL_RENDERER));
  sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE); sceSystemServiceHideSplashScreen();
  glPixelStorei(GL_UNPACK_ALIGNMENT,1); glGenTextures(1,&gTex); glBindTexture(GL_TEXTURE_2D,gTex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,WZ_IW,WZ_IH,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  if(glGetError()){wzlog("tex fail");return false;}
  unsigned char*vp,*fp; uint64_t vl,fl; if(!fetchSh("texmap/v_2.vert",vp,vl)||!fetchSh("texmap/f_2.frag",fp,fl)){wzlog("shader entries missing");return false;}
  GLuint fs=glCreateShader(GL_FRAGMENT_SHADER),vs=glCreateShader(GL_VERTEX_SHADER); gProg=glCreateProgram();
  glShaderBinary(1,&vs,0,vp,vl); glShaderBinary(1,&fs,0,fp,fl); glAttachShader(gProg,vs); glAttachShader(gProg,fs); glLinkProgram(gProg);
  if(glGetError()){wzlog("shader link fail");return false;}
  uMV=glGetUniformLocation(gProg,"u_modelViewMatrix"); uPR=glGetUniformLocation(gProg,"u_projectionMatrix"); uTS=glGetUniformLocation(gProg,"u_textureSpaceMatrix");
  uSam=glGetUniformLocation(gProg,"s_sampler"); uOp=glGetUniformLocation(gProg,"u_opacity"); aV=glGetAttribLocation(gProg,"a_vertex");
  if(uMV<0||uPR<0||uTS<0||uSam<0||uOp<0||aV<0){wzlog("uniform lookup fail");return false;}
  wzlog("piglet ok");
  return true;
}
static void glPresent(const uint32_t*up){
  glViewport(0,0,OUT_W,OUT_H); glClearColor(0,0,0,1); glClear(GL_COLOR_BUFFER_BIT);
  glBindTexture(GL_TEXTURE_2D,gTex); glTexSubImage2D(GL_TEXTURE_2D,0,0,0,WZ_IW,WZ_IH,GL_RGBA,GL_UNSIGNED_BYTE,up);
  static const GLfloat v[]={0,1,0,1, 1,0,1,0, 0,0,0,0, 0,1,0,1, 1,1,1,1, 1,0,1,0};
  GLfloat id[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  GLfloat mv[16]={(GLfloat)OUT_W,0,0,0, 0,(GLfloat)OUT_H,0,0, 0,0,1,0, 0,0,0,1};
  GLfloat pr[16]={2.f/OUT_W,0,0,0, 0,-2.f/OUT_H,0,0, 0,0,-1,0, -1,1,0,1};
  glUseProgram(gProg); glUniform1f(uOp,1.f); glUniformMatrix4fv(uMV,1,GL_FALSE,mv); glUniformMatrix4fv(uPR,1,GL_FALSE,pr); glUniformMatrix4fv(uTS,1,GL_FALSE,id);
  glVertexAttribPointer(aV,4,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),v); glEnableVertexAttribArray(aV);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,gTex); glUniform1i(uSam,0); glDrawArrays(GL_TRIANGLES,0,6);
  eglSwapBuffers(gD,gS);
}

int main(int,char**){
  setvbuf(stdout,NULL,_IONBF,0);
  if(!glInit())for(;;);
  scePadInit();
  OrbisUserServiceInitializeParams up; up.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST; sceUserServiceInitialize(&up);
  int uid=0; sceUserServiceGetInitialUser(&uid);
  padH=scePadOpen(uid,0,0,NULL);
  static Game g; g.r.startThreads(6); g.menu=1; g.init(WZ_IW,WZ_IH); static std::vector<uint32_t> upb(WZ_IW*WZ_IH); // GPU يكبّر إلى 1080p
  uint32_t last=ticks(); bool dbg=false; uint32_t prevB=0; float acc=0,fpsT=0,fps=30; int fpsN=0; float smx=0,smy=0; int frame=0;
  for(;;){
    memset(&pd,0,sizeof(pd)); if(padH>=0)scePadReadState(padH,&pd); else padH=scePadOpen(uid,0,0,NULL);
    uint32_t now=ticks(); float dt=(now-last)/1000.f; last=now; if(dt>0.1f)dt=0.1f; if(dt<=0)dt=0.001f; frame++;
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
    in.tri=B(ORBIS_PAD_BUTTON_TRIANGLE); in.start=B(ORBIS_PAD_BUTTON_OPTIONS); in.confirm=B(ORBIS_PAD_BUTTON_CROSS); in.back=B(ORBIS_PAD_BUTTON_CIRCLE);
    if(g.menu||g.paused){ if(B(ORBIS_PAD_BUTTON_RIGHT))in.mx=1; else if(B(ORBIS_PAD_BUTTON_LEFT))in.mx=-1; if(g.menu)in.my=B(ORBIS_PAD_BUTTON_UP)?1.f:B(ORBIS_PAD_BUTTON_DOWN)?-1.f:0.f;
      if((edge&ORBIS_PAD_BUTTON_R1)&&(g.menu==1||g.paused))g.swapSticks=!g.swapSticks; }
    if(edge&ORBIS_PAD_BUTTON_TOUCH_PAD)dbg=!dbg;
    acc+=dt; int steps=0; while(acc>=1.f/60.f&&steps<4){ g.update(1.f/60.f,in); acc-=1.f/60.f; steps++; } if(steps==4)acc=0;
    if(g.menu)g.renderMenu(); else g.render();
    if(dbg){ char b[96]; snprintf(b,96,"FPS %d RES %d",(int)fps,g.r.W); g.text(40,40,b,2,0xFFFFFF00);
      snprintf(b,96,"LS %d %d RS %d %d L2 %d R2 %d",pd.leftStick.x,pd.leftStick.y,pd.rightStick.x,pd.rightStick.y,pd.analogButtons.l2,pd.analogButtons.r2); g.text(40,60,b,2,0xFFFFFF00);
      snprintf(b,96,"BTN %05X PAD %d",(unsigned)pd.buttons,padH); g.text(40,80,b,2,0xFFFFFF00); }
    g.r.flush(); { const uint32_t*fbp=g.r.fb.data(); uint32_t*ub=upb.data(); int n=WZ_IW*WZ_IH; std::atomic<int> nx(0); g.r.pool.exec([&]{ for(;;){ int i0=nx.fetch_add(65536); if(i0>=n)break; int i1=std::min(n,i0+65536); for(int i=i0;i<i1;i++){ uint32_t p=fbp[i]; ub[i]=(p&0x0000FF00u)|((p>>16)&0xFFu)|((p&0xFFu)<<16)|0xFF000000u; } } }); }
    glPresent(upb.data());
    { uint32_t el=ticks()-now; if(el<33)sceKernelUsleep((33-el)*1000); } // سقف 30fps
  }
}
