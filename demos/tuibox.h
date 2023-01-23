/*
 * tuibox.h: simple tui library
 */

#ifndef TUIBOX_H
#define TUIBOX_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include "vec.h"

/*
 * PREPROCESSOR
 */
#define MAXCACHESIZE 65535

#define CURSOR_Y(b) (b->y + (n + 1) + (u->canscroll ? u->scroll : 0))

#define box_contains(x, y, b)                                                  \
  (x >= b->x && x <= b->x + b->w && y >= b->y && y <= b->y + b->h)

#define ui_screen(s, u)                                                        \
  u->screen = s;                                                               \
  u->force = 1

#define ui_center_x(w, u) (((u)->ws.ws_col - w) / 2)
#define ui_center_y(h, u) (((u)->ws.ws_row - h) / 2)

#define UI_CENTER_X -1
#define UI_CENTER_Y -1

/* The argument isn't actually necessary here, but it helps with design
 * consistency */
#define ui_loop(u)                                                             \
  char buf[64];                                                                \
  int n;                                                                       \
  while ((n = read(STDIN_FILENO, buf, sizeof(buf))) > 0)

#define ui_update(u) _ui_update(buf, n, u)

#define ui_get(id, u) ((u)->b.data[id])

#define COORDINATE_DECODE()                                                    \
  tok = strtok(NULL, ";");                                                     \
  x = atoi(tok);                                                               \
  tok = strtok(NULL, ";");                                                     \
  y = strtol(tok, NULL, 10) - (u->canscroll ? u->scroll : 0)

#define LOOP_AND_EXECUTE(f)                                                    \
  do {                                                                         \
    vec_foreach(&(u->b), tmp, ind) {                                           \
      if (tmp->screen == u->screen && f != NULL && box_contains(x, y, tmp)) {  \
        f(tmp, x, y, u->mouse);                                                \
      }                                                                        \
    }                                                                          \
  } while (0)

/*
 * TYPES
 */
typedef void (*func)();
using draw_func = void (*)(struct ui_box_t *, char *);
using loop_func = void (*)(struct ui_box_t *, int, int, int);

struct ui_box_t {
  int id;
  int x, y;
  int w, h;
  int screen;
  char *cache;
  char *watch;
  char last;
  draw_func draw;
  loop_func onclick;
  loop_func onhover;
  void *data1;
  void *data2;
};

struct ui_evt_t {
  const char *c;
  func f;
};

typedef vec_t(ui_box_t *) vec_box_t;
typedef vec_t(ui_evt_t *) vec_evt_t;

struct ui_t {
  struct termios tio;
  struct winsize ws;
  vec_box_t b;
  vec_evt_t e;
  int mouse, screen, scroll, canscroll, id, force;
};

/* =========================== */

/*
 * Initializes a new UI struct,
 *   puts the terminal into raw
 *   mode, and prints out the
 *   necessary escape codes
 *   for mouse support.
 */
inline void ui_new(int s, ui_t *u) {
  struct termios raw;

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &(u->ws));

  tcgetattr(STDIN_FILENO, &(u->tio));
  raw = u->tio;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  vec_init(&(u->b));
  vec_init(&(u->e));

  printf("\x1b[?1049h\x1b[0m\x1b[2J\x1b[?1003h\x1b[?1015h\x1b[?1006h\x1b[?25l");

  u->mouse = 0;

  u->screen = s;
  u->scroll = 0;
  u->canscroll = 1;

  u->id = 0;

  u->force = 0;
}

/*
 * Frees the given UI struct,
 *   and takes the terminal
 *   out of raw mode.
 */
inline void ui_free(ui_t *u) {
  ui_box_t *val;
  ui_evt_t *evt;
  int i;
  char *term;

  printf("\x1b[0m\x1b[2J\x1b[?1049l\x1b[?1003l\x1b[?1015l\x1b[?1006l\x1b[?25h");
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &(u->tio));

  vec_foreach(&(u->b), val, i) {
    free(val->cache);
    free(val);
  }
  vec_deinit(&(u->b));

  vec_foreach(&(u->e), evt, i) { free(evt); }
  vec_deinit(&(u->e));

  term = getenv("TERM");
  if (strncmp(term, "screen", 6) == 0 || strncmp(term, "tmux", 4) == 0) {
    printf("Note: Terminal multiplexer detected.\n  For best performance (i.e. "
           "reduced flickering), running natively inside\n  a GPU-accelerated "
           "terminal such as alacritty or kitty is recommended.\n");
  }
}

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
inline int ui_add(int x, int y, int w, int h, int screen, char *watch, char initial,
           draw_func draw, loop_func onclick, loop_func onhover, void *data1,
           void *data2, ui_t *u) {
  char *buf = (char *)malloc(MAXCACHESIZE);

  auto b = (ui_box_t *)malloc(sizeof(ui_box_t));

  b->id = u->id++;

  b->x = (x == UI_CENTER_X ? ui_center_x(w, u) : x);
  b->y = (y == UI_CENTER_Y ? ui_center_y(h, u) : y);
  b->w = w;
  b->h = h;

  b->screen = u->screen;

  b->watch = watch;
  b->last = initial;

  b->draw = draw;
  b->onclick = onclick;
  b->onhover = onhover;

  b->data1 = data1;
  b->data2 = data2;

  draw(b, buf);
  b->cache = (char *)realloc(buf, strlen(buf) * 2);

  vec_push(&(u->b), b);

  return b->id;
}

/*
 * Adds a new key event listener
 *   to the UI.
 */
inline void ui_key(const char *c, func f, ui_t *u) {
  auto e = (ui_evt_t *)malloc(sizeof(ui_evt_t));
  e->c = c;
  e->f = f;

  vec_push(&(u->e), e);
}

/*
 * Clears all elements from
 *   the UI.
 */
inline void ui_clear(ui_t *u) {
  int tmp = u->screen;

  ui_free(u);
  ui_new(tmp, u);
}

/*
 * Draws a single box to the
 *   screen.
 */
inline void ui_draw_one(ui_box_t *tmp, int flush, ui_t *u) {
  char *buf, *tok;
  int n = -1;

  if (tmp->screen != u->screen)
    return;

  buf = (char *)calloc(1, strlen(tmp->cache) * 2);
  if (u->force || tmp->watch == NULL || *(tmp->watch) != tmp->last) {
    tmp->draw(tmp, buf);
    if (tmp->watch != NULL)
      tmp->last = *(tmp->watch);
    strcpy(tmp->cache, buf);
  } else {
    /* buf is allocated proportionally to tmp->cache, so strcpy is safe */
    strcpy(buf, tmp->cache);
  }
  tok = strtok(buf, "\n");
  while (tok != NULL) {
    if (tmp->x > 0 && tmp->x < u->ws.ws_col && CURSOR_Y(tmp) > 0 &&
        CURSOR_Y(tmp) < u->ws.ws_row) {
      printf("\x1b[%i;%iH%s", CURSOR_Y(tmp), tmp->x, tok);
      n++;
    }
    tok = strtok(NULL, "\n");
  }
  free(buf);

  if (flush)
    fflush(stdout);
}

/*
 * Draws all boxes to the screen.
 */
inline void ui_draw(ui_t *u) {
  ui_box_t *tmp;
  int i;

  printf("\x1b[0m\x1b[2J");

  vec_foreach(&(u->b), tmp, i) { ui_draw_one(tmp, 0, u); }
  fflush(stdout);
  u->force = 0;
}

/*
 * Forces a redraw of the screen,
 *   updating all boxes' caches.
 */
inline void ui_redraw(ui_t *u) {
  u->force = 1;
  ui_draw(u);
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
inline void _ui_update(char *c, int n, ui_t *u) {
  ui_box_t *tmp;
  ui_evt_t *evt;
  int ind, x, y;
  char cpy[n], *tok;

  if (n >= 4 && c[0] == '\x1b' && c[1] == '[' && c[2] == '<') {
    strncpy(cpy, c, n);
    tok = strtok(cpy + 3, ";");

    switch (tok[0]) {
    case '0':
      u->mouse = (strchr(c, 'm') == NULL);
      if (u->mouse) {
        COORDINATE_DECODE();
        LOOP_AND_EXECUTE(tmp->onclick);
      }
      break;
    case '3':
      u->mouse = (strcmp(tok, "32") == 0);
      COORDINATE_DECODE();
      LOOP_AND_EXECUTE(tmp->onhover);
      break;
    case '6':
      if (u->canscroll) {
        u->scroll += (4 * (tok[1] == '4')) - 2;
        printf("\x1b[0m\x1b[2J");
        ui_draw(u);
      }
      break;
    }
  }

  vec_foreach(&(u->e), evt, ind) {
    if (strncmp(c, evt->c, strlen(evt->c)) == 0)
      evt->f();
  }
}

/*
 * HELPERS
 */
inline void _ui_text(ui_box_t *b, char *out) { sprintf(out, "%s", (char *)b->data1); }

inline int ui_text(int x, int y, char *str, int screen, loop_func click,
            loop_func hover, ui_t *u) {
  return ui_add(x, y, strlen(str), 1, screen, NULL, 0, _ui_text, click, hover,
                str, NULL, u);
}

#endif
