#include "tuibox.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define MAXCACHESIZE 65535

class ui_t_impl {
  struct termios tio;
  struct winsize ws;

public:
  ui_t_impl() {
    struct termios raw;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &(this->ws));
    tcgetattr(STDIN_FILENO, &(this->tio));
    raw = this->tio;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf(
        "\x1b[?1049h\x1b[0m\x1b[2J\x1b[?1003h\x1b[?1015h\x1b[?1006h\x1b[?25l");
  }

  ~ui_t_impl() {
    printf(
        "\x1b[0m\x1b[2J\x1b[?1049l\x1b[?1003l\x1b[?1015l\x1b[?1006l\x1b[?25h");
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &(this->tio));
  }

  bool Contains(int x, int y) const {
    return x > 0 && x < this->ws.ws_col && y > 0 && y < this->ws.ws_row;
  }

  uint16_t Cols() const { return ws.ws_col; }

  uint16_t Rows() const { return ws.ws_row; }
};

/*
 * Initializes a new UI struct,
 *   puts the terminal into raw
 *   mode, and prints out the
 *   necessary escape codes
 *   for mouse support.
 */
ui_t::ui_t(int s) : impl_(new ui_t_impl), screen(s) {}

/*
 * Frees the given UI struct,
 *   and takes the terminal
 *   out of raw mode.
 */
ui_t::~ui_t() {
  delete impl_;
  auto term = getenv("TERM");
  if (strncmp(term, "screen", 6) == 0 || strncmp(term, "tmux", 4) == 0) {
    printf("Note: Terminal multiplexer detected.\n  For best performance (i.e. "
           "reduced flickering), running natively inside\n  a GPU-accelerated "
           "terminal such as alacritty or kitty is recommended.\n");
  }
}

uint16_t ui_t::cols() const { return impl_->Cols(); }
uint16_t ui_t::rows() const { return impl_->Rows(); }

/*
 * Adds a new box to the UI.
 *
 * This function is very simple in
 *   nature, but the variety of
 *   properties associated with
 *   an individual box makes it
 *   intimidating to look at.
 * TODO: Find some way to
 *   strip this down.
 */
int ui_t::ui_add(int x, int y, int w, int h, int screen, char *watch,
                 char initial, draw_func draw, loop_func onclick,
                 loop_func onhover, void *data1, void *data2) {

  ui_box_t b = {};

  b.id = this->id++;

  b.x = (x == UI_CENTER_X ? this->ui_center_x(w) : x);
  b.y = (y == UI_CENTER_Y ? this->ui_center_y(h) : y);
  b.w = w;
  b.h = h;

  b.screen = this->screen;

  b.watch = watch;
  b.last = initial;

  b.draw = draw;
  b.onclick = onclick;
  b.onhover = onhover;

  b.data1 = data1;
  b.data2 = data2;

  b.cache = draw(&b);

  this->b.push_back(b);

  return b.id;
}

/*
 * HELPERS
 */
std::string _ui_text(ui_box_t *b) { return std::string((char *)b->data1); }

int ui_t::ui_text(int x, int y, char *str, int screen, loop_func click,
                  loop_func hover) {
  return this->ui_add(x, y, strlen(str), 1, screen, NULL, 0, _ui_text, click,
                      hover, str, NULL);
}

int ui_t::CURSOR_Y(ui_box_t *b, int n) {
  return (b->y + (n + 1) + (canscroll ? scroll : 0));
}

/*
 * Draws a single box to the
 *   screen.
 */
void ui_t::ui_draw_one(ui_box_t *tmp, int flush) {

  if (tmp->screen != this->screen)
    return;

  // buf = (char *)calloc(1, strlen(tmp->cache) * 2);
  std::string buf;
  if (this->force || tmp->watch == NULL || *(tmp->watch) != tmp->last) {
    buf = tmp->draw(tmp);
    if (tmp->watch != NULL)
      tmp->last = *(tmp->watch);
    tmp->cache = buf;
  } else {
    /* buf is allocated proportionally to tmp->cache, so strcpy is safe */
    buf = tmp->cache;
  }

  auto tok = strtok((char *)buf.data(), "\n");
  int n = -1;
  while (tok) {
    if (impl_->Contains(tmp->x, CURSOR_Y(tmp, n))) {
      printf("\x1b[%i;%iH%s", CURSOR_Y(tmp, n), tmp->x, tok);
      n++;
    }
    tok = strtok(NULL, "\n");
  }

  if (flush)
    fflush(stdout);
}

/*
 * Draws all boxes to the screen.
 */
void ui_t::ui_draw() {
  ui_box_t *tmp;
  int i;

  printf("\x1b[0m\x1b[2J");

  for (auto &tmp : this->b) {
    this->ui_draw_one(&tmp, 0);
  }
  fflush(stdout);
  this->force = 0;
}

/*
 * Forces a redraw of the screen,
 *   updating all boxes' caches.
 */
void ui_t::ui_redraw() {
  this->force = 1;
  ui_draw();
}

/*
 * Adds a new key event listener
 *   to the UI.
 */
void ui_t::ui_key(const char *c, func f) {
  this->e.push_back({
      .c = c,
      .f = f,
  });
}

/*
 * Handles mouse and keyboard
 *   events, given a read()
 *   buffer.
 *
 * This is prefixed with an underscore
 *   to ensure consistency with the
 *   ui_loop macro, ensuring that the
 *   variables buf and n remain
 *   opaque to the user.
 */
void ui_t::_ui_update(char *c, int n) {
  int x, y;
  char cpy[n], *tok;

  if (n >= 4 && c[0] == '\x1b' && c[1] == '[' && c[2] == '<') {
    strncpy(cpy, c, n);
    tok = strtok(cpy + 3, ";");

    switch (tok[0]) {
    case '0':
      this->mouse = (strchr(c, 'm') == NULL);
      if (this->mouse) {
        tok = strtok(NULL, ";");
        x = atoi(tok);
        tok = strtok(NULL, ";");
        y = strtol(tok, NULL, 10) - (this->canscroll ? this->scroll : 0);
        for (auto &tmp : this->b) {
          if (tmp.screen == this->screen && tmp.box_contains(x, y)) {
            tmp.onclick(&tmp, x, y, this->mouse);
          }
        }
      }
      break;
    case '3': {
      this->mouse = (strcmp(tok, "32") == 0);
      tok = strtok(NULL, ";");
      x = atoi(tok);
      tok = strtok(NULL, ";");
      y = strtol(tok, NULL, 10) - (this->canscroll ? this->scroll : 0);
      for (auto &tmp : this->b) {
        if (tmp.screen == this->screen && tmp.box_contains(x, y)) {
          tmp.onhover(&tmp, x, y, this->mouse);
        }
      }
    } break;
    case '6':
      if (this->canscroll) {
        this->scroll += (4 * (tok[1] == '4')) - 2;
        printf("\x1b[0m\x1b[2J");
        this->ui_draw();
      }
      break;
    }
  }

  for (auto &evt : this->e) {
    if (strncmp(c, evt.c, strlen(evt.c)) == 0)
      evt.f();
  }
}

void ui_t::ui_mainloop() {
  char buf[64];
  while (int n = read(STDIN_FILENO, buf, sizeof(buf))) {
    this->_ui_update(buf, n);
  }
}
