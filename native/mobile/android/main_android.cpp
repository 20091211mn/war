// أندرويد أصلي بالكامل (NativeActivity): لا Java ولا HTML. العرض: ANativeWindow (نسخة البكسلات) ، الصوت: AAudio ، اللمس: AInputEvent.
#include <android_native_app_glue.h>
#include <android/native_window.h>
#include <android/log.h>
#include <aaudio/AAudio.h>
#include <jni.h>
#include <time.h>
#include <unistd.h>
#include "../mobile.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"warzone",__VA_ARGS__)
static Mobile* M=nullptr; static bool g_run=false; static ANativeWindow* g_win=nullptr; static int g_bufW=0,g_bufH=0,g_seq=-1;
static AAudioStream* g_as=nullptr;

static aaudio_data_callback_result_t audioCb(AAudioStream*,void*,void*data,int32_t frames){ SFX::mix((int16_t*)data,frames); return AAUDIO_CALLBACK_RESULT_CONTINUE; }
static void startAudio(){
  std::thread([]{ SFX::initBuffers(); AAudioStreamBuilder*b=nullptr; if(AAudio_createStreamBuilder(&b)!=AAUDIO_OK)return;
    AAudioStreamBuilder_setDirection(b,AAUDIO_DIRECTION_OUTPUT); AAudioStreamBuilder_setFormat(b,AAUDIO_FORMAT_PCM_I16); AAudioStreamBuilder_setChannelCount(b,2); AAudioStreamBuilder_setSampleRate(b,48000);
    AAudioStreamBuilder_setPerformanceMode(b,AAUDIO_PERFORMANCE_MODE_LOW_LATENCY); AAudioStreamBuilder_setSharingMode(b,AAUDIO_SHARING_MODE_SHARED); AAudioStreamBuilder_setDataCallback(b,audioCb,nullptr);
    AAudioStream*s=nullptr; aaudio_result_t r=AAudioStreamBuilder_openStream(b,&s); AAudioStreamBuilder_delete(b); LOGI("audio open %d",(int)r); if(r!=AAUDIO_OK)return;
    g_as=s; AAudioStream_requestStart(s); }).detach();
}

// وضع ملء الشاشة الغامر (إخفاء شريط التنقل) عبر JNI
static void immersive(struct android_app*app){
  JavaVM*vm=app->activity->vm; JNIEnv*env=nullptr; if(vm->AttachCurrentThread(&env,nullptr)!=JNI_OK||!env)return;
  jobject act=app->activity->clazz; jclass ac=env->GetObjectClass(act); jmethodID gw=env->GetMethodID(ac,"getWindow","()Landroid/view/Window;"); if(!gw){ env->ExceptionClear(); return; }
  jobject win=env->CallObjectMethod(act,gw); jclass wc=env->GetObjectClass(win); jmethodID gd=env->GetMethodID(wc,"getDecorView","()Landroid/view/View;"); jobject dv=env->CallObjectMethod(win,gd);
  jclass vc=env->GetObjectClass(dv); jmethodID sf=env->GetMethodID(vc,"setSystemUiVisibility","(I)V");
  if(sf){ const int flags=0x1000|0x4|0x2|0x400|0x200|0x100; /* IMMERSIVE_STICKY|FULLSCREEN|HIDE_NAV|LAYOUT_FULLSCREEN|LAYOUT_HIDE_NAV|LAYOUT_STABLE */ env->CallVoidMethod(dv,sf,flags); }
  if(env->ExceptionCheck())env->ExceptionClear();
  // شاشات بها حاجب (notch): استعمال كامل الشاشة
  if(app->activity->sdkVersion>=28){ jmethodID ga=env->GetMethodID(wc,"getAttributes","()Landroid/view/WindowManager$LayoutParams;"); if(ga){ jobject lp=env->CallObjectMethod(win,ga); jclass lc=env->GetObjectClass(lp); jfieldID f=env->GetFieldID(lc,"layoutInDisplayCutoutMode","I"); if(f){ env->SetIntField(lp,f,1); jmethodID sa=env->GetMethodID(wc,"setAttributes","(Landroid/view/WindowManager$LayoutParams;)V"); if(sa)env->CallVoidMethod(win,sa,lp); } } if(env->ExceptionCheck())env->ExceptionClear(); }
  vm->DetachCurrentThread();
}
// يبقي الشاشة مضاءة
static void keepOn(struct android_app*app){ ANativeActivity_setWindowFlags(app->activity,0x80/*FLAG_KEEP_SCREEN_ON*/|0x400/*FULLSCREEN*/,0); }

static void setGeometry(){ if(!g_win||!M)return; ANativeWindow_setBuffersGeometry(g_win,M->W,M->H,WINDOW_FORMAT_RGBX_8888); g_bufW=M->W; g_bufH=M->H; g_seq=M->resizeSeq; }

static int32_t onInput(struct android_app*,AInputEvent*ev){
  if(!M||AInputEvent_getType(ev)!=AINPUT_EVENT_TYPE_MOTION)return 0;
  int32_t a=AMotionEvent_getAction(ev); int act=a&AMOTION_EVENT_ACTION_MASK; int idx=(a&AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>>AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
  size_t n=AMotionEvent_getPointerCount(ev);
  switch(act){
    case AMOTION_EVENT_ACTION_DOWN: case AMOTION_EVENT_ACTION_POINTER_DOWN: M->touchDown(AMotionEvent_getPointerId(ev,idx),AMotionEvent_getX(ev,idx),AMotionEvent_getY(ev,idx)); break;
    case AMOTION_EVENT_ACTION_MOVE: for(size_t i=0;i<n;i++)M->touchMove(AMotionEvent_getPointerId(ev,i),AMotionEvent_getX(ev,i),AMotionEvent_getY(ev,i)); break;
    case AMOTION_EVENT_ACTION_UP: case AMOTION_EVENT_ACTION_POINTER_UP: M->touchUp(AMotionEvent_getPointerId(ev,idx)); break;
    case AMOTION_EVENT_ACTION_CANCEL: M->releaseAll(); break;
  }
  return 1;
}
static void onCmd(struct android_app*app,int32_t cmd){
  switch(cmd){
    case APP_CMD_INIT_WINDOW: g_win=app->window; if(g_win){ int w=ANativeWindow_getWidth(g_win),h=ANativeWindow_getHeight(g_win);
        if(!M){ M=new Mobile(); int cores=(int)sysconf(_SC_NPROCESSORS_ONLN); M->start(w,h,cores); startAudio(); } else M->setScreen(w,h);
        setGeometry(); immersive(app); g_run=true; LOGI("window %dx%d internal %dx%d",w,h,M->W,M->H); } break;
    case APP_CMD_TERM_WINDOW: g_run=false; g_win=nullptr; if(M)M->releaseAll(); break;
    case APP_CMD_GAINED_FOCUS: immersive(app); break;
    case APP_CMD_WINDOW_RESIZED: case APP_CMD_CONFIG_CHANGED: if(g_win&&M){ M->setScreen(ANativeWindow_getWidth(g_win),ANativeWindow_getHeight(g_win)); setGeometry(); } break;
    case APP_CMD_PAUSE: if(M){ M->releaseAll(); if(!M->g.menu&&!M->g.over&&M->g.role==0)M->g.paused=true; } if(g_as)AAudioStream_requestPause(g_as); break;
    case APP_CMD_RESUME: if(g_as)AAudioStream_requestStart(g_as); break;
  }
}
static double nowSec(){ timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }

void android_main(struct android_app*app){
  app->onAppCmd=onCmd; app->onInputEvent=onInput; keepOn(app); double last=nowSec();
  for(;;){
    int ev; struct android_poll_source*src; while(ALooper_pollOnce(g_run?0:200,nullptr,&ev,(void**)&src)>=0){ if(src)src->process(app,src); if(app->destroyRequested){ return; } }
    if(!g_run||!g_win||!M)continue;
    double now=nowSec(); float dt=(float)(now-last); last=now;
    M->frame(dt);
    if(g_seq!=M->resizeSeq)setGeometry();
    ANativeWindow_Buffer b; if(ANativeWindow_lock(g_win,&b,nullptr)==0){
      int rows=std::min(b.height,M->H),cols=std::min(b.width,M->W); const uint32_t*src2=M->out.data();
      for(int y=0;y<rows;y++)memcpy((uint32_t*)b.bits+(size_t)y*b.stride,src2+(size_t)y*M->W,(size_t)cols*4);
      ANativeWindow_unlockAndPost(g_win); }
  }
}
