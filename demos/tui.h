/*
 * tui.h: simple tui library
 */
#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>

const int UI_CENTER_X = -1;
const int UI_CENTER_Y = -1;

/*
 * TYPES
 */
typedef void (*func)();
using draw_func = std::string (*)(struct tui_box *);
using loop_func = void (*)(struct tui_box *, int, int, int);

struct tui_box {
  int x, y;
  int w, h;
  int screen_;
  std::string cache;
  char *watch;
  char last;
  draw_func draw;
  loop_func onhover;
  void *data1;
  void *data2;
  tui_box() {}

public:
  loop_func onclick;
  tui_box(const tui_box &) = delete;
  tui_box &operator=(const tui_box &) = delete;
  ~tui_box() {}
  static std::shared_ptr<tui_box> create(int x, int y, int w, int h, int screen,
                                         char *watch, char initial,
                                         draw_func draw, loop_func onclick,
                                         loop_func onhover, void *data1,
                                         void *data2);

  int screen() const { return screen_; }

  bool box_contains(int x, int y) const {
    return x >= this->x && x <= this->x + this->w && y >= this->y &&
           y <= this->y + this->h;
  }
};

struct tui_event {
  std::string c;
  func f;
};

class tui {
  class ui_t_impl *impl_ = nullptr;
  std::vector<std::shared_ptr<tui_box>> b;
  std::vector<tui_event> e;
  bool mouse = false;
  int screen;
  int scroll = 0;
  bool canscroll = true;
  int force = 0;

public:
  tui(const tui &) = delete;
  tui &operator=(const tui &) = delete;

  /*
   * Initializes a new UI struct,
   *   puts the terminal into raw
   *   mode, and prints out the
   *   necessary escape codes
   *   for mouse support.
   */
  tui(int s);

  /*
   * Frees the given UI struct,
   *   and takes the terminal
   *   out of raw mode.
   */
  ~tui();

  uint16_t cols() const;
  uint16_t rows() const;
  int center_x(int w) const { return (cols() - w) / 2; }
  int center_y(int h) const { return (rows() - h) / 2; }

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
  void add(int x, int y, int w, int h, char *watch, char initial,
           draw_func draw, loop_func onclick, loop_func onhover, void *data1,
           void *data2);

  /*
   * HELPERS
   */
  void add_text(int x, int y, char *str, loop_func click, loop_func hover);

  int cursor_y(tui_box *b, int n);

  /*
   * Draws a single box to the
   *   screen.
   */
  void draw_one(tui_box *tmp, int flush);

  /*
   * Draws all boxes to the screen.
   */
  void draw();

  /*
   * Forces a redraw of the screen,
   *   updating all boxes' caches.
   */
  void redraw();

  /*
   * Adds a new key event listener
   *   to the UI.
   */
  void on_key(const char *c, func f);

  void mainloop();

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
  void update(std::string_view c);
};
