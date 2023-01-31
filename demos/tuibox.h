/*
 * tuibox.h: simple tui library
 */
#pragma once
#include <string>
#include <vector>

const int UI_CENTER_X = -1;
const int UI_CENTER_Y = -1;

/*
 * TYPES
 */
typedef void (*func)();
using draw_func = std::string (*)(struct ui_box_t *);
using loop_func = void (*)(struct ui_box_t *, int, int, int);

struct ui_box_t {
  int id;
  int x, y;
  int w, h;
  int screen;
  std::string cache;
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

class tuibox {
  class ui_t_impl *impl_ = nullptr;
  std::vector<ui_box_t> b;
  std::vector<ui_evt_t> e;
  int mouse = 0;
  int screen;
  int scroll = 0;
  bool canscroll = true;
  int id = 0;
  int force = 0;

public:
  tuibox(const tuibox &) = delete;
  tuibox &operator=(const tuibox &) = delete;

  /*
   * Initializes a new UI struct,
   *   puts the terminal into raw
   *   mode, and prints out the
   *   necessary escape codes
   *   for mouse support.
   */
  tuibox(int s);

  /*
   * Frees the given UI struct,
   *   and takes the terminal
   *   out of raw mode.
   */
  ~tuibox();

  uint16_t cols() const;
  uint16_t rows() const;
  int ui_center_x(int w) const { return (cols() - w) / 2; }
  int ui_center_y(int h) const { return (rows() - h) / 2; }

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
