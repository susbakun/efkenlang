#pragma once

#include "value.hpp"
#include <cstddef>
#include <vector>

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
  int add_constant(Value value);

  std::uint8_t get_code(const std::size_t offset) const;
  Value get_constant(const std::size_t offset) const;
  LineRun get_lines(const std::size_t offset) const;

  std::uint8_t &get_code_ref(const std::size_t offset);
  Value &get_constant_ref(const std::size_t offset);
  LineRun &get_lines_ref(const std::size_t offset);

  std::size_t code_size() const;
  std::size_t constants_size() const;
  std::size_t lines_size() const;

private:
  std::vector<std::uint8_t> m_code{};
  std::vector<LineRun> m_lines{};
  ValueArray m_constants{};
};
