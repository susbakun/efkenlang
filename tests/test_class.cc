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

TEST(Methods, CallWithArguments) {
  EXPECT_LOX(R"(
    class Scone {
      topping(first, second) {
        print "scone with " + first + " and " + second;
      }
    }
    Scone().topping("berries", "cream");
  )",
             "scone with berries and cream\n");
}

TEST(Methods, ReturnValues) {
  EXPECT_LOX(R"(
    class Calc { double(x) { return x * 2; } }
    print Calc().double(21);
  )",
             "42\n");
}

TEST(Methods, CallingAnotherMethodThroughThis) {
  EXPECT_LOX(R"(
    class Calc {
      double(x) { return x * 2; }
      quad(x) { return this.double(this.double(x)); }
    }
    print Calc().quad(3);
  )",
             "12\n");
}

TEST(Methods, CanCallAMethodDefinedLaterInTheClass) {
  EXPECT_LOX(R"(
    class A {
      first() { return this.second(); }
      second() { return 7; }
    }
    print A().first();
  )",
             "7\n");
}

TEST(Methods, ChainingByReturningThis) {
  EXPECT_LOX(R"(
    class A { set(v) { this.v = v; return this; } }
    print A().set(1).set(2).v;
  )",
             "2\n");
}

TEST(Methods, UndefinedMethodIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR("class Foo {} Foo().bar();");
}

TEST(Methods, CallingAMethodOnANonInstanceIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR("var x; x.foo();");
}

TEST(Methods, FieldShadowsMethodWhenCalled) {
  EXPECT_LOX(R"(
    class Foo { bar() { print "method"; } }
    var foo = Foo();
    fun fieldFn() { print "field"; }
    foo.bar = fieldFn;
    foo.bar();
  )",
             "field\n");
}

// ---- bound methods --------------------------------------------------------

// Property access without a call must keep the receiver.
TEST(BoundMethods, RememberTheirReceiver) {
  EXPECT_LOX(R"(
    class Box {
      init(v) { this.v = v; }
      get() { return this.v; }
    }
    var g = Box(42).get;
    print g();
  )",
             "42\n");
}

TEST(BoundMethods, ReceiverIsFixedWhenTheMethodIsAccessed) {
  EXPECT_LOX(R"(
    class Person {
      init(name) { this.name = name; }
      sayName() { print this.name; }
    }
    var jane = Person("Jane");
    var bill = Person("Bill");
    bill.sayName = jane.sayName;
    bill.sayName();
  )",
             "Jane\n");
}

TEST(BoundMethods, BoundMethodsCanBePassedAround) {
  EXPECT_LOX(R"(
    class Greeter {
      init(name) { this.name = name; }
      greet() { return "hi " + this.name; }
    }
    fun call(f) { return f(); }
    print call(Greeter("ann").greet);
  )",
             "hi ann\n");
}

// ---- this -----------------------------------------------------------------

TEST(This, PrintsTheInstance) {
  EXPECT_LOX(R"(
    class Foo { me() { print this; } }
    Foo().me();
  )",
             "Foo instance\n");
}

TEST(This, WorksInsideNestedFunctions) {
  EXPECT_LOX(R"(
    class Nested {
      method() {
        fun function() { print this; }
        function();
      }
    }
    Nested().method();
  )",
             "Nested instance\n");
}

TEST(This, ClosureKeepsTheInstanceAlive) {
  EXPECT_LOX(R"(
    class Counter {
      init() { this.n = 0; }
      incrementer() {
        fun inc() { this.n = this.n + 1; return this.n; }
        return inc;
      }
    }
    var inc = Counter().incrementer();
    print inc();
    print inc();
  )",
             "1\n2\n");
}

TEST(This, OutsideAClassIsACompileError) {
  EXPECT_LOX_COMPILE_ERROR("print this;");
}

TEST(This, InsideAPlainFunctionIsACompileError) {
  EXPECT_LOX_COMPILE_ERROR("fun notMethod() { print this; }");
}

// ---- initializers ---------------------------------------------------------

TEST(Initializers, SetFieldsFromArguments) {
  EXPECT_LOX(R"(
    class Point { init(x, y) { this.x = x; this.y = y; } }
    var p = Point(1, 2);
    print p.x + p.y;
  )",
             "3\n");
}

TEST(Initializers, CallingTheClassReturnsTheInstance) {
  EXPECT_LOX(R"(
    class Point { init(x) { this.x = x; } }
    print Point(1);
  )",
             "Point instance\n");
}

TEST(Initializers, CallingInitDirectlyReturnsThis) {
  EXPECT_LOX(R"(
    class Foo { init() { print this; } }
    var foo = Foo();
    print foo.init();
  )",
             "Foo instance\nFoo instance\nFoo instance\n");
}

TEST(Initializers, EarlyReturnStillYieldsTheInstance) {
  EXPECT_LOX(R"(
    class Foo {
      init(early) {
        this.a = 1;
        if (early) return;
        this.a = 2;
      }
    }
    print Foo(true).a;
    print Foo(false).a;
  )",
             "1\n2\n");
}

TEST(Initializers, ReturningAValueIsACompileError) {
  EXPECT_LOX_COMPILE_ERROR("class Foo { init() { return 1; } }");
}

TEST(Initializers, WrongArgumentCountIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR("class Foo { init(a) {} } Foo();");
  EXPECT_LOX_RUNTIME_ERROR("class Foo { init(a) {} } Foo(1, 2);");
}

TEST(Initializers, ClassWithoutInitRejectsArguments) {
  EXPECT_LOX_RUNTIME_ERROR("class Foo {} Foo(1);");
}

TEST(Initializers, EachInstanceGetsItsOwnState) {
  EXPECT_LOX(R"(
    class P { init(n) { this.n = n; } }
    var a = P(1); var b = P(2);
    print a.n; print b.n;
  )",
             "1\n2\n");
}

// ---- GC with methods ------------------------------------------------------

TEST(MethodsGC, MethodCallsSurviveCollections) {
  EXPECT_LOX(R"(
    class Acc {
      init() { this.sum = 0; }
      add(n) { this.sum = this.sum + n; }
    }
    var a = Acc();
    for (var i = 0; i < 50000; i = i + 1) {
      a.add(1);
      var junk = "junk" + i;
    }
    print a.sum;
  )",
             "50000\n");
}

TEST(MethodsGC, BoundMethodSurvivesCollection) {
  EXPECT_LOX(R"(
    class P {
      init(n) { this.n = n; }
      get() { return this.n; }
    }
    var g = P("kept").get;
    for (var i = 0; i < 50000; i = i + 1) { var s = "junk" + i; }
    print g();
  )",
             "kept\n");
}