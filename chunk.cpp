#include "chunk.hpp"
#include "value.hpp"
#include <cstddef>
#include <limits>

void write_chunk(Chunk &chunk, std::uint8_t byte, int line) {
  chunk.code.push_back(byte);

  if (chunk.lines.size() > 0 && chunk.lines.back().line == line) {
    chunk.lines.back().count += 1;
  } else {
    chunk.lines.push_back({line, 1});
  }
}

void write_constant(Chunk &chunk, Value value, int line) {
  auto ind{add_constant(chunk, value)};

  if (ind > std::numeric_limits<std::uint8_t>::max()) {

    auto ind_first_byte{static_cast<std::uint8_t>(ind >> 16)};
    auto ind_second_byte{static_cast<std::uint8_t>(ind >> 8)};
    auto ind_third_byte{static_cast<std::uint8_t>(ind)};

    write_chunk(chunk, OP_CONSTANT_LONG, line);
    write_chunk(chunk, ind_first_byte, line);
    write_chunk(chunk, ind_second_byte, line);
    write_chunk(chunk, ind_third_byte, line);
  } else {
    write_chunk(chunk, OP_CONSTANT, line);
    write_chunk(chunk, ind, line);
  }
}

int add_constant(Chunk &chunk, Value value) {
  write_value(chunk.constants, value);
  return chunk.constants.values.size() - 1;
}
