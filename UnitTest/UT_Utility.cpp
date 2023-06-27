//
// Created by Nathaniel Rupprecht on 6/26/23.
//

//
// Created by Nathaniel Rupprecht on 4/15/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/GreasedLightning.h"

using namespace lightning;
using namespace std::string_literals;


namespace Testing {

TEST(Utility, NumberOfDigits) {
  EXPECT_EQ(formatting::NumberOfDigits(0ull), 1);
  EXPECT_EQ(formatting::NumberOfDigits(1ull), 1);
  EXPECT_EQ(formatting::NumberOfDigits(99ull), 2);
  EXPECT_EQ(formatting::NumberOfDigits(100ull), 3);
  EXPECT_EQ(formatting::NumberOfDigits(101ull), 3);
  EXPECT_EQ(formatting::NumberOfDigits(100'002ull), 6);
}

TEST(Utility, CopyPaddedInt) {
  std::string buffer(8, ' ');
  auto start = &buffer[0], end = start + buffer.size();
  formatting::CopyPaddedInt(start, end, 56, 8);
  EXPECT_EQ(buffer, "00000056");

  formatting::CopyPaddedInt(start, end, 11, 3);
  EXPECT_EQ(buffer, "01100056");

  formatting::CopyPaddedInt(start + 3, end, 2, 4);
  EXPECT_EQ(buffer, "01100026");
}

TEST(Utility, FormatDateTo) {
  std::string buffer(26, ' ');
  formatting::FormatDateTo(&buffer[0], &buffer[0] + 26, time::DateTime::YMD_Time(2023'06'01, 12, 15, 6, 10023));
  EXPECT_EQ(buffer, "2023-06-01 12:15:06.010023");
}

} // namespace Testing