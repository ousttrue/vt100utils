/*
 * hover.c: A simple truncation/hover animation
 */

#include "../vt100utils.h"
#include "tui.h"
#include <sstream>
#if _WIN32
#else
#include <unistd.h>
#endif

#define MIN(a, b) (a < b ? a : b)

tui *g_u = nullptr;
struct vt100_node_t *head;
int w = 12;

std::string draw(tui_box *b) {
  ;
  int len = 0;
  std::stringstream ss;
  for (struct vt100_node_t *tmp = head->next; tmp != NULL; tmp = tmp->next) {
    auto sgr = vt100_sgr(tmp, NULL);
    ss << sgr << std::string_view(tmp->str).substr(0, MAX(0, w - len));
    free(sgr);
  }
  ss << (w < 47 ? "..." : "") << "\n";
  return ss.str();
}

void click(tui_box *b, int x, int y, int) {
  while (w < 50) {
    w+=10;
    g_u->redraw();
    usleep(10000);
  }
}

void hover(tui_box *b, int x, int y, int down) {
  if (down) {
    click(b, x, y, {});
  } else {
    while (w > 12) {
      w--;
      g_u->redraw();
      usleep(10000);
    }
  }
}

void stop() {
  delete g_u;
  vt100_free(head);
  exit(0);
}

int main(void) {
  head = vt100_decode(
      "\x1b[36mClick to see "
      "\x1b[4;38;5;125mt\x1b[38;5;127mh\x1b[38;5;130mi\x1b[38;5;135ms "
      "\x1b[38;5;140mt\x1b[38;5;145me\x1b[38;5;150mx\x1b[38;5;155mt\x1b[0;36m "
      "un-truncate!");

  g_u = new tui(0);
  g_u->add(g_u->get_center(35, 1), draw, click, hover);
  g_u->on_key("q", stop);
  g_u->draw();
  g_u->mainloop();

  return 0;
}
