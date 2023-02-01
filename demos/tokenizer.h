#pragma once
#include <string_view>

class Tokenizer {
  std::string_view m_src;
  char m_delimiter;
  size_t m_head = 0;
  size_t m_end = 0;

public:
  Tokenizer(std::string_view src, char delimiter)
      : m_src(src), m_delimiter(delimiter) {}

  bool next() {
    if (m_end >= m_src.size()) {
      return false;
    }

    if (m_end) {
      ++m_end;
    }
    m_head = m_end;
    for (; m_end < m_src.size(); ++m_end) {
      if (m_src[m_end] == m_delimiter) {
        break;
      }
    }
    return true;
  }
  std::string_view current() const {
    return m_src.substr(m_head, m_end - m_head);
  }
};
