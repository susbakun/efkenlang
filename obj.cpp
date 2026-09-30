#include "obj.hpp"
#include "value.hpp"
#include "vm.hpp"
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <print>

ObjType obj_type(const Value &value) { return value.as_obj()->type; }

ObjString *allocate_string(VM &vm, std::string str) {
  auto string{vm.find_string(str)};
  if (string)
    return as_string(string.value());

  auto *object = new ObjString{};
  object->type = OBJ_STRING;
  object->str = str; // copy str here

  // then move it
  vm.add_string(std::move(str), object);

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

std::string object_to_string(const Value &value) {
  switch (obj_type(value)) {
  case OBJ_STRING:
    return std::format("{}", as_cpp_str(value));
  }
}

void print_object(const Value &value) {
  switch (obj_type(value)) {
  case OBJ_STRING:
    std::print("{}", as_cpp_str(value));
    break;
  }
}
