#pragma once

#include <cstddef>
#include <vector>

struct Obj;

enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ };

class Value {
public:
  Value() : m_type{VAL_NIL}, as{.number = 0} {};
  Value(const bool value) : m_type{VAL_BOOL}, as{.boolean = value} {};
  Value(const double value) : m_type{VAL_NUMBER}, as{.number = value} {};
  Value(Obj *value) : m_type{VAL_OBJ}, as{.obj = value} {};
  Value(const Value &value) : m_type{value.m_type}, as{value.as} {}

  Value &operator=(const Value &value) = default;

  bool as_boolean() const;
  double as_number() const;
  Obj *as_obj() const;

  bool is_bool() const;
  bool is_nil() const;
  bool is_number() const;
  bool is_obj() const;

  bool is_equal(const Value &b) const;

  void print_value() const;

private:
  ValueType m_type;
  union {
    bool boolean;
    double number;
    Obj *obj;
  } as;
};

class ValueArray {
public:
  ValueArray() = default;

  void write_value(const Value &value);

  std::vector<Value> m_values{};
};
