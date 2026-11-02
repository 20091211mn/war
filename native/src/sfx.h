// محرك صوت برمجي بسيط: مؤثرات مولَّدة (بلا ملفات) تُمزج إلى S16 ستيريو 48kHz
#pragma once
#include <cstdint>
#include <cmath>
#include <atomic>
namespace SFX{
  enum{SHOT=0,SHOTGUN,EXPLODE,HIT,HURT,KILL,EMPTY,RELOAD,STEP,SNIPER,PISTOL,N};
  static std::atomic<float> engSpeed{-1.f},droneVol{0.f}; static float engPh=0,drPh=0,engN=0;
  struct Voice{ int id; int pos; int len; float vol,pan; };
  static const int MAXV=24;
  static Voice vs[MAXV]; static std::atomic_flag lk=ATOMIC_FLAG_INIT; static unsigned rs=12345;
  static inline float rnd(){ rs=rs*1664525u+1013904223u; return ((rs>>9)&0xFFFF)/32768.f-1.f; }
  static inline void lock(){ while(lk.test_and_set(std::memory_order_acquire)){} }
  static inline void unlock(){ lk.clear(std::memory_order_release); }
  static inline int lenOf(int id){ switch(id){case SHOT:return 9600;case SHOTGUN:return 16000;case EXPLODE:return 48000;case HIT:return 4800;case RELOAD:return 17000;case STEP:return 3000;case SNIPER:return 24000;case PISTOL:return 7200;case HURT:return 8000;case KILL:return 14400;default:return 2400;} }
  // pan: -1..1، vol: 0..1
  static inline void play(int id,float vol=1.f,float pan=0.f){
    if(vol<0.02f)return; lock();
    int k=-1; for(int i=0;i<MAXV;i++)if(vs[i].pos>=vs[i].len){k=i;break;}
    if(k<0){ int b=0; for(int i=1;i<MAXV;i++)if(vs[i].pos>vs[b].pos)b=i; k=b; }
    vs[k]={id,0,lenOf(id),vol>1?1:vol,pan}; unlock();
  }
  static inline float sample(int id,int n,float&st){
    float t=n/48000.f,x=0;
    switch(id){
      case SHOT:{ float e=expf(-t*38.f); st=0.55f*st+0.45f*rnd(); x=(st*0.9f+sinf(t*2*3.14159f*(150-t*400))*0.7f)*e; }break;
      case SHOTGUN:{ float e=expf(-t*20.f); st=0.7f*st+0.3f*rnd(); x=(st*1.1f+sinf(t*2*3.14159f*(90-t*150))*0.8f)*e; }break;
      case EXPLODE:{ float e=expf(-t*4.5f); st=0.97f*st+0.03f*rnd(); x=(st*14.f+sinf(t*2*3.14159f*(55-t*25))*0.9f)*e; if(x>1.2f)x=1.2f; }break;
      case HIT:{ x=sinf(t*2*3.14159f*1500.f)*expf(-t*50.f)*0.7f; }break;
      case HURT:{ float e=expf(-t*18.f); st=0.85f*st+0.15f*rnd(); x=(st*2.f+sinf(t*2*3.14159f*80.f))*e*0.7f; }break;
      case KILL:{ float f=t<0.1f?880.f:1320.f; x=sinf(t*2*3.14159f*f)*expf(-t*9.f)*0.6f; }break;
      case RELOAD:{ float a=expf(-t*60.f)*sinf(t*2*3.14159f*1800.f), b=t>0.18f?expf(-(t-0.18f)*50.f)*sinf((t-0.18f)*2*3.14159f*1100.f):0.f, c=t>0.27f?expf(-(t-0.27f)*70.f)*sinf((t-0.27f)*2*3.14159f*2400.f)*0.8f:0.f; st=0.6f*st+0.4f*rnd(); x=(a+b)*0.6f+c*0.5f+st*0.05f*(t<0.01f); }break;
      case STEP:{ float e=expf(-t*45.f); st=0.8f*st+0.2f*rnd(); x=st*e*0.8f+sinf(t*2*3.14159f*70.f)*e*0.3f; }break;
      case SNIPER:{ float e=expf(-t*9.f); st=0.4f*st+0.6f*rnd(); x=(st*1.2f+sinf(t*2*3.14159f*(70-t*60))*1.0f)*e+(t>0.25f?expf(-(t-0.25f)*40.f)*sinf((t-0.25f)*2*3.14159f*1400.f)*0.3f:0.f); }break;
      case PISTOL:{ float e=expf(-t*55.f); st=0.4f*st+0.6f*rnd(); x=(st*0.9f+sinf(t*2*3.14159f*(260-t*500))*0.5f)*e; }break;
      default:{ x=sinf(t*2*3.14159f*600.f)*expf(-t*80.f)*0.4f; }
    } return x; }
  static float stt[MAXV];
  // يملأ frames إطار ستيريو
  static inline void mix(int16_t*out,int frames){
    float L[512],R[512]; if(frames>512)frames=512;
    for(int i=0;i<frames;i++)L[i]=R[i]=0;
    lock();
    for(int v=0;v<MAXV;v++){ Voice&a=vs[v]; if(a.pos>=a.len)continue; float st=stt[v]; if(a.pos==0)st=0;
      float gl=a.vol*(a.pan>0?1-a.pan*0.6f:1),gr=a.vol*(a.pan<0?1+a.pan*0.6f:1);
      for(int i=0;i<frames&&a.pos<a.len;i++,a.pos++){ float s=sample(a.id,a.pos,st); L[i]+=s*gl; R[i]+=s*gr; }
      stt[v]=st; }
    unlock();
    float es=engSpeed.load(),dv=droneVol.load();
    if(es>=0.f){ float f=38.f+es*3.2f; for(int i=0;i<frames;i++){ engPh+=f/48000.f; if(engPh>1)engPh-=1; engN=0.9f*engN+0.1f*rnd(); float sq=(engPh<0.5f?1.f:-1.f)*0.5f+sinf(engPh*6.28318f*2)*0.5f; float v=(sq*0.35f+engN*0.25f)*(0.15f+std::min(1.f,es/26.f)*0.25f); L[i]+=v; R[i]+=v; } }
    if(dv>0.f){ for(int i=0;i<frames;i++){ drPh+=(190.f+sinf(drPh*0.05f)*6.f)/48000.f; if(drPh>1)drPh-=1; float v=((drPh<0.5f?1.f:-1.f)*0.5f+rnd()*0.3f)*0.07f*dv; L[i]+=v; R[i]+=v; } }
    for(int i=0;i<frames;i++){ float l=L[i]*0.5f,r=R[i]*0.5f; l=l>1?1:l<-1?-1:l; r=r>1?1:r<-1?-1:r; out[2*i]=(int16_t)(l*30000); out[2*i+1]=(int16_t)(r*30000); }
  }
}
