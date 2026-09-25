#pragma once

#include "chunk.hpp"

enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
};

class VM {
public:
  VM() = default;
  InterpretResult interpret(Chunk &chunk);

private:
  InterpretResult run();

  std::uint8_t read_byte();
  Value read_constant();

  Chunk m_chunk;
  uint8_t *m_ip;
};
