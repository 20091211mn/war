#define THREADS 2
#include "game.h"
#include <cstdio>
static void ppm(Renderer&r,const char*fn){FILE*f=fopen(fn,"wb");fprintf(f,"P6\n%d %d\n255\n",r.W,r.H);for(auto p:r.fb){fputc((p>>16)&255,f);fputc((p>>8)&255,f);fputc(p&255,f);}fclose(f);}
int main(int argc,char**argv){
  int mi=argc>1?atoi(argv[1]):0; int coop=argc>2?atoi(argv[2]):1; srand(7);
  Game g; g.r.startThreads(THREADS); g.nBots=5; g.menu=0; g.mapIdx=mi; g.nLocal=2; g.coop=coop; g.duel=argc>3&&atoi(argv[3]);  g.fovMul=1.12f; g.hudK=0.8f; g.init(480,540);
  printf("teams: %d %d bot %d %d alive %d %d\n",g.e[0].team,g.e[1].team,g.e[0].bot,g.e[1].bot,g.e[0].alive,g.e[1].alive);
  Input a,b; float x0=g.e[0].b.x,x1=g.e[1].b.x,z1=g.e[1].b.z;
  for(int f=0;f<60*20;f++){ a.my=1; a.lx=0.3f*std::sin(f*0.02f); a.fire=(f%90)<30; b.my=(f<600)?1.f:0.f; b.mx=0.5f; b.lx=-0.2f; b.fire=(f%120)<20; b.jump=(f%200)<5; g.in2=b; g.update(1/60.f,a);
    if(f==1199){ for(int p=0;p<2;p++){ if(p)g.lsx(); g.render(); g.r.flush(); if(getenv("SPLIT_PPM")){ char fn[64]; snprintf(fn,64,"/tmp/sp%d_%d.ppm",mi,p); ppm(g.r,fn); } if(p)g.lsx(); } } }
  printf("P1 pos %.1f,%.1f kills %d deaths %d | P2 pos %.1f,%.1f (moved %.1f) kills %d deaths %d | act=%d meI=%d score %d-%d\n",g.e[0].b.x,g.e[0].b.z,g.e[0].kills,g.e[0].deaths,g.e[1].b.x,g.e[1].b.z,hypotf(g.e[1].b.x-x1,g.e[1].b.z-z1),g.e[1].kills,g.e[1].deaths,g.act,g.meI,g.score[0],g.score[1]);
  int bad=0; if(g.duel&&(g.e.size()!=2||g.score[0]+g.score[1]!=g.e[0].kills+g.e[1].kills))bad|=64; printf("entities %zu\n",g.e.size()); if(g.act!=0||g.meI!=0)bad|=1; if(g.e[0].bot||g.e[1].bot)bad|=2; if(hypotf(g.e[1].b.x-x1,g.e[1].b.z-z1)<3.f)bad|=4; if(g.e[1].team!=(coop?0:1))bad|=8; if(g.e[1].ammo==g.e[0].ammo&&g.e[1].ammo==30)bad|=16;
  if(bad){ printf("SPLIT TEST FAILED %d\n",bad); return 1; } printf("split ok\n");
  printf("ammo P1 %d P2 %d\n",g.e[0].ammo,g.e[1].ammo); return 0;
}
