// جسر JNI رفيع: يمرّر نداءات Kotlin إلى واجهة C للمحرك
#include <jni.h>
#include "../../mobile/bridge/wz_bridge.h"
#define FN(r,n) extern "C" JNIEXPORT r JNICALL Java_com_warzone_kotlin_Native_##n
FN(void,init)(JNIEnv*,jobject,jint w,jint h,jint c){ wz_init(w,h,c); }
FN(void,setScreen)(JNIEnv*,jobject,jint w,jint h){ wz_set_screen(w,h); }
FN(void,touch)(JNIEnv*,jobject,jint a,jint id,jfloat x,jfloat y){ wz_touch(a,id,x,y); }
FN(void,frame)(JNIEnv*,jobject,jfloat dt){ wz_frame(dt); }
FN(jint,width)(JNIEnv*,jobject){ return wz_width(); }
FN(jint,height)(JNIEnv*,jobject){ return wz_height(); }
FN(jint,resizeSeq)(JNIEnv*,jobject){ return wz_resize_seq(); }
FN(jobject,pixels)(JNIEnv*e,jobject){ const uint32_t*p=wz_pixels(); int n=wz_width()*wz_height(); if(!p||n<=0)return nullptr; return e->NewDirectByteBuffer((void*)p,(jlong)n*4); }
FN(void,mix)(JNIEnv*e,jobject,jshortArray arr,jint frames){ void*p=e->GetPrimitiveArrayCritical(arr,nullptr); if(!p)return; wz_mix((int16_t*)p,frames); e->ReleasePrimitiveArrayCritical(arr,p,0); }
FN(void,pause)(JNIEnv*,jobject){ wz_pause(); }
