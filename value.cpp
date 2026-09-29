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

bool is_equal(Value v1, Value v2) {
  if (v1.type != v2.type)
    return false;

  switch (v1.type) {
  case VAL_BOOL:
    return as_boolean(v1) == as_boolean(v2);
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return as_number(v1) == as_number(v2);
  }
}

void print_value(Value value) {
  switch (value.type) {
  case VAL_BOOL:
    std::print("{}", as_boolean(value) ? "true" : "false");
    break;
  case VAL_NIL:
    std::print("nil");
    break;
  case VAL_NUMBER:
    std::print("{}", as_number(value));
    break;
  }
}

void ValueArray::write_value(Value value) { m_values.push_back(value); }
