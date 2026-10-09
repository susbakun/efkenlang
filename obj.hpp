#pragma once

#include <optional>

#include "chunk.hpp"
#include "value.hpp"

class VM;

enum ObjType { OBJ_STRING, OBJ_FUNCTION, OBJ_CLOSURE, OBJ_UPVALUE, OBJ_NATIVE };

struct Obj {
  ObjType type;
  bool is_marked{false};
  std::size_t size{};  // set at allocation, subtracted when freed

  virtual ~Obj() = default;
};

struct ObjString : public Obj {
  std::string str;
};

struct ObjUpvalue : public Obj {
  Value* location;
  Value closed;
};

struct ObjFunction : public Obj {
  int arity;
  Chunk chunk;
  ObjString* name;
  int upvalue_count{};
};

struct ObjClosure : public Obj {
  ObjFunction* function;
  std::vector<ObjUpvalue*> upvalues;
  int upvalue_count;
};

using NativeFn = std::optional<Value> (VM::*)(int arg_count, Value* args);

struct ObjNative : public Obj {
  NativeFn function;
  int arity;
};

ObjType obj_type(const Value& value);

ObjString* allocate_string(VM& vm, std::string str);
ObjFunction* new_function(VM& vm);
ObjClosure* new_closure(VM& vm, ObjFunction* function);
ObjNative* new_native(VM& vm, NativeFn function, int arity);
ObjUpvalue* new_upvalue(VM& vm, Value& slot);

ObjString* as_string(const Value& value);
std::string& as_cpp_str(const Value& value);
ObjFunction* as_function(const Value& value);
ObjNative* as_native(const Value& value);
ObjClosure* as_closure(const Value& value);

bool is_obj_type(const Value& value, const ObjType type);

void print_object(const Value& value);
