#pragma once

#include <vector>
using Value = double;

class ValueArray {
public:
  ValueArray() = default;

  void write_value(Value value);

  std::vector<Value> m_values{};
};
