#pragma once

#include "value.hpp"
#include <cstddef>
#include <vector>

enum OpCode : std::uint8_t { OP_RETURN, OP_CONSTANT };

struct LineRun {
  int line;
  int count;
};

struct Chunk {
  std::vector<std::uint8_t> code{};
  std::vector<LineRun> lines{};
  ValueArray constants{};
};

void write_chunk(Chunk &chunk, std::uint8_t byte, int line);
std::size_t add_constant(Chunk &chunk, Value value);
