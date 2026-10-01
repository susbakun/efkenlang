#pragma once

#include "chunk.hpp"
#include "obj.hpp"
#include "value.hpp"
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#define STACK_MAX 256

enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
public:
  VM() = default;

  InterpretResult interpret(const std::string_view source);

  void push(Value value);
  Value pop();
  Value peek(int distance);

  void add_string(std::string key, ObjString *value);
  std::optional<Value> find_string(const std::string &key);

private:
  InterpretResult run();

  std::uint8_t read_byte();
  Value read_constant();
  Value read_constant_long();
  ObjString *read_string();
  void concatenate_two_strings();
  void concatenate_string_and_number();
  ObjString *take_string(std::string str);

  bool is_stack_full() const;

  void runtime_error(const std::string_view format, ...);
  void reset_stack();

  Chunk m_chunk;
  uint8_t *m_ip;
  std::array<Value, STACK_MAX> m_stack{};
  Value *m_sp{m_stack.data()};
  std::unordered_map<std::string, ObjString *> m_strings{};
  std::unordered_map<ObjString *, Value> m_globals{};
};
