#pragma once

#include <vector>
using Value = double;

struct ValueArray {
  std::vector<Value> values{};
};

void write_value(ValueArray &va, Value value);
