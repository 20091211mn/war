// شبكة LAN بـ UDP (مقابس POSIX غير حاجبة) — تعمل على PS4 والحاسوب بنفس الشيفرة
#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <memory>
#include <chrono>

enum { PORT_GAME=9330, PORT_BEACON=9331, NET_VER=3 };
enum { T_HELLO=1, T_WELCOME=2, T_INPUT=3, T_SNAP=4, T_BYE=5, T_BEACON=6, T_PROBE=7 };

#pragma pack(push,1)
struct NetHello{uint8_t t,ver;};
struct NetWelcome{uint8_t t,slot,map,total;};
struct NetInput{uint8_t t,slot;uint16_t seq;float x,y,z,yaw,pitch;uint8_t flags,wpn;float vx,vz,vh,vsp;uint8_t veh;uint8_t c4p,c4d;};      // veh: 0 لا شيء ، k+1 = يقود الشاص k ؛ flags: 1 fire,2 crouch,4 reload,8 sprint,16 moving,32 ads
struct NetEnt{float x,y,z,yaw,pitch,flash,phase,reloadT;int16_t hp;uint8_t flags,wpn,ammo,mags,spawnSeq;uint16_t kills,deaths;uint8_t c4;}; // flags: 1 alive,2 crouch,4 moving,8 sprint,16 team,32 human,64 prot
struct NetVeh{float x,z,h,speed,steer;int16_t hp;int8_t driver;uint8_t flags;}; // flags: 1 dead,2 burn
struct NetC4{float x,y,z;uint8_t team,owner;};
struct NetEvt{uint8_t id,killer,victim,head;};
struct NetTr{float x0,y0,z0,x1,y1,z1;};
struct NetSnapHead{uint8_t t,map,matchId,n,over,winner;int16_t s0,s1;uint8_t nEvt,nTr;uint16_t seq;uint8_t nVeh;uint8_t nC4;};
struct NetBeacon{uint8_t t,map,players,maxp;char name[16];uint8_t lim5,mins,bots,style;}; // أول 20 بايت قديمة؛ الأربعة الأخيرة (حد النقاط/5، الدقائق، البوتات، النمط) اختيارية
#pragma pack(pop)

struct Pkt{uint8_t d[1400];int n=0;sockaddr_in from;};

// كل العمليات التي قد تتجمد على PS4 (socket/setsockopt/bind/recvfrom) تجري في خيط مستقل؛ الخيط الرئيسي لا يتجمد أبداً.
// المقابس تُحفظ في مسبح دائم (لا نغلقها) لتفادي تعارض الأرقام والخيوط العالقة.
struct Chan{
  int fd=-1; std::atomic<int> state{0}; std::atomic<const char*> what{"START"}; std::mutex mu; std::deque<Pkt> q;
};
struct Udp{
  std::shared_ptr<Chan> ch;
  static std::shared_ptr<Chan>& pool(int id){ static std::shared_ptr<Chan> P[8]; return P[id&7]; }
  void open(int id,int port,bool bcast){
    auto&slot=pool(id);
    if(slot&&slot->state.load()>=0){ std::lock_guard<std::mutex> g(slot->mu); slot->q.clear(); ch=slot; return; } // إعادة استخدام (قيد التهيئة أو جاهز)
    auto c=std::make_shared<Chan>(); slot=c; ch=c;
    std::thread([c,port,bcast]{
      int one=1; c->what="SOCKET"; int fd=socket(AF_INET,SOCK_DGRAM,0); if(fd<0){c->state=-1;return;} c->fd=fd;
      c->what="REUSE"; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one);
      c->what="BROADCAST"; if(bcast)setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&one,sizeof one);
      c->what="BIND"; sockaddr_in a; memset(&a,0,sizeof a); a.sin_family=AF_INET; a.sin_addr.s_addr=htonl(INADDR_ANY); a.sin_port=htons(port);
      if(bind(fd,(sockaddr*)&a,sizeof a)<0){c->state=-1;return;}
      c->what="READY"; c->state=1; Pkt p;
      for(;;){ socklen_t l=sizeof p.from; int n=recvfrom(fd,p.d,sizeof p.d,0,(sockaddr*)&p.from,&l);
        if(n<=0){ std::this_thread::sleep_for(std::chrono::milliseconds(3)); continue; }
        p.n=n; std::lock_guard<std::mutex> g(c->mu); if(c->q.size()<256)c->q.push_back(p); }
    }).detach();
  }
  int state()const{ return ch?ch->state.load():0; }
  const char* what()const{ return ch?ch->what.load():"-"; }
  bool ready()const{ return ch&&ch->state.load()==1; }
  bool recv(Pkt&p){ if(!ch)return false; std::lock_guard<std::mutex> g(ch->mu); if(ch->q.empty())return false; p=ch->q.front(); ch->q.pop_front(); return true; }
  void send(const sockaddr_in&to,const void*d,int n){ if(ready())sendto(ch->fd,d,n,0,(const sockaddr*)&to,sizeof to); }
  void close_(){ ch.reset(); }
  bool opened()const{ return (bool)ch; }
};
static sockaddr_in mkAddr(uint32_t ipNet,int port){ sockaddr_in a; memset(&a,0,sizeof a); a.sin_family=AF_INET; a.sin_addr.s_addr=ipNet; a.sin_port=htons(port); return a; }
// عنوان الجهاز المحلي (بدون إرسال حزم فعلية)
static uint32_t localIp(){
  int s=socket(AF_INET,SOCK_DGRAM,0); if(s<0)return 0; sockaddr_in a=mkAddr(inet_addr("192.168.1.1"),9);
  uint32_t r=0; if(connect(s,(sockaddr*)&a,sizeof a)==0){ sockaddr_in me; socklen_t l=sizeof me; memset(&me,0,sizeof me); if(getsockname(s,(sockaddr*)&me,&l)==0)r=me.sin_addr.s_addr; }
  close(s); return r;
}
static void ipStr(uint32_t ipNet,char*o){ uint8_t*b=(uint8_t*)&ipNet; snprintf(o,20,"%d.%d.%d.%d",b[0],b[1],b[2],b[3]); }
