// قارئ JSON صغير (للتخاطب مع نسخة أندرويد)
#pragma once
#include <string>
#include <vector>
#include <utility>
#include <cstdlib>
#include <cstring>

struct J{
  enum T{NUL,NUM,STR,ARR,OBJ,BOOL}t=NUL; double n=0; std::string s; std::vector<J> a; std::vector<std::pair<std::string,J>> o;
  bool isNum()const{return t==NUM;} bool isArr()const{return t==ARR;} bool isStr()const{return t==STR;}
  size_t size()const{return t==ARR?a.size():t==OBJ?o.size():0;}
  const J& operator[](size_t i)const{ static const J nul; return (t==ARR&&i<a.size())?a[i]:nul; }
  const J& operator[](int i)const{ return (*this)[(size_t)i]; }
  const J& operator[](const char*k)const{ static const J nul; if(t==OBJ)for(auto&kv:o)if(kv.first==k)return kv.second; return nul; }
  double num(double d=0)const{ return t==NUM?n:t==BOOL?n:d; }
  int i(int d=0)const{ return t==NUM?(int)n:t==BOOL?(int)n:d; }
  std::string str(const char*d="")const{ return t==STR?s:std::string(d); }
};
struct JParser{
  const char*p;const char*e; bool ok=true;
  JParser(const char*b,size_t n):p(b),e(b+n){}
  void ws(){while(p<e&&(*p==' '||*p=='\n'||*p=='\r'||*p=='\t'))p++;}
  static void utf8(std::string&o,unsigned c){ if(c<0x80)o+=(char)c; else if(c<0x800){o+=(char)(0xC0|(c>>6));o+=(char)(0x80|(c&63));} else {o+=(char)(0xE0|(c>>12));o+=(char)(0x80|((c>>6)&63));o+=(char)(0x80|(c&63));} }
  std::string str(){ std::string r; p++; while(p<e&&*p!='"'){ if(*p=='\\'&&p+1<e){ p++; char c=*p++; switch(c){case 'n':r+='\n';break;case 't':r+='\t';break;case 'r':r+='\r';break;case 'b':case 'f':break;case 'u':{ if(p+4<=e){ char h[5]={p[0],p[1],p[2],p[3],0}; utf8(r,(unsigned)strtoul(h,nullptr,16)); p+=4; } break;} default:r+=c;} } else r+=*p++; } if(p<e)p++; else ok=false; return r; }
  J val(int depth=0){ J v; ws(); if(p>=e||depth>24){ok=false;return v;}
    char c=*p;
    if(c=='{'){ v.t=J::OBJ; p++; ws(); if(p<e&&*p=='}'){p++;return v;} for(;;){ ws(); if(p>=e||*p!='"'){ok=false;return v;} std::string k=str(); ws(); if(p>=e||*p!=':'){ok=false;return v;} p++; v.o.push_back({k,val(depth+1)}); ws(); if(p<e&&*p==','){p++;continue;} if(p<e&&*p=='}'){p++;break;} ok=false;return v; } }
    else if(c=='['){ v.t=J::ARR; p++; ws(); if(p<e&&*p==']'){p++;return v;} for(;;){ v.a.push_back(val(depth+1)); ws(); if(p<e&&*p==','){p++;continue;} if(p<e&&*p==']'){p++;break;} ok=false;return v; } }
    else if(c=='"'){ v.t=J::STR; v.s=str(); }
    else if(c=='t'&&e-p>=4&&!strncmp(p,"true",4)){ v.t=J::BOOL; v.n=1; p+=4; }
    else if(c=='f'&&e-p>=5&&!strncmp(p,"false",5)){ v.t=J::BOOL; v.n=0; p+=5; }
    else if(c=='n'&&e-p>=4&&!strncmp(p,"null",4)){ p+=4; }
    else { char*q; v.n=strtod(p,&q); if(q==p){ok=false;return v;} v.t=J::NUM; p=q; }
    return v; }
};
static J jparse(const std::string&s,bool*ok=nullptr){ JParser jp(s.data(),s.size()); J v=jp.val(); if(ok)*ok=jp.ok; return v; }
