#include "tuibox.h"

#define MAXCACHESIZE 65535

#define COORDINATE_DECODE()                                                    \
  tok = strtok(NULL, ";");                                                     \
  x = atoi(tok);                                                               \
  tok = strtok(NULL, ";");                                                     \
  y = strtol(tok, NULL, 10) - (this->canscroll ? this->scroll : 0)

#define LOOP_AND_EXECUTE(f)                                                    \
  do {                                                                         \
    vec_foreach(&(this->b), tmp, ind) {                                        \
      if (tmp->screen == this->screen && f != NULL &&                          \
          tmp->box_contains(x, y)) {                                           \
        f(tmp, x, y, this->mouse);                                             \
      }                                                                        \
    }                                                                          \
  } while (0)

/*
 * Initializes a new UI struct,
 *   puts the terminal into raw
 *   mode, and prints out the
 *   necessary escape codes
 *   for mouse support.
 */
void ui_t::ui_new(int s) {
  struct termios raw;

  ioctl(STDOUT_FILENO, TIOCGWINSZ, &(this->ws));

  tcgetattr(STDIN_FILENO, &(this->tio));
  raw = this->tio;
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  vec_init(&(this->b));
  vec_init(&(this->e));

  printf("\x1b[?1049h\x1b[0m\x1b[2J\x1b[?1003h\x1b[?1015h\x1b[?1006h\x1b[?25l");

  this->mouse = 0;

  this->screen = s;
  this->scroll = 0;
  this->canscroll = 1;

  this->id = 0;

  this->force = 0;
}

/*
 * Frees the given UI struct,
 *   and takes the terminal
 *   out of raw mode.
 */
void ui_t::ui_free() {
  ui_box_t *val;
  ui_evt_t *evt;
  int i;
  char *term;

  printf("\x1b[0m\x1b[2J\x1b[?1049l\x1b[?1003l\x1b[?1015l\x1b[?1006l\x1b[?25h");
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &(this->tio));

  vec_foreach(&(this->b), val, i) {
    free(val->cache);
    free(val);
  }
  vec_deinit(&(this->b));

  vec_foreach(&(this->e), evt, i) { free(evt); }
  vec_deinit(&(this->e));

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
int ui_t::ui_add(int x, int y, int w, int h, int screen, char *watch,
                 char initial, draw_func draw, loop_func onclick,
                 loop_func onhover, void *data1, void *data2) {
  char *buf = (char *)malloc(MAXCACHESIZE);

  auto b = (ui_box_t *)malloc(sizeof(ui_box_t));

  b->id = this->id++;

  b->x = (x == UI_CENTER_X ? this->ui_center_x(w) : x);
  b->y = (y == UI_CENTER_Y ? this->ui_center_y(h) : y);
  b->w = w;
  b->h = h;

  b->screen = this->screen;

  b->watch = watch;
  b->last = initial;

  b->draw = draw;
  b->onclick = onclick;
  b->onhover = onhover;

  b->data1 = data1;
  b->data2 = data2;

  draw(b, buf);
  b->cache = (char *)realloc(buf, strlen(buf) * 2);

  vec_push(&(this->b), b);

  return b->id;
}

/*
 * HELPERS
 */
void ui_t::_ui_text(ui_box_t *b, char *out) {
  sprintf(out, "%s", (char *)b->data1);
}

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
  char *buf, *tok;
  int n = -1;

  if (tmp->screen != this->screen)
    return;

  buf = (char *)calloc(1, strlen(tmp->cache) * 2);
  if (this->force || tmp->watch == NULL || *(tmp->watch) != tmp->last) {
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
    if (tmp->x > 0 && tmp->x < this->ws.ws_col && CURSOR_Y(tmp, n) > 0 &&
        CURSOR_Y(tmp, n) < this->ws.ws_row) {
      printf("\x1b[%i;%iH%s", CURSOR_Y(tmp, n), tmp->x, tok);
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
void ui_t::ui_draw() {
  ui_box_t *tmp;
  int i;

  printf("\x1b[0m\x1b[2J");

  vec_foreach(&(this->b), tmp, i) { this->ui_draw_one(tmp, 0); }
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
  auto e = (ui_evt_t *)malloc(sizeof(ui_evt_t));
  e->c = c;
  e->f = f;

  vec_push(&(this->e), e);
}

/*
 * Clears all elements from
 *   the UI.
 */
void ui_t::ui_clear() {
  int tmp = this->screen;
  this->ui_free();
  this->ui_new(tmp);
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
  ui_box_t *tmp;
  ui_evt_t *evt;
  int ind, x, y;
  char cpy[n], *tok;

  if (n >= 4 && c[0] == '\x1b' && c[1] == '[' && c[2] == '<') {
    strncpy(cpy, c, n);
    tok = strtok(cpy + 3, ";");

    switch (tok[0]) {
    case '0':
      this->mouse = (strchr(c, 'm') == NULL);
      if (this->mouse) {
        COORDINATE_DECODE();
        LOOP_AND_EXECUTE(tmp->onclick);
      }
      break;
    case '3':
      this->mouse = (strcmp(tok, "32") == 0);
      COORDINATE_DECODE();
      LOOP_AND_EXECUTE(tmp->onhover);
      break;
    case '6':
      if (this->canscroll) {
        this->scroll += (4 * (tok[1] == '4')) - 2;
        printf("\x1b[0m\x1b[2J");
        this->ui_draw();
      }
      break;
    }
  }

  vec_foreach(&(this->e), evt, ind) {
    if (strncmp(c, evt->c, strlen(evt->c)) == 0)
      evt->f();
  }
}
