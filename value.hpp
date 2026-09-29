#pragma once

#include <cstddef>
#include <vector>

enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER };

struct Value {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as;
};

Value bool_val(bool value);
Value nil_val();
Value number_val(double value);

bool as_boolean(Value value);
double as_number(Value value);

bool is_bool(Value value);
bool is_nil(Value value);
bool is_number(Value value);

bool is_equal(Value v1, Value v2);

void print_value(Value value);

class ValueArray {
public:
  ValueArray() = default;

  void write_value(Value value);

  std::vector<Value> m_values{};
};
