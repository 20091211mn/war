// عميل WebSocket (TCP) + ماسح الشبكة — كل شيء داخل خيوط حتى لا يتجمد الخيط الرئيسي
#pragma once
#include "net.h"
#include <netinet/tcp.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <cstdlib>
#include "json.h"

static std::string b64(const uint8_t*d,int n){ static const char*T="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; std::string o; for(int i=0;i<n;i+=3){ uint32_t v=d[i]<<16|(i+1<n?d[i+1]<<8:0)|(i+2<n?d[i+2]:0); o+=T[v>>18];o+=T[(v>>12)&63];o+=i+1<n?T[(v>>6)&63]:'=';o+=i+2<n?T[v&63]:'='; } return o; }
static void setTimeouts(int fd,int ms){ timeval tv; tv.tv_sec=ms/1000; tv.tv_usec=(ms%1000)*1000; setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv); setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv); }
static bool sendAll(int fd,const void*d,size_t n){ const char*p=(const char*)d; while(n){ ssize_t k=::send(fd,p,n,0); if(k<=0)return false; p+=k;n-=k; } return true; }

struct WsState{
  std::atomic<int> st{0}; std::atomic<const char*> what{"START"}; std::mutex mu,smu; std::deque<std::string> rx; int fd=-1; std::atomic<bool> stop{false};
};
struct WsClient{
  std::shared_ptr<WsState> s;
  void connectTo(uint32_t ipNet,int port){
    close(); auto c=std::make_shared<WsState>(); s=c;
    std::thread([c,ipNet,port]{
      c->what="SOCKET"; int fd=socket(AF_INET,SOCK_STREAM,0); if(fd<0){c->st=-1;return;} c->fd=fd;
      int one=1; c->what="OPTIONS"; setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,&one,sizeof one); setTimeouts(fd,4000);
      c->what="CONNECT"; sockaddr_in a=mkAddr(ipNet,port); if(::connect(fd,(sockaddr*)&a,sizeof a)<0){c->st=-1;::close(fd);return;}
      c->what="HANDSHAKE"; uint8_t key[16]; for(int i=0;i<16;i++)key[i]=rand()&255; char ipb[24]; ipStr(ipNet,ipb);
      char req[400]; int n=snprintf(req,400,"GET / HTTP/1.1\r\nHost: %s:%d\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n\r\n",ipb,port,b64(key,16).c_str());
      if(!sendAll(fd,req,n)){c->st=-1;::close(fd);return;}
      std::string hdr; char ch; while(hdr.size()<4096){ ssize_t k=::recv(fd,&ch,1,0); if(k<=0){c->st=-1;::close(fd);return;} hdr+=ch; if(hdr.size()>=4&&hdr.compare(hdr.size()-4,4,"\r\n\r\n")==0)break; }
      if(hdr.find(" 101")==std::string::npos){c->st=-1;::close(fd);return;}
      setTimeouts(fd,0); c->what="OPEN"; c->st=1;
      std::string buf,msg; uint8_t tmp[4096];
      for(;;){ ssize_t k=::recv(fd,tmp,sizeof tmp,0); if(k<=0){ if(c->stop)break; if(k<0&&(errno==EAGAIN||errno==EINTR)){std::this_thread::sleep_for(std::chrono::milliseconds(2));continue;} break; } buf.append((char*)tmp,k);
        for(;;){ if(buf.size()<2)break; uint8_t b0=buf[0],b1=buf[1]; bool fin=b0&0x80; int op=b0&15; bool mk=b1&0x80; uint64_t len=b1&0x7f; size_t off=2;
          if(len==126){ if(buf.size()<4)break; len=((uint8_t)buf[2]<<8)|(uint8_t)buf[3]; off=4; } else if(len==127){ if(buf.size()<10)break; len=0; for(int i=0;i<8;i++)len=(len<<8)|(uint8_t)buf[2+i]; off=10; }
          uint8_t m4[4]={0,0,0,0}; if(mk){ if(buf.size()<off+4)break; memcpy(m4,&buf[off],4); off+=4; }
          if(len>(1u<<20)){ c->st=-1; ::close(fd); return; }
          if(buf.size()<off+len)break;
          std::string pl=buf.substr(off,len); if(mk)for(size_t i=0;i<pl.size();i++)pl[i]^=m4[i&3]; buf.erase(0,off+len);
          if(op==8){ c->st=-1; ::close(fd); return; }
          else if(op==9){ std::string f; f+=(char)0x8A; f+=(char)(0x80|pl.size()); uint8_t mk4[4]={1,2,3,4}; f.append((char*)mk4,4); for(size_t i=0;i<pl.size();i++)f+=(char)(pl[i]^mk4[i&3]); std::lock_guard<std::mutex> g(c->smu); sendAll(fd,f.data(),f.size()); }
          else if(op==1||op==2||op==0){ msg+=pl; if(fin){ std::lock_guard<std::mutex> g(c->mu); if(c->rx.size()<512)c->rx.push_back(msg); msg.clear(); } }
        } }
      c->st=-1; ::close(fd);
    }).detach();
  }
  int state()const{return s?s->st.load():0;} const char* what()const{return s?s->what.load():"-";}
  bool poll(std::string&o){ if(!s)return false; std::lock_guard<std::mutex> g(s->mu); if(s->rx.empty())return false; o=std::move(s->rx.front()); s->rx.pop_front(); return true; }
  bool send(const std::string&t){ if(!s||s->st.load()!=1)return false; std::string f; f+=(char)0x81; size_t n=t.size(); if(n<126)f+=(char)(0x80|n); else if(n<65536){f+=(char)(0x80|126);f+=(char)(n>>8);f+=(char)(n&255);} else return false;
    uint8_t mk[4]={(uint8_t)(rand()&255),(uint8_t)(rand()&255),(uint8_t)(rand()&255),(uint8_t)(rand()&255)}; f.append((char*)mk,4); for(size_t i=0;i<n;i++)f+=(char)(t[i]^mk[i&3]);
    std::lock_guard<std::mutex> g(s->smu); return sendAll(s->fd,f.data(),f.size()); }
  void close(){ if(s){ s->stop=true; if(s->st.load()==1){ /* إغلاق مهذب */ uint8_t f[6]={0x88,0x80,1,2,3,4}; std::lock_guard<std::mutex> g(s->smu); sendAll(s->fd,f,6); } s.reset(); } }
};

// ---------- ماسح الشبكة: يبحث عن مضيفي أندرويد (منفذ 3000 + /api/info) ----------
struct ScanHit{uint32_t ip;int port;int players;std::string phase;};
struct ScanState{
  std::mutex mu; std::vector<ScanHit> hits; std::vector<uint32_t> targets; std::atomic<int> next{0},done{0},total{0}; std::atomic<bool> stop{false}; std::atomic<int> ready{0}; uint32_t myIp=0;
};
static void probeHost(ScanState&S,uint32_t ip){
  int fd=socket(AF_INET,SOCK_STREAM,0); if(fd<0)return; setTimeouts(fd,1500);
  sockaddr_in a=mkAddr(ip,3000);
  if(::connect(fd,(sockaddr*)&a,sizeof a)==0){
    const char*rq="GET /api/info HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n"; std::string rsp; char b[1024];
    if(sendAll(fd,rq,strlen(rq))){ for(int i=0;i<8;i++){ ssize_t k=::recv(fd,b,sizeof b,0); if(k<=0)break; rsp.append(b,k); if(rsp.size()>3000)break; } }
    size_t br=rsp.find('{'); if(br!=std::string::npos){ bool ok=false; J j=jparse(rsp.substr(br,rsp.rfind('}')==std::string::npos?0:rsp.rfind('}')-br+1),&ok);
      if(ok&&(j["port"].isNum()||j["app"].isStr())){ ScanHit h{ip,j["port"].i(3000),j["players"].i(0),j["phase"].str("lobby")}; std::lock_guard<std::mutex> g(S.mu); S.hits.push_back(h); } }
  }
  ::close(fd);
}
static void scanStart(std::shared_ptr<ScanState> S,uint32_t extraTarget=0){
  std::thread([S,extraTarget]{
    uint32_t me=0; { int s=socket(AF_INET,SOCK_DGRAM,0); if(s>=0){ sockaddr_in a=mkAddr(inet_addr("192.168.1.1"),9); if(::connect(s,(sockaddr*)&a,sizeof a)==0){ sockaddr_in m; socklen_t l=sizeof m; memset(&m,0,sizeof m); if(getsockname(s,(sockaddr*)&m,&l)==0)me=m.sin_addr.s_addr; } ::close(s); } }
    S->myIp=me; std::vector<uint32_t> subs; if(me)subs.push_back(ntohl(me)&0xFFFFFF00u);
    static const char*COMMON[]={"192.168.1.0","192.168.0.0","192.168.43.0","192.168.8.0","192.168.100.0","192.168.2.0","10.0.0.0","10.0.1.0","172.20.10.0","192.168.3.0","192.168.10.0","192.168.137.0"};
    if(!me)for(auto c:COMMON)subs.push_back(ntohl(inet_addr(c)));
    if(extraTarget)S->targets.push_back(extraTarget);
    for(uint32_t sb:subs)for(int i=1;i<255;i++){ uint32_t ip=htonl(sb|i); if(ip!=me)S->targets.push_back(ip); }
    S->total=(int)S->targets.size(); S->ready=1;
    for(int w=0;w<40;w++)std::thread([S]{ for(;;){ if(S->stop)break; int i=S->next++; if(i>=S->total)break; probeHost(*S,S->targets[i]); S->done++; } }).detach();
  }).detach();
}
