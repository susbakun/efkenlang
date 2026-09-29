#pragma once

#include "chunk.hpp"
#include "obj.hpp"
#include "value.hpp"
#include <array>
#include <string_view>

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

private:
  InterpretResult run();

  std::uint8_t read_byte();
  Value read_constant();
  Value read_constant_long();
  void concatenate();
  ObjString *take_string(std::string str);

  bool is_stack_full() const;

  void runtime_error(const std::string_view format, ...);
  void reset_stack();

  Chunk m_chunk;
  uint8_t *m_ip;
  std::array<Value, STACK_MAX> m_stack{};
  Value *m_sp{m_stack.data()};
};
