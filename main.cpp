#include "chunk.hpp"
#include "debug.hpp"
#include "vm.hpp"
#include <cstddef>

int main() {
  Chunk chunk{};

  chunk.write_constant(1.2, 123);
  chunk.write_chunk(OP_RETURN, 123);

  disassemble_chunk(chunk, "test chunk");

  VM vm{};
  vm.interpret(chunk);

  return 0;
}
