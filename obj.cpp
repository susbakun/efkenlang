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
  if (auto existing{vm.find_string(str)}) return as_string(*existing);

  auto* object{vm.allocate_object<ObjString>(OBJ_STRING)};
  object->str = str;  // copy str here

  // then move it
  vm.add_string(std::move(str), object);

  return object;
}

ObjFunction* new_function(VM& vm) {
  auto* object{vm.allocate_object<ObjFunction>(OBJ_FUNCTION)};
  object->type = OBJ_FUNCTION;
  object->arity = 0;
  object->name = nullptr;
  object->chunk = Chunk{};

  return object;
}

ObjClosure* new_closure(VM& vm, ObjFunction* function) {
  ObjClosure* object{vm.allocate_object<ObjClosure>(OBJ_CLOSURE)};
  object->type = OBJ_CLOSURE;
  object->function = function;
  object->upvalues.assign(function->upvalue_count, nullptr);
  object->upvalue_count = function->upvalue_count;

  return object;
}

ObjNative* new_native(VM& vm, NativeFn function, int arity) {
  ObjNative* object{vm.allocate_object<ObjNative>(OBJ_NATIVE)};
  object->type = OBJ_NATIVE;
  object->function = function;
  object->arity = arity;

  return object;
}

ObjUpvalue* new_upvalue(VM& vm, Value& slot) {
  ObjUpvalue* object{vm.allocate_object<ObjUpvalue>(OBJ_UPVALUE)};
  object->type = OBJ_UPVALUE;
  object->location = &slot;
  object->closed = Value{};

  return object;
}

ObjClass* new_class(VM& vm, ObjString* name) {
  ObjClass* object{vm.allocate_object<ObjClass>(OBJ_CLASS)};
  object->type = OBJ_CLASS;
  object->name = name;

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

ObjClass* as_class(const Value& value) {
  return static_cast<ObjClass*>(value.as_obj());
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

    case OBJ_CLOSURE: {
      auto function{as_closure(value)->function};
      return std::format(
          "{}", function->name != nullptr ? function->name->str : "<script>");
    }

    case OBJ_UPVALUE:
      return std::format("upvalue");

    case OBJ_NATIVE:
      return std::format("<native fn>");

    case OBJ_CLASS:
      return std::format("{}", as_class(value)->name->str);
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
    case OBJ_CLOSURE: {
      auto function{as_closure(value)->function};
      std::print("{}",
                 function->name != nullptr ? function->name->str : "<script>");
      break;
    }

    case OBJ_UPVALUE:
      std::print("upvalue");
      break;

    case OBJ_NATIVE:
      std::print("<native fn>");
      break;

    case OBJ_CLASS:
      std::print("{}", as_class(value)->name->str);
      break;
  }
}
