#include "game.h"
#include <cstdio>
#include <unistd.h>
int main(){
  setvbuf(stdout,0,_IONBF,0); srand(3); Game h,c; h.menu=0; h.menuSel=4; h.init(320,180); h.hostStart(); c.init(320,180); c.joinDirect("127.0.0.1");
  Input idle,tri,mv; tri.tri=true; mv.my=1;
  for(int f=0;f<60*14;f++){
    Input ci=idle;
    if(f==150)printf("hrole %d crole %d joining %d hmap %d nv %d hvh %zu cvh %zu\n",h.role,c.role,c.joining,h.mapIdx,h.w.m->nv,h.vh.size(),c.vh.size()); if(c.role==2&&f==150&&!c.vh.empty()){ auto&v=c.vh[0]; c.e[c.meI].b.x=v.x+2.2f; c.e[c.meI].b.z=v.z; printf("veh count h %zu c %zu\n",h.vh.size(),c.vh.size()); }
    if(f==155)ci=tri; if(f>170)ci=mv;
    usleep(8000); h.update(1/60.f,idle); c.update(1/60.f,ci);
    if(f>=156&&f<400&&f%20==0)printf("f%d drive %d hdrv %d cveh %.1f,%.1f sp %.1f dead %d hp %.0f alive %d\n",f,c.drive,h.vh[0].driver,c.vh[0].x,c.vh[0].z,c.vh[0].speed,(int)c.vh[0].dead,c.vh[0].hp,(int)c.e[c.meI].alive); if(f==160)printf("client drive %d | host driver %d\n",c.drive,h.vh.empty()?-9:h.vh[0].driver);
    if(f==60*14-1){ auto&a=h.vh[0];auto&b=c.vh[0]; printf("host veh %.1f,%.1f | client veh %.1f,%.1f speed %.1f drive %d driver %d | start %.1f,%.1f\n",a.x,a.z,b.x,b.z,b.speed,c.drive,a.driver,a.sx,a.sz);
      printf("host sees client ent at %.1f,%.1f\n",h.e[c.meI].b.x,h.e[c.meI].b.z); }
  }
  // خروج
  c.update(1/60.f,tri); for(int f=0;f<30;f++){ usleep(8000); h.update(1/60.f,idle); c.update(1/60.f,idle);} printf("after exit: drive %d host driver %d\n",c.drive,h.vh[0].driver);
}
