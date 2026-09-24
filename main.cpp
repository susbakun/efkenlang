#include "chunk.hpp"
#include "debug.hpp"

int main() {
  Chunk chunk{};

  auto constant{add_constant(chunk, 1.2)};

  write_chunk(chunk, OP_CONSTANT, 123);
  write_chunk(chunk, constant, 123);
  write_chunk(chunk, OP_RETURN, 123);

  disassemble_chunk(chunk, "test chunk");

  return 0;
}
