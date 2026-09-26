#include "scanner.hpp"
#include <cstring>

Token Scanner::scan_token() {
  skip_whitespace();
  m_start = m_current;

  if (is_at_end())
    return make_token(TOKEN_EOF);

  char c{advance()};

  if (is_alpha(c))
    return identifier();
  if (is_digit(c))
    return number();

  switch (c) {
  case '(':
    return make_token(TOKEN_LEFT_PAREN);
  case ')':
    return make_token(TOKEN_RIGHT_PAREN);
  case '{':
    return make_token(TOKEN_LEFT_BRACE);
  case '}':
    return make_token(TOKEN_RIGHT_BRACE);
  case ';':
    return make_token(TOKEN_SEMICOLON);
  case ',':
    return make_token(TOKEN_COMMA);
  case '.':
    return make_token(TOKEN_DOT);
  case '-':
    return make_token(TOKEN_MINUS);
  case '+':
    return make_token(TOKEN_PLUS);
  case '/':
    if (peek_next() == '/') {
      while (peek() != '\n' && !is_at_end())
        advance();
      break;
    }
    return make_token(TOKEN_SLASH);
  case '*':
    return make_token(TOKEN_STAR);

  case '!':
    return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_EQUAL);
  case '>':
    return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

  case '"':
    return string();
  }

  return error_token("Unexpected character.");
}

bool Scanner::is_alpha(char c) const {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

bool Scanner::is_digit(char c) const { return c >= '0' && c <= '9'; }

bool Scanner::is_at_end() const { return *m_current == '\0'; }

char Scanner::advance() {
  m_current++;
  return m_current[-1];
}

char Scanner::peek() { return *m_current; }

char Scanner::peek_next() {
  if (!is_at_end())
    return '\0';
  return m_current[1];
}

bool Scanner::match(char expected) {
  if (is_at_end())
    return false;
  if (*m_current != expected)
    return false;

  m_current++;
  return true;
}

Token Scanner::make_token(TokenType type) const {
  Token token{};

  token.type = type;
  token.start = m_start;
  token.length = static_cast<int>(m_current - m_start);
  token.line = m_line;

  return token;
}

Token Scanner::error_token(const std::string_view message) const {
  Token token{};

  token.type = TOKEN_ERROR;
  token.start = message.data();
  token.length = static_cast<int>(message.length());
  token.line = m_line;

  return token;
}

void Scanner::skip_whitespace() {
  while (true) {
    char c{peek()};

    switch (c) {
    case ' ':
    case '\t':
    case '\n':
      advance();
      break;
    default:
      return;
    }
  }
}

Token Scanner::identifier() {
  while (is_alpha(peek()) || is_digit(peek())) {
    advance();
  }

  return make_token(identifier_type());
}

TokenType Scanner::identifier_type() {
  switch (*m_start) {
  case 'a':
    return check_type(1, 2, "nd", TOKEN_AND);
  case 'c':
    return check_type(1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return check_type(1, 3, "lse", TOKEN_ELSE);
  case 'i':
    return check_type(1, 1, "f", TOKEN_IF);
  case 'n':
    return check_type(1, 2, "il", TOKEN_NIL);
  case 'o':
    return check_type(1, 1, "r", TOKEN_OR);
  case 'p':
    return check_type(1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return check_type(1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return check_type(1, 4, "uper", TOKEN_SUPER);
  case 'v':
    return check_type(1, 2, "ar", TOKEN_VAR);
  case 'w':
    return check_type(1, 4, "hile", TOKEN_WHILE);
  case 'f':
    if (m_current - m_start > 1) {
      switch (m_start[1]) {
      case 'a':
        return check_type(2, 4, "lse", TOKEN_FALSE);
      case 'o':
        return check_type(2, 1, "r", TOKEN_FOR);
      case 'u':
        return check_type(2, 1, "n", TOKEN_FUN);
      }
    }
    break;
  case 't':
    if (m_current - m_start > 1) {
      switch (m_start[1]) {
      case 'h':
        return check_type(2, 2, "is", TOKEN_THIS);
      case 'r':
        return check_type(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  }

  return TOKEN_IDENTIFIER;
}

TokenType Scanner::check_type(int start, int length, const char *rest,
                              TokenType type) {
  if (((m_current - m_start) == (start + length)) &&
      (std::memcmp(m_start + start, rest, length))) {
    return type;
  }
  return TOKEN_IDENTIFIER;
}

Token Scanner::number() {
  while (is_digit(peek())) {
    advance();
  }

  if (peek() == '.' && is_digit(peek_next())) {
    // consume '.'
    advance();
    while (is_digit(peek()))
      advance();
  }

  return make_token(TOKEN_NUMBER);
}

Token Scanner::string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n')
      m_line++;

    advance();
  }

  if (is_at_end())
    return error_token("Unterminated string");

  // consume the trailing '"'
  advance();

  return make_token(TOKEN_STRING);
}
