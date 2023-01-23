/*
 * tuibox.h: simple tui library
 */

#ifndef TUIBOX_H
#define TUIBOX_H

#include "vec.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

const int UI_CENTER_X = -1;
const int UI_CENTER_Y = -1;

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

  bool box_contains(int x, int y) const {
    return x >= this->x && x <= this->x + this->w && y >= this->y &&
           y <= this->y + this->h;
  }
};

struct ui_evt_t {
  const char *c;
  func f;
};

typedef vec_t(ui_box_t *) vec_box_t;
typedef vec_t(ui_evt_t *) vec_evt_t;

class ui_t {
  struct termios tio;
  vec_box_t b;
  vec_evt_t e;
  int mouse, screen, scroll, canscroll, id, force;

public:
  struct winsize ws;

  int ui_center_x(int w) const { return (this->ws.ws_col - w) / 2; }
  int ui_center_y(int h) const { return (this->ws.ws_row - h) / 2; }

  /*
   * Initializes a new UI struct,
   *   puts the terminal into raw
   *   mode, and prints out the
   *   necessary escape codes
   *   for mouse support.
   */
  void ui_new(int s);

  /*
   * Frees the given UI struct,
   *   and takes the terminal
   *   out of raw mode.
   */
  void ui_free();

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
  int ui_add(int x, int y, int w, int h, int screen, char *watch, char initial,
             draw_func draw, loop_func onclick, loop_func onhover, void *data1,
             void *data2);

  /*
   * HELPERS
   */
  static void _ui_text(ui_box_t *b, char *out);

  int ui_text(int x, int y, char *str, int screen, loop_func click,
              loop_func hover);

  int CURSOR_Y(ui_box_t *b, int n);

  /*
   * Draws a single box to the
   *   screen.
   */
  void ui_draw_one(ui_box_t *tmp, int flush);

  /*
   * Draws all boxes to the screen.
   */
  void ui_draw();

  /*
   * Forces a redraw of the screen,
   *   updating all boxes' caches.
   */
  void ui_redraw();

  /*
   * Adds a new key event listener
   *   to the UI.
   */
  void ui_key(const char *c, func f);

  /*
   * Clears all elements from
   *   the UI.
   */
  void ui_clear();

  void ui_mainloop();

private:
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
  void _ui_update(char *c, int n);
};
#endif
