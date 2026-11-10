// اختبار توافق شبكة بين المنصات: يعمل كمضيف أو عميل C++ حقيقي (نفس شيفرة PS4/iOS) ليتصل به Kotlin
#include "game.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
int main(int argc,char**argv){
  bool host=argc>1&&!strcmp(argv[1],"host"); srand(5); Game g; g.menu=0; g.menuSel=0; g.init(320,180);
  if(host)g.hostStart(); else g.joinDirect("127.0.0.1");
  Input idle; int ok=0; int secs=host?40:14;
  for(int f=0;f<60*secs;f++){ g.update(1/60.f,idle); if(f%4==0)g.render(); usleep(16000);
    if(host&&g.humans()>=2&&g.role==1)ok++; if(!host&&g.role==2&&g.menu==0&&g.e.size()>=2&&g.e[g.meI].alive)ok++;
    if(host&&ok>120){ printf("CROSS HOST OK humans=%d\n",g.humans()); fflush(stdout); return 0; }
    if(!host&&ok>240){ printf("CROSS JOIN OK slot=%d ents=%zu\n",g.meI,g.e.size()); fflush(stdout); return 0; } }
  printf("CROSS %s FAIL role=%d humans=%d\n",host?"HOST":"JOIN",g.role,g.humans()); return 1;
}
