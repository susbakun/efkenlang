#pragma once

#include "chunk.hpp"
#include "value.hpp"
#include <cstddef>
#include <string_view>

void disassemble_chunk(Chunk &chunk, const std::string_view name);
int disassemble_instruction(Chunk &chunk, const std::size_t offset);
std::size_t simple_instruction(const std::string_view name,
                               const std::size_t offset);
std::size_t constant_instruction(const std::string_view name, Chunk &chunk,
                                 std::size_t offset);
void print_value(Value value);
int lookup_line(const Chunk &chunk, std::size_t offset);
