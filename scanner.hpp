#pragma once

#include <string_view>

enum TokenType {
  // Single-character tokens.
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_SEMICOLON,
  TOKEN_COLON,
  TOKEN_SLASH,
  TOKEN_STAR,
  // One or two character tokens.
  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,
  // Literals.
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,
  // Keywords.
  TOKEN_AND,
  TOKEN_CLASS,
  TOKEN_ELSE,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_FUN,
  TOKEN_IF,
  TOKEN_SWITCH,
  TOKEN_CASE,
  TOKEN_DEFAULT_CASE,
  TOKEN_NIL,
  TOKEN_OR,
  TOKEN_PRINT,
  TOKEN_RETURN,
  TOKEN_SUPER,
  TOKEN_THIS,
  TOKEN_TRUE,
  TOKEN_VAR,
  TOKEN_CONST,
  TOKEN_WHILE,
  TOKEN_CONTINUE,
  TOKEN_BREAK,

  TOKEN_ERROR,
  TOKEN_EOF
};

struct Token {
  TokenType type;
  const char *start;
  int length;
  int line;
};

class Scanner {
public:
  Scanner(const std::string_view source)
      : m_start{source.data()}, m_current{source.data()}, m_line{1} {}

  Token scan_token();
  bool is_alpha(char c) const;
  bool is_digit(char c) const;
  bool is_at_end() const;
  char advance();
  char peek();
  char peek_next();
  bool match(char expected);
  Token make_token(TokenType type) const;
  Token error_token(const std::string_view message) const;
  void skip_whitespace();
  Token identifier();
  TokenType identifier_type();
  TokenType check_type(int start, int length, const char *rest, TokenType type);
  Token number();
  Token string();

private:
  const char *m_start;
  const char *m_current;
  int m_line;
};
