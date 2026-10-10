#pragma once

#include <gtest/gtest.h>

#include <string>
#include <utility>

#include "vm.hpp"

struct RunResult {
  InterpretResult result;
  std::string out;  // everything the program printed
  std::string err;  // compile/runtime error messages
};

// Runs `source` in a fresh VM and captures stdout/stderr.
inline RunResult run_lox(std::string source) {
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();
  InterpretResult result{};
  {
    VM vm;
    result = vm.interpret(source);
  }
  std::string out{testing::internal::GetCapturedStdout()};
  std::string err{testing::internal::GetCapturedStderr()};
  return {result, std::move(out), std::move(err)};
}

// Program must run cleanly and print exactly `expected`.
#define EXPECT_LOX(source, expected)              \
  do {                                            \
    const RunResult r_{run_lox(source)};          \
    EXPECT_EQ(r_.result, INTERPRET_OK) << r_.err; \
    EXPECT_EQ(r_.out, expected);                  \
  } while (false)

#define EXPECT_LOX_COMPILE_ERROR(source)                     \
  do {                                                       \
    const RunResult r_{run_lox(source)};                     \
    EXPECT_EQ(r_.result, INTERPRET_COMPILE_ERROR) << r_.out; \
  } while (false)

#define EXPECT_LOX_RUNTIME_ERROR(source)                     \
  do {                                                       \
    const RunResult r_{run_lox(source)};                     \
    EXPECT_EQ(r_.result, INTERPRET_RUNTIME_ERROR) << r_.out; \
  } while (false)

// Program must NOT run cleanly (either kind of error is fine).
#define EXPECT_LOX_REJECTED(source)               \
  do {                                            \
    const RunResult r_{run_lox(source)};          \
    EXPECT_NE(r_.result, INTERPRET_OK) << r_.out; \
  } while (false)