//
// Created by Nathaniel Rupprecht on 4/15/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace std::string_literals;

namespace {

struct MAYBE_UNUSED Nonstreamable {};

struct MAYBE_UNUSED ToStringable { char c; };

MAYBE_UNUSED inline std::string to_string(const ToStringable x) { return "<"s + x.c + ">"; }

} // namespace <unnamed>

namespace Testing {

TEST(TypeTraits, Basic) {
  EXPECT_TRUE(typetraits::is_ostreamable_v<std::string>);
  EXPECT_TRUE(typetraits::is_ostreamable_v<char[14]>);
  EXPECT_TRUE(typetraits::is_ostreamable_v<bool>);

  EXPECT_FALSE(typetraits::is_ostreamable_v<Nonstreamable>);
}

TEST(Lightning, IsOstreamable) {
  EXPECT_TRUE(typetraits::is_ostreamable_v<std::string>);
  EXPECT_TRUE(typetraits::is_ostreamable_v<char[14]>);
  EXPECT_TRUE(typetraits::is_ostreamable_v<bool>);

  EXPECT_FALSE(typetraits::is_ostreamable_v<Nonstreamable>);
}

TEST(Lightning, Unconst_t) {
  EXPECT_TRUE((std::is_same_v<typetraits::Unconst_t<const char*>, char*>));
  EXPECT_TRUE((std::is_same_v<typetraits::Unconst_t<char*>, char*>));
  EXPECT_TRUE((std::is_same_v<typetraits::Unconst_t<char const* const>, char*>));
  EXPECT_TRUE((std::is_same_v<typetraits::Unconst_t<const double>, double>));
}

TEST(Lightning, IsCstrRelated_v) {
  EXPECT_TRUE(typetraits::IsCstrRelated_v<char*>);
  EXPECT_TRUE(typetraits::IsCstrRelated_v<const char*>);
  EXPECT_TRUE(typetraits::IsCstrRelated_v<char[5]>);
  EXPECT_TRUE(typetraits::IsCstrRelated_v<char[]>);

  EXPECT_FALSE(typetraits::IsCstrRelated_v<int>);
  EXPECT_FALSE(typetraits::IsCstrRelated_v<double>);
}

TEST(Lightning, HasToString) {
  EXPECT_TRUE(lightning::typetraits::has_to_string_v<ToStringable>);

  EXPECT_FALSE(lightning::typetraits::has_to_string_v<std::string>);
  EXPECT_FALSE(lightning::typetraits::has_to_string_v<char[14]>);
  EXPECT_FALSE(lightning::typetraits::has_to_string_v<Nonstreamable>);
}

} // namespace Testing