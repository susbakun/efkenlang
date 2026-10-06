#pragma once

#include "common.hpp"
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

struct CallFrame {
  ObjFunction *function;
  uint8_t *ip;
  Value *slots;
};

class VM {
public:
  VM() = default;

  InterpretResult interpret(const std::string_view source);

  void add_string(std::string key, ObjString *value);
  std::optional<Value> find_string(const std::string &key);

private:
  InterpretResult run();

  void push(Value value);
  Value pop();
  Value pop(int n);
  Value peek(int distance);

  std::uint8_t read_byte();
  Value read_constant();
  Value read_constant_long();
  std::uint16_t read_short();
  ObjString *read_string();
  void concatenate_two_strings();
  void concatenate_string_and_number();
  ObjString *take_string(std::string str);

  bool is_stack_full() const;

  void runtime_error(const std::string_view format, ...);
  void reset_stack();

  std::array<CallFrame, FRAMES_MAX> m_frames{};
  int m_frame_count{};
  CallFrame *m_frame{&m_frames[0]};
  std::array<Value, STACK_MAX> m_stack{};
  Value *m_sp{m_stack.data()};
  std::unordered_map<std::string, ObjString *> m_strings{};
  std::unordered_map<ObjString *, Value> m_globals{};
};
