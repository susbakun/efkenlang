#include "compiler.hpp"
#include "chunk.hpp"
#include "common.hpp"
#include "debug.hpp"
#include "obj.hpp"
#include "scanner.hpp"
#include "value.hpp"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <print>
#include <string>
#include <string_view>

#define DEBUG_PRINT_CODE

bool Compiler::compile() {
  advance();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  end_compiler();

  return !m_parser.had_error;
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

void Compiler::consume(const TokenType type, const std::string_view message) {
  if (check(type)) {
    advance();
    return;
  }

  error_at_current(message);
}

bool Compiler::match(TokenType type) {
  if (!check(type))
    return false;

  advance();
  return true;
}

bool Compiler::check(TokenType type) { return m_parser.current.type == type; }

void Compiler::declaration() {
  if (match(TOKEN_VAR)) {
    var_declration();
  } else {
    statement();
  }

  if (m_parser.panic_mode)
    syncronize();
}

void Compiler::var_declration() {
  std::uint8_t global{parse_variable("Expect variable name")};

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after varialbe declration.");

  define_variable(global);
}

std::uint8_t Compiler::parse_variable(const std::string_view error_message) {
  consume(TOKEN_IDENTIFIER, error_message);

  declare_variable();
  // when we're in a local scope
  // don't need to store the variable's name
  // in the constant table of the chunk
  // so we just return a dummy index
  if (m_scope_depth > 0)
    return 0;

  return identifier_constant(m_parser.previous);
}

std::uint8_t Compiler::identifier_constant(Token &name) {
  ObjString *obj_name{
      allocate_string(m_vm, std::string(name.start, name.length))};

  // we check if we already encountered the
  // variable's name
  if (m_variables_index.contains(obj_name))
    return m_variables_index[obj_name];

  auto ind{m_compiling_chunk.add_constant(obj_name)};
  m_variables_index[obj_name] = ind;

  return ind;
}

void Compiler::declare_variable() {
  if (m_scope_depth == 0)
    return;

  auto name{m_parser.previous};

  for (int i{m_local_count - 1}; i >= 0; i--) {
    if (m_locals[i].depth != -1 && m_locals[i].depth < m_scope_depth)
      break;

    if (identifiers_equal(name, m_locals[i].name)) {
      error("Already declared variable with the name " +
            std::string(name.start, name.length) + " in the scope.");
    }
  }

  add_local(name);
}

bool Compiler::identifiers_equal(Token &name1, Token &name2) {
  if (name1.length != name2.length)
    return false;

  return std::memcmp(name1.start, name2.start, name1.length);
}

void Compiler::add_local(Token &name) {
  if (m_local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local local{};
  local.depth = m_scope_depth;
  local.name = name;

  m_locals[m_local_count++] = local;
}

void Compiler::define_variable(std::uint8_t global) {
  // no need to emit_bytes for local variables
  if (m_scope_depth > 0)
    return;

  emit_bytes(OP_DEFINE_GLOBAL, global);
}

void Compiler::statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else {
    expression_statement();
  }
}

void Compiler::print_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after a print statement.");
  emit_byte(OP_PRINT);
}

void Compiler::begin_scope() { m_scope_depth++; }

void Compiler::block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after a block statement.");
}

void Compiler::end_scope() {
  m_scope_depth--;

  while (m_local_count > 0 &&
         m_locals[m_local_count - 1].depth > m_scope_depth) {
    emit_byte(OP_POP);
    m_local_count--;
  }
}

void Compiler::expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after an expression");
  emit_byte(OP_POP);
}

void Compiler::expression() { parse_precedence(PREC_COMMA); }

void Compiler::syncronize() {
  m_parser.panic_mode = false;

  while (m_parser.current.type != TOKEN_EOF) {
    if (m_parser.previous.type == TOKEN_SEMICOLON)
      return;

    switch (m_parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_FUN:
    case TOKEN_WHILE:
    case TOKEN_VAR:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;
    default:
      // do nothing
    }
  }

  advance();
}

ParseRule &Compiler::get_rule(const TokenType type) { return m_rules[type]; }

void Compiler::number(bool can_assign) {
  double value{std::stod(m_parser.previous.start)};
  emit_constant(Value{value});
}

void Compiler::string(bool can_assign) {
  auto str{
      std::string(m_parser.previous.start + 1, m_parser.previous.length - 2)};

  auto *string{allocate_string(m_vm, std::move(str))};

  emit_constant(Value{string});
}

void Compiler::variable(bool can_assign) {
  named_variable(m_parser.previous, can_assign);
}

void Compiler::named_variable(Token &name, bool can_assign) {
  std::uint8_t arg{identifier_constant(name)};

  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_bytes(OP_SET_GLOBAL, arg);
  } else {
    emit_bytes(OP_GET_GLOBAL, arg);
  }
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

void Compiler::grouping(bool can_assign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after a grouping expression.");
}

void Compiler::unary(bool can_assign) {
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

void Compiler::binary(bool can_assign) {
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

void Compiler::literal(bool can_assign) {
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
    error(" Expect expression.");
    return;
  }

  bool can_assign{precedence <= PREC_ASSIGNMENT};
  // fucking weird syntax because ParseFn
  // needs to know which object to operate on
  (this->*prefix_rule)(can_assign);

  while (precedence <= get_rule(m_parser.current.type).precedence) {
    advance();
    auto inline_rule{get_rule(m_parser.previous.type).infix};
    (this->*inline_rule)(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
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
