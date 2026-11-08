// واجهة C للمحرك (C++) تستعملها تطبيقات Kotlin (JNI) و Swift (جسر): كل واجهة التطبيق والشاشة واللمس والصوت في لغة المنصة نفسها
#ifndef WZ_BRIDGE_H
#define WZ_BRIDGE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void wz_set_crash_file(const char* path);          // يكتب سبب الانهيار الأصلي (إشارة + المرحلة) في ملف ليظهر في التشغيل التالي
void wz_init(int screenW,int screenH,int cores);   // مرة واحدة
void wz_set_screen(int screenW,int screenH);       // تغيّر حجم الشاشة (بنفس وحدات اللمس)
void wz_touch(int action,int id,float x,float y);  // action: 0 ضغط ، 1 حركة ، 2 رفع ، 3 إلغاء الكل
void wz_frame(float dt);                           // تحديث + رسم
int  wz_width(void);  int wz_height(void);         // أبعاد الصورة الداخلية
int  wz_resize_seq(void);                          // يتغير عند تغيّر الدقة التلقائية
const uint32_t* wz_pixels(void);                   // RGBA (بايتات R,G,B,A) بحجم width*height
int  wz_copy_pixels(uint32_t* dst,int capPixels,int* w,int* h); // ينسخ الصورة تحت قفل المحرك (آمن مع تغيّر الدقة من خيط آخر)؛ يرجع 1 عند النجاح
void wz_mix(int16_t* stereo,int frames);           // صوت 48kHz ستيريو 16 بت (آمن للاستدعاء من خيط الصوت)
void wz_pause(void);                               // يوقف اللعبة مؤقتاً (عند الخروج من التطبيق)
#ifdef __cplusplus
}
#endif
#endif
