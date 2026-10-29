// Warzone PS4 — خادم HTTP صغير داخل الحزمة يقدّم صفحة اللعبة المدمجة، ثم يفتح متصفح PS4 عليها.
// اللعب المنفرد (بوتات) يعمل كاملًا داخل الصفحة؛ والانضمام لغرفة LAN من هاتف/حاسوب يعمل عبر WebSocket من الصفحة.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <orbis/libkernel.h>
#include "game.h"

#define PORT 8080
#ifdef AUTOLAUNCH
extern "C" int ps4_launch_browser(const char *url, void *param) __asm__("sceSystemServiceLaunchWebBrowser");
#endif

static void notify(const char *msg) {
  OrbisNotificationRequest req;
  memset(&req, 0, sizeof(req));
  req.type = NotificationRequest;
  req.unk3 = 0;
  req.useIconImageUri = 1;
  req.targetId = -1;
  strncpy(req.message, msg, sizeof(req.message) - 1);
  strcpy(req.iconUri, "cxml://psnotification/tex_icon_system");
  sceKernelSendNotificationRequest(0, &req, 3120, 0);
}

static const unsigned char *g_page = GAME_HTML;
static size_t g_len = GAME_HTML_LEN;
static void load_page(void) {}

static int write_all(int fd, const unsigned char *p, size_t n) {
  while (n > 0) {
    ssize_t w = write(fd, p, n > 16384 ? 16384 : n);
    if (w < 0) { if (errno == EINTR) continue; return -1; }
    if (w == 0) return -1;
    p += w; n -= (size_t)w;
  }
  return 0;
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  int srv = socket(AF_INET, SOCK_STREAM, 0);
  if (srv < 0) { notify("Warzone: socket failed"); for (;;) sleep(5); }
  int one = 1;
  setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  struct sockaddr_in a;
  memset(&a, 0, sizeof(a));
  a.sin_family = AF_INET;
  a.sin_addr.s_addr = htonl(INADDR_ANY);
  a.sin_port = htons(PORT);
  if (bind(srv, (struct sockaddr *)&a, sizeof(a)) != 0 || listen(srv, 8) != 0) {
    notify("Warzone: cannot start server on port 8080"); for (;;) sleep(5);
  }
  load_page();
  notify(g_page ? "Warzone ready: open the PS4 Internet Browser and go to http://127.0.0.1:8080" : "Warzone: page.html missing from package");

#ifdef AUTOLAUNCH
  ps4_launch_browser("http://127.0.0.1:8080/", NULL);
#endif
  char hdr[256];
  for (;;) {
    struct sockaddr_in c; socklen_t cl = sizeof(c);
    int fd = accept(srv, (struct sockaddr *)&c, &cl);
    if (fd < 0) { if (errno == EINTR) continue; sleep(1); continue; }
    char req[1024]; ssize_t n = read(fd, req, sizeof(req) - 1);
    if (n > 0) {
      req[n] = 0;
      int root = (strncmp(req, "GET / ", 6) == 0) || (strncmp(req, "GET /index", 10) == 0) || (strncmp(req, "GET /?", 6) == 0);
      if (root && g_page) {
        int h = snprintf(hdr, sizeof(hdr), "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %u\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n", (unsigned)g_len);
        write_all(fd, (const unsigned char *)hdr, (size_t)h);
        write_all(fd, g_page, g_len);
      } else {
        const char *nf = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        write_all(fd, (const unsigned char *)nf, strlen(nf));
      }
    }
    close(fd);
  }
}
