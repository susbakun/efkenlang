#include "chunk.hpp"
#include "value.hpp"
#include <cstddef>
#include <limits>

void Chunk::write_chunk(std::uint8_t byte, int line) {
  m_code.push_back(byte);

  if (m_lines.size() > 0 && m_lines.back().line == line) {
    m_lines.back().count += 1;
  } else {
    m_lines.push_back({line, 1});
  }
}

void Chunk::write_constant(Value value, int line) {
  auto ind{add_constant(value)};

  if (ind > std::numeric_limits<std::uint8_t>::max()) {
    write_chunk(OP_CONSTANT_LONG, line);

    auto ind_first_byte{static_cast<std::uint8_t>(ind >> 16)};
    auto ind_second_byte{static_cast<std::uint8_t>(ind >> 8)};
    auto ind_third_byte{static_cast<std::uint8_t>(ind)};

    write_chunk(ind_first_byte, line);
    write_chunk(ind_second_byte, line);
    write_chunk(ind_third_byte, line);
  } else {
    write_chunk(OP_CONSTANT, line);
    write_chunk(ind, line);
  }
}

int Chunk::add_constant(Value value) {
  m_constants.write_value(value);
  return constants_size() - 1;
}

std::uint8_t Chunk::get_code(const std::size_t offset) const {
  return m_code[offset];
}

Value Chunk::get_constant(const std::size_t offset) const {
  return m_constants.m_values[offset];
}

LineRun Chunk::get_lines(const std::size_t offset) const {
  return m_lines[offset];
}

std::uint8_t &Chunk::get_code_ref(const std::size_t offset) {
  return m_code[offset];
}

Value &Chunk::get_constant_ref(const std::size_t offset) {
  return m_constants.m_values[offset];
}

LineRun &Chunk::get_lines_ref(const std::size_t offset) {
  return m_lines[offset];
}

std::size_t Chunk::code_size() const { return m_code.size(); }

std::size_t Chunk::constants_size() const {
  return m_constants.m_values.size();
}

std::size_t Chunk::lines_size() const { return m_lines.size(); };
