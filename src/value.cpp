#include "value.hpp"

#include <cstddef>
#include <cstring>
#include <format>
#include <print>
#include <utility>

#include "obj.hpp"

#ifdef NAN_BOXING

bool Value::as_boolean() const { return m_bits == TRUE; }
double Value::as_number() const { return std::bit_cast<double>(m_bits); }
Obj* Value::as_obj() const { return reinterpret_cast<Obj*>(m_bits & ~OBJ_TAG); }

bool Value::is_nil() const { return m_bits == NIL; }
bool Value::is_bool() const { return (m_bits == TRUE) || (m_bits == FALSE); }
bool Value::is_number() const { return (QNAN & m_bits) != QNAN; }
bool Value::is_obj() const { return (m_bits & OBJ_TAG) == OBJ_TAG; }

bool Value::is_equal(const Value& v2) const {
  if (this->is_number() && v2.is_number()) {
    return this->as_number() == v2.as_number();
  }

  return this->m_bits == v2.m_bits;
}

std::string Value::number_to_string() const {
  if (is_number()) {
    return std::format("{}", as_number());
  }

  return "";
}

std::string_view Value::value_type() const {
  if (is_bool()) {
    return "bool";
  } else if (is_nil()) {
    return "nil";
  } else if (is_number()) {
    return "number";
  } else if (is_obj()) {
    return "obj";
  } else {
    std::unreachable();
  }
}

void Value::print_value() const {
  if (is_bool()) {
    std::print("{}", as_boolean() ? "true" : "false");
  } else if (is_nil()) {
    std::print("nil");
  } else if (is_number()) {
    std::print("{}", as_number());
  } else if (is_obj()) {
    print_object(*this);
  } else {
    std::unreachable();
  }
}

#else

bool Value::as_boolean() const { return as.boolean; }
double Value::as_number() const { return as.number; }
Obj* Value::as_obj() const { return as.obj; }

bool Value::is_bool() const { return m_type == VAL_BOOL; }
bool Value::is_nil() const { return m_type == VAL_NIL; }
bool Value::is_number() const { return m_type == VAL_NUMBER; }
bool Value::is_obj() const { return m_type == VAL_OBJ; }

bool Value::is_equal(const Value& v2) const {
  if (m_type != v2.m_type) return false;

  switch (m_type) {
    case VAL_BOOL:
      return this->as_boolean() == v2.as_boolean();
    case VAL_NIL:
      return true;
    case VAL_NUMBER:
      return this->as_number() == v2.as_number();
    case VAL_OBJ: {
      return this->as_obj() == v2.as_obj();
    }
  }
}

std::string Value::number_to_string() const {
  if (is_number()) {
    return std::format("{}", as_number());
  }

  return "";
}

std::string_view Value::value_type() const {
  switch (m_type) {
    case VAL_BOOL:
      return "bool";
    case VAL_NIL:
      return "nil";
    case VAL_NUMBER:
      return "number";
    case VAL_OBJ:
      return "obj";
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

#endif

void ValueArray::write_value(const Value& value) { m_values.push_back(value); }
