/*
 * tui.h: simple tui library
 */
#pragma once
#include <stdint.h>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

/*
 * TYPES
 */
typedef void (*func)();
using draw_func = std::function<std::string(struct tui_box *)>;
using loop_func = std::function<void(struct tui_box *, int, int, int)>;

struct tui_rect {
  int x, y;
  int w, h;

  bool contains(int x, int y) const {
    return x >= this->x && x <= this->x + this->w && y >= this->y &&
           y <= this->y + this->h;
  }
};

struct tui_box {
  tui_rect rect_;
  int screen_;
  std::string cache;
  draw_func draw;
  loop_func onhover;
  tui_box() {}

public:
  loop_func onclick;
  tui_box(const tui_box &) = delete;
  tui_box &operator=(const tui_box &) = delete;
  ~tui_box() {}
  static std::shared_ptr<tui_box> create(const tui_rect &rect, int screen,
                                         draw_func draw, loop_func onclick,
                                         loop_func onhover);

  int screen() const { return screen_; }
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
  tui_rect get_center(int w, int h) const {
    return {this->center_x(w), this->center_y(h), w, h};
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
  void add(const tui_rect &rect, draw_func draw, loop_func onclick,
           loop_func onhover);

  /*
   * HELPERS
   */
  void add_text(int x, int y, std::string_view str, loop_func click,
                loop_func hover);

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
