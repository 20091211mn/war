// Golden-state dumper for the Kotlin port: drives the C++ engine with a fixed seed + scripted input and prints state.
// Build: g++ -std=c++17 -O2 -pthread -I../../src golden.cpp -o golden   (run from this directory)
#define THREADS 1
#include "../../src/game.h"
#include <cstdio>
int main(int argc,char**argv){
  int mi=argc>1?atoi(argv[1]):0; int seed=argc>2?atoi(argv[2]):7; srand(seed);
  Game g; g.nBots=6; g.menu=0; g.mapIdx=mi; g.init(480,270);
  Input in;
  auto dump=[&](int f){
    printf("F %d score %d %d t %.4f\n",f,g.score[0],g.score[1],g.t);
    for(size_t i=0;i<g.e.size();i++){ const Ent&p=g.e[i]; printf("E %zu %.4f %.4f %.4f hp %.3f al %d k %d d %d ammo %d wpn %d yaw %.4f\n",i,p.b.x,p.b.y,p.b.z,p.hp,(int)p.alive,p.kills,p.deaths,p.ammo,p.wpn,p.yaw); }
    for(size_t i=0;i<g.vh.size();i++){ const auto&v=g.vh[i]; printf("V %zu %.3f %.3f %.3f hp %.2f dead %d\n",i,v.x,v.z,v.h,v.hp,(int)v.dead); }
  };
  for(int f=0;f<=900;f++){
    in=Input(); in.my=1; in.lx=0.3f*std::sin(f*0.02f); in.fire=(f%90)<30; in.reload=(f%400)==399; in.jump=(f%150)<3;
    g.update(1/60.f,in);
    if(f==0||f==59||f==299||f==599||f==899)dump(f+1);
  }
}
