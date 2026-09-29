#include "obj.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <print>

ObjType obj_type(const Value &value) { return value.as_obj()->type; }

ObjString *allocate_string(std::string str) {
  auto *object = new ObjString{};
  object->type = OBJ_STRING;
  object->str = std::move(str);

  return object;
}

ObjString *as_string(const Value &value) {
  return static_cast<ObjString *>(value.as_obj());
}
std::string &as_cpp_str(const Value &value) {
  return static_cast<ObjString *>(value.as_obj())->str;
}

bool is_obj_type(const Value &value, const ObjType type) {
  return value.is_obj() && value.as_obj()->type == type;
}

void print_object(const Value &value) {
  switch (obj_type(value)) {
  case OBJ_STRING:
    std::print("{}", as_cpp_str(value));
    break;
  }
}
