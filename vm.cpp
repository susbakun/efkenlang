#include "vm.hpp"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <print>
#include <string>
#include <string_view>

#include "chunk.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "obj.hpp"
#include "scanner.hpp"
#include "value.hpp"

#define DEBUG_TRACE_EXECUTION

#define BINARY_OP(op)                                   \
  do {                                                  \
    if (!peek(0).is_number() || !peek(1).is_number()) { \
      runtime_error("Operands must be numbers.");       \
      return INTERPRET_RUNTIME_ERROR;                   \
    }                                                   \
    double b{pop().as_number()};                        \
    double a{pop().as_number()};                        \
    push(Value{a op b});                                \
  } while (false)

#define COMMA_OP() \
  do {             \
    auto b{pop()}; \
    pop();         \
    push(b);       \
  } while (false)

InterpretResult VM::interpret(const std::string_view source) {
  Scanner scanner{source};
  Parser parser{};

  Compiler compiler{*this, scanner, parser, TYPE_SCRIPT};
  auto function{compiler.compile()};

  if (function == nullptr) return INTERPRET_COMPILE_ERROR;

  push(Value{function});
  call(function, 0);

  return run();
}

InterpretResult VM::run() {
  while (true) {
#ifdef DEBUG_TRACE_EXECUTION
    std::print("          ");
    for (Value* slot{m_stack.data()}; slot < m_sp; slot++) {
      std::print("[ ");
      slot->print_value();
      std::print(" ]");
    }
    std::println();

    disassemble_instruction(
        m_frame->function->chunk,
        static_cast<std::size_t>(m_frame->ip -
                                 &m_frame->function->chunk.get_code_ref(0)));

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

      case OP_PRINT:
        pop().print_value();
        std::println();
        break;
      case OP_POP:
        pop();
        break;
      case OP_POPN: {
        auto n{static_cast<int>(read_constant().as_number())};
        pop(n);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name{read_string()};
        m_globals[name] = peek(0);
        pop();
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name{read_string()};
        auto value{m_globals.find(name)};

        if (value == m_globals.end()) {
          runtime_error("Undefined variable '" + name->str + "'");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value->second);
        break;
      }
      case OP_SET_GLOBAL: {
        ObjString* name{read_string()};
        if (!m_globals.contains(name)) {
          runtime_error("Undefined variable " + name->str + " .");
          return INTERPRET_RUNTIME_ERROR;
        }

        m_globals[name] = peek(0);

        break;
      }

      case OP_GET_LOCAL: {
        auto slot{read_byte()};
        push(m_frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        auto slot{read_byte()};
        m_frame->slots[slot] = peek(0);
        break;
      }

      case OP_JUMP_IF_FALSE: {
        std::uint16_t offset{read_short()};
        if (!peek(0).as_boolean()) m_frame->ip += offset;
        break;
      }
      case OP_JUMP: {
        std::uint16_t offset{read_short()};
        m_frame->ip += offset;
        break;
      }
      case OP_LOOP: {
        std::uint16_t offset{read_short()};
        m_frame->ip -= offset;
        break;
      }

      case OP_CALL: {
        int arg_count{read_byte()};
        if (!call_value(peek(arg_count), arg_count)) {
          return INTERPRET_RUNTIME_ERROR;
        }

        m_frame = &m_frames[m_frame_count - 1];

        break;
      }

      case OP_DUP:
        push(m_sp[-1]);
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

      case OP_RETURN: {
        Value result{pop()};
        m_frame_count--;
        // end of the program
        if (m_frame_count == 0) {
          pop();
          return INTERPRET_OK;
        }

        m_sp = m_frame->slots;
        push(result);
        m_frame = &m_frames[m_frame_count - 1];

        break;
      }
    }
  }
}

std::uint8_t VM::read_byte() { return *m_frame->ip++; }

Value VM::read_constant() {
  return m_frame->function->chunk.get_constant(read_byte());
}

Value VM::read_constant_long() {
  auto ind_first_byte{read_byte()};
  auto ind_second_byte{read_byte()};
  auto ind_third_byte{read_byte()};

  auto ind{(ind_first_byte << 16) + (ind_second_byte << 8) + ind_third_byte};

  return m_frame->function->chunk.get_constant(ind);
}

std::uint16_t VM::read_short() {
  m_frame->ip += 2;
  return (m_frame->ip[-2] << 8) | m_frame->ip[-1];
}

ObjString* VM::read_string() { return as_string(read_constant()); }

void VM::concatenate_two_strings() {
  ObjString* b{as_string(pop())};
  ObjString* a{as_string(pop())};

  ObjString* result{take_string(a->str + b->str)};
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

  ObjString* result{take_string(std::move(concatenate))};

  push(Value{result});
}

ObjString* VM::take_string(std::string str) {
  return allocate_string(*this, std::move(str));
}

void VM::push(Value value) {
  if (is_stack_full()) throw "Stack overflow";

  *m_sp = value;
  m_sp++;
}

Value VM::pop() {
  m_sp--;
  return *m_sp;
}

Value VM::pop(int n) {
  for (std::size_t i{}; i < n; i++) {
    m_sp--;
  }
  return *m_sp;
}

Value VM::peek(int distance) { return m_sp[-1 - distance]; }

bool VM::call_value(const Value& callee, int arg_count) {
  if (callee.is_obj()) {
    switch (obj_type(callee)) {
      case OBJ_FUNCTION:
        return call(as_function(callee), arg_count);
      case OBJ_NATIVE: {
        NativeFn native{as_native(callee)};
        Value result{native(arg_count, m_sp - arg_count)};
        m_sp -= (arg_count + 1);
        push(result);
        return true;
      }
      default:
        break;
    }
  }
  runtime_error("Can only call function types.");
  return false;
}

bool VM::call(ObjFunction* function, int arg_count) {
  if (function->arity != arg_count) {
    runtime_error(std::format("Expected {} received {} arguments.\n",
                              function->arity, arg_count));
    return false;
  }

  if (m_frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame& frame{m_frames[m_frame_count++]};
  frame.function = function;
  frame.ip = &function->chunk.get_code_ref(0);
  frame.slots = m_sp - arg_count - 1;
  return true;
}

Value VM::clock_native(int arg_count, Value* args) {
  return Value{static_cast<double>(clock()) / CLOCKS_PER_SEC};
}

void VM::define_native(const std::string& name, NativeFn function) {
  auto name_obj{allocate_string(*this, name)};
  push(Value{name_obj});
  push(new_native(function));
  m_globals.insert({as_string(m_stack[0]), m_stack[1]});
  pop();
  pop();
}

void VM::add_string(std::string key, ObjString* value) {
  m_strings.insert({key, value});
}

std::optional<Value> VM::find_string(const std::string& key) {
  auto it{m_strings.find(key)};
  if (it != m_strings.end()) {
    return it->second;
  }
  return {};
}

bool VM::is_stack_full() const {
  return m_sp == (m_stack.data() + m_stack.size());
}

void VM::runtime_error(const std::string_view format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format.data(), args);
  va_end(args);

  for (int i{m_frame_count - 1}; i >= 0; i--) {
    CallFrame* frame{&m_frames[i]};
    ObjFunction* function{frame->function};
    auto instruction{frame->ip - &function->chunk.get_code_ref(0) - 1};
    std::print(stderr, "[line {}] in ",
               lookup_line(function->chunk, instruction));

    if (function->name == nullptr) {
      std::println(stderr, "script");
    } else {
      std::println(stderr, "({})", function->name->str);
    }
  }

  reset_stack();
}

void VM::reset_stack() { m_sp = m_stack.data(); }
