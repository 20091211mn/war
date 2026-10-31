#pragma once
// تشكيل العربية (ربط الحروف) + ترتيب مرئي مبسط RTL/LTR
#include <string>
#include <vector>
#include <cstdint>
#include "fontdata.h"
namespace AR{
struct Form{uint16_t b,iso,fin,ini,med;};
static const Form FM[]={
{0x621,0xFE80,0,0,0},{0x622,0xFE81,0xFE82,0,0},{0x623,0xFE83,0xFE84,0,0},{0x624,0xFE85,0xFE86,0,0},{0x625,0xFE87,0xFE88,0,0},
{0x626,0xFE89,0xFE8A,0xFE8B,0xFE8C},{0x627,0xFE8D,0xFE8E,0,0},{0x628,0xFE8F,0xFE90,0xFE91,0xFE92},{0x629,0xFE93,0xFE94,0,0},
{0x62A,0xFE95,0xFE96,0xFE97,0xFE98},{0x62B,0xFE99,0xFE9A,0xFE9B,0xFE9C},{0x62C,0xFE9D,0xFE9E,0xFE9F,0xFEA0},
{0x62D,0xFEA1,0xFEA2,0xFEA3,0xFEA4},{0x62E,0xFEA5,0xFEA6,0xFEA7,0xFEA8},{0x62F,0xFEA9,0xFEAA,0,0},{0x630,0xFEAB,0xFEAC,0,0},
{0x631,0xFEAD,0xFEAE,0,0},{0x632,0xFEAF,0xFEB0,0,0},{0x633,0xFEB1,0xFEB2,0xFEB3,0xFEB4},{0x634,0xFEB5,0xFEB6,0xFEB7,0xFEB8},
{0x635,0xFEB9,0xFEBA,0xFEBB,0xFEBC},{0x636,0xFEBD,0xFEBE,0xFEBF,0xFEC0},{0x637,0xFEC1,0xFEC2,0xFEC3,0xFEC4},
{0x638,0xFEC5,0xFEC6,0xFEC7,0xFEC8},{0x639,0xFEC9,0xFECA,0xFECB,0xFECC},{0x63A,0xFECD,0xFECE,0xFECF,0xFED0},
{0x641,0xFED1,0xFED2,0xFED3,0xFED4},{0x642,0xFED5,0xFED6,0xFED7,0xFED8},{0x643,0xFED9,0xFEDA,0xFEDB,0xFEDC},
{0x644,0xFEDD,0xFEDE,0xFEDF,0xFEE0},{0x645,0xFEE1,0xFEE2,0xFEE3,0xFEE4},{0x646,0xFEE5,0xFEE6,0xFEE7,0xFEE8},
{0x647,0xFEE9,0xFEEA,0xFEEB,0xFEEC},{0x648,0xFEED,0xFEEE,0,0},{0x649,0xFEEF,0xFEF0,0,0},{0x64A,0xFEF1,0xFEF2,0xFEF3,0xFEF4},
{0x640,0x640,0x640,0x640,0x640}};
static const Form* fm(uint32_t c){ for(const auto&f:FM)if(f.b==c)return &f; return nullptr; }
static bool isAr(uint32_t c){ return (c>=0x600&&c<=0x6FF)||(c>=0xFE70&&c<=0xFEFF); }
static bool isMark(uint32_t c){ return c>=0x64B&&c<=0x65F; }
static bool isAlnum(uint32_t c){ return (c>='0'&&c<='9')||(c>='a'&&c<='z')||(c>='A'&&c<='Z'); }
inline std::vector<uint32_t> utf8(const char*s){ std::vector<uint32_t> o; while(*s){ unsigned char c=*s; uint32_t cp; int n; if(c<0x80){cp=c;n=1;} else if((c>>5)==6){cp=c&31;n=2;} else if((c>>4)==14){cp=c&15;n=3;} else {cp=c&7;n=4;} for(int i=1;i<n;i++){ if(!s[i])break; cp=(cp<<6)|(s[i]&63);} int k=1; while(k<n&&s[k])k++; s+=k; o.push_back(cp);} return o; }
// تشكيل منطقي: حروف عربية -> أشكال العرض
inline std::vector<uint32_t> shape(const std::vector<uint32_t>&in){
  std::vector<uint32_t> s; for(auto c:in)if(!isMark(c))s.push_back(c);
  std::vector<uint32_t> o; int n=s.size();
  auto joinsNext=[&](int i){ const Form*f=fm(s[i]); return f&&f->ini; };       // يتصل بما بعده
  auto joinsPrev=[&](int i){ if(i<0||i>=n)return false; const Form*f=fm(s[i]); return f&&f->fin; };
  for(int i=0;i<n;i++){
    uint32_t c=s[i]; const Form*f=fm(c); if(!f){ o.push_back(c); continue; }
    bool pj=i>0&&joinsNext(i-1)&&f->fin; // السابق يمدّ إليّ
    if(c==0x644&&i+1<n&&(s[i+1]==0x627||s[i+1]==0x623||s[i+1]==0x625||s[i+1]==0x622)){
      uint32_t a=s[i+1]; uint32_t iso=a==0x627?0xFEFB:a==0x623?0xFEF7:a==0x625?0xFEF9:0xFEF5; o.push_back(pj?iso+1:iso); i++; continue; }
    bool nj=f->ini&&joinsPrev(i+1);
    uint32_t g= pj&&nj?f->med: pj?f->fin: nj?f->ini: f->iso; if(!g)g=f->iso; o.push_back(g);
  }
  return o;
}
inline uint32_t mirror(uint32_t c){ switch(c){case '(':return ')';case ')':return '(';case '<':return '>';case '>':return '<';case '[':return ']';case ']':return '[';case 0x25B6:return 0x25C0;case 0x25C0:return 0x25B6;case 0x2190:return 0x2192;case 0x2192:return 0x2190;} return c; }
// ترتيب مرئي (يسار->يمين) لسطر واحد
inline std::vector<uint32_t> visual(const char*str){
  std::vector<uint32_t> L=utf8(str); int n=L.size(); if(!n)return L;
  // 0 = LTR , 1 = RTL , 2 = محايد
  std::vector<int> cl(n); bool anyAr=false;
  for(int i=0;i<n;i++){ cl[i]=isAr(L[i])?1:isAlnum(L[i])?0:2; if(cl[i]==1)anyAr=true; }
  if(!anyAr) return L;
  int para=1; for(int i=0;i<n;i++){ if(cl[i]!=2){ para=cl[i]; break; } }
  // حل المحايد
  for(int i=0;i<n;i++)if(cl[i]==2){ int a=-1,b=-1; for(int j=i-1;j>=0;j--)if(cl[j]!=2){a=cl[j];break;} for(int j=i+1;j<n;j++)if(cl[j]!=2){b=cl[j];break;} cl[i]=(a==b&&a>=0)?a:para; if(cl[i]==2)cl[i]=para; }
  // أرقام: النقطة/الفاصلة بين أرقام تبقى LTR
  for(int i=1;i+1<n;i++)if((L[i]=='.'||L[i]==':'||L[i]==','||L[i]=='/')&&L[i-1]>='0'&&L[i-1]<='9'&&L[i+1]>='0'&&L[i+1]<='9')cl[i]=0;
  struct Run{int d;std::vector<uint32_t> t;}; std::vector<Run> runs;
  for(int i=0;i<n;i++){ if(runs.empty()||runs.back().d!=cl[i])runs.push_back({cl[i],{}}); runs.back().t.push_back(L[i]); }
  std::vector<uint32_t> o; auto emit=[&](Run&r){ if(r.d==1){ auto sh=shape(r.t); for(int k=(int)sh.size()-1;k>=0;k--)o.push_back(mirror(sh[k])); } else for(auto c:r.t)o.push_back(c); };
  if(para==1) for(int i=(int)runs.size()-1;i>=0;i--)emit(runs[i]); else for(auto&r:runs)emit(r);
  return o;
}
// بحث عن حرف في حجم
inline const FGlyph* find(const FGlyph*a,int n,uint32_t cp){ int lo=0,hi=n-1; while(lo<=hi){ int m=(lo+hi)/2; if(a[m].cp==cp)return &a[m]; if(a[m].cp<cp)lo=m+1; else hi=m-1; } return nullptr; }
}
