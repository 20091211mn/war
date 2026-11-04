// اختبار عميل أندرويد ضد خادم JS الحقيقي: node server/server.js (المنفذ 3000)
#include "game.h"
#include <cstdio>
#include <unistd.h>
int main(int argc,char**argv){
  srand(5); Game g; g.menu=1; g.init(320,180); g.aclStart(inet_addr(argc>1?argv[1]:"127.0.0.1"),3000);
  Input idle,mv; mv.my=1; mv.fire=true; int started=0,fedBots=0,played=0;
  for(int f=0;f<60*20;f++){
    usleep(8000); Input in=idle; if(g.menu==0&&g.role==3&&f>600)in=mv; if(g.menu==0&&f>600&&(f/120)%2)in.lx=1.5f;
    if(f==700)in.c4=true; if(f>=700&&f<=780&&f%10==0)printf("f%d c4s %zu left %d\n",f,g.c4s.size(),g.e.empty()?-1:g.e[g.meI].c4Left); if(f==760)printf("c4 placed: %zu left %d\n",g.c4s.size(),g.e.empty()?-1:g.e[g.meI].c4Left); if(f==780)in.det=true; if(f==840)printf("c4 after det: %zu\n",g.c4s.size());
    if(g.ac&&g.menu==8){ if(!fedBots&&f>30){ g.ac->ws.send("{\"t\":\"cfg\",\"fill\":4,\"map\":4}"); fedBots=1; } if(f>90&&!started){ in.confirm=true; started=1; } }
    if(g.role==3||g.menu>=5)g.update(1/60.f,in); else { printf("dropped to role %d menu %d msg '%s'\n",g.role,g.menu,g.msg); break; }
    if(g.menu==0&&g.role==3){g.render();played++;} else g.renderMenu();
    if(f%300==299&&g.ac){ int alive=0;for(auto&p:g.e)alive+=p.alive; printf("f%d menu %d role %d ents %zu alive %d me %d pos %.1f,%.1f hp %.0f score %d-%d t %d veh %zu feed '%s'\n",f,g.menu,g.role,g.e.size(),alive,g.meI,g.e.empty()?0:g.e[g.meI].b.x,g.e.empty()?0:g.e[g.meI].b.z,g.e.empty()?0:g.e[g.meI].hp,g.score[0],g.score[1],g_aclTime,g.ac->vs.size(),g.feed[0].c_str()); }
  }
  printf("vh %zu types:",g.vh.size()); for(auto&v:g.vh)printf(" %d",v.type); printf("\n");
  if(g.ac)g.aclStop();
  printf("played frames %d\n",played); return played>600?0:1;
}
