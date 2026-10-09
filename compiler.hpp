#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "chunk.hpp"
#include "common.hpp"
#include "obj.hpp"
#include "scanner.hpp"
#include "vm.hpp"

struct Parser {
  Token current;
  Token previous;
  bool had_error;
  bool panic_mode;
};

enum Precedence {
  PREC_NONE,
  PREC_COMMA,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
};

class Compiler;

using ParseFn = void (Compiler::*)(bool can_assign);

// (index, is_const)
using VarInfo = std::tuple<int, bool>;

struct ParseRule {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
};

struct Global {
  std::uint8_t slot;
  bool is_const;
};

struct Local {
  Token name;
  int depth{};
  bool is_const;
  bool is_captured{false};
};

struct Upvalue {
  std::uint8_t index;
  bool is_local;
  bool is_const;
};

struct Loop {
  int target_loop{};
  int scope_depth{};
  std::vector<int> break_jumps{};
};

enum FunctionType { TYPE_FUNCTION, TYPE_SCRIPT };

class Compiler {
 public:
  Compiler(VM& vm, Scanner& scanner, Parser& parser, FunctionType type)
      : m_vm{vm}, m_scanner{scanner}, m_parser{parser}, m_type{type} {
    auto& local{m_locals[m_local_count++]};
    local.depth = 0;
    local.is_const = false;
    local.name.start = "";
    local.name.length = 0;

    std::string fname_str{""};

    m_vm.set_compiler(this);

    m_local_slots.insert({fname_str, 0});
  }

  Compiler(Compiler& enclosing, FunctionType type)
      : m_vm{enclosing.m_vm},
        m_enclosing{&enclosing},
        m_type{type},
        m_scanner{enclosing.m_scanner},
        m_parser{enclosing.m_parser} {
    auto& local{m_locals[m_local_count++]};
    local.depth = 0;
    local.is_const = false;
    local.name.start = "";
    local.name.length = 0;

    std::string fname_str{""};

    m_local_slots.insert({fname_str, 0});

    if (type != TYPE_SCRIPT) {
      std::string name{
          enclosing.m_parser.previous.start,
          static_cast<std::size_t>(enclosing.m_parser.previous.length)};

      m_vm.set_compiler(this);

      m_function->name = allocate_string(enclosing.m_vm, name);
    }
  }

  ObjFunction* compile();

  Compiler* enclosing() const;
  ObjFunction* get_function() const;

 private:
  void advance();
  void consume(const TokenType type, const std::string_view message);
  bool match(TokenType type);
  bool check(TokenType type);
  Chunk& compiling_chunk();

  void declaration();
  void fun_declration();
  void function(FunctionType type);
  void var_declration(bool is_const);
  std::uint8_t parse_variable(const std::string_view error_message,
                              bool is_const);
  void declare_variable(bool is_const);
  void mark_as_initilized();
  std::tuple<std::uint8_t, bool> identifier_constant(Token& name,
                                                     bool is_const);
  bool identifiers_equal(Token& name1, Token& name2);

  void add_local(Token& name, bool is_const);
  VarInfo add_upvalue(std::uint8_t index, bool is_local, bool is_const);
  void define_variable(std::uint8_t global);
  void statement();
  void print_statement();
  void if_statement();
  void return_statement();
  void switch_statement();
  void while_statement();
  void for_statement();
  void begin_scope();
  void block();
  void end_scope();
  void discard_locals(int depth);
  void expression_statement();
  void expression();

  void syncronize();

  ParseRule& get_rule(const TokenType type);
  void number(bool can_assign);
  void string(bool can_assign);
  void variable(bool can_assign);
  void named_variable(Token& name, bool can_assign);
  VarInfo resolve_local(Token& name);
  VarInfo resolve_up_value(Token& name);
  std::uint8_t argument_list();
  void emit_constant(Value value);
  void emit_byte(std::uint8_t byte);
  void emit_bytes(std::uint8_t byte1, std::uint8_t byte2);
  ObjFunction* end_compiler();
  int emit_jump(std::uint8_t instruction);
  void patch_jump(int loop_start);
  void emit_loop(int offset);
  void emit_return();

  void grouping(bool can_assign);
  void unary(bool can_assign);
  void binary(bool can_assign);
  void _and(bool can_assign);
  void _or(bool can_assign);
  void continue_stmt(bool can_assign);
  void break_stmt(bool can_assign);
  void call(bool can_assign);
  void literal(bool can_assign);

  void parse_precedence(Precedence precedence);

  void error_at_current(const std::string_view message);
  void error(const std::string_view message);
  void error_at(Token& token, const std::string_view message);

  VM& m_vm;
  Compiler* m_enclosing{nullptr};
  ObjFunction* m_function{new_function(m_vm)};
  FunctionType m_type;
  Scanner& m_scanner;
  Parser& m_parser;
  std::unordered_map<ObjString*, Global> m_variables_index{};
  std::array<Local, UINT8_COUNT> m_locals{};
  int m_scope_depth{};
  int m_local_count{};
  std::unordered_map<std::string, std::uint8_t> m_local_slots{};
  std::array<Upvalue, UINT8_COUNT> m_upvalues{};
  // kept for continue statements
  std::vector<Loop> m_target_loops{};

  std::array<ParseRule, 47> m_rules{{
      {&Compiler::grouping, &Compiler::call, PREC_CALL},  // TOKEN_LEFT_PAREN
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_RIGHT_PAREN
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_LEFT_BRACE
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_RIGHT_BRACE
      {nullptr, &Compiler::binary, PREC_COMMA},           // TOKEN_COMMA
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_DOT
      {&Compiler::unary, &Compiler::binary, PREC_TERM},   // TOKEN_MINUS
      {nullptr, &Compiler::binary, PREC_TERM},            // TOKEN_PLUS
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_SEMICOLON
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_COLON
      {nullptr, &Compiler::binary, PREC_FACTOR},          // TOKEN_SLASH
      {nullptr, &Compiler::binary, PREC_FACTOR},          // TOKEN_STAR
      {&Compiler::unary, nullptr, PREC_NONE},             // TOKEN_BANG
      {nullptr, &Compiler::binary, PREC_EQUALITY},        // TOKEN_BANG_EQUAL
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_EQUAL
      {nullptr, &Compiler::binary, PREC_EQUALITY},        // TOKEN_EQUAL_EQUAL
      {nullptr, &Compiler::binary, PREC_COMPARISON},      // TOKEN_GREATER
      {nullptr, &Compiler::binary, PREC_COMPARISON},      // TOKEN_GREATER_EQUAL
      {nullptr, &Compiler::binary, PREC_COMPARISON},      // TOKEN_LESS
      {nullptr, &Compiler::binary, PREC_COMPARISON},      // TOKEN_LESS_EQUAL
      {&Compiler::variable, nullptr, PREC_NONE},          // TOKEN_IDENTIFIER
      {&Compiler::string, nullptr, PREC_NONE},            // TOKEN_STRING
      {&Compiler::number, nullptr, PREC_NONE},            // TOKEN_NUMBER
      {nullptr, &Compiler::_and, PREC_AND},               // TOKEN_AND
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_CLASS
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_ELSE
      {&Compiler::literal, nullptr, PREC_NONE},           // TOKEN_FALSE
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_FOR
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_FUN
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_IF
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_SWITCH
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_CASE
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_DEFAULT_CASE
      {&Compiler::literal, nullptr, PREC_NONE},           // TOKEN_NIL
      {nullptr, &Compiler::_or, PREC_OR},                 // TOKEN_OR
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_PRINT
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_RETURN
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_SUPER
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_THIS
      {&Compiler::literal, nullptr, PREC_NONE},           // TOKEN_TRUE
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_VAR
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_CONST
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_WHILE
      {&Compiler::continue_stmt, nullptr, PREC_NONE},     // TOKEN_CONTINUE
      {&Compiler::break_stmt, nullptr, PREC_NONE},        // TOKEN_BREAK
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_ERROR
      {nullptr, nullptr, PREC_NONE},                      // TOKEN_EOF
  }};
};
