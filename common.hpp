#pragma once

#include <cstddef>
#include <cstdint>

#define DEBUG_STRESS_GC
#define DEBUG_LOG_GC
#define DEBUG_PRINT_CODE
#define MEASURE
#define DEBUG_TRACE_EXECUTION

constexpr int STACK_MAX = 256;

constexpr int UINT8_COUNT = UINT8_MAX + 1;
constexpr int FRAMES_MAX = 64;
