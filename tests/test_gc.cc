#include "test_helpers.hpp"

// Each loop allocates far more than the 1 MB first-GC threshold, so at
// least one real collection happens while the program runs.

TEST(GC, ManyTemporaryStringsDoNotBreakTheProgram) {
  EXPECT_LOX(R"(
    var keep = "keep";
    for (var i = 0; i < 50000; i = i + 1) { var s = "junk" + i; }
    print keep;
  )",
             "keep\n");
}

TEST(GC, ClosureAndItsUpvalueSurviveCollection) {
  EXPECT_LOX(R"(
    fun make() { var x = "captured"; fun get() { return x; } return get; }
    var f = make();
    for (var i = 0; i < 50000; i = i + 1) { var s = "junk" + i; }
    print f();
  )",
             "captured\n");
}

TEST(GC, InternedStringsStayEqualAfterCollection) {
  EXPECT_LOX(R"(
    var a = "persist";
    for (var i = 0; i < 50000; i = i + 1) { var s = "junk" + i; }
    var b = "per" + "sist";
    print a == b;
  )",
             "true\n");
}

TEST(GC, InstancesAndTheirFieldsSurviveCollection) {
  EXPECT_LOX(R"(
    class P {}
    var p = P();
    p.name = "alive";
    for (var i = 0; i < 50000; i = i + 1) { var s = "junk" + i; }
    print p.name;
  )",
             "alive\n");
}