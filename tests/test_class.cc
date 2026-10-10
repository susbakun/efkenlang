#include "test_helpers.hpp"

TEST(Classes, PrintClassAndInstance) {
  EXPECT_LOX("class Point {} print Point; print Point();",
             "Point\nPoint instance\n");
}

TEST(Classes, FieldsAreCreatedOnAssignment) {
  EXPECT_LOX(R"(
    class P {}
    var p = P();
    p.x = 1; p.y = 2;
    print p.x + p.y;
  )",
             "3\n");
}

TEST(Classes, InstancesHaveIndependentFields) {
  EXPECT_LOX(R"(
    class P {}
    var a = P(); var b = P();
    a.x = 1; b.x = 2;
    print a.x; print b.x;
  )",
             "1\n2\n");
}

TEST(Classes, MissingFieldIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR("class P {} var p = P(); print p.nope;");
}

TEST(Classes, OnlyInstancesHaveProperties) {
  const RunResult r{run_lox("var x = 1; x.y = 2;")};
  EXPECT_EQ(r.result, INTERPRET_RUNTIME_ERROR);
  EXPECT_NE(r.err.find("Only instances"), std::string::npos) << r.err;
}

TEST(Classes, HasAttribute) {
  EXPECT_LOX(R"(
    class P {}
    var p = P();
    print has_attribute(p, "x");
    p.x = nil;                      // set to nil, but the field still exists
    print has_attribute(p, "x");
  )",
             "false\ntrue\n");
}

TEST(Classes, RemoveAttribute) {
  EXPECT_LOX(R"(
    class P {}
    var p = P();
    p.x = 1;
    remove_attribute(p, "x");
    print has_attribute(p, "x");
  )",
             "false\n");
}