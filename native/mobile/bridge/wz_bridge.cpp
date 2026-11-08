#include "wz_bridge.h"
#include "../mobile.h"
#include <mutex>
#include <atomic>
#include <cstring>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
static volatile const char* g_phase="idle"; static char g_crashPath[512]={0};
static void crashHandler(int sig){ if(g_crashPath[0]){ int fd=open(g_crashPath,O_WRONLY|O_CREAT|O_TRUNC,0600); if(fd>=0){ char b[200]; int n=0; const char*a="NATIVE CRASH signal "; while(*a)b[n++]=*a++; b[n++]=(char)('0'+sig/10); b[n++]=(char)('0'+sig%10); const char*c=" in: "; while(*c)b[n++]=*c++; const char*ph=(const char*)g_phase; while(*ph&&n<190)b[n++]=*ph++; b[n++]='\n'; ssize_t r=write(fd,b,n); (void)r; close(fd); } } signal(sig,SIG_DFL); raise(sig); }
static Mobile* M=nullptr; static std::mutex mu; static std::atomic<bool> audioReady{false};
extern "C" {
void wz_set_crash_file(const char*path){ strncpy(g_crashPath,path,511); int sg[]={SIGSEGV,SIGABRT,SIGBUS,SIGFPE,SIGILL}; for(int x:sg){ struct sigaction a; memset(&a,0,sizeof a); a.sa_handler=crashHandler; sigaction(x,&a,nullptr); } }
void wz_init(int sw,int sh,int cores){ g_phase="init"; std::lock_guard<std::mutex> l(mu); if(M)return; M=new Mobile(); M->start(sw,sh,cores); std::thread([]{ SFX::initBuffers(); audioReady=true; }).detach(); }
void wz_set_screen(int sw,int sh){ g_phase="set_screen"; std::lock_guard<std::mutex> l(mu); if(M)M->setScreen(sw,sh); }
void wz_touch(int a,int id,float x,float y){ g_phase="touch"; std::lock_guard<std::mutex> l(mu); if(!M)return; if(a==0)M->touchDown(id,x,y); else if(a==1)M->touchMove(id,x,y); else if(a==2)M->touchUp(id); else M->releaseAll(); }
void wz_frame(float dt){ g_phase="frame"; std::lock_guard<std::mutex> l(mu); if(M)M->frame(dt); }
int wz_width(void){ return M?M->W:0; } int wz_height(void){ return M?M->H:0; } int wz_resize_seq(void){ return M?M->resizeSeq:0; }
const uint32_t* wz_pixels(void){ return M?M->out.data():nullptr; }
int wz_copy_pixels(uint32_t*d,int cap,int*w,int*h){ g_phase="copy_pixels"; std::lock_guard<std::mutex> l(mu); if(!M)return 0; int n=M->W*M->H; if(n<=0||n>cap||M->out.size()<(size_t)n)return 0; memcpy(d,M->out.data(),(size_t)n*4); *w=M->W; *h=M->H; return 1; }
void wz_mix(int16_t*b,int n){ g_phase="audio_mix"; if(!audioReady||!M){ for(int i=0;i<n*2;i++)b[i]=0; return; } SFX::mix(b,n); }
void wz_pause(void){ std::lock_guard<std::mutex> l(mu); if(M){ M->releaseAll(); if(!M->g.menu&&!M->g.over&&M->g.role==0)M->g.paused=true; } }
}
