// منصة PS4 (OpenOrbis + SDL2 للعرض + libScePad لليد مباشرة: أزرار وعصي مضبوطة بدون تخمين)
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <orbis/libkernel.h>
#include <orbis/Pigletv2VSH.h>
#include <orbis/SystemService.h>
#include <orbis/Sysmodule.h>
#include <string>
#include <stdarg.h>
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


// ===== نسخة تجريبية: الـ GPU (Piglet/GLES2) يكبّر الصورة إلى 1080p؛ المعالج يرسم المشهد بدقة أقل =====
#ifndef WZ_IW
#define WZ_IW 1280
#endif
#ifndef WZ_IH
#define WZ_IH 720
#endif
#define OUT_W 1920
#define OUT_H 1080
#ifndef WZ_VARIANT
#define WZ_VARIANT 1
#endif
#if WZ_VARIANT==4||WZ_VARIANT==5
#define WZ_GPU 1
#else
#define WZ_GPU 0
#endif
#ifndef WZ_THREADS
#define WZ_THREADS 1
#endif
#if WZ_VARIANT==2
#define WZ_LOGFILE "/data/wz_gl_log2.txt"
#elif WZ_VARIANT==3
#define WZ_LOGFILE "/data/wz_gl_log3.txt"
#elif WZ_VARIANT==4
#define WZ_LOGFILE "/data/wz_gl_log4.txt"
#elif WZ_VARIANT==5
#define WZ_LOGFILE "/data/wz_gl_log5.txt"
#else
#define WZ_LOGFILE "/data/wz_gl_log.txt"
#endif
static void wzlog(const char*f,...){ FILE*fp=fopen(WZ_LOGFILE,"a"); if(!fp)return; va_list a; va_start(a,f); vfprintf(fp,f,a); va_end(a); fputc('\n',fp); fclose(fp); }
#if WZ_VARIANT==5
static void* audioThread(void*){ wzlog("audio: thread start"); SFX::initBuffers(); wzlog("audio: buffers ok"); { int am=sceSysmoduleLoadModuleInternal((decltype(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE))0x80000001); wzlog("audio: internal AUDIOOUT module 0x%x",(unsigned)am); } int ai=sceAudioOutInit(); wzlog("audio: init 0x%x",(unsigned)ai); int h=sceAudioOutOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM,ORBIS_AUDIO_OUT_PORT_TYPE_MAIN,0,256,48000,ORBIS_AUDIO_OUT_PARAM_FORMAT_S16_STEREO); wzlog("audio: open %d",h);
  if(h<=0)return NULL; static int16_t buf[256*2]; for(;;){ SFX::mix(buf,256); sceAudioOutOutput(h,buf); } return NULL; }
#endif
static uint32_t ticks(){ return (uint32_t)(sceKernelGetProcessTime()/1000); }
struct NikShader{ char*ident; unsigned char hash[16]; uint64_t len; unsigned char*code; };
extern "C" NikShader scePrecompiledShaderEntries[];
static EGLDisplay gD=EGL_NO_DISPLAY; static EGLSurface gS=EGL_NO_SURFACE; static EGLContext gC=EGL_NO_CONTEXT;
static GLuint gTex=0,gProg=0; static GLint uMV=-1,uPR=-1,uTS=-1,uSam=-1,uOp=-1,aV=-1;
static int gDepthBits=0;
static int gShN=0; // عدد الشيدرات الجاهزة في النظام (تشخيص: لنعرف هل يتوفّر شيدر ثلاثي الأبعاد بألوان رؤوس)
static void dumpShaders(){ FILE*fp=fopen("/data/wz_shaders.txt","w"); int n=0; for(NikShader*e=scePrecompiledShaderEntries;e->ident;++e){ n++; if(fp)fprintf(fp,"%s %llu\n",e->ident,(unsigned long long)e->len); } if(fp)fclose(fp); gShN=n; wzlog("precompiled shaders: %d (list in /data/wz_shaders.txt)",n); }
static bool fetchSh(const char*n,unsigned char*&p,uint64_t&l){ for(NikShader*e=scePrecompiledShaderEntries;e->ident;++e) if(!strcmp(n,e->ident)){p=e->code;l=e->len;return true;} return false; }
static void probeProg(const char*vn,const char*fn){ unsigned char*vp,*fp; uint64_t vl,fl; if(!fetchSh(vn,vp,vl)||!fetchSh(fn,fp,fl)){wzlog("probe %s+%s: missing",vn,fn);return;}
  wzlog("probe %s + %s: linking",vn,fn); GLuint vs=glCreateShader(GL_VERTEX_SHADER),fs=glCreateShader(GL_FRAGMENT_SHADER),pg=glCreateProgram(); glShaderBinary(1,&vs,0,vp,vl); glShaderBinary(1,&fs,0,fp,fl); glAttachShader(pg,vs); glAttachShader(pg,fs); glLinkProgram(pg);
  GLint na=0,nu=0; glGetProgramiv(pg,GL_ACTIVE_ATTRIBUTES,&na); glGetProgramiv(pg,GL_ACTIVE_UNIFORMS,&nu); wzlog("  err=0x%x attrs=%d uniforms=%d",(unsigned)glGetError(),na,nu);
  for(int i=0;i<na&&i<16;i++){ char nm[64]; GLsizei l=0; GLint sz=0; GLenum ty=0; glGetActiveAttrib(pg,i,64,&l,&sz,&ty,nm); wzlog("  attr %d: %s type=0x%x size=%d loc=%d",i,nm,(unsigned)ty,sz,glGetAttribLocation(pg,nm)); }
  for(int i=0;i<nu&&i<16;i++){ char nm[64]; GLsizei l=0; GLint sz=0; GLenum ty=0; glGetActiveUniform(pg,i,64,&l,&sz,&ty,nm); wzlog("  uniform %d: %s type=0x%x size=%d",i,nm,(unsigned)ty,sz); } }
static bool glInit(){
  wzlog("glInit begin"); int mret=0; const char*rw=sceKernelGetFsSandboxRandomWord(); wzlog("sandbox word: %s",rw?rw:"(null)"); std::string pre=std::string("/")+(rw?rw:"")+"/common/lib/";
  wzlog("loading piglet: %s",(pre+"libScePigletv2VSH.sprx").c_str()); int pg=(int)sceKernelLoadStartModule((pre+"libScePigletv2VSH.sprx").c_str(),0,NULL,0,NULL,&mret); if(pg<0){wzlog("piglet load fail 0x%x",pg);return false;}
  wzlog("piglet loaded id=%d",pg); int pc=(int)sceKernelLoadStartModule((pre+"libScePrecompiledShaders.sprx").c_str(),0,NULL,0,NULL,&mret); if(pc<0){wzlog("shaders load fail 0x%x",pc);return false;}
  wzlog("shaders module loaded id=%d",pc); OrbisPglConfig c; memset(&c,0,sizeof(c)); c.size=sizeof(c); c.flags=ORBIS_PGL_FLAGS_USE_COMPOSITE_EXT|ORBIS_PGL_FLAGS_USE_FLEXIBLE_MEMORY|0x60; c.processOrder=1;
  c.systemSharedMemorySize=250*1024*1024; c.videoSharedMemorySize=512*1024*1024; c.maxMappedFlexibleMemory=170*1024*1024; c.drawCommandBufferSize=1*1024*1024; c.lcueResourceBufferSize=1*1024*1024;
  c.dbgPosCmd_0x40=OUT_W; c.dbgPosCmd_0x44=OUT_H; c.dbgPosCmd_0x48=0; c.dbgPosCmd_0x4C=0; c.unk_0x5C=2;
  if(!scePigletSetConfigurationVSH(&c)){wzlog("piglet config fail");return false;}
  wzlog("piglet config set"); OrbisPglWindow w={0,(khronos_uint32_t)OUT_W,(khronos_uint32_t)OUT_H};
  EGLint at[]={EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,0,EGL_STENCIL_SIZE,0,EGL_SAMPLE_BUFFERS,0,EGL_SAMPLES,0,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_NONE};
  const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE}; const EGLint wa[]={EGL_RENDER_BUFFER,EGL_BACK_BUFFER,EGL_NONE};
  EGLConfig cfg; EGLint nc=0,maj=0,mi=0;
  gD=eglGetDisplay(EGL_DEFAULT_DISPLAY); if(gD==EGL_NO_DISPLAY){wzlog("no display");return false;}
  if(!eglInitialize(gD,&maj,&mi)){wzlog("egl init fail 0x%x",eglGetError());return false;}
  wzlog("egl init %d.%d",maj,mi); eglBindAPI(EGL_OPENGL_ES_API); eglSwapInterval(gD,0);
#if WZ_GPU
  const int dl[3]={24,16,0};
#else
  const int dl[1]={0};
#endif
  for(size_t di=0;di<sizeof(dl)/sizeof(dl[0]);di++){ at[9]=dl[di]; nc=0; if(!eglChooseConfig(gD,at,&cfg,1,&nc)||nc<1){wzlog("choose cfg fail depth %d",dl[di]);continue;}
    gS=eglCreateWindowSurface(gD,cfg,&w,wa); if(gS==EGL_NO_SURFACE){wzlog("surface fail depth %d 0x%x",dl[di],eglGetError());continue;}
    EGLint gotD=-1; eglGetConfigAttrib(gD,cfg,EGL_DEPTH_SIZE,&gotD); gDepthBits=gotD; wzlog("config chosen: requested depth %d got %d",dl[di],(int)gotD); break; }
  if(gS==EGL_NO_SURFACE){wzlog("no usable config/surface");return false;}
  wzlog("surface created"); gC=eglCreateContext(gD,cfg,EGL_NO_CONTEXT,ca); if(gC==EGL_NO_CONTEXT){wzlog("ctx fail 0x%x",eglGetError());return false;}
  wzlog("context created"); if(!eglMakeCurrent(gD,gS,gS,gC)){wzlog("makecurrent fail 0x%x",eglGetError());return false;}
  wzlog("GL %s | %s",(const char*)glGetString(GL_VERSION),(const char*)glGetString(GL_RENDERER));
  wzlog("sysmodule internal"); sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE); sceSystemServiceHideSplashScreen();
  glPixelStorei(GL_UNPACK_ALIGNMENT,1); glGenTextures(1,&gTex); glBindTexture(GL_TEXTURE_2D,gTex);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,WZ_IW,WZ_IH,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  if(glGetError()){wzlog("tex fail");return false;}
  wzlog("texture ok"); unsigned char*vp,*fp; uint64_t vl,fl; if(!fetchSh("texmap/v_2.vert",vp,vl)||!fetchSh("texmap/f_2.frag",fp,fl)){wzlog("shader entries missing");return false;}
  dumpShaders();
  wzlog("shader entries found %llu/%llu",(unsigned long long)vl,(unsigned long long)fl); GLuint fs=glCreateShader(GL_FRAGMENT_SHADER),vs=glCreateShader(GL_VERTEX_SHADER); gProg=glCreateProgram();
  glShaderBinary(1,&vs,0,vp,vl); glShaderBinary(1,&fs,0,fp,fl); glAttachShader(gProg,vs); glAttachShader(gProg,fs); glLinkProgram(gProg);
  if(glGetError()){wzlog("shader link fail");return false;}
  uMV=glGetUniformLocation(gProg,"u_modelViewMatrix"); uPR=glGetUniformLocation(gProg,"u_projectionMatrix"); uTS=glGetUniformLocation(gProg,"u_textureSpaceMatrix");
  uSam=glGetUniformLocation(gProg,"s_sampler"); uOp=glGetUniformLocation(gProg,"u_opacity"); aV=glGetAttribLocation(gProg,"a_vertex");
  if(uMV<0||uPR<0||uTS<0||uSam<0||uOp<0||aV<0){wzlog("uniform lookup fail");return false;}
  wzlog("piglet ok");
  probeProg("swordfish/vpBasic.vert","swordfish/fpSolidColor.frag"); probeProg("swordfish/vpBasic.vert","swordfish/fpAlphaColor.frag"); probeProg("trilithium/vpBasic.vert","trilithium/fpTexture.frag"); probeProg("khronos/earth.vert","khronos/earth.frag"); probeProg("polymer/orbis_highlight.vert","polymer/orbis_highlight.frag");
  wzlog("probe done");
  return true;
}
static void glPresent(const uint32_t*up,bool overlay=false){
  glViewport(0,0,OUT_W,OUT_H); if(!overlay){ glClearColor(0.05f,0.2f,0.6f,1.f); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT); } glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); if(overlay)glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA); else glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glBindTexture(GL_TEXTURE_2D,gTex); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,WZ_IW,WZ_IH,0,GL_RGBA,GL_UNSIGNED_BYTE,up);
  static const GLfloat v[]={0,1,0,1, 1,0,1,0, 0,0,0,0, 0,1,0,1, 1,1,1,1, 1,0,1,0};
  GLfloat id[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  GLfloat mv[16]={(GLfloat)OUT_W,0,0,0, 0,(GLfloat)OUT_H,0,0, 0,0,1,0, 0,0,0,1};
  GLfloat pr[16]={2.f/OUT_W,0,0,0, 0,-2.f/OUT_H,0,0, 0,0,-1,0, -1,1,0,1};
  glUseProgram(gProg); glUniform1f(uOp,1.f); glUniformMatrix4fv(uMV,1,GL_FALSE,mv); glUniformMatrix4fv(uPR,1,GL_FALSE,pr); glUniformMatrix4fv(uTS,1,GL_FALSE,id);
  glVertexAttribPointer(aV,4,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),v); glEnableVertexAttribArray(aV);
  glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,gTex); glUniform1i(uSam,0); glDrawArrays(GL_TRIANGLES,0,6);
  static int pf=0; if(++pf<=2)wzlog("draw issued err=0x%x",(unsigned)glGetError());
  if(pf<=2)wzlog("calling swap"); EGLBoolean sr=eglSwapBuffers(gD,gS); if(pf<=2)wzlog("swap ret=%d err=0x%x",(int)sr,(unsigned)eglGetError());
}

// ===== نسخة 4: المشهد ثلاثي الأبعاد يُرسم على الـGPU (Piglet) بشيدر trilithium (نسيج فقط) ؛ اللون = نسيج لوحة 6-6-6 ؛ الواجهة (HUD) طبقة شفافة من المعالج فوقه =====
static void m4mul(const float*a,const float*b,float*o);
#if WZ_GPU
static GLuint gpPg=0,gpPal=0,gpSky=0,gpDet[3]={0,0,0}; static bool gpDetOk=false; static GLint gpVP=-1,gpM=-1,gpSam=-1,gpaP=-1,gpaT=-1; static const int SKW=96,SKH=54;
static bool gpuInit(){
  unsigned char*vp,*fp; uint64_t vl,fl; if(!fetchSh("trilithium/vpBasic.vert",vp,vl)||!fetchSh("trilithium/fpTexture.frag",fp,fl)){wzlog("gpu: trilithium shaders missing");return false;}
  GLuint vs=glCreateShader(GL_VERTEX_SHADER),fs=glCreateShader(GL_FRAGMENT_SHADER); gpPg=glCreateProgram(); glShaderBinary(1,&vs,0,vp,vl); glShaderBinary(1,&fs,0,fp,fl); glAttachShader(gpPg,vs); glAttachShader(gpPg,fs); glLinkProgram(gpPg);
  GLint ok=0; glGetProgramiv(gpPg,GL_LINK_STATUS,&ok); GLint nu=0; glGetProgramiv(gpPg,GL_ACTIVE_UNIFORMS,&nu); wzlog("gpu: link ok=%d err=0x%x uniforms=%d",(int)ok,(unsigned)glGetError(),(int)nu);
  for(int i=0;i<nu&&i<16;i++){ char nm[64]; GLsizei l=0; GLint sz=0; GLenum ty=0; glGetActiveUniform(gpPg,i,64,&l,&sz,&ty,nm); GLint loc=glGetUniformLocation(gpPg,nm); wzlog("gpu: uniform %s type=0x%x size=%d loc=%d",nm,(unsigned)ty,(int)sz,(int)loc); if(ty==0x8B5E)gpSam=loc; }
  gpVP=glGetUniformLocation(gpPg,"uVPMatrix"); gpM=glGetUniformLocation(gpPg,"uMMatrix"); gpaP=glGetAttribLocation(gpPg,"aPosition"); gpaT=glGetAttribLocation(gpPg,"aTextureCoord");
  wzlog("gpu: VP%d M%d sampler%d aP%d aT%d",(int)gpVP,(int)gpM,(int)gpSam,(int)gpaP,(int)gpaT); if(gpVP<0||gpaP<0||gpaT<0)return false;
  static std::vector<uint32_t> pal(1024*1024); for(int i=0;i<1024*1024;i++){ int r7=(i>>13)&127,g7=(i>>6)&127,b6=i&63; pal[i]=0xFF000000u|((uint32_t)(b6*255/63)<<16)|((uint32_t)(g7*255/127)<<8)|(uint32_t)(r7*255/127); }
  glGenTextures(1,&gpPal); glBindTexture(GL_TEXTURE_2D,gpPal); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,1024,1024,0,GL_RGBA,GL_UNSIGNED_BYTE,pal.data());
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glGenTextures(1,&gpSky); glBindTexture(GL_TEXTURE_2D,gpSky); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,SKW,SKH,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  wzlog("gpu: init done err=0x%x depthBits=%d",(unsigned)glGetError(),gDepthBits); return true;
}
static void gpuDraw(const std::vector<float>&v,const float*vp){
  static const float ID[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; glUniformMatrix4fv(gpVP,1,GL_FALSE,vp); if(gpM>=0)glUniformMatrix4fv(gpM,1,GL_FALSE,ID);
  size_t nv=v.size()/5; for(size_t o=0;o<nv;o+=60000){ size_t c=std::min<size_t>(60000,nv-o); c-=c%3; if(!c)break; glVertexAttribPointer(gpaP,3,GL_FLOAT,GL_FALSE,20,&v[o*5]); glVertexAttribPointer(gpaT,2,GL_FLOAT,GL_FALSE,20,&v[o*5+3]); glDrawArrays(GL_TRIANGLES,0,(GLsizei)c); } }
static void gpuVP(const Renderer&r,float n,float f,float*o){ float P[16]={0},V[16]={0}; P[0]=2.f*r.focal/r.W; P[5]=2.f*r.focal/r.H; P[10]=(f+n)/(n-f); P[11]=-1; P[14]=2*f*n/(n-f);
  V[0]=r.rx;V[4]=0;V[8]=r.rz;V[12]=-(r.rx*r.cam.x+r.rz*r.cam.z); V[1]=r.ux;V[5]=r.uy;V[9]=r.uz;V[13]=-(r.ux*r.cam.x+r.uy*r.cam.y+r.uz*r.cam.z); V[2]=-r.fx;V[6]=-r.fy;V[10]=-r.fz;V[14]=r.fx*r.cam.x+r.fy*r.cam.y+r.fz*r.cam.z; V[15]=1; m4mul(P,V,o); }
static void gpuFrame(Game&g,int frame){
  Renderer&r=g.r; for(int i=0;i<8;i++)glDisableVertexAttribArray(i); glViewport(0,0,OUT_W,OUT_H); glDisable(GL_BLEND); glDisable(GL_CULL_FACE); glDepthMask(GL_TRUE); glClearColor(0.45f,0.55f,0.75f,1.f); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glUseProgram(gpPg); glActiveTexture(GL_TEXTURE0); if(gpSam>=0)glUniform1i(gpSam,0); glEnableVertexAttribArray(gpaP); glEnableVertexAttribArray(gpaT);
  size_t nw=r.gv[0].size()/15,nvm=r.gv[1].size()/15;
  if(r.m&&r.W>0){ // السماء: نسيج صغير بتدرّج السماء الحقيقي (skyAt) على مستطيل كامل الشاشة
    static uint8_t sk[SKW*SKH*4]; float th=tanf(r.cam.fov*0.5f),asp=(float)r.W/r.H; auto c8=[](float v){int i=(int)(v*255.f+0.5f);return (uint8_t)(i<0?0:i>255?255:i);};
    for(int j=0;j<SKH;j++)for(int i=0;i<SKW;i++){ float R,G,B,s; r.skyAt(((i+0.5f)/SKW*2.f-1.f)*th*asp,(1.f-(j+0.5f)/SKH*2.f)*th,R,G,B,s); uint8_t*q=&sk[(j*SKW+i)*4]; q[0]=c8(R);q[1]=c8(G);q[2]=c8(B);q[3]=255; }
    glBindTexture(GL_TEXTURE_2D,gpSky); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,SKW,SKH,0,GL_RGBA,GL_UNSIGNED_BYTE,sk);
    static const float ID[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; static const float Q[]={-1,-1,0.f,0,1, 1,-1,0.f,1,1, 1,1,0.f,1,0, -1,-1,0.f,0,1, 1,1,0.f,1,0, -1,1,0.f,0,0};
    glDisable(GL_DEPTH_TEST); std::vector<float> qv(Q,Q+30); gpuDraw(qv,ID); }
  if(!gpDetOk){ gpDetOk=true; static uint8_t dt[256*256*4]; const float fade[3]={1.f,0.6f,0.3f}; glGenTextures(3,gpDet);
    for(int L=0;L<3;L++){ for(int V=0;V<256;V++)for(int U=0;U<256;U++){ int K=(r.LL[((V&31)<<5)|(U&31)]*r.HH[(((U>>4)&15)<<4)|((V>>4)&15)])>>8; float k=1.f+((float)K/256.f-1.f)*fade[L]; int t=(int)(k*0.5f*255.f+0.5f); if(t>255)t=255; uint8_t*q=&dt[(V*256+U)*4]; q[0]=q[1]=q[2]=(uint8_t)t; q[3]=255; }
      glBindTexture(GL_TEXTURE_2D,gpDet[L]); glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,256,256,0,GL_RGBA,GL_UNSIGNED_BYTE,dt);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT); }
    wzlog("gpu: detail textures built err=0x%x",(unsigned)glGetError()); }
  glBindTexture(GL_TEXTURE_2D,gpPal);
  if(gDepthBits>0){ glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); } else glDisable(GL_DEPTH_TEST);
  float vp[16];
  if(nw){ gpuVP(r,0.15f,std::max(r.fogF*1.5f,160.f),vp); gpuDraw(r.gv[0],vp);
    if(gDepthBits>0){ glEnable(GL_BLEND); glBlendFunc(GL_DST_COLOR,GL_SRC_COLOR); glDepthMask(GL_FALSE); // نقش الشبكة: ضرب ×2 (النسيج = K/2)
      for(int L=0;L<3;L++)if(!r.gd[L].empty()){ glBindTexture(GL_TEXTURE_2D,gpDet[L]); gpuDraw(r.gd[L],vp); }
      glDepthMask(GL_TRUE); glDisable(GL_BLEND); glBindTexture(GL_TEXTURE_2D,gpPal); } }
  if(nvm){ if(gDepthBits>0)glClear(GL_DEPTH_BUFFER_BIT); gpuVP(r,0.04f,12.f,vp); gpuDraw(r.gv[1],vp); }
  if(frame<=3||frame%300==0)wzlog("gpuFrame %d world tris %d viewmodel tris %d err=0x%x",frame,(int)nw,(int)nvm,(unsigned)glGetError());
  for(int i=0;i<8;i++)glDisableVertexAttribArray(i); r.gv[0].clear(); r.gv[1].clear(); for(int L=0;L<3;L++)r.gd[L].clear();
}
#endif

// ===== اختبار ثلاثي الأبعاد على الـGPU: مكعب يدور بشيدر khronos/earth الجاهز (إضاءة + نسيج) — للتأكد من العرض والـ3D بدون يد/لعبة =====
static void m4mul(const float*a,const float*b,float*o){ float r[16]; for(int c=0;c<4;c++)for(int rI=0;rI<4;rI++){ float v=0; for(int k=0;k<4;k++)v+=a[k*4+rI]*b[c*4+k]; r[c*4+rI]=v; } memcpy(o,r,sizeof(r)); }
static void cubeLoop(bool usePad){
  unsigned char*vp,*fp; uint64_t vl,fl; if(!fetchSh("khronos/earth.vert",vp,vl)||!fetchSh("khronos/earth.frag",fp,fl)){wzlog("earth shader missing");for(;;)sceKernelUsleep(100000);}
  GLuint vs=glCreateShader(GL_VERTEX_SHADER),fs=glCreateShader(GL_FRAGMENT_SHADER),pg=glCreateProgram(); glShaderBinary(1,&vs,0,vp,vl); glShaderBinary(1,&fs,0,fp,fl); glAttachShader(pg,vs); glAttachShader(pg,fs); glLinkProgram(pg);
  GLint uM=glGetUniformLocation(pg,"u_modelViewProjMatrix"),uN=glGetUniformLocation(pg,"u_normalMatrix"),uL=glGetUniformLocation(pg,"lightDir"),uS=glGetUniformLocation(pg,"sampler2d"),aN=glGetAttribLocation(pg,"vNormal"),aP=glGetAttribLocation(pg,"vPosition"),aT=glGetAttribLocation(pg,"vTexCoord");
  wzlog("cube: link err=0x%x loc M%d N%d L%d S%d aN%d aP%d aT%d",(unsigned)glGetError(),uM,uN,uL,uS,aN,aP,aT);
  static GLfloat nr[36*3],ps[36*4],tc[36*4]; static const float F[6][9]={{1,0,0, 0,1,0, 0,0,1},{-1,0,0, 0,0,1, 0,1,0},{0,1,0, 0,0,1, 1,0,0},{0,-1,0, 1,0,0, 0,0,1},{0,0,1, 1,0,0, 0,1,0},{0,0,-1, 0,1,0, 1,0,0}};
  static const int Q[6]={0,1,2, 0,2,3}; static const float cu[4][2]={{-1,-1},{1,-1},{1,1},{-1,1}}; int vi=0;
  for(int f=0;f<6;f++){ const float*n=F[f],*u=F[f]+3,*v=F[f]+6; for(int q=0;q<6;q++){ int k=Q[q]; float a=cu[k][0],b=cu[k][1]; for(int i=0;i<3;i++){ nr[vi*3+i]=n[i]; ps[vi*4+i]=n[i]+u[i]*a+v[i]*b; } ps[vi*4+3]=1; tc[vi*4]=(a+1)*0.5f; tc[vi*4+1]=(b+1)*0.5f; tc[vi*4+2]=0; tc[vi*4+3]=1; vi++; } }
  GLuint tex; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex); static unsigned char px[64*64*4]; for(int y=0;y<64;y++)for(int x=0;x<64;x++){ bool c=((x>>3)+(y>>3))&1; unsigned char*q=&px[(y*64+x)*4]; q[0]=c?230:40; q[1]=c?160:90; q[2]=c?30:200; q[3]=255; }
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,px); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  wzlog("cube: ready, looping pad=%d",(int)usePad); float angA=0,angB=0; uint32_t lastBtn=0xFFFFFFFFu; int btnLogs=0; for(int fr=0;;fr++){
    if(usePad&&padH>=0){ memset(&pd,0,sizeof(pd)); int rr=scePadReadState(padH,&pd); if(fr<3)wzlog("padRead ret=0x%x",(unsigned)rr); angA+=stickN(pd.rightStick.x)*0.06f; angB+=stickN(pd.rightStick.y)*0.06f; if(pd.buttons!=lastBtn&&btnLogs<40){ lastBtn=pd.buttons; btnLogs++; wzlog("pad btn=%05X LS %d,%d RS %d,%d L2 %d R2 %d",(unsigned)pd.buttons,pd.leftStick.x,pd.leftStick.y,pd.rightStick.x,pd.rightStick.y,pd.analogButtons.l2,pd.analogButtons.r2); } }
    float a=fr*0.02f+angA,ca=cosf(a),sa=sinf(a),cb=cosf(a*0.7f+angB),sb=sinf(a*0.7f+angB);
    float RY[16]={ca,0,-sa,0, 0,1,0,0, sa,0,ca,0, 0,0,0,1}, RX[16]={1,0,0,0, 0,cb,sb,0, 0,-sb,cb,0, 0,0,0,1}, T[16]={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,-4.5f,1}; float R[16],MV[16],P[16]={0},MVP[16];
    m4mul(RY,RX,R); m4mul(T,R,MV); float f=1.f/tanf(0.5f),asp=(float)OUT_W/OUT_H,n=0.1f,fa=50.f; P[0]=f/asp; P[5]=f; P[10]=(fa+n)/(n-fa); P[11]=-1; P[14]=2*fa*n/(n-fa); m4mul(P,MV,MVP);
    glViewport(0,0,OUT_W,OUT_H); glClearColor(0.1f,0.12f,0.2f,1.f); glClear(GL_COLOR_BUFFER_BIT); glDisable(GL_BLEND); glEnable(GL_CULL_FACE); glUseProgram(pg);
    glUniformMatrix4fv(uM,1,GL_FALSE,MVP); glUniformMatrix4fv(uN,1,GL_FALSE,R); glUniform3f(uL,0.4f,0.7f,0.6f); glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,tex); glUniform1i(uS,0);
    glVertexAttribPointer(aN,3,GL_FLOAT,GL_FALSE,0,nr); glEnableVertexAttribArray(aN); glVertexAttribPointer(aP,4,GL_FLOAT,GL_FALSE,0,ps); glEnableVertexAttribArray(aP); glVertexAttribPointer(aT,4,GL_FLOAT,GL_FALSE,0,tc); glEnableVertexAttribArray(aT);
    glDrawArrays(GL_TRIANGLES,0,36); EGLBoolean sr=eglSwapBuffers(gD,gS); if(fr<=2||fr%300==0)wzlog("cube frame %d err=0x%x swap=%d",fr,(unsigned)glGetError(),(int)sr); sceKernelUsleep(16000); } }
int main(int,char**){
  wzlog("== main start build %d variant %d threads %d ==",(int)BUILD_ID,(int)WZ_VARIANT,(int)WZ_THREADS);
  setvbuf(stdout,NULL,_IONBF,0);
  wzlog("gl first"); if(!glInit()){wzlog("glInit FAILED");for(;;);} wzlog("gl up");
#if WZ_VARIANT==1
  wzlog("variant 1: 3D cube test only (no pad/game)"); cubeLoop(false);
#elif WZ_VARIANT==3
  { wzlog("variant 3: privileged + UserService init + pad, then cube"); OrbisUserServiceInitializeParams up3; up3.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST; { int m1=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE); wzlog("v3: load internal UserService ret=0x%x",(unsigned)m1); int m2=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD); wzlog("v3: load internal Pad ret=0x%x",(unsigned)m2); }
    wzlog("v3: calling sceUserServiceInitialize"); int u3=sceUserServiceInitialize(&up3); wzlog("v3: userservice init ret=0x%x",(unsigned)u3);
    int uid3=0; wzlog("v3: calling GetInitialUser"); int g3=sceUserServiceGetInitialUser(&uid3); wzlog("v3: GetInitialUser ret=0x%x uid=%d",(unsigned)g3,uid3);
    wzlog("v3: calling scePadInit"); int p3=scePadInit(); wzlog("v3: scePadInit ret=0x%x",(unsigned)p3); padH=scePadOpen(uid3,0,0,NULL); wzlog("v3: scePadOpen=%d",padH); cubeLoop(true); }
#endif
#if WZ_GPU
  int uid=0; { wzlog("variant 4: GPU-rendered scene (privileged)"); OrbisUserServiceInitializeParams up4; up4.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST;
    int m1=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE); int m2=sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD); wzlog("v4: internal modules ret=0x%x 0x%x",(unsigned)m1,(unsigned)m2);
    int u4=sceUserServiceInitialize(&up4); wzlog("v4: userservice init ret=0x%x",(unsigned)u4); int g4=sceUserServiceGetInitialUser(&uid); wzlog("v4: GetInitialUser ret=0x%x uid=%d",(unsigned)g4,uid);
    int p4=scePadInit(); wzlog("v4: scePadInit ret=0x%x",(unsigned)p4); padH=scePadOpen(uid,0,0,NULL); wzlog("v4: scePadOpen=%d",padH);
#if WZ_VARIANT==5
#endif
    if(!gpuInit()){ wzlog("gpuInit FAILED"); for(;;)sceKernelUsleep(100000); } }
#else
#ifdef WZ_SKIP_USINIT
  wzlog("skip sceUserServiceInitialize (Piglet/privileged may have done it)");
#else
  OrbisUserServiceInitializeParams up; up.priority=ORBIS_KERNEL_PRIO_FIFO_LOWEST; wzlog("calling userservice init"); int ur=sceUserServiceInitialize(&up); wzlog("userservice init ret=0x%x",(unsigned)ur);
#endif
#ifdef WZ_SKIP_USINIT
  int uid=0xFF; wzlog("privileged: skip GetInitialUser (it crashed: CE-34878-0), use SYSTEM user id 0xFF");
#else
  int uid=0; wzlog("calling get initial user"); int gr=sceUserServiceGetInitialUser(&uid); wzlog("get initial user ret=0x%x uid=%d",(unsigned)gr,uid);
#endif
  wzlog("calling scePadInit"); int pr=scePadInit(); wzlog("scePadInit ret=0x%x",(unsigned)pr);
  padH=scePadOpen(uid,0,0,NULL); wzlog("pad open %d",padH);
#endif
  static Game g; wzlog("game constructed"); g.r.startThreads(WZ_THREADS); wzlog("threads started"); g.menu=1; g.init(WZ_IW,WZ_IH);
#if WZ_GPU
  g.r.gpu=true;
#endif
 wzlog("game init"); static std::vector<uint32_t> upb(WZ_IW*WZ_IH); wzlog("upload buf ok");
  wzlog("entering loop");
  uint32_t last=ticks(); bool dbg=false; uint32_t prevB=0; float acc=0,fpsT=0,fps=30; int fpsN=0; float smx=0,smy=0; int frame=0;
  for(;;){
    memset(&pd,0,sizeof(pd));     if(padH>=0)scePadReadState(padH,&pd); else padH=scePadOpen(uid,0,0,NULL);

    uint32_t now=ticks(); float dt=(now-last)/1000.f; last=now; if(dt>0.1f)dt=0.1f; if(dt<=0)dt=0.001f; frame++;
    fpsT+=dt; fpsN++; if(fpsT>=1.f){fps=fpsN/fpsT;g.fpsShow=fps;fpsT=0;fpsN=0;}
#if WZ_VARIANT==5
    if(frame==120){ pthread_t at; int pr=pthread_create(&at,NULL,audioThread,NULL); wzlog("audio thread create %d",pr); }
#endif
    if(frame==1)wzlog("f1 pad read"); uint32_t edge=pd.buttons&~prevB; prevB=pd.buttons;
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
    in.use=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_LEFT); in.drone=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_DOWN); in.grenade=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_UP); in.tri=B(ORBIS_PAD_BUTTON_TRIANGLE); in.c4=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_RIGHT); in.det=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_TOUCH_PAD); in.seat=!g.menu&&!g.paused&&B(ORBIS_PAD_BUTTON_R3); in.start=B(ORBIS_PAD_BUTTON_OPTIONS); in.confirm=B(ORBIS_PAD_BUTTON_CROSS); in.back=B(ORBIS_PAD_BUTTON_CIRCLE);
    if(g.menu||g.paused){ if(B(ORBIS_PAD_BUTTON_RIGHT))in.mx=1; else if(B(ORBIS_PAD_BUTTON_LEFT))in.mx=-1; if(g.menu)in.my=B(ORBIS_PAD_BUTTON_UP)?1.f:B(ORBIS_PAD_BUTTON_DOWN)?-1.f:0.f;
      if((edge&ORBIS_PAD_BUTTON_R1)&&(g.menu==1||g.paused))g.swapSticks=!g.swapSticks; }
    if((edge&ORBIS_PAD_BUTTON_TOUCH_PAD)&&(g.menu||g.paused))dbg=!dbg;
#if WZ_GPU
    memset(g.r.fb.data(),0,g.r.fb.size()*4); // طبقة HUD شفافة
#endif
    acc+=dt; int steps=0; while(acc>=1.f/60.f&&steps<4){ g.update(1.f/60.f,in); acc-=1.f/60.f; steps++; } if(steps==4)acc=0;
    if(frame==1)wzlog("f1 updated"); if(g.menu)g.renderMenu(); else g.render(); if(frame==1)wzlog("f1 rendered");
    if(dbg){ char b[96]; snprintf(b,96,"FPS %d RES %d SHADERS %d",(int)fps,g.r.W,gShN); g.text(40,40,b,2,0xFFFFFF00);
      { int pg=(frame/120)%std::max(1,(gShN+11)/12),k=0; for(NikShader*e=scePrecompiledShaderEntries;e->ident;++e,++k){ if(k<pg*12||k>=pg*12+12)continue; snprintf(b,96,"%d %.40s",k,e->ident); g.text(40,110+(k-pg*12)*16,b,1,0xFF66FFCC); } }
      snprintf(b,96,"LS %d %d RS %d %d L2 %d R2 %d",pd.leftStick.x,pd.leftStick.y,pd.rightStick.x,pd.rightStick.y,pd.analogButtons.l2,pd.analogButtons.r2); g.text(40,60,b,2,0xFFFFFF00);
      snprintf(b,96,"BTN %05X PAD %d",(unsigned)pd.buttons,padH); g.text(40,80,b,2,0xFFFFFF00); }
    g.r.flush(); { const uint32_t*fbp=g.r.fb.data(); uint32_t*ub=upb.data(); int n=WZ_IW*WZ_IH; std::atomic<int> nx(0); g.r.pool.exec([&]{ for(;;){ int i0=nx.fetch_add(65536); if(i0>=n)break; int i1=std::min(n,i0+65536); for(int i=i0;i<i1;i++){ uint32_t p=fbp[i]; ub[i]=(p&0xFF00FF00u)|((p>>16)&0xFFu)|((p&0xFFu)<<16)|(WZ_GPU?0u:0xFF000000u); } } }); }
    if(frame==1)wzlog("f1 converted");
    static std::vector<uint32_t> tst; if(tst.empty()){ static const uint32_t cb[8]={0xFF0000FFu,0xFF00FF00u,0xFFFF0000u,0xFFFFFFFFu,0xFF00FFFFu,0xFFFF00FFu,0xFFFFFF00u,0xFF808080u}; tst.resize(WZ_IW*WZ_IH); for(int y=0;y<WZ_IH;y++)for(int x=0;x<WZ_IW;x++)tst[y*WZ_IW+x]=(y<WZ_IH/8)?(0xFF000000u|(uint32_t)(x*255/WZ_IW)*0x010101u):cb[x*8/WZ_IW]; }
#if WZ_GPU
    gpuFrame(g,frame); glPresent(upb.data(),true); if(frame<=3||frame%600==0)wzlog("frame %d presented fps=%d",frame,(int)fps); { uint32_t el=ticks()-now; if(el<16)sceKernelUsleep((16-el)*1000); } continue;
#endif
    bool testMode=(frame<=180); if(frame==1)wzlog("test pattern ON"); if(frame==181)wzlog("test pattern OFF -> game");
    glPresent(testMode?tst.data():upb.data()); if(frame<=3||frame==60||(frame>180&&frame<=184)||frame%600==0)wzlog("frame %d presented fps=%d",frame,(int)fps);
    { uint32_t el=ticks()-now; if(el<33)sceKernelUsleep((33-el)*1000); } // سقف 30fps
  }
}
