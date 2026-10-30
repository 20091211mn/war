#include "game.h"
#include <cstdio>
#include <chrono>
static void ppm(Renderer&r,const char*fn){FILE*f=fopen(fn,"wb");fprintf(f,"P6\n%d %d\n255\n",r.W,r.H);for(auto p:r.fb){fputc((p>>16)&255,f);fputc((p>>8)&255,f);fputc(p&255,f);}fclose(f);}
int main(int argc,char**argv){
  int mi=argc>1?atoi(argv[1]):0; int secs=argc>2?atoi(argv[2]):10; srand(7);
  Game g; g.nBots=7; g.menu=0; g.mapIdx=mi; g.init(960,540);
  Input in; auto t0=std::chrono::steady_clock::now(); int frames=0; double upd=0,rnd=0;
  for(int f=0;f<secs*60;f++){
    in.my=1; in.lx=0.3f*std::sin(f*0.02f); in.fire=(f%90)<30;
    auto a=std::chrono::steady_clock::now(); g.update(1/60.f,in); auto b=std::chrono::steady_clock::now();
    g.render(); auto c=std::chrono::steady_clock::now(); upd+=std::chrono::duration<double,std::milli>(b-a).count(); rnd+=std::chrono::duration<double,std::milli>(c-b).count(); frames++;
    if(f==secs*60/2||f==secs*60-1){char fn[64];snprintf(fn,64,"/tmp/g%d_%d.ppm",mi,f==secs*60-1);ppm(g.r,fn);}
  }
  printf("map %d: update %.2f ms render %.2f ms | score %d-%d | kills me %d\n",mi,upd/frames,rnd/frames,g.score[0],g.score[1],g.e[0].kills);
  int alive=0;for(auto&p:g.e)alive+=p.alive;printf("alive %d/%zu\n",alive,g.e.size());
}
