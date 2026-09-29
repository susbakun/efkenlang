#include "vm.hpp"
#include "chunk.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "obj.hpp"
#include "value.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <print>
#include <string>

#define DEBUG_TRACE_EXECUTION

#define BINARY_OP(op)                                                          \
  do {                                                                         \
    if (!peek(0).is_number() || !peek(1).is_number()) {                        \
      runtime_error("Operands must be numbers.");                              \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
    double b{pop().as_number()};                                               \
    double a{pop().as_number()};                                               \
    push(Value{a op b});                                                       \
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
      slot->print_value();
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
      push(Value{false});
      break;
    case OP_TRUE:
      push(Value{true});
      break;
    case OP_NIL:
      push(Value{});
      break;

    case OP_NOT:
      if (!peek(0).is_bool()) {
        runtime_error("Operand must be a boolean");
        return INTERPRET_RUNTIME_ERROR;
      }
      m_sp[-1] = Value{!m_sp[-1].as_boolean()};
      break;

    case OP_NEGATE:
      // in place
      if (!peek(0).is_number()) {
        runtime_error("Operand must be a number");
        return INTERPRET_RUNTIME_ERROR;
      }
      m_sp[-1] = Value{-m_sp[-1].as_number()};
      break;

    // binary
    case OP_EQUAL: {
      auto v1{pop()};
      auto v2{pop()};
      push(Value{v1.is_equal(v2)});
      break;
    }

    case OP_GREATER:
      BINARY_OP(>);
      break;
    case OP_LESS:
      BINARY_OP(<);
      break;

    case OP_ADD:
      if (is_obj_type(peek(0), OBJ_STRING) &&
          is_obj_type(peek(1), OBJ_STRING)) {
        concatenate_two_strings();
      } else if (is_obj_type(peek(0), OBJ_STRING) ||
                 is_obj_type(peek(1), OBJ_STRING)) {
        concatenate_string_and_number();
      } else if (peek(0).is_number() && peek(1).is_number()) {
        double b{pop().as_number()};
        double a{pop().as_number()};
        push(Value{a + b});
      } else {
        runtime_error("Operands must be two numbers or two string ");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    case OP_SUBTRACT:
      BINARY_OP(-);
      break;
    case OP_MULTIPLY:
      BINARY_OP(*);
      break;
    case OP_DIVIDE:
      BINARY_OP(/);
      break;
    case OP_COMMA:
      COMMA_OP();
      break;

    case OP_RETURN:
      pop().print_value();
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

void VM::concatenate_two_strings() {
  ObjString *b{as_string(pop())};
  ObjString *a{as_string(pop())};

  ObjString *result{take_string(a->str + b->str)};
  push(Value{result});
}

void VM::concatenate_string_and_number() {
  auto b{pop()};
  auto a{pop()};

  std::string concatenate{};
  if (a.is_number()) {
    concatenate = (a.number_to_string() + as_string(b)->str);
  } else if (b.is_number()) {
    concatenate = (as_string(a)->str + b.number_to_string());
  }

  ObjString *result{take_string(std::move(concatenate))};
  push(Value{result});
}

ObjString *VM::take_string(std::string str) {
  return allocate_string(std::move(str));
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
