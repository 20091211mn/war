#include "game.h"
#include <cstdio>
int main(){ srand(3); Game g; g.menu=0; g.init(320,180); g.startMatch(4); Input in; for(int i=0;i<10;i++)g.update(1/60.f,in);
  auto&v=g.vh[0]; auto&me=g.e[g.meI]; me.b.x=v.x+3; me.b.z=v.z; me.b.y=0; g.update(1/60.f,in);
  Input u=in; u.tri=true; g.update(1/60.f,u); g.update(1/60.f,in); printf("drive %d gunner %d\n",g.drive,g.gunner);
  float x0=v.x,z0=v.z; in.my=1; for(int i=0;i<120;i++)g.update(1/60.f,in); printf("driver %d alive %d dead %d hp %.0f; ",v.driver,(int)me.alive,(int)v.dead,v.hp); printf("speed %.1f moved %.1f drive %d\n",v.speed,hypotf(v.x-x0,v.z-z0),g.drive); 
  Input s=in; s.seat=true; g.update(1/60.f,s); in.my=0; for(int i=0;i<180;i++)g.update(1/60.f,in); printf("gunner %d passenger %d speed %.2f y %.2f\n",g.gunner,g.passenger(),v.speed,me.b.y);
  int a0=me.ammo; Input f=in; f.fire=true; for(int i=0;i<20;i++)g.update(1/60.f,f); printf("ammo %d -> %d mnt %d\n",a0,me.ammo,(int)me.mnt);
  { int ei=-1; for(int j=0;j<(int)g.e.size();j++)if(g.e[j].team!=me.team&&g.e[j].alive){ei=j;break;} if(ei>=0){ auto&E=g.e[ei]; E.bot=false; E.b.x=me.b.x-sinf(me.yaw)*14.f; E.b.z=me.b.z-cosf(me.yaw)*14.f; E.b.y=me.b.y; E.hp=100; me.pitch=0; Input f2=in; f2.fire=true; for(int i=0;i<60;i++){ E.b.x=me.b.x-sinf(me.yaw)*14.f; E.b.z=me.b.z-cosf(me.yaw)*14.f; E.b.y=me.b.y; me.pitch=0; g.update(1/60.f,f2); } printf("mount dmg: enemy hp 100 -> %.0f alive %d prot %.2f tr %d cd %.3f yaw %.2f drive %d\n",E.hp,(int)E.alive,E.prot,(int)g.tr.size(),me.cd,me.yaw,g.drive); printf("E %.1f %.1f  me %.1f %.2f %.1f yaw %.2f\n",E.b.x,E.b.z,me.b.x,me.b.y,me.b.z,me.yaw); } }
  g.update(1/60.f,s); g.update(1/60.f,in); printf("gunner back %d\n",g.gunner); for(int i=0;i<5;i++)g.render();   bool okd=hypotf(v.x-x0,v.z-z0)>5.f; return (okd&&g.gunner==0)?0:1; }
