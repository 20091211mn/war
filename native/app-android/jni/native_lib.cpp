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
FN(jint,copyPixels)(JNIEnv*e,jobject,jobject buf,jintArray wh){ void*p=e->GetDirectBufferAddress(buf); if(!p)return 0; jlong cap=e->GetDirectBufferCapacity(buf)/4; int w=0,h=0; int ok=wz_copy_pixels((uint32_t*)p,(int)cap,&w,&h); if(ok){ jint v[2]={w,h}; e->SetIntArrayRegion(wh,0,2,v); } return ok; }
FN(void,mix)(JNIEnv*e,jobject,jshortArray arr,jint frames){ void*p=e->GetPrimitiveArrayCritical(arr,nullptr); if(!p)return; wz_mix((int16_t*)p,frames); e->ReleasePrimitiveArrayCritical(arr,p,0); }
FN(void,pause)(JNIEnv*,jobject){ wz_pause(); }
