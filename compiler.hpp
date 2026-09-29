#pragma once

#include "chunk.hpp"
#include "scanner.hpp"
#include <array>
#include <cstdint>
#include <string_view>

struct Parser {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
};

enum Precedence {
  PREC_NONE,
  PREC_COMMA,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
};

class Compiler;

using ParseFn = void (Compiler::*)();

struct ParseRule {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
};

class Compiler {
public:
  Compiler(const std::string_view source, Chunk &chunk)
      : m_scanner{source}, m_compiling_chunk{chunk} {}

  bool compile();
  void consume(const TokenType type, const std::string_view message);
  void advance();
  void expression();
  ParseRule &get_rule(const TokenType type);
  void number();
  void emit_constant(Value value);
  void emit_byte(std::uint8_t byte);
  void emit_bytes(std::uint8_t byte1, std::uint8_t byte2);
  void end_compiler();
  void emit_return();

  void grouping();
  void unary();
  void binary();
  void literal();

  void parse_precedence(Precedence precedence);

  void error_at_current(const std::string_view message);
  void error(const std::string_view message);
  void error_at(Token &token, const std::string_view message);

private:
  Scanner m_scanner;
  Parser m_parser{};
  Chunk &m_compiling_chunk;

  std::array<ParseRule, 40> m_rules{{
      {&Compiler::grouping, nullptr, PREC_NONE},        // TOKEN_LEFT_PAREN
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_RIGHT_PAREN
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_LEFT_BRACE
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_RIGHT_BRACE
      {nullptr, &Compiler::binary, PREC_COMMA},         // TOKEN_COMMA
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_DOT
      {&Compiler::unary, &Compiler::binary, PREC_TERM}, // TOKEN_MINUS
      {nullptr, &Compiler::binary, PREC_TERM},          // TOKEN_PLUS
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_SEMICOLON
      {nullptr, &Compiler::binary, PREC_FACTOR},        // TOKEN_SLASH
      {nullptr, &Compiler::binary, PREC_FACTOR},        // TOKEN_STAR
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_BANG
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_BANG_EQUAL
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_EQUAL
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_EQUAL_EQUAL
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_GREATER
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_GREATER_EQUAL
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_LESS
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_LESS_EQUAL
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_IDENTIFIER
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_STRING
      {&Compiler::number, nullptr, PREC_NONE},          // TOKEN_NUMBER
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_AND
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_CLASS
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_ELSE
      {&Compiler::literal, nullptr, PREC_NONE},         // TOKEN_FALSE
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_FOR
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_FUN
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_IF
      {&Compiler::literal, nullptr, PREC_NONE},         // TOKEN_NIL
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_OR
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_PRINT
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_RETURN
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_SUPER
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_THIS
      {&Compiler::literal, nullptr, PREC_NONE},         // TOKEN_TRUE
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_VAR
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_WHILE
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_ERROR
      {nullptr, nullptr, PREC_NONE},                    // TOKEN_EOF
  }};
};
