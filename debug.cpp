#include "debug.hpp"

#include <cstddef>
#include <cstdint>
#include <print>

#include "chunk.hpp"
#include "obj.hpp"

void disassemble_chunk(Chunk& chunk, const std::string_view name) {
  std::println("== {} ==", name);

  for (std::size_t offset{}; offset < chunk.code_size();) {
    offset = disassemble_instruction(chunk, offset);
  }
}

int disassemble_instruction(Chunk& chunk, std::size_t offset) {
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
    case OP_FALSE:
      return simple_instruction("OP_FALSE", offset);
    case OP_TRUE:
      return simple_instruction("OP_TRUE", offset);
    case OP_NIL:
      return simple_instruction("OP_NIL", offset);

    case OP_NEGATE:
      return simple_instruction("OP_NEGATE", offset);
    case OP_NOT:
      return simple_instruction("OP_NOT", offset);

    case OP_PRINT:
      return simple_instruction("OP_PRINT", offset);
    case OP_POP:
      return simple_instruction("OP_POP", offset);
    case OP_POPN:
      return constant_instruction("OP_POPN", chunk, offset);
    case OP_DEFINE_GLOBAL:
      return constant_instruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL:
      return constant_instruction("OP_GET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL:
      return constant_instruction("OP_SET_GLOBAL", chunk, offset);
    case OP_GET_UPVALUE:
      return byte_instruction("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byte_instruction("OP_SET_UPVALUE", chunk, offset);
    case OP_GET_LOCAL:
      return byte_instruction("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byte_instruction("OP_SET_LOCAL", chunk, offset);

    case OP_JUMP:
      return jump_instruction("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jump_instruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
      return jump_instruction("OP_LOOP", -1, chunk, offset);
    case OP_CALL:
      return byte_instruction("OP_CALL", chunk, offset);
    case OP_CLOSURE: {
      offset++;
      std::uint8_t constant{chunk.get_code(offset++)};
      std::print("{:16} {:4} ", "OP_CLOSURE", constant);
      chunk.get_constant(constant).print_value();
      std::println();

      ObjFunction* function{as_function(chunk.get_constant(constant))};
      for (std::size_t i{}; i < function->upvalue_count; i++) {
        int is_local{chunk.get_code(offset++)};
        int index{chunk.get_code(offset++)};
        std::println("{:4}      |                     {} {}", offset - 2,
                     is_local ? "local" : "upvalue", index);
      }

      return offset;
    }

    case OP_DUP:
      return simple_instruction("OP_DUP", offset);

    case OP_CLOSE_UPVALUE:
      return simple_instruction("OP_CLOSE_UPVALUE", offset);

      // binary
    case OP_EQUAL:
      return simple_instruction("OP_EQUAL", offset);
    case OP_GREATER:
      return simple_instruction("OP_GREATER", offset);
    case OP_LESS:
      return simple_instruction("OP_LESS", offset);
    case OP_ADD:
      return simple_instruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simple_instruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simple_instruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simple_instruction("OP_DIVIDE", offset);
    case OP_COMMA:
      return simple_instruction("OP_COMMA", offset);

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

std::size_t constant_instruction(const std::string_view name, Chunk& chunk,
                                 std::size_t offset) {
  auto constant{chunk.get_code(offset + 1)};

  std::print("{:16} {:4} '", name, constant);
  chunk.get_constant(constant).print_value();
  std::println();
  return offset + 2;
}

std::size_t constant_long_instruction(const std::string_view name, Chunk& chunk,
                                      std::size_t offset) {
  auto first_byte{chunk.get_code(offset + 1)};
  auto second_byte{chunk.get_code(offset + 2)};
  auto third_byte{chunk.get_code(offset + 3)};

  // big-endian format
  auto constant{(first_byte << 16) + (second_byte << 8) + third_byte};

  std::print("{:16} {:4} '", name, constant);
  chunk.get_constant(constant).print_value();
  std::println();
  return offset + 4;
}

std::size_t byte_instruction(const std::string_view name, Chunk& chunk,
                             std::size_t offset) {
  std::uint8_t slot{chunk.get_code(offset + 1)};
  std::println("{:16} {:4}", name, slot);
  return offset + 2;
}

std::size_t jump_instruction(const std::string_view name, int sign,
                             Chunk& chunk, int offset) {
  std::uint16_t jump{
      static_cast<std::uint16_t>(chunk.get_code_ref(offset + 1) << 8)};
  jump |= chunk.get_code_ref(offset + 2);

  std::println("{:16} {:4} -> {}", name, offset, offset + 3 + sign * jump);
  return offset + 3;
}

int lookup_line(const Chunk& chunk, std::size_t offset) {
  for (std::size_t ind{}; ind < chunk.lines_size(); ind++) {
    auto run{chunk.get_lines(ind)};
    if (offset < run.count) return run.line;
    offset -= run.count;
  }
  return -1;
}