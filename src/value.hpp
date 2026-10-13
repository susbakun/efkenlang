#pragma once

#include <cstddef>
#include <vector>

#include "common.hpp"

#ifdef NAN_BOXING

struct Obj;

class Value {
 public:
  constexpr Value() : m_bits{NIL} {}
  constexpr Value(bool value) : m_bits{value ? TRUE : FALSE} {}
  constexpr Value(double value) : m_bits{std::bit_cast<std::uint64_t>(value)} {}
  Value(Obj* value)
      : m_bits{OBJ_TAG | reinterpret_cast<std::uint64_t>(value)} {}
  Value(const void*) =
      delete;  // a stray pointer must not silently become `true`
  constexpr Value(const Value&) = default;
  constexpr Value& operator=(const Value&) = default;

  bool as_boolean() const;
  double as_number() const;
  Obj* as_obj() const;

  bool is_nil() const;
  bool is_bool() const;
  bool is_number() const;
  bool is_obj() const;

  bool is_equal(const Value& v2) const;

  std::string number_to_string() const;

  std::string_view value_type() const;
  void print_value() const;

 private:
  static constexpr std::uint64_t SIGN_BIT{0x8000000000000000};
  static constexpr std::uint64_t QNAN{0x7ffc000000000000};
  static constexpr std::uint64_t OBJ_TAG{SIGN_BIT | QNAN};
  static constexpr std::uint64_t NIL{QNAN | 1};
  static constexpr std::uint64_t FALSE{QNAN | 2};
  static constexpr std::uint64_t TRUE{QNAN | 3};

  std::uint64_t m_bits;
};

#else

enum ValueType { VAL_BOOL, VAL_NIL, VAL_NUMBER, VAL_OBJ };

class Value {
 public:
  Value() : m_type{VAL_NIL}, as{.number = 0} {};
  Value(const bool value) : m_type{VAL_BOOL}, as{.boolean = value} {};
  Value(const double value) : m_type{VAL_NUMBER}, as{.number = value} {};
  Value(Obj* value) : m_type{VAL_OBJ}, as{.obj = value} {};
  Value(const Value& value) : m_type{value.m_type}, as{value.as} {}

  Value& operator=(const Value& value) = default;

  bool as_boolean() const;
  double as_number() const;
  Obj* as_obj() const;

  bool is_bool() const;
  bool is_nil() const;
  bool is_number() const;
  bool is_obj() const;
  bool is_equal(const Value& b) const;

  std::string number_to_string() const;

  std::string_view value_type() const;
  void print_value() const;

 private:
  ValueType m_type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as;
};

#endif
class ValueArray {
 public:
  ValueArray() = default;

  void write_value(const Value& value);

  std::vector<Value> m_values{};
};
