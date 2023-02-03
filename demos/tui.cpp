#include "tui.h"
#include "tokenizer.h"
#include <optional>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
std::shared_ptr<tui_box> tui_box::create(const tui_rect &rect, int screen,
                                         draw_func draw, loop_func onclick,
                                         loop_func onhover) {

  auto b = std::shared_ptr<tui_box>(new tui_box);
  b->rect_ = rect;
  b->screen_ = screen;
  // b->watch = watch;
  // b->last = initial;
  b->draw = draw;
  b->onclick = onclick;
  b->onhover = onhover;
  // b->data2 = data2;
  b->cache = draw(b.get());
  return b;
}

#if _WIN32
#include <Windows.h>

class ui_t_impl {
  HANDLE hStdin_ = nullptr;
  DWORD fdwSaveOldMode_ = {};
  uint16_t cols_ = 0;
  uint16_t rows_ = 0;

public:
  ui_t_impl() {
    // Get the standard input handle.
    hStdin_ = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin_ == INVALID_HANDLE_VALUE) {
      throw std::runtime_error("GetStdHandle");
    }

    // Save the current input mode, to be restored on exit.
    if (!GetConsoleMode(hStdin_, &fdwSaveOldMode_)) {
      throw std::runtime_error("GetConsoleMode");
    }

    // Enable the window and mouse input events.
    auto fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT |
                   ENABLE_VIRTUAL_TERMINAL_INPUT;
    if (!SetConsoleMode(hStdin_, fdwMode)) {
      throw std::runtime_error("SetConsoleMode");
    }

    CONSOLE_SCREEN_BUFFER_INFO info;

    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
      throw std::runtime_error("GetConsoleScreenBufferInfo");
    }
    cols_ = info.dwSize.X;
    rows_ = info.dwSize.Y;
  }

  ~ui_t_impl() {
    // Restore input mode on exit.
    SetConsoleMode(hStdin_, fdwSaveOldMode_);
  }
  bool Contains(int x, int y) const {
    return false;
    return x > 0 && x < this->Cols() && y > 0 && y < this->Rows();
  }
  uint16_t Cols() const { return cols_; }
  uint16_t Rows() const { return rows_; }

  std::string str_;

  std::optional<std::string_view> Read() {
    str_.clear();
    INPUT_RECORD irInBuf[128];
    DWORD cNumRead = 0;

    // use cache
    // Wait for the events.
    if (!ReadConsoleInput(hStdin_,      // input buffer handle
                          irInBuf,      // buffer to read into
                          128,          // size of read buffer
                          &cNumRead)) { // number of records read
      throw std::runtime_error("ReadConsoleInput");
    }

    for (int i = 0; i < cNumRead; ++i) {
      auto &ir = irInBuf[i];

      // Dispatch the events to the appropriate handler.
      switch (ir.EventType) {
      case KEY_EVENT: // keyboard input
      {
        KEY_EVENT_RECORD &key = ir.Event.KeyEvent;
        if (key.bKeyDown) {
          if (key.uChar.AsciiChar) {
            str_.push_back(key.uChar.AsciiChar);
          }
        }
        // KeyEventProc(irInBuf[i].Event.KeyEvent);
      } break;

      case MOUSE_EVENT: // mouse input
      {
        // MouseEventProc(irInBuf[i].Event.MouseEvent);
      } break;

      case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
      {
        // ResizeEventProc(irInBuf[i].Event.WindowBufferSizeEvent);
      } break;

      case FOCUS_EVENT: // disregard focus events
      case MENU_EVENT:  // disregard menu events
        break;

      default:
        throw std::runtime_error("Unknown event type");
        break;
      }
    }

    return str_;
  }
};
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

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

  char buf[64];
  std::optional<std::string_view> Read() {
    auto n = read(STDIN_FILENO, buf, sizeof(buf));
    return std::string_view(buf, buf + n);
  }
};
#endif

/*
 * Initializes a new UI struct,
 *   puts the terminal into raw
 *   mode, and prints out the
 *   necessary escape codes
 *   for mouse support.
 */
tui::tui(int s) : impl_(new ui_t_impl), screen_(s) {}

/*
 * Frees the given UI struct,
 *   and takes the terminal
 *   out of raw mode.
 */
tui::~tui() {
  delete impl_;
  auto term = getenv("TERM");
  if (strncmp(term, "screen", 6) == 0 || strncmp(term, "tmux", 4) == 0) {
    printf("Note: Terminal multiplexer detected.\n  For best performance (i.e. "
           "reduced flickering), running natively inside\n  a GPU-accelerated "
           "terminal such as alacritty or kitty is recommended.\n");
  }
}

uint16_t tui::cols() const { return impl_->Cols(); }
uint16_t tui::rows() const { return impl_->Rows(); }

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
void tui::add(const tui_rect &rect, draw_func draw, loop_func onclick,
              loop_func onhover) {

  auto box = tui_box::create(rect, screen_, draw, onclick, onhover);
  this->boxes_.push_back(box);
}

void tui::add_text(int x, int y, std::string_view str, loop_func click,
                   loop_func hover) {
  std::string buf(str);
  draw_func callback = [buf](tui_box *b) { return buf; };
  this->add({x, y, (int)str.size(), 1}, callback, click, hover);
}

int tui::cursor_y(tui_box *b, int n) {
  return (b->rect_.y + (n + 1) + (canscroll_ ? scroll_ : 0));
}

/*
 * Draws a single box to the
 *   screen.
 */
void tui::draw_one(tui_box *tmp, int flush) {

  if (tmp->screen() != this->screen_) {
    return;
  }

  std::string buf;
  if (this->force_) {
    buf = tmp->draw(tmp);
    // if (tmp->watch != NULL)
    //   tmp->last = *(tmp->watch);
    tmp->cache = buf;
  } else {
    // buf is allocated proportionally to tmp->cache, so strcpy is safe
    buf = tmp->cache;
  }

  auto tok = strtok((char *)buf.data(), "\n");
  int n = -1;
  while (tok) {
    if (impl_->Contains(tmp->rect_.x, cursor_y(tmp, n))) {
      printf("\x1b[%i;%iH%s", cursor_y(tmp, n), tmp->rect_.x, tok);
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
void tui::draw() {
  printf("\x1b[0m\x1b[2J");
  for (auto &tmp : this->boxes_) {
    this->draw_one(tmp.get(), 0);
  }
  fflush(stdout);
  this->force_ = 0;
}

/*
 * Forces a redraw of the screen,
 *   updating all boxes' caches.
 */
void tui::redraw() {
  this->force_ = 1;
  draw();
}

/*
 * Adds a new key event listener
 *   to the UI.
 */
void tui::on_key(const char *c, func f) {
  this->events_.push_back({
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
void tui::update(std::string_view c) {
  if (c.starts_with("\x1b[<")) {
    Tokenizer tok(c.substr(3), ';');
    tok.next();

    switch (tok.current()[0]) {
    case '0':
      this->mouse_ = (c.find('m') != std::string_view::npos);
      if (this->mouse_) {
        tok.next();
        int x = atoi(tok.current().data());
        tok.next();
        int y = strtol(tok.current().data(), NULL, 10) -
                (this->canscroll_ ? this->scroll_ : 0);
        for (auto &tmp : this->boxes_) {
          if (tmp->screen() == this->screen_ && tmp->rect_.contains(x, y)) {
            if (tmp->onclick) {
              tmp->onclick(tmp.get(), x, y, this->mouse_);
            }
          }
        }
      }
      break;

    case '3': {
      this->mouse_ = tok.current() == "32";
      tok.next();
      int x = atoi(tok.current().data());
      tok.next();
      int y = strtol(tok.current().data(), NULL, 10) -
              (this->canscroll_ ? this->scroll_ : 0);
      for (auto &tmp : this->boxes_) {
        if (tmp->screen() == this->screen_ && tmp->rect_.contains(x, y)) {
          if (tmp->onhover) {
            tmp->onhover(tmp.get(), x, y, this->mouse_);
          }
        }
      }
    } break;

    case '6':
      if (this->canscroll_) {
        this->scroll_ += (4 * (tok.current()[1] == '4')) - 2;
        printf("\x1b[0m\x1b[2J");
        this->draw();
      }
      break;
    }
  }

  for (auto &evt : this->events_) {
    if (c.starts_with(evt.c)) {
      evt.f();
    }
  }
}

void tui::mainloop() {
  while (auto buf = impl_->Read()) {
    this->update(*buf);
  }
}
