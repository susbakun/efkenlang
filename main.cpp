#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "common.hpp"
#include "cpp_perf.hpp"
#include "vm.hpp"

void repl() {
  char line[1024];

  VM vm{};

  while (true) {
    std::print("> ");
    std::string line{};
    std::getline(std::cin, line);

    if (line == "") break;

    vm.interpret(line);
  }
}

std::string read_file(std::string_view path) {
  std::ifstream file{path.data()};

  if (!file.is_open()) {
    std::cerr << "Couldn't open the file" << "\n";
    exit(65);
  }

  file.seekg(0, std::fstream::end);
  auto size{file.tellg()};
  file.seekg(0, std::fstream::beg);

  std::string source{};
  source.resize(size);

  if (!file.read(source.data(), size)) {
    std::cerr << "Failed to read the file" << "\n";
    exit(65);
  }

  return source;
}

void run_file(const std::string_view path) {
  auto source{read_file(path)};

  VM vm{};

  InterpretResult result{vm.interpret(source)};

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
#ifdef MEASURE
  perf::start();
#endif

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  } else {
    std::cerr << "Usage: clox [path]" << "\n";
    exit(64);
  }

#ifdef MEASURE
  perf::stop();
#endif

  return 0;
}
