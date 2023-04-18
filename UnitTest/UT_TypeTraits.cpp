//
// Created by Nathaniel Rupprecht on 4/15/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

namespace {

struct Nonstreamable {};

}

namespace Testing {

TEST(TypeTraits, Basic) {
  EXPECT_TRUE(lightning::typetraits::is_ostreamable_v<std::string>);
  EXPECT_TRUE(lightning::typetraits::is_ostreamable_v<char[14]>);
  EXPECT_TRUE(lightning::typetraits::is_ostreamable_v<bool>);

  EXPECT_FALSE(lightning::typetraits::is_ostreamable_v<Nonstreamable>);
}


} // namespace Testing