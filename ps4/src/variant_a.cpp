// المتغير A: أقل ما يمكن — إشعار على الشاشة ثم انتظار (لعزل سبب فشل الإقلاع)
#include <stdio.h>
#include <string.h>
#include <orbis/libkernel.h>

int main(void) {
  OrbisNotificationRequest req;
  memset(&req, 0, sizeof(req));
  req.type = NotificationRequest;
  req.unk3 = 0;
  req.useIconImageUri = 1;
  req.targetId = -1;
  strncpy(req.message, "Warzone variant A started OK", sizeof(req.message) - 1);
  strcpy(req.iconUri, "cxml://psnotification/tex_icon_system");
  sceKernelSendNotificationRequest(0, &req, 3120, 0);
  for (;;) sceKernelUsleep(1000000);
}
