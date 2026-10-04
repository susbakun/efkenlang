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
#include <tuple>

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
    var_declration(false);
  } else if (match(TOKEN_CONST)) {
    var_declration(true);
  } else {
    statement();
  }

  if (m_parser.panic_mode)
    syncronize();
}

void Compiler::var_declration(bool is_const) {
  std::uint8_t global{parse_variable("Expect variable name", is_const)};

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    if (is_const) {
      error("Const variables must be initilized.");
    }
    emit_byte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after varialbe declration.");

  define_variable(global);
}

std::uint8_t Compiler::parse_variable(const std::string_view error_message,
                                      bool is_const) {
  consume(TOKEN_IDENTIFIER, error_message);

  declare_variable(is_const);
  // when we're in a local scope
  // don't need to store the variable's name
  // in the constant table of the chunk
  // so we just return a dummy index
  if (m_scope_depth > 0)
    return 0;

  // just return the index
  return std::get<0>(identifier_constant(m_parser.previous, is_const));
}

void Compiler::declare_variable(bool is_const) {
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

  add_local(name, is_const);
}

void Compiler::mark_as_initilized() {
  m_locals[m_local_count - 1].depth = m_scope_depth;
}

std::tuple<std::uint8_t, bool>
Compiler::identifier_constant(Token &name, bool is_const = false) {
  ObjString *obj_name{
      allocate_string(m_vm, std::string(name.start, name.length))};

  // we check if we already encountered the
  // variable's name
  if (m_variables_index.contains(obj_name))
    return std::make_tuple(m_variables_index[obj_name].slot,
                           m_variables_index[obj_name].is_const);

  auto ind{m_compiling_chunk.add_constant(obj_name)};
  m_variables_index[obj_name] = {ind, is_const};

  return std::make_tuple(ind, is_const);
}

bool Compiler::identifiers_equal(Token &name1, Token &name2) {
  if (name1.length != name2.length)
    return false;

  return std::memcmp(name1.start, name2.start, name1.length) == 0;
}

void Compiler::add_local(Token &name, bool is_const) {
  if (m_local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local local{};
  local.depth = -1;
  local.name = name;
  local.is_const = is_const;

  auto name_str{std::string(name.start, name.length)};

  m_local_slots.insert({name_str, m_local_count});
  m_locals[m_local_count++] = local;
}

void Compiler::define_variable(std::uint8_t global) {
  // no need to emit_bytes for local variables
  if (m_scope_depth > 0) {
    mark_as_initilized();
    return;
  }

  emit_bytes(OP_DEFINE_GLOBAL, global);
}

void Compiler::statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
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

void Compiler::if_statement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int then_jump{emit_jump(OP_JUMP_IF_FALSE)};
  emit_byte(OP_POP);
  statement();

  int else_jump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);

  if (match(TOKEN_ELSE)) {
    statement();
    patch_jump(else_jump);
  }
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

  double n{0};

  while (m_local_count > 0 &&
         m_locals[m_local_count - 1].depth > m_scope_depth) {
    n++;
    m_local_count--;
  }

  auto ind{m_compiling_chunk.add_constant(Value{n})};

  emit_bytes(OP_POPN, ind);
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
  std::uint8_t get_op, set_op;
  auto arg{resolve_local(name)};

  if (std::get<0>(arg) != -1) {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  } else {
    arg = identifier_constant(name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    if (std::get<1>(arg)) {
      error("Cannot reassign to const vairables");
    }
    expression();
    emit_bytes(set_op, static_cast<std::uint8_t>(std::get<0>(arg)));
  } else {
    emit_bytes(get_op, static_cast<std::uint8_t>(std::get<0>(arg)));
  }
}

std::tuple<int, bool> Compiler::resolve_local(Token &name) {
  auto name_str{std::string(name.start, name.length)};
  auto it{m_local_slots.find(name_str)};

  if (it == m_local_slots.end())
    return std::make_tuple(-1, false);

  auto index{it->second};
  if (m_locals[index].depth == -1) {
    error("Can't read local variables in its own initilizer");
  }

  return std::make_tuple(index, m_locals[index].is_const);
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

int Compiler::emit_jump(std::uint8_t instruction) {
  emit_byte(instruction);
  emit_byte(0xff);
  emit_byte(0xff);
  return m_compiling_chunk.code_size() - 2;
}

void Compiler::patch_jump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump{static_cast<int>(m_compiling_chunk.code_size()) - offset - 2};

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  m_compiling_chunk.get_code_ref(offset) = (jump >> 8) & 0xff;
  m_compiling_chunk.get_code_ref(offset + 1) = jump & 0xff;
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

void Compiler::_and(bool can_assign) {
  int end_jump{emit_jump(OP_JUMP_IF_FALSE)};
  emit_byte(OP_POP);

  parse_precedence(PREC_AND);
  patch_jump(end_jump);
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

  std::println(std::cerr, " {}", message);
  m_parser.had_error = true;
}
