/*
 * words.c: Per-word events
 */

#include "../vt100utils.h"
#include "tui.h"
#include <sstream>

#define MIN(a, b) (a < b ? a : b)

tui *g_u = nullptr;
struct vt100_node_t *head;

std::string draw(tui_box *b) {
  struct vt100_node_t *node = (struct vt100_node_t *)b->data1;
  char *sgr = vt100_sgr(node, NULL);
  std::stringstream ss;
  ss << sgr << node->str;
  free(sgr);
  return ss.str();
}

void click(tui_box *b, int x, int y, int) {
  struct vt100_node_t *node = (struct vt100_node_t *)b->data1;
  node->fg.value += 10;
  if (node->fg.value > 255)
    node->fg.value = 10;
  g_u->draw();
}

void stop() {
  delete g_u;
  vt100_free(head);
  exit(0);
}

int main(void) {
  struct vt100_node_t *tmp;
  int x;
  int y;

  /* String manually generated with Javascript */
  head = vt100_decode(
      "\x1B[38;5;100mClick\x1B[38;5;101many\x1B[38;5;102mword\x1B[38;5;"
      "103mto\x1B[38;5;104mchange\x1B[38;5;105mits\x1B[38;5;106mcolor!\x1B[38;"
      "5;107mThis\x1B[38;5;108mis\x1B[38;5;109ma\x1B[38;5;110mlong\x1B[38;5;"
      "111mparagraph\x1B[38;5;112mof\x1B[38;5;113mtext,\x1B[38;5;114mand\x1B["
      "38;5;115mevery\x1B[38;5;116mword\x1B[38;5;117mcan\x1B[38;5;118mbe\x1B["
      "38;5;119mclicked.\x1B[38;5;120mWhile\x1B[38;5;121mbehavior\x1B[38;5;"
      "122mlike\x1B[38;5;123mthis\x1B[38;5;124mis\x1B[38;5;125mpossible\x1B[38;"
      "5;126mwith\x1B[38;5;127mstandalone\x1B[38;5;128mtui\x1B[38;5;129m("
      "or\x1B[38;5;130mother\x1B[38;5;131mlibraries),\x1B[38;5;132mit\x1B[38;5;"
      "133mwould\x1B[38;5;134mbe\x1B[38;5;135mincredibly\x1B[38;5;"
      "136mchallenging\x1B[38;5;137mand\x1B[38;5;138mcumbersome.\x1B[38;5;"
      "139mThis\x1B[38;5;140mdemo\x1B[38;5;141mis\x1B[38;5;142mwritten\x1B[38;"
      "5;143min\x1B[38;5;144mless\x1B[38;5;145mthan\x1B[38;5;146m100\x1B[38;5;"
      "147mlines\x1B[38;5;148mof\x1B[38;5;149mcode.");

  g_u = new tui(0);

  x = (g_u->cols() - 50) / 2;
  y = (g_u->rows() - 10) / 2;

  tmp = head->next;
  while (tmp != NULL) {
    g_u->add(x, y, tmp->len, 1, NULL, 0, draw, click, NULL, tmp, NULL);
    x += tmp->len;
    if (x > (g_u->cols() + 50) / 2) {
      x = (g_u->cols() - 50) / 2;
      y += 2;
    }
    tmp = tmp->next;
  }

  g_u->on_key("q", stop);
  g_u->draw();

  g_u->mainloop();

  return 0;
}
