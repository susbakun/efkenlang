
#include "compiler.hpp"
#include "scanner.hpp"
#include <print>

void compile(const std::string_view source) {
  Scanner scanner{source};

  int line{-1};

  while (true) {
    Token token{scanner.scan_token()};

    if (token.line != line) {
      std::print("{:4}", token.line);
      line = token.line;
    } else {
      std::print("   | ");
    }

    auto lexeme{std::string{token.start, token.start + token.length}};
    std::println("{} '{}'", std::to_string(token.type), lexeme);

    if (token.type == TOKEN_EOF)
      break;
  }
}
