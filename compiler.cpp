
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
    std::println("{} '{}'", std::to_string(token.type), token.start);

    if (token.type == TOKEN_EOF)
      break;
  }
}
