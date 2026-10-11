#pragma once

#include <cstddef>
#include <string_view>

#include "chunk.hpp"

void disassemble_chunk(Chunk& chunk, const std::string_view name);
int disassemble_instruction(Chunk& chunk, const std::size_t offset);
std::size_t simple_instruction(const std::string_view name,
                               const std::size_t offset);
std::size_t constant_instruction(const std::string_view name, Chunk& chunk,
                                 std::size_t offset);
std::size_t constant_long_instruction(const std::string_view name, Chunk& chunk,
                                      std::size_t offset);
std::size_t byte_instruction(const std::string_view name, Chunk& chunk,
                             std::size_t offset);
std::size_t jump_instruction(const std::string_view name, int sign,
                             Chunk& chunk, int offset);
std::size_t invoke_instruction(const std::string_view name, Chunk& chunk,
                               std::size_t offset);
int lookup_line(const Chunk& chunk, std::size_t offset);
