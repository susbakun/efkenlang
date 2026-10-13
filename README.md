# efkenlang

A bytecode virtual machine and compiler for the **Lox** language, written in modern C++ (C++23).

This project follows the second half of [_Crafting Interpreters_](https://craftinginterpreters.com) by **Robert Nystrom**. The original C implementation (`clox`) lives in the author's [repository](https://github.com/munificent/craftinginterpreters). This version re-implements the VM in C++ using the standard library where the book hand-rolls its own data structures.

## Example

```lox
class Counter {
  init(start) { this.n = start; }
  next() {
    this.n = this.n + 1;
    return this.n;
  }
}

var c = Counter(10);
print c.next();   // 11
print c.next();   // 12

fun makeAdder(a) {
  fun add(b) { return a + b; }
  return add;
}
print makeAdder(1)(2);   // 3

class Animal {
  init(name) { this.name = name; }
  speak() { print this.name + " makes a sound"; }
}

class Dog < Animal {
  speak() {
    super.speak();
    print this.name + " barks";
  }
}

Dog("Rex").speak();
// Rex makes a sound
// Rex barks
```

## Features

**From the book**

- Single-pass compiler (Pratt parser) that emits bytecode
- Stack-based VM with call frames
- Global and local variables, block scoping
- `if` / `while` / `for`, logical `and` / `or`
- Functions, recursion, and native functions
- Closures with open and closed upvalues
- Mark-and-sweep garbage collector with a weak string-interning table
- Classes, instances, bound methods, `this`, and initializers
- Single inheritance with `super`
- NaN-boxed values: every `Value` fits in 64 bits (switchable at build time)

**Additions of my own**

- `const` variables, plus const instances (a `const` instance can't gain new fields)
- `break` and `continue`
- Comma operator and `switch` statements
- String concatenation with numbers (`"n=" + 5`)
- Extra native functions (below)
- Long constants (`OP_CONSTANT_LONG`) and run-length-encoded line information

## Native functions

| Function                             | Description                                                               |
| ------------------------------------ | ------------------------------------------------------------------------- |
| `input()`                            | Reads a line from stdin and returns it as a string                        |
| `clock()`                            | CPU time in seconds                                                       |
| `sqrt(n)` / `abs(n)`                 | Square root and absolute value                                            |
| `type(value)`                        | Returns `"bool"`, `"nil"`, `"number"`, or `"obj"`                         |
| `sleep(ms)`                          | Sleeps for the given number of milliseconds                               |
| `exit(code)`                         | Exits the interpreter                                                     |
| `has_attribute(instance, "name")`    | `true` if the instance has that field (a field set to `nil` still counts) |
| `remove_attribute(instance, "name")` | Removes a field from an instance                                          |

## Building

**Requirements**

- A C++23 compiler with `<print>` support (GCC 14+, or a recent Clang with libc++)
- CMake 3.20 or newer
- Network access on the first configure, which downloads GoogleTest. You can skip it with `-DLOX_BUILD_TESTS=OFF`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Running**

```bash
./build/efkenlang              # start the REPL
./build/efkenlang script.lox   # run a file
```

**CMake options**

| Option            | Default | Effect                                                                           |
| ----------------- | ------- | -------------------------------------------------------------------------------- |
| `LOX_NAN_BOXING`  | `ON`    | Store values as 64-bit NaN-boxed words instead of a 16-byte tagged union         |
| `LOX_TRACE`       | `OFF`   | Print the compiled bytecode and trace every instruction and the stack            |
| `LOX_LOG_GC`      | `OFF`   | Log every GC allocation, mark, and free                                          |
| `LOX_STRESS_GC`   | `OFF`   | Collect garbage on every allocation, which is good for shaking out missing roots |
| `LOX_ASAN`        | `OFF`   | Build with AddressSanitizer                                                      |
| `LOX_BUILD_TESTS` | `ON`    | Build the test suite                                                             |

A handy debugging build:

```bash
cmake -S . -B build-debug -DLOX_TRACE=ON -DLOX_BUILD_TESTS=OFF
```

And a harsh GC check (run the tests with this):

```bash
cmake -S . -B build-gc -DLOX_STRESS_GC=ON -DLOX_ASAN=ON
```

## Tests

The suite uses [GoogleTest](https://github.com/google/googletest). Each test runs a Lox program in a fresh VM, captures what it prints, and checks the output or the error status. It covers expressions, control flow, functions, closures, classes, methods, initializers, inheritance and `super`, const rules, and garbage collection under load.

```bash
cmake -S . -B build-test
cmake --build build-test
ctest --test-dir build-test --output-on-failure
```

The tests compare printed output exactly, so build them with `LOX_TRACE` and `LOX_LOG_GC` turned **off**. Both value representations should pass the same suite, so it's worth running it with `-DLOX_NAN_BOXING=ON` and `OFF`.

## Project layout

All sources live in `src/`.

| File                    | Role                                                                     |
| ----------------------- | ------------------------------------------------------------------------ |
| `main.cpp`              | Entry point: the REPL and the script runner                              |
| `scanner.cpp`           | Turns source text into tokens                                            |
| `compiler.cpp` / `.hpp` | Single-pass Pratt parser that emits bytecode                             |
| `chunk.cpp` / `.hpp`    | Bytecode, constants, and line information                                |
| `value.cpp` / `.hpp`    | The `Value` type (tagged union or NaN-boxed)                             |
| `obj.cpp` / `.hpp`      | Heap objects: strings, functions, closures, upvalues, classes, instances |
| `vm.cpp` / `.hpp`       | The interpreter loop, calls, natives, and the garbage collector          |
| `debug.cpp`             | The disassembler                                                         |

The GoogleTest suite is in `tests/`.

## Acknowledgements

All the credit for the design of Lox and the architecture of this VM goes to **Robert Nystrom** and _Crafting Interpreters_.
