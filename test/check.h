// A tiny test runner: TEST(name) { CHECK(...); CHECK_EQ(a, b); }
#pragma once
#include <stdio.h>

struct TestCase { const char *name; void (*fn)(); TestCase *next; };
inline TestCase *&testList() { static TestCase *head = nullptr; return head; }
inline int &testFailures() { static int n = 0; return n; }

struct TestRegistrar {
  TestRegistrar(TestCase *t) {
    TestCase **p = &testList();          // keep file order
    while (*p) p = &(*p)->next;
    *p = t;
  }
};

#define TEST(name)                                                  \
  static void name();                                               \
  static TestCase name##_case = { #name, name, nullptr };           \
  static TestRegistrar name##_reg(&name##_case);                    \
  static void name()

#define CHECK(cond)                                                 \
  do {                                                              \
    if (!(cond)) {                                                  \
      testFailures()++;                                             \
      printf("    FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
    }                                                               \
  } while (0)

#define CHECK_EQ(a, b)                                              \
  do {                                                              \
    long long a_ = (long long)(a), b_ = (long long)(b);             \
    if (a_ != b_) {                                                 \
      testFailures()++;                                             \
      printf("    FAIL %s:%d: %s == %s (got %lld, want %lld)\n",    \
             __FILE__, __LINE__, #a, #b, a_, b_);                   \
    }                                                               \
  } while (0)

// Runs every TEST in registration order; returns the process exit code.
inline int runTests(const char *suite) {
  int failedTests = 0, count = 0;
  for (TestCase *t = testList(); t; t = t->next) {
    int before = testFailures();
    t->fn();
    bool ok = testFailures() == before;
    printf("  %s  %s\n", ok ? "pass" : "FAIL", t->name);
    failedTests += !ok;
    count++;
  }
  printf("%s: %d/%d passed\n", suite, count - failedTests, count);
  return failedTests ? 1 : 0;
}
