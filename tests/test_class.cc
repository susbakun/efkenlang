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

TEST(Inheritance, SubclassInheritsMethods) {
  EXPECT_LOX(R"(
    class Animal { speak() { print "..."; } }
    class Dog < Animal {}
    Dog().speak();
  )",
             "...\n");
}

TEST(Inheritance, SubclassMethodOverridesWithoutTouchingTheSuperclass) {
  EXPECT_LOX(R"(
    class A { m() { return "A"; } }
    class B < A { m() { return "B"; } }
    print B().m();
    print A().m();
  )",
             "B\nA\n");
}

TEST(Inheritance, MethodsAreInheritedThroughSeveralLevels) {
  EXPECT_LOX(R"(
    class A { a() { return "a"; } }
    class B < A { b() { return "b"; } }
    class C < B { c() { return "c"; } }
    var c = C();
    print c.a() + c.b() + c.c();
  )",
             "abc\n");
}

TEST(Inheritance, InheritedMethodsSeeTheSubclassFields) {
  EXPECT_LOX(R"(
    class A { describe() { return "I am " + this.name; } }
    class B < A { init(name) { this.name = name; } }
    print B("bee").describe();
  )",
             "I am bee\n");
}

TEST(Inheritance, InstancePrintsItsOwnClassName) {
  EXPECT_LOX("class A {} class B < A {} print B();", "B instance\n");
}

TEST(Inheritance, SubclassInitializerOverridesTheSuperclassOne) {
  EXPECT_LOX(R"(
    class A { init() { this.who = "A"; } }
    class B < A { init() { this.who = "B"; } }
    print B().who;
  )",
             "B\n");
}

// ---- super ----------------------------------------------------------------

TEST(Super, CallsTheSuperclassMethod) {
  EXPECT_LOX(R"(
    class A { greet() { print "A"; } }
    class B < A { greet() { super.greet(); print "B"; } }
    B().greet();
  )",
             "A\nB\n");
}

TEST(Super, UsesTheReceiverAsThis) {
  EXPECT_LOX(R"(
    class A { who() { return this.name; } }
    class B < A {
      init() { this.name = "b"; }
      get() { return super.who(); }
    }
    print B().get();
  )",
             "b\n");
}

// `super` is resolved from the class where the method is *written*, not from
// the class of the receiver.
TEST(Super, IsBoundToTheEnclosingClassNotTheReceiversClass) {
  EXPECT_LOX(R"(
    class Base { hello() { print "Base hello"; } }
    class Mid < Base {
      hello() { print "Mid hello"; }
      test() { super.hello(); }
    }
    class Leaf < Mid {}
    Leaf().test();
  )",
             "Base hello\n");
}

TEST(Super, ChainsThroughEveryLevel) {
  EXPECT_LOX(R"(
    class A { name() { return "A"; } }
    class B < A { name() { return super.name() + "B"; } }
    class C < B { name() { return super.name() + "C"; } }
    print C().name();
  )",
             "ABC\n");
}

TEST(Super, AccessWithoutCallingGivesABoundMethod) {
  EXPECT_LOX(R"(
    class A { hi() { return "hi from A"; } }
    class B < A { grab() { return super.hi; } }
    var f = B().grab();
    print f();
  )",
             "hi from A\n");
}

TEST(Super, WorksInsideAClosure) {
  EXPECT_LOX(R"(
    class A { say() { print "A"; } }
    class B < A {
      run() {
        fun inner() { super.say(); }
        inner();
      }
    }
    B().run();
  )",
             "A\n");
}

TEST(Super, InitChain) {
  EXPECT_LOX(R"(
    class A { init(a) { this.a = a; } }
    class B < A {
      init(a, b) {
        super.init(a);
        this.b = b;
      }
    }
    var o = B(1, 2);
    print o.a + o.b;
    print o;
  )",
             "3\nB instance\n");
}

TEST(Super, LooksOnlyAtMethodsNotFields) {
  EXPECT_LOX_RUNTIME_ERROR(R"(
    class A {}
    class B < A {
      init() { this.x = 1; }
      get() { return super.x; }
    }
    B().get();
  )");
}

TEST(Super, MissingSuperMethodIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR(R"(
    class A {}
    class B < A { m() { super.nope(); } }
    B().m();
  )");
}

TEST(Super, SuperInitWithWrongArityIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR(R"(
    class A { init(a) {} }
    class B < A { init() { super.init(); } }
    B();
  )");
}

TEST(Super, LocalClassesCanInherit) {
  EXPECT_LOX(R"(
    fun make() {
      class A { hi() { print "A hi"; } }
      class B < A { hi() { super.hi(); print "B hi"; } }
      return B();
    }
    make().hi();
  )",
             "A hi\nB hi\n");
}

// ---- errors ---------------------------------------------------------------

TEST(InheritanceErrors, ClassCannotInheritFromItself) {
  EXPECT_LOX_COMPILE_ERROR("class A < A {}");
}

TEST(InheritanceErrors, SuperclassMustBeAClass) {
  EXPECT_LOX_RUNTIME_ERROR(R"(
    var notAClass = "nope";
    class B < notAClass {}
  )");
}

TEST(InheritanceErrors, SuperclassCannotBeAnInstance) {
  EXPECT_LOX_RUNTIME_ERROR("class A {} var a = A(); class B < a {}");
}

TEST(InheritanceErrors, UndefinedSuperclassIsRuntimeError) {
  EXPECT_LOX_RUNTIME_ERROR("class B < Missing {}");
}

TEST(SuperErrors, OutsideAClassIsACompileError) {
  EXPECT_LOX_COMPILE_ERROR("print super.x;");
  EXPECT_LOX_COMPILE_ERROR("fun f() { super.x(); }");
}

TEST(SuperErrors, InAClassWithoutASuperclassIsACompileError) {
  EXPECT_LOX_COMPILE_ERROR("class A { m() { super.m(); } }");
}

TEST(SuperErrors, MustBeFollowedByADot) {
  EXPECT_LOX_COMPILE_ERROR("class A {} class B < A { m() { super; } }");
}

// ---- GC with inheritance --------------------------------------------------

TEST(InheritanceGC, SuperCallsSurviveCollections) {
  EXPECT_LOX(R"(
    class A {
      init() { this.n = 0; }
      bump() { this.n = this.n + 1; }
    }
    class B < A { init() { super.init(); } bump() { super.bump(); super.bump(); } }
    var b = B();
    for (var i = 0; i < 50000; i = i + 1) {
      b.bump();
      var junk = "junk" + i;
    }
    print b.n;
  )",
             "1e+05\n");
}

// After make() returns, class A is reachable only through the closed `super`
// upvalue and the copied-down methods.
TEST(InheritanceGC, SuperclassReachableOnlyThroughUpvalueSurvives) {
  EXPECT_LOX(R"(
    fun make() {
      class A { hi() { return "hi"; } }
      class B < A { shout() { return super.hi() + "!"; } }
      return B();
    }
    var b = make();
    for (var i = 0; i < 50000; i = i + 1) { var s = "junk" + i; }
    print b.shout();
  )",
             "hi!\n");
}

TEST(InheritanceGC, BoundSuperMethodSurvivesCollection) {
  EXPECT_LOX(R"(
    class A { get() { return "kept"; } }
    class B < A { grab() { return super.get; } }
    var g = B().grab();
    for (var i = 0; i < 50000; i = i + 1) { var s = "junk" + i; }
    print g();
  )",
             "kept\n");
}