#pragma once

#include <cstddef>
#include <vector>

#include "value.hpp"

enum OpCode : std::uint8_t {
  OP_RETURN,
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_TRUE,
  OP_FALSE,
  OP_NIL,

  OP_NOT,
  OP_NEGATE,

  OP_PRINT,
  OP_POP,
  OP_POPN,
  OP_DEFINE_GLOBAL,
  OP_DEFINE_CONST_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_GET_PROPERTY,
  OP_SET_PROPERTY,
  OP_SEAL,

  OP_JUMP_IF_FALSE,
  OP_JUMP,
  OP_LOOP,
  OP_CALL,
  OP_CLOSURE,

  OP_CLOSE_UPVALUE,
  OP_DUP,

  OP_CLASS,

  // binary
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_COMMA
};

struct LineRun {
  int line;
  int count;
};

class Chunk {
 public:
  Chunk() = default;

  void write_chunk(std::uint8_t byte, int line);
  void write_constant(Value value, int line);
  std::uint8_t add_constant(Value value);

  std::uint8_t get_code(const std::size_t offset) const;
  Value get_constant(const std::size_t offset) const;
  LineRun get_lines(const std::size_t offset) const;

  std::uint8_t& get_code_ref(const std::size_t offset);
  Value& get_constant_ref(const std::size_t offset);
  LineRun& get_lines_ref(const std::size_t offset);

  std::size_t code_size() const;
  std::size_t constants_size() const;
  std::size_t lines_size() const;

  ValueArray m_constants{};

 private:
  std::vector<std::uint8_t> m_code{};
  std::vector<LineRun> m_lines{};
};
