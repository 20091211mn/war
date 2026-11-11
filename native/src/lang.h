#pragma once
// ترجمة بسيطة: النص العربي هو المفتاح. g_lang: 0 عربي، 1 English، 2 Español. g_quality: 0..4
#include <unordered_map>
#include <string>
#include "lang_gen.h"
static int g_lang=0, g_quality=3;
static inline const char* trl(const char*s){
  if(g_lang<=0||!s||!*s) return s;
  static std::unordered_map<std::string,const TrE*> mp; if(mp.empty()) for(int i=0;i<TR_N;i++) mp[TR_TAB[i].ar]=&TR_TAB[i];
  auto it=mp.find(s); if(it==mp.end()) return s; return g_lang==1?it->second->en:it->second->es;
}
#define T(x) trl(x)
#include <cstdio>
static std::string g_cfgPath;
static inline void saveSettings(){ if(g_cfgPath.empty())return; FILE*f=fopen(g_cfgPath.c_str(),"w"); if(!f)return; fprintf(f,"%d %d\n",g_quality,g_lang); fclose(f); }
static inline void loadSettings(){ if(g_cfgPath.empty())return; FILE*f=fopen(g_cfgPath.c_str(),"r"); if(!f)return; int q=3,l=0; if(fscanf(f,"%d %d",&q,&l)==2){ if(q>=0&&q<5)g_quality=q; if(l>=0&&l<3)g_lang=l; } fclose(f); }
