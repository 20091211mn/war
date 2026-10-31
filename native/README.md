# Warzone Native (PS4)
C++ خالص بلا متصفح: مرسم برمجي (CPU) + فيزياء + بوتات → سطح SDL2. الأزرار: L-stick حركة، R-stick نظر، R2 إطلاق، L2 تصويب، X قفز، O انحناء، مربع إعادة تعبئة، L1/R1 سلاح، L3 عدو، OPTIONS إيقاف. مثلث = شاشة تشخيص (قيم المحاور والأزرار).
اختبار على الحاسوب: `node tools/export-native.js && g++ -O2 -std=c++17 native/src/headless.cpp -o hl && ./hl 0 10`

## LAN
القائمة الرئيسية: صف MODE ← HOST LAN GAME (المضيف يظهر عنوان IP ويبث إشارة كل ثانية) أو JOIN LAN GAME (يبحث عن المضيفين ويعرضهم). حتى 8 لاعبين، والبوتات تملأ الباقي. المنفذان UDP 9330 (لعب) و9331 (اكتشاف).
اختبار: `g++ -O2 -std=c++17 native/src/nettest.cpp -o nt && ./nt`
