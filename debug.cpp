#include "debug.hpp"
#include "chunk.hpp"
#include <cstddef>
#include <print>

void disassemble_chunk(Chunk &chunk, const std::string_view name) {
  std::println("== {} ==", name);

  for (std::size_t offset{}; offset < chunk.code_size();) {
    offset = disassemble_instruction(chunk, offset);
  }
}

int disassemble_instruction(Chunk &chunk, std::size_t offset) {
  std::print("{:04} ", offset);

  if (offset > 0 &&
      lookup_line(chunk, offset) == lookup_line(chunk, offset - 1)) {
    std::print("   | ");
  } else {
    std::print("{:4} ", lookup_line(chunk, offset));
  }

  auto instruction{chunk.get_code(offset)};

  switch (instruction) {
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_CONSTANT_LONG:
    return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
  case OP_NEGATE:
    return simple_instruction("OP_NEGATE", offset);

    // binary
  case OP_ADD:
    return simple_instruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simple_instruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simple_instruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simple_instruction("OP_DIVIDE", offset);

  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  default:
    std::println("Unknown opcode {}", instruction);
    return offset + 1;
  }
}

std::size_t simple_instruction(const std::string_view name,
                               const std::size_t offset) {
  std::println("{}", name);
  return offset + 1;
}

std::size_t constant_instruction(const std::string_view name, Chunk &chunk,
                                 std::size_t offset) {
  auto constant{chunk.get_code(offset + 1)};

  std::print("{:16} {:4} '", name, constant);
  print_value(chunk.get_constant(constant));
  std::println();
  return offset + 2;
}

std::size_t constant_long_instruction(const std::string_view name, Chunk &chunk,
                                      std::size_t offset) {

  auto first_byte{chunk.get_code(offset + 1)};
  auto second_byte{chunk.get_code(offset + 2)};
  auto third_byte{chunk.get_code(offset + 3)};

  // big-endian format
  auto constant{(first_byte << 16) + (second_byte << 8) + third_byte};

  std::print("{:16} {:4} '", name, constant);
  print_value(chunk.get_constant(constant));
  std::println();
  return offset + 4;
}

void print_value(Value value) { std::print("{}", value); }

int lookup_line(const Chunk &chunk, std::size_t offset) {
  std::size_t ind{0};

  while (ind < chunk.lines_size()) {
    auto line_run{chunk.get_lines(ind)};
    if (offset < line_run.count) {
      return line_run.line;
    } else {
      offset -= line_run.count;
    }
  }

  return -1;
}
