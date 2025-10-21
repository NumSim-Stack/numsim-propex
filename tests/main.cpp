#include "gtest/gtest.h"
#include "key_traits_test.h"
#include "registry_test.h"

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
