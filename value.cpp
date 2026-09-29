#include "value.hpp"
#include <cstddef>
#include <print>

Value bool_val(bool value) { return Value{VAL_BOOL, {.boolean = value}}; }
Value nil_val() { return Value{VAL_NIL, {.number = 0}}; }
Value number_val(double value) { return Value{VAL_NUMBER, {.number = value}}; }

bool as_boolean(Value value) { return value.as.boolean; }
double as_number(Value value) { return value.as.number; }

bool is_bool(Value value) { return value.type == VAL_BOOL; }
bool is_nil(Value value) { return value.type == VAL_NIL; }
bool is_number(Value value) { return value.type == VAL_NUMBER; }

void print_value(Value value) {
  switch (value.type) {
  case VAL_BOOL:
    std::println("{}", as_boolean(value) ? "true" : "false");
    break;
  case VAL_NIL:
    std::println("nil");
    break;
  case VAL_NUMBER:
    std::println("{}", as_number(value));
    break;
  }
}

void ValueArray::write_value(Value value) { m_values.push_back(value); }
