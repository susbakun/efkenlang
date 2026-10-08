#pragma once

#include <optional>

#include "chunk.hpp"
#include "value.hpp"

class VM;

enum ObjType { OBJ_STRING, OBJ_FUNCTION, OBJ_CLOSURE, OBJ_NATIVE };

struct Obj {
  ObjType type;

  virtual ~Obj() = default;
};

struct ObjString : public Obj {
  std::string str;
};

struct ObjFunction : public Obj {
  int arity;
  Chunk chunk;
  ObjString* name;
  int upvalue_count{};
};

using NativeFn = std::optional<Value> (VM::*)(int arg_count, Value* args);

struct ObjNative : public Obj {
  NativeFn function;
  int arity;
};

struct ObjClosure : public Obj {
  ObjFunction* function;
};

ObjType obj_type(const Value& value);

ObjString* allocate_string(VM& vm, std::string str);
ObjFunction* new_function();
ObjNative* new_native(NativeFn function, int arity);
ObjClosure* new_closure(ObjFunction* function);

ObjString* as_string(const Value& value);
std::string& as_cpp_str(const Value& value);
ObjFunction* as_function(const Value& value);
ObjNative* as_native(const Value& value);
ObjClosure* as_closure(const Value& value);

bool is_obj_type(const Value& value, const ObjType type);

void print_object(const Value& value);
