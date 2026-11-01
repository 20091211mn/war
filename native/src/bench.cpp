#include "game.h"
#include <chrono>
static double ms(std::chrono::steady_clock::time_point a,std::chrono::steady_clock::time_point b){return std::chrono::duration<double,std::milli>(b-a).count();}
int main(int argc,char**argv){ int mi=argc>1?atoi(argv[1]):0; int th=argc>2?atoi(argv[2]):1; int W=argc>3?atoi(argv[3]):1920; srand(7);
  Game g; g.r.startThreads(th); g.nBots=7; g.menu=0; g.mapIdx=mi; g.init(W,W*9/16); Input in; double tj=0,tf=0,th_=0; int N=0;
  for(int f=0;f<240;f++){ in.my=1; in.lx=0.3f*std::sin(f*0.02f); in.fire=(f%90)<30; g.update(1/60.f,in);
    g_flushMs=0; auto a=std::chrono::steady_clock::now(); g.render(); auto b=std::chrono::steady_clock::now(); g.r.flush(); auto c=std::chrono::steady_clock::now();
    if(f>=60){ tj+=ms(a,b); tf+=g_flushMs; N++; } }
  printf("map %d threads %d %dx%d: render(job gen+hud) %.2f ms, flush(raster) %.2f ms, jobs ~%zu\n",mi,th,g.r.W,g.r.H,tj/N,tf/N,g.r.jobs.size()); }
