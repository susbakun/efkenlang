#include "vm.hpp"

#include <chrono>
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
#include <thread>

#include "chunk.hpp"
#include "common.hpp"
#include "compiler.hpp"
#include "debug.hpp"
#include "obj.hpp"
#include "scanner.hpp"
#include "value.hpp"

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
  ObjClosure* closure{new_closure(*this, function)};
  pop();
  push(Value{closure});
  call(closure, 0);

  return run();
}

void VM::set_compiler(Compiler* c) { m_current_compiler = c; }

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
        m_frame->closure->function->chunk,
        static_cast<std::size_t>(
            m_frame->ip - &m_frame->closure->function->chunk.get_code_ref(0)));

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
      case OP_GET_UPVALUE: {
        auto slot{read_byte()};
        push(*m_frame->closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        auto slot{read_byte()};
        *m_frame->closure->upvalues[slot]->location = peek(0);
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

      case OP_CLOSURE: {
        ObjFunction* function{as_function(read_constant())};
        ObjClosure* closure{new_closure(*this, function)};
        push(Value{closure});

        for (std::size_t i{}; i < closure->upvalue_count; i++) {
          auto is_local{read_byte()};
          auto index{read_byte()};
          if (is_local) {
            closure->upvalues[i] = capture_upvalue(*(m_frame->slots + index));
          } else {
            closure->upvalues[i] = m_frame->closure->upvalues[index];
          }
        }

        break;
      }

      case OP_CLOSE_UPVALUE:
        close_upvalues(m_sp - 1);
        pop();
        break;

      case OP_DUP:
        push(m_sp[-1]);
        break;

      case OP_CLASS: {
        auto name{read_string()};
        push(Value{new_class(*this, name)});
        break;
      }

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
        close_upvalues(m_frame->slots);
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
  return m_frame->closure->function->chunk.get_constant(read_byte());
}

Value VM::read_constant_long() {
  auto ind_first_byte{read_byte()};
  auto ind_second_byte{read_byte()};
  auto ind_third_byte{read_byte()};

  auto ind{(ind_first_byte << 16) + (ind_second_byte << 8) + ind_third_byte};

  return m_frame->closure->function->chunk.get_constant(ind);
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
      case OBJ_CLOSURE:
        return call(as_closure(callee), arg_count);
      case OBJ_NATIVE: {
        return call_native(as_native(callee), arg_count);
      }
      default:
        break;
    }
  }
  runtime_error("Can only call function types.");
  return false;
}

ObjUpvalue* VM::capture_upvalue(Value& local) {
  auto it{m_open_upvalues.begin()};
  for (; it != m_open_upvalues.end(); it++) {
    if ((*it)->location == &local) return *it;
    if ((*it)->location < &local) break;
  }

  ObjUpvalue* created_upvalue{new_upvalue(*this, local)};
  m_open_upvalues.insert(it, created_upvalue);
  return created_upvalue;
}

void VM::close_upvalues(const Value* last) {
  auto it{m_open_upvalues.begin()};
  while (it != m_open_upvalues.end() && (*it)->location >= last) {
    ObjUpvalue* up{*it};
    up->closed = *up->location;
    up->location = &up->closed;
    ++it;
  }
  m_open_upvalues.erase(m_open_upvalues.begin(), it);
}

bool VM::call(ObjClosure* closure, int arg_count) {
  if (closure->function->arity != arg_count) {
    runtime_error(std::format("Expected {} received {} arguments.\n",
                              closure->function->arity, arg_count));
    return false;
  }

  if (m_frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow.");
    return false;
  }

  CallFrame& frame{m_frames[m_frame_count++]};
  frame.closure = closure;
  frame.ip = &closure->function->chunk.get_code_ref(0);
  frame.slots = m_sp - arg_count - 1;
  return true;
}

bool VM::call_native(ObjNative* native, int arg_count) {
  if (native->arity != arg_count) {
    runtime_error(std::format("Expected {} received {} arguments.\n",
                              native->arity, arg_count));
    return false;
  }

  // ugly shit again
  std::optional<Value> result{
      (this->*native->function)(arg_count, m_sp - arg_count)};

  if (!result) {
    return false;
  }

  m_sp -= (arg_count + 1);

  // for void functions
  if (!result->is_nil()) {
    push(*result);
  }

  return true;
}

std::optional<Value> VM::input_native(int arg_count, Value* args) {
  std::string input{};
  std::getline(std::cin >> std::ws, input);

  auto str_obj{allocate_string(*this, input)};

  return Value{str_obj};
}

std::optional<Value> VM::clock_native(int arg_count, Value* args) {
  return Value{static_cast<double>(clock()) / CLOCKS_PER_SEC};
}

std::optional<Value> VM::sqrt_native(int arg_count, Value* args) {
  if (arg_count != 1 || !args[0].is_number()) {
    runtime_error("sqrt() expects one number.");
    return {};
  }

  double num{args[0].as_number()};
  if (num < 0.0) {
    runtime_error("sqrt() expects positive numbers.");
    return {};
  }

  return std::sqrt(num);
}

std::optional<Value> VM::abs_native(int arg_count, Value* args) {
  if (arg_count != 1 || !args[0].is_number()) {
    runtime_error("abs() expects one number.");
    return {};
  }

  double num{args[0].as_number()};
  return std::abs(num);
}

std::optional<Value> VM::type_native(int arg_count, Value* args) {
  auto type_str_obj{allocate_string(*this, std::string(args[0].value_type()))};
  return Value{type_str_obj};
}

std::optional<Value> VM::sleep_native(int arg_count, Value* args) {
  if (arg_count != 1 || !args[0].is_number()) {
    runtime_error("sleep() expects one number.");
    return {};
  }

  double num{args[0].as_number()};
  if (num < 0.0) {
    runtime_error("sleep() expects positive numbers.");
    return {};
  }

  std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(num));

  return Value{};
}

std::optional<Value> VM::exit_native(int arg_count, Value* args) {
  if (arg_count != 1 || !args[0].is_number()) {
    runtime_error("exit() expects one number.");
    return {};
  }

  double num{args[0].as_number()};
  if (num < 0.0) {
    runtime_error("exit() expects positive numbers.");
    return {};
  }

  std::exit(num);
}

void VM::define_native(const std::string& name, NativeFn function, int arity) {
  auto name_obj{allocate_string(*this, name)};
  push(Value{name_obj});
  push(new_native(*this, function, arity));
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

void VM::collect_garbadge() {
#ifdef DEBUG_LOG_GC
  std::println("-- gc begin");
  std::size_t before{m_bytes_allocated};
#endif

  mark_roots();
  trace_references();
  remove_white_strings();
  sweep();
  m_next_gc = m_bytes_allocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  std::println("-- gc end");
  std::println("   collected {} bytes (from {} to {}) next at {}",
               before - m_bytes_allocated, before, m_bytes_allocated,
               m_next_gc);
#endif
}

void VM::mark_roots() {
  for (Value* slot{m_stack.data()}; slot < m_sp; slot++) mark_value(*slot);
  for (std::size_t i{}; i < m_frame_count; i++)
    mark_object(m_frames[i].closure);
  for (ObjUpvalue* up : m_open_upvalues) mark_object(up);

  for (auto& [name, value] : m_globals) {
    mark_object(name);
    mark_value(value);
  }

  for (Compiler* c{m_current_compiler}; c != nullptr; c = c->enclosing()) {
    mark_object(c->get_function());
  }
}

void VM::mark_array(const ValueArray& array) {
  for (auto& value : array.m_values) {
    mark_value(value);
  }
}

void VM::mark_value(const Value& value) {
  if (value.is_obj()) mark_object(value.as_obj());
}

void VM::mark_object(Obj* object) {
  if (object == nullptr || object->is_marked) return;

#ifdef DEBUG_LOG_GC
  std::print("{} mark ", static_cast<void*>(object));
  Value{object}.print_value();
  std::println();
#endif

  object->is_marked = true;
  m_gray_stacks.push_back(object);
}

void VM::trace_references() {
  while (!m_gray_stacks.empty()) {
    Obj* object{m_gray_stacks.back()};
    m_gray_stacks.pop_back();
    blacken_object(object);
  }
}

void VM::blacken_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  std::print("{} blacken ", static_cast<void*>(object));
  Value{object}.print_value();
  std::println();
#endif

  switch (object->type) {
    case OBJ_CLASS: {
      ObjClass* klass{static_cast<ObjClass*>(object)};
      mark_object(klass->name);
      break;
    }
    case OBJ_CLOSURE: {
      ObjClosure* closure{static_cast<ObjClosure*>(object)};
      mark_object(closure->function);
      for (ObjUpvalue* upvalue : closure->upvalues) {
        mark_object(upvalue);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function{static_cast<ObjFunction*>(object)};
      mark_object(function->name);
      mark_array(function->chunk.m_constants);
      break;
    }

    case OBJ_UPVALUE:
      mark_value(static_cast<ObjUpvalue*>(object)->closed);
      break;

    case OBJ_STRING:
    case OBJ_NATIVE:
      break;
  }
}

void VM::remove_white_strings() {
  std::erase_if(m_strings,
                [](const auto& entry) { return !entry.second->is_marked; });
}

void VM::free_object(Obj* object) {
#ifdef DEBUG_LOG_GC
  std::println("{} free type {}", static_cast<void*>(object),
               static_cast<int>(object->type));
#endif
  m_bytes_allocated -= object->size;
  delete object;
}

void VM::sweep() {
  std::size_t kept{};

  for (Obj* obj : m_objects) {
    if (obj->is_marked) {
      obj->is_marked = false;
      m_objects[kept++] = obj;
    } else {
      free_object(obj);
    }
  }

  m_objects.resize(kept);
}

void VM::runtime_error(const std::string_view format, ...) {
  va_list args;
  va_start(args, format);
  std::vfprintf(stderr, format.data(), args);
  va_end(args);

  for (int i{m_frame_count - 1}; i >= 0; i--) {
    CallFrame* frame{&m_frames[i]};
    ObjFunction* function{frame->closure->function};
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

VM::~VM() {
  for (Obj* obj : m_objects) {
    free_object(obj);
  }
}
