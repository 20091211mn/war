// اختبار شبكة على الحلقة المحلية: مضيف + عميل في عملية واحدة
#include "game.h"
#include <cstdio>
int main(){
  srand(3); Game h,c; h.menu=0; h.menuSel=1; h.init(320,180); h.hostStart(); c.init(320,180); c.joinDirect("127.0.0.1");
  Input idle,mv; mv.my=1; mv.fire=true; int ok=0;
  for(int f=0;f<60*12;f++){
    h.update(1/60.f,idle); c.update(1/60.f,f>120?mv:idle); h.render(); if(c.role==2)c.render();
    if(f==40)printf("client role %d joining %d meI %d ents %zu\n",c.role,c.joining,c.meI,c.e.size());
    if(f==60*12-1){ printf("end: host humans %d | client role %d meI %d alive %d pos %.1f,%.1f | host view of client: pos %.1f,%.1f ammo %d | score %d-%d host %d-%d\n",h.humans(),c.role,c.meI,c.e[c.meI].alive,c.e[c.meI].b.x,c.e[c.meI].b.z,h.e[c.meI].b.x,h.e[c.meI].b.z,h.e[c.meI].ammo,c.score[0],c.score[1],h.score[0],h.score[1]); }
  }
  // دقة: الأعداء عند العميل = المضيف تقريبا
  float md=0; for(size_t i=0;i<h.e.size();i++){ if((int)i==c.meI)continue; md=std::max(md,std::hypot(h.e[i].b.x-c.e[i].b.x,h.e[i].b.z-c.e[i].b.z)); } printf("max pos diff %.2f\n",md);
  c.netClose(); for(int f=0;f<30;f++)h.update(1/60.f,idle); printf("after bye humans %d\n",h.humans());
}
