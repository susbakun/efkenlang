
#include "compiler.hpp"
#include "chunk.hpp"
#include "debug.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <iostream>
#include <print>
#include <string>
#include <string_view>

#define DEBUG_PRINT_CODE

bool Compiler::compile() {
  advance();
  expression();
  consume(TOKEN_EOF, "Expect end of expression");
  end_compiler();

  return !m_parser.had_error;
}

void Compiler::consume(const TokenType type, const std::string_view message) {
  if (m_parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

void Compiler::advance() {
  m_parser.previous = m_parser.current;

  while (true) {
    m_parser.current = m_scanner.scan_token();
    if (m_parser.current.type != TOKEN_ERROR)
      break;

    error_at_current(m_parser.current.start);
  }
}

void Compiler::expression() { parse_precedence(PREC_COMMA); }

ParseRule &Compiler::get_rule(const TokenType type) { return m_rules[type]; }

void Compiler::number() {
  double value{std::stod(m_parser.previous.start)};
  emit_constant(number_val(value));
}

void Compiler::emit_constant(Value value) {
  m_compiling_chunk.write_constant(value, m_parser.previous.line);
}

void Compiler::emit_byte(std::uint8_t byte) {
  m_compiling_chunk.write_chunk(byte, m_parser.previous.line);
}

void Compiler::emit_bytes(std::uint8_t byte1, std::uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

void Compiler::end_compiler() {
  emit_return();

#ifdef DEBUG_PRINT_CODE
  if (!m_parser.had_error) {
    disassemble_chunk(m_compiling_chunk, "code");
  }
#endif
}

void Compiler::emit_return() { emit_byte(OP_RETURN); }

void Compiler::grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after a grouping expression.");
}

void Compiler::unary() {
  auto type{m_parser.previous.type};

  // Compile the operand
  parse_precedence(PREC_UNARY);

  switch (type) {
  case TOKEN_BANG:
    emit_byte(OP_NOT);
    break;
  case TOKEN_MINUS:
    emit_byte(OP_NEGATE);
    break;
  default:
    return;
  }
}

void Compiler::binary() {
  auto operator_type{m_parser.previous.type};
  ParseRule &rule{get_rule(operator_type)};
  parse_precedence(static_cast<Precedence>(rule.precedence + 1));

  switch (operator_type) {
  case TOKEN_BANG_EQUAL:
    emit_bytes(OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emit_byte(OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emit_byte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emit_bytes(OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emit_byte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emit_bytes(OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    emit_byte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emit_byte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emit_byte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emit_byte(OP_DIVIDE);
    break;

  case TOKEN_COMMA:
    emit_byte(OP_COMMA);
  default:
    return;
  }
}

void Compiler::literal() {
  switch (m_parser.previous.type) {
  case TOKEN_FALSE:
    emit_byte(OP_FALSE);
    break;
  case TOKEN_TRUE:
    emit_byte(OP_TRUE);
    break;
  case TOKEN_NIL:
    emit_byte(OP_NIL);
    break;
  default:
    return;
  }
}

void Compiler::parse_precedence(Precedence precedence) {
  advance();
  auto prefix_rule{get_rule(m_parser.previous.type).prefix};

  if (prefix_rule == nullptr) {
    error("Expect expression.");
    return;
  }

  // fucking weird syntax because ParseFn
  // needs to know which object to operate on
  (this->*prefix_rule)();

  while (precedence <= get_rule(m_parser.current.type).precedence) {
    advance();
    auto inline_rule{get_rule(m_parser.previous.type).infix};
    (this->*inline_rule)();
  }
}

void Compiler::error_at_current(const std::string_view message) {
  error_at(m_parser.current, message);
}

void Compiler::error(const std::string_view message) {
  error_at(m_parser.previous, message);
}

void Compiler::error_at(Token &token, const std::string_view message) {
  // Don't output any other errors while we already have one
  if (m_parser.panic_mode)
    return;

  m_parser.panic_mode = true;

  std::print(std::cerr, "[line {}] Error", token.line);

  if (token.type == TOKEN_EOF) {
    std::print(std::cerr, " at end");
  } else if (token.type == TOKEN_ERROR) {
    // do nothing
  } else {
    std::string lexeme{token.start, token.start + token.length};
    std::print(std::cerr, " at '{}'", lexeme);
  }

  std::println(std::cerr, "{}", message);
  m_parser.had_error = true;
}
