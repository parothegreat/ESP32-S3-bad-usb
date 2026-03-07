#include <unity.h>

void setUp(void) {
  // Set up test environment
}

void tearDown(void) {
  // Clean up after tests
}

void test_basic_math(void) {
  TEST_ASSERT_EQUAL(4, 2 + 2);
}

void test_simple_assertion(void) {
  TEST_ASSERT_TRUE(1 == 1);
}

