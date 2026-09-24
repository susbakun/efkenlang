#include "chunk.hpp"
#include "value.hpp"
#include <cstddef>
#include <cstdio>

void write_chunk(Chunk &chunk, std::uint8_t byte, int line) {
  chunk.code.push_back(byte);

  if (chunk.lines.size() > 0 && chunk.lines.back().line == line) {
    chunk.lines.back().count += 1;
  } else {
    chunk.lines.push_back({line, 1});
  }
}

std::size_t add_constant(Chunk &chunk, Value value) {
  write_value(chunk.constants, value);
  return chunk.constants.values.size() - 1;
}
