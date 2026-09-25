#include "vm.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include "value.hpp"
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
    std::print("          ");
    for (Value *slot{m_stack.data()}; slot < m_sp; slot++) {
      std::print("[ ");
      print_value(*slot);
      std::print(" ]");
    }
    std::println();

    disassemble_instruction(
        m_chunk, static_cast<std::size_t>(m_ip - &m_chunk.get_code_ref(0)));
#endif

    std::uint8_t instruction{read_byte()};

    switch (instruction) {
    case OP_CONSTANT: {
      auto constant{read_constant()};
      push(constant);
      break;
    }
    case OP_CONSTANT_LONG: {
      auto constant{read_constant_long()};
      push(constant);
      break;
    }

    case OP_RETURN:
      print_value(pop());
      std::println();
      return INTERPRET_OK;
    }
  }
}

std::uint8_t VM::read_byte() { return *m_ip++; }

Value VM::read_constant() { return m_chunk.get_constant(read_byte()); }

Value VM::read_constant_long() {
  auto ind_first_byte{read_byte()};
  auto ind_second_byte{read_byte()};
  auto ind_third_byte{read_byte()};

  auto ind{(ind_first_byte << 16) + (ind_second_byte << 8) + ind_third_byte};

  return m_chunk.get_constant(ind);
}

void VM::push(Value value) {
  *m_sp = value;
  m_sp++;
}

Value VM::pop() {
  m_sp--;
  return *m_sp;
}
