#include "value.hpp"
#include "obj.hpp"
#include <cstddef>
#include <cstring>
#include <print>

bool Value::as_boolean() const { return as.boolean; }
double Value::as_number() const { return as.number; }
Obj *Value::as_obj() const { return as.obj; }

bool Value::is_bool() const { return m_type == VAL_BOOL; }
bool Value::is_nil() const { return m_type == VAL_NIL; }
bool Value::is_number() const { return m_type == VAL_NUMBER; }
bool Value::is_obj() const { return m_type == VAL_OBJ; }

bool Value::is_equal(const Value &v2) const {
  if (m_type != v2.m_type)
    return false;

  switch (m_type) {
  case VAL_BOOL:
    return this->as_boolean() == v2.as_boolean();
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return this->as_number() == v2.as_number();
  case VAL_OBJ: {
    ObjString *a_string{as_string(*this)};
    ObjString *b_string{as_string(*this)};
    return a_string->str == b_string->str;
  }
  }
}

void Value::print_value() const {
  switch (m_type) {
  case VAL_BOOL:
    std::print("{}", this->as_boolean() ? "true" : "false");
    break;
  case VAL_NIL:
    std::print("nil");
    break;
  case VAL_NUMBER:
    std::print("{}", this->as_number());
    break;
  case VAL_OBJ:
    print_object(*this);
    break;
  }
}

void ValueArray::write_value(const Value &value) { m_values.push_back(value); }
