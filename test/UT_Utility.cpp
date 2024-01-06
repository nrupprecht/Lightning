//
// Created by Nathaniel Rupprecht on 6/26/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace std::string_literals;

namespace testing {

struct TestStruct {
  int x;
  int y;
  int z;
};

// To-string function for TestStruct
std::string to_string(const TestStruct& value) {
  return lightning::formatting::Format("TestStruct({},{},{})", value.x, value.y, value.z);
}

} // namespace

namespace Testing {


TEST(Utility, HasToString) {
  EXPECT_TRUE(typetraits::has_to_string_v<int>);
  EXPECT_TRUE(typetraits::has_std_to_string_v<int>);

  EXPECT_TRUE(typetraits::has_to_string_v<char>);
  EXPECT_TRUE(typetraits::has_std_to_string_v<int>);

  EXPECT_TRUE(typetraits::has_to_string_v<testing::TestStruct>);
  EXPECT_FALSE(typetraits::has_std_to_string_v<testing::TestStruct>);
}

TEST(Utility, NumberOfDigits_ULL) {
  EXPECT_EQ(formatting::NumberOfDigits(0ull), 1u);
  EXPECT_EQ(formatting::NumberOfDigits(1ull), 1u);
  EXPECT_EQ(formatting::NumberOfDigits(99ull), 2u);
  EXPECT_EQ(formatting::NumberOfDigits(100ull), 3u);
  EXPECT_EQ(formatting::NumberOfDigits(101ull), 3u);
  EXPECT_EQ(formatting::NumberOfDigits(100'002ull), 6u);

  EXPECT_EQ(formatting::NumberOfDigits(12'235'938'546'972'340'928ull), 20u);
}

TEST(Utility, NumberOfDigits_LL) {
  EXPECT_EQ(formatting::NumberOfDigits(0ll), 1u);
  EXPECT_EQ(formatting::NumberOfDigits(-0ll), 1u);
  EXPECT_EQ(formatting::NumberOfDigits(1ll), 1u);
  EXPECT_EQ(formatting::NumberOfDigits(-1ll), 1u);
  EXPECT_EQ(formatting::NumberOfDigits(99ll), 2u);
  EXPECT_EQ(formatting::NumberOfDigits(-99ll), 2u);
  EXPECT_EQ(formatting::NumberOfDigits(100ll), 3u);
  EXPECT_EQ(formatting::NumberOfDigits(-100ll), 3u);
  EXPECT_EQ(formatting::NumberOfDigits(101ll), 3u);
  EXPECT_EQ(formatting::NumberOfDigits(-101ll), 3u);
  EXPECT_EQ(formatting::NumberOfDigits(100'002ll), 6u);
  EXPECT_EQ(formatting::NumberOfDigits(-100'002ll), 6u);
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
  {
    std::string buffer(26, ' ');
    formatting::FormatDateTo(&buffer[0],
                             &buffer[0] + 26,
                             time::DateTime::YMD_Time(2023'06'01, 12, 15, 6, 10023));
    EXPECT_EQ(buffer, "2023-06-01 12:15:06.010023");
  }
  {
    // You can format into the middle of a buffer.
    std::string buffer(30, 'x');
    formatting::FormatDateTo(&buffer[0] + 2,
                             &buffer[0] + 30,
                             time::DateTime::YMD_Time(2023'01'01, 1, 1, 1, 5));
    EXPECT_EQ(buffer, "xx2023-01-01 01:01:01.000005xx");
  }
  {
    // Not enough space in the buffer.
    std::string buffer(20, ' ');
    EXPECT_ANY_THROW(
        formatting::FormatDateTo(&buffer[0], &buffer[0] + 20, time::DateTime::YMD_Time(2023'01'01, 1, 1, 1, 5)
        ));
  }
}
} // namespace Testing
