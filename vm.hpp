#pragma once

#include "chunk.hpp"
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

private:
  InterpretResult run();

  std::uint8_t read_byte();
  Value read_constant();
  Value read_constant_long();

  bool is_stack_full() const;

  Chunk m_chunk;
  uint8_t *m_ip;
  std::array<Value, STACK_MAX> m_stack{};
  Value *m_sp{m_stack.data()};
};
