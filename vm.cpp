#include "vm.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include <cstddef>
#include <cstdint>
#include <print>

#define DEBUG_TRACE_EXECUTION

InterpretResult VM::interpret(Chunk &chunk) {
  m_chunk = chunk;
  m_ip = &m_chunk.get_code_ref(0);
  return run();
}

InterpretResult VM::run() {
  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    disassemble_instruction(
        m_chunk, static_cast<std::size_t>(m_ip - &m_chunk.get_code_ref(0)));
#endif

    std::uint8_t instruction{read_byte()};

    switch (instruction) {
    case OP_CONSTANT: {
      auto constant{read_constant()};
      print_value(constant);
      std::println();
      break;
    }

    case OP_RETURN:
      return INTERPRET_OK;
    }
  }
}

std::uint8_t VM::read_byte() { return *m_ip++; }

Value VM::read_constant() { return m_chunk.get_constant(read_byte()); }
