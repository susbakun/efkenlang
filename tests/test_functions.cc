#include "test_helpers.hpp"

TEST(Functions, CallAndReturn) {
  EXPECT_LOX("fun add(a, b) { return a + b; } print add(1, 2);", "3\n");
}

TEST(Functions, ImplicitReturnIsNil) {
  EXPECT_LOX("fun f() {} print f();", "nil\n");
}

TEST(Functions, Recursion) {
  EXPECT_LOX(R"(
    fun fib(n) { if (n < 2) return n; return fib(n - 1) + fib(n - 2); }
    print fib(10);
  )",
             "55\n");
}

TEST(Functions, WrongArityIsRuntimeError) {
  const RunResult r{run_lox("fun f(a) {} f();")};
  EXPECT_EQ(r.result, INTERPRET_RUNTIME_ERROR);
  EXPECT_NE(r.err.find("Expected 1"), std::string::npos) << r.err;
}

TEST(Functions, CallingANonFunctionIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR("var x = 1; x();");
}

TEST(Natives, SqrtAbsType) {
  EXPECT_LOX("print sqrt(16);", "4\n");
  EXPECT_LOX("print abs(-3);", "3\n");
  EXPECT_LOX("print type(1);", "number\n");
  EXPECT_LOX("print type(nil);", "nil\n");
}

// A native that returns nil (sleep) must still leave the stack balanced,
// because the statement `sleep(0);` is followed by an OP_POP. If the native
// pushes nothing, the POP eats a local and `c` overwrites `b`'s slot.
TEST(Natives, VoidNativeLeavesStackBalanced) {
  EXPECT_LOX(R"(
    fun f() {
      var a = "a";
      var b = "b";
      sleep(0);
      var c = "c";
      print b;
    }
    f();
  )",
             "b\n");
}

TEST(Closures, CapturesEnclosingLocal) {
  EXPECT_LOX(R"(
    fun outer() { var x = "outside"; fun inner() { print x; } return inner; }
    outer()();
  )",
             "outside\n");
}

TEST(Closures, CounterKeepsState) {
  EXPECT_LOX(R"(
    fun counter() {
      var n = 0;
      fun inc() { n = n + 1; return n; }
      return inc;
    }
    var c = counter();
    print c(); print c(); print c();
  )",
             "1\n2\n3\n");
}

TEST(Closures, TwoClosuresShareOneVariable) {
  EXPECT_LOX(R"(
    var get; var set;
    fun make() {
      var x = "before";
      fun g() { print x; }
      fun s() { x = "after"; }
      get = g; set = s;
    }
    make();
    set();
    get();
  )",
             "after\n");
}

// The captured value must live in the upvalue, not in the dead stack slot.
TEST(Closures, SurvivesStackReuse) {
  EXPECT_LOX(R"(
    fun make(n) { fun get() { return n; } return get; }
    var a = make(1);
    var b = make(2);
    print 1 + 2;
    print a();
    print b();
  )",
             "3\n1\n2\n");
}

// Returning from a *different* function must not close upvalues that
// still belong to a live frame.
TEST(Closures, ReturnDoesNotCloseUpvaluesOfLiveFrames) {
  EXPECT_LOX(R"(
    fun outer() {
      var x = 1;
      fun inc() { x = x + 1; }
      fun nop() {}
      inc();
      nop();
      inc();
      print x;
    }
    outer();
  )",
             "3\n");
}

TEST(Closures, CapturesThroughTwoLevels) {
  EXPECT_LOX(R"(
    fun outer() {
      var a = 1;
      var b = 2;
      fun middle() {
        var c = 3;
        var d = 4;
        fun inner() { print a + c + b + d; }
        return inner;
      }
      return middle;
    }
    outer()()();
  )",
             "10\n");
}

TEST(Closures, VectorObjectExercise) {
  EXPECT_LOX(R"(
    fun vector(x, y) {
      fun dispatch(msg, other) {
        if (msg == 0) { return x; }
        if (msg == 1) { return y; }
        if (msg == 2) { return vector(x + getX(other), y + getY(other)); }
      }
      return dispatch;
    }
    fun getX(v)   { return v(0, nil); }
    fun getY(v)   { return v(1, nil); }
    fun add(a, b) { return a(2, b); }

    var a = vector(1, 2);
    var b = vector(10, 20);
    var c = add(a, b);
    print getX(c); print getY(c);
    print getX(a);
    var d = add(c, a);
    print getX(d); print getY(d);
  )",
             "11\n22\n1\n12\n24\n");
}