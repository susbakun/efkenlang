#include "test_helpers.hpp"

// These cover the const rules (compile-time for locals/upvalues,
// runtime for globals and instances).

TEST(Const, ReassigningAtTopLevelIsRejected) {
  EXPECT_LOX_REJECTED("const a = 1; a = 2;");
}

TEST(Const, ReassigningALocalIsRejected) {
  EXPECT_LOX_REJECTED("{ const a = 1; a = 2; }");
}

TEST(Const, ReassigningThroughAnUpvalueIsRejected) {
  EXPECT_LOX_REJECTED(R"(
    fun f() { const a = 1; fun g() { a = 2; } g(); }
    f();
  )");
}

TEST(Const, ReassigningAConstGlobalFromAFunctionIsRejected) {
  EXPECT_LOX_REJECTED("const g = 1; fun f() { g = 2; } f();");
}

TEST(Const, ConstMustBeInitialized) { EXPECT_LOX_COMPILE_ERROR("const a;"); }

TEST(Const, ReadingConstsWorks) {
  EXPECT_LOX("const a = 40; const b = 2; print a + b;", "42\n");
}

TEST(Const, ConstInstanceRejectsNewFields) {
  EXPECT_LOX_RUNTIME_ERROR("class P {} const p = P(); p.x = 1;");
}

TEST(Const, ConstInstanceStillAllowsChangingExistingFields) {
  EXPECT_LOX(R"(
    class P {}
    var p = P();
    p.x = 1;
    const q = p;     // seals the object that p also points at
    q.x = 2;
    print p.x;
  )",
             "2\n");
}

TEST(Const, SealCatchesAliases) {
  EXPECT_LOX_RUNTIME_ERROR(R"(
    class P {}
    const p = P();
    var q = p;
    q.z = 1;
  )");
}