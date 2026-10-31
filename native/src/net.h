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

enum { PORT_GAME=9330, PORT_BEACON=9331, NET_VER=1 };
enum { T_HELLO=1, T_WELCOME=2, T_INPUT=3, T_SNAP=4, T_BYE=5, T_BEACON=6 };

#pragma pack(push,1)
struct NetHello{uint8_t t,ver;};
struct NetWelcome{uint8_t t,slot,map,total;};
struct NetInput{uint8_t t,slot;uint16_t seq;float x,y,z,yaw,pitch;uint8_t flags,wpn;};      // flags: 1 fire,2 crouch,4 reload,8 sprint,16 moving,32 ads
struct NetEnt{float x,y,z,yaw,pitch,flash,phase,reloadT;int16_t hp;uint8_t flags,wpn,ammo,mags,spawnSeq;uint16_t kills,deaths;}; // flags: 1 alive,2 crouch,4 moving,8 sprint,16 team,32 human,64 prot
struct NetEvt{uint8_t id,killer,victim,head;};
struct NetTr{float x0,y0,z0,x1,y1,z1;};
struct NetSnapHead{uint8_t t,map,matchId,n,over,winner;int16_t s0,s1;uint8_t nEvt,nTr;uint16_t seq;};
struct NetBeacon{uint8_t t,map,players,maxp;char name[16];};
#pragma pack(pop)

struct Pkt{uint8_t d[1400];int n=0;sockaddr_in from;};

struct Udp{
  int fd=-1;
  bool open(int port,bool bcast){
    close_(); fd=socket(AF_INET,SOCK_DGRAM,0); if(fd<0)return false;
    int one=1; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one); if(bcast)setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&one,sizeof one);
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK);
    sockaddr_in a; memset(&a,0,sizeof a); a.sin_family=AF_INET; a.sin_addr.s_addr=htonl(INADDR_ANY); a.sin_port=htons(port);
    if(port&&bind(fd,(sockaddr*)&a,sizeof a)<0){close(fd);fd=-1;return false;}
    return true;
  }
  // فتح على خطوات (خطوة لكل إطار) لمعرفة أين يتجمد الجهاز إن حدث
  // يعيد: 0 جارٍ ، 1 تم ، -1 فشل ؛ st يحمل رقم الخطوة ، what اسم الخطوة
  int stepOpen(int port,bool bcast,int&st,const char*&what){
    int one=1;
    switch(st){
      case 0: what="SOCKET"; fd=socket(AF_INET,SOCK_DGRAM,0); if(fd<0)return -1; st++; return 0;
      case 1: what="REUSE"; setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one); st++; return 0;
      case 2: what="BROADCAST"; if(bcast)setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&one,sizeof one); st++; return 0;
      case 3: what="NONBLOCK"; fcntl(fd,F_SETFL,fcntl(fd,F_GETFL,0)|O_NONBLOCK); st++; return 0;
      case 4: { what="BIND"; if(port){ sockaddr_in a; memset(&a,0,sizeof a); a.sin_family=AF_INET; a.sin_addr.s_addr=htonl(INADDR_ANY); a.sin_port=htons(port); if(bind(fd,(sockaddr*)&a,sizeof a)<0){close(fd);fd=-1;return -1;} } st++; return 1; }
    }
    return 1;
  }
  bool recv(Pkt&p){ if(fd<0)return false; socklen_t l=sizeof p.from; int n=recvfrom(fd,p.d,sizeof p.d,MSG_DONTWAIT,(sockaddr*)&p.from,&l); if(n<=0)return false; p.n=n; return true; }
  void send(const sockaddr_in&to,const void*d,int n){ if(fd>=0)sendto(fd,d,n,0,(const sockaddr*)&to,sizeof to); }
  void close_(){ if(fd>=0)close(fd); fd=-1; }
};
static sockaddr_in mkAddr(uint32_t ipNet,int port){ sockaddr_in a; memset(&a,0,sizeof a); a.sin_family=AF_INET; a.sin_addr.s_addr=ipNet; a.sin_port=htons(port); return a; }
// عنوان الجهاز المحلي (بدون إرسال حزم فعلية)
static uint32_t localIp(){
  int s=socket(AF_INET,SOCK_DGRAM,0); if(s<0)return 0; sockaddr_in a=mkAddr(inet_addr("192.168.1.1"),9);
  uint32_t r=0; if(connect(s,(sockaddr*)&a,sizeof a)==0){ sockaddr_in me; socklen_t l=sizeof me; memset(&me,0,sizeof me); if(getsockname(s,(sockaddr*)&me,&l)==0)r=me.sin_addr.s_addr; }
  close(s); return r;
}
static void ipStr(uint32_t ipNet,char*o){ uint8_t*b=(uint8_t*)&ipNet; snprintf(o,20,"%d.%d.%d.%d",b[0],b[1],b[2],b[3]); }
