#include "test_helpers.hpp"

TEST(Expressions, Arithmetic) {
  EXPECT_LOX("print 1 + 2 * 3;", "7\n");
  EXPECT_LOX("print (1 + 2) * 3;", "9\n");
  EXPECT_LOX("print 10 / 4;", "2.5\n");
  EXPECT_LOX("print -(2 + 3);", "-5\n");
}

TEST(Expressions, Literals) {
  EXPECT_LOX("print nil;", "nil\n");
  EXPECT_LOX("print true;", "true\n");
  EXPECT_LOX("print false;", "false\n");
}

TEST(Expressions, Strings) {
  EXPECT_LOX(R"(print "foo" + "bar";)", "foobar\n");
  EXPECT_LOX(R"(print "n=" + 5;)", "n=5\n");
}

TEST(Expressions, ComparisonAndEquality) {
  EXPECT_LOX("print 1 < 2;", "true\n");
  EXPECT_LOX("print 2 < 1;", "false\n");
  EXPECT_LOX("print 1 == 1;", "true\n");
  EXPECT_LOX("print nil == nil;", "true\n");
  // A string built at runtime must equal the interned literal.
  EXPECT_LOX(R"(print "ab" == "a" + "b";)", "true\n");
}

TEST(Variables, GlobalsCanBeReassigned) {
  EXPECT_LOX("var a = 1; a = a + 1; print a;", "2\n");
}

TEST(Variables, UninitializedIsNil) { EXPECT_LOX("var a; print a;", "nil\n"); }

TEST(Variables, BlockScopeShadowing) {
  EXPECT_LOX(R"(
    var a = "global";
    { var a = "local"; print a; }
    print a;
  )",
             "local\nglobal\n");
}

TEST(Variables, NestedBlocksSeeOuterLocals) {
  EXPECT_LOX(R"(
    { var a = 1; { var b = 2; print a + b; } }
  )",
             "3\n");
}

TEST(ControlFlow, IfElse) {
  EXPECT_LOX(R"(if (1 < 2) print "yes"; else print "no";)", "yes\n");
  EXPECT_LOX(R"(if (2 < 1) print "yes"; else print "no";)", "no\n");
}

TEST(ControlFlow, While) {
  EXPECT_LOX(R"(
    var i = 0; var sum = 0;
    while (i < 5) { sum = sum + i; i = i + 1; }
    print sum;
  )",
             "10\n");
}

TEST(ControlFlow, For) {
  EXPECT_LOX("for (var i = 0; i < 3; i = i + 1) print i;", "0\n1\n2\n");
}

TEST(ControlFlow, LogicalOperators) {
  EXPECT_LOX("print true and false;", "false\n");
  EXPECT_LOX("print false or true;", "true\n");
}

// These two exercise your own extensions; adjust if your syntax differs.
TEST(ControlFlow, Break) {
  EXPECT_LOX(R"(
    for (var i = 0; i < 10; i = i + 1) { if (i == 3) break; print i; }
  )",
             "0\n1\n2\n");
}

TEST(ControlFlow, Continue) {
  EXPECT_LOX(R"(
    for (var i = 0; i < 3; i = i + 1) { if (i == 1) continue; print i; }
  )",
             "0\n2\n");
}

TEST(Errors, UndefinedGlobalIsRuntimeError) {
  const RunResult r{run_lox("print missing;")};
  EXPECT_EQ(r.result, INTERPRET_RUNTIME_ERROR);
  EXPECT_NE(r.err.find("Undefined variable"), std::string::npos) << r.err;
}

TEST(Errors, SyntaxErrorIsCompileError) { EXPECT_LOX_COMPILE_ERROR("print ;"); }

TEST(Errors, NegatingAStringIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR(R"(print -"a";)");
}