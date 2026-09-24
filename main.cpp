#include "chunk.hpp"
#include "debug.hpp"
#include <cstddef>

int main() {
  Chunk chunk{};

  for (std::size_t i{}; i < 300; i++) {
    write_constant(chunk, 1.2 + i, 123);
  }

  write_chunk(chunk, OP_RETURN, 123);

  disassemble_chunk(chunk, "test chunk");

  return 0;
}
