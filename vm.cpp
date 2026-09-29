#include "vm.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "value.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <print>

#define DEBUG_TRACE_EXECUTION

#define BINARY_OP(value_type, op)                                              \
  do {                                                                         \
    if (!is_number(peek(0)) || !is_number(peek(1))) {                          \
      runtime_error("Operands must be numbers.");                              \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b{as_number(pop())};                                                \
    double a{as_number(pop())};                                                \
    push(value_type(a op b));                                                  \
  } while (false)

#define COMMA_OP()                                                             \
  do {                                                                         \
    auto b{pop()};                                                             \
    pop();                                                                     \
    push(b);                                                                   \
  } while (false)

InterpretResult VM::interpret(const std::string_view source) {
  Chunk chunk{};
  Compiler compiler{source, chunk};

  if (!compiler.compile()) {
    return INTERPRET_COMPILE_ERROR;
  }

  m_chunk = chunk;
  m_ip = &m_chunk.get_code_ref(0);

  auto result{run()};

  return result;
}

InterpretResult VM::run() {
  while (true) {
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
    case OP_FALSE:
      push(bool_val(false));
      break;
    case OP_TRUE:
      push(bool_val(true));
      break;
    case OP_NIL:
      push(nil_val());
      break;

    case OP_NOT:
      if (!is_bool(peek(0))) {
        runtime_error("Operand must be a boolean");
        return INTERPRET_RUNTIME_ERROR;
      }
      m_sp[-1] = bool_val(!as_boolean(m_sp[-1]));
      break;

    case OP_NEGATE:
      // in place
      if (!is_number(peek(0))) {
        runtime_error("Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }
      m_sp[-1] = number_val(-as_number(m_sp[-1]));
      break;

    // binary
    case OP_EQUAL: {
      auto v1{pop()};
      auto v2{pop()};
      push(bool_val(is_equal(v1, v2)));
      break;
    }

    case OP_GREATER:
      BINARY_OP(number_val, >);
      break;
    case OP_LESS:
      BINARY_OP(number_val, <);
      break;

    case OP_ADD:
      BINARY_OP(number_val, +);
      break;
    case OP_SUBTRACT:
      BINARY_OP(number_val, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(number_val, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(number_val, /);
      break;
    case OP_COMMA:
      COMMA_OP();
      break;

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
  if (is_stack_full())
    throw "Stack overflow";

  *m_sp = value;
  m_sp++;
}

Value VM::pop() {
  m_sp--;
  return *m_sp;
}

Value VM::peek(int distance) { return m_sp[-1 - distance]; }

bool VM::is_stack_full() const {
  return m_sp == (m_stack.data() + m_stack.size());
}

void VM::runtime_error(const std::string_view format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format.data(), args);
  va_end(args);

  auto instruction{
      static_cast<std::size_t>(m_ip - &m_chunk.get_code_ref(0) - 1)};

  int line{m_chunk.get_lines(instruction).line};
  std::println("[line {}] in script", line);
  reset_stack();
}

void VM::reset_stack() { m_sp = m_stack.data(); }
