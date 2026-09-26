#pragma once

#include <string_view>
class Scanner {
public:
  Scanner(const std::string_view source)
      : m_start{source.data()}, m_current{source.data()}, m_line{1} {}

private:
  const char *m_start;
  const char *m_current;
  int m_line;
};
