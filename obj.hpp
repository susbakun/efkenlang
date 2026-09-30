#pragma once

#include "value.hpp"

class VM;

enum ObjType { OBJ_STRING };

struct Obj {
  ObjType type;
  Obj *next{};

  virtual ~Obj() = default;
};

struct ObjString : public Obj {
  std::string str;
};

ObjType obj_type(const Value &value);

ObjString *allocate_string(VM &vm, std::string str);

ObjString *as_string(const Value &value);
std::string &as_cpp_str(const Value &value);

bool is_obj_type(const Value &value, const ObjType type);

void print_object(const Value &value);
