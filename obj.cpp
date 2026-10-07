#include "obj.hpp"

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <print>

#include "chunk.hpp"
#include "value.hpp"
#include "vm.hpp"

ObjType obj_type(const Value& value) { return value.as_obj()->type; }

ObjString* allocate_string(VM& vm, std::string str) {
  auto string{vm.find_string(str)};
  if (string) return as_string(string.value());

  auto* object{new ObjString{}};
  object->type = OBJ_STRING;
  object->str = str;  // copy str here

  // then move it
  vm.add_string(std::move(str), object);

  return object;
}

ObjFunction* new_function() {
  auto* object{new ObjFunction{}};
  object->type = OBJ_FUNCTION;
  object->arity = 0;
  object->name = nullptr;
  object->chunk = Chunk{};

  return object;
}

ObjNative* new_native(NativeFn function, int arity) {
  ObjNative* object{new ObjNative{}};
  object->type = OBJ_NATIVE;
  object->function = function;
  object->arity = arity;

  return object;
}

ObjClosure* new_closure(ObjFunction* function) {
  ObjClosure* object{new ObjClosure{}};
  object->type = OBJ_CLOSURE;
  object->function = function;

  return object;
}

ObjString* as_string(const Value& value) {
  return static_cast<ObjString*>(value.as_obj());
}
std::string& as_cpp_str(const Value& value) {
  return static_cast<ObjString*>(value.as_obj())->str;
}

ObjFunction* as_function(const Value& value) {
  return static_cast<ObjFunction*>(value.as_obj());
}

ObjNative* as_native(const Value& value) {
  return static_cast<ObjNative*>(value.as_obj());
}

ObjClosure* as_closure(const Value& value) {
  return static_cast<ObjClosure*>(value.as_obj());
}

bool is_obj_type(const Value& value, const ObjType type) {
  return value.is_obj() && value.as_obj()->type == type;
}

std::string object_to_string(const Value& value) {
  switch (obj_type(value)) {
    case OBJ_STRING:
      return std::format("{}", as_cpp_str(value));
    case OBJ_FUNCTION: {
      auto function{as_function(value)};
      return std::format(
          "{}", function->name != nullptr ? function->name->str : "<script>");
    }
    case OBJ_NATIVE: {
      return std::format("<native fn>");
      break;
    }

    case OBJ_CLOSURE: {
      auto function{as_closure(value)->function};
      return std::format(
          "{}", function->name != nullptr ? function->name->str : "<script>");
    }
  }
}

void print_object(const Value& value) {
  switch (obj_type(value)) {
    case OBJ_STRING:
      std::print("{}", as_cpp_str(value));
      break;
    case OBJ_FUNCTION: {
      auto function{as_function(value)};
      std::print("{}",
                 function->name != nullptr ? function->name->str : "<script>");
      break;
    }
    case OBJ_NATIVE:
      std::print("<native fn>");
      break;
    case OBJ_CLOSURE: {
      auto function{as_closure(value)->function};
      return std::print(
          "{}", function->name != nullptr ? function->name->str : "<script>");
      break;
    }
  }
}
