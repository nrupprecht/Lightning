//
// Created by Nathaniel Rupprecht on 12/24/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"
#include <string>

using namespace lightning;
using namespace lightning::formatting;

namespace Testing {
TEST(Formatting, Basic) {
  // No spaces for arguments - just returns the format string.
  EXPECT_NO_THROW(EXPECT_EQ(formatting::Format("No spaces!", "Hi"), "No spaces!"));
  // No arguments - just returns format string.
  EXPECT_NO_THROW(EXPECT_EQ(formatting::Format("One space {}"), "One space {}"));
}

TEST(Formatting, String) {
  EXPECT_EQ(formatting::Format("{} there.", "Hello"), "Hello there.");
  EXPECT_EQ(formatting::Format("{} there.", std::string("Hello")), "Hello there.");
  EXPECT_EQ(formatting::Format("Richard {} York {} battle {} {}", "of", "gave", "in", "vain"),
            "Richard of York gave battle in vain");
}

TEST(Formatting, Integers) {
  EXPECT_EQ(formatting::Format("{} + {} = {}", 1, 2, 3), "1 + 2 = 3");
  EXPECT_EQ(formatting::Format("Print: {}", 'a'), "Print: a");
  EXPECT_EQ(formatting::Format("Print: {}", short{12}), "Print: 12");
  EXPECT_EQ(formatting::Format("Print: {}", 12u), "Print: 12");
  EXPECT_EQ(formatting::Format("Print: {}", 12), "Print: 12");
  EXPECT_EQ(formatting::Format("Print: {}", 12l), "Print: 12");
  EXPECT_EQ(formatting::Format("Print: {}", 12ll), "Print: 12");
  EXPECT_EQ(formatting::Format("Print: {}", 12ul), "Print: 12");
  EXPECT_EQ(formatting::Format("Print: {}", 12ull), "Print: 12");
  EXPECT_EQ(formatting::Format("Print: {:L}X", 1'345'562), "Print: 1,345,562X");
}

TEST(Formatting, FormatIntegerWithCommas) {
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, 120);
    EXPECT_EQ(buffer.ToString(), "120");
    EXPECT_EQ(formatting::Format("{:L}", 120), "120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, -120);
    EXPECT_EQ(buffer.ToString(), "-120");
    EXPECT_EQ(formatting::Format("{:L}", -120), "-120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, 24'998);
    EXPECT_EQ(buffer.ToString(), "24,998");
    EXPECT_EQ(formatting::Format("{:L}", 24'998), "24,998");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, -24'998);
    EXPECT_EQ(buffer.ToString(), "-24,998");
    EXPECT_EQ(formatting::Format("{:L}", -24'998), "-24,998");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, 34'567'890);
    EXPECT_EQ(buffer.ToString(), "34,567,890");
    EXPECT_EQ(formatting::Format("{:L}", 34'567'890), "34,567,890");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, -34'567'890);
    EXPECT_EQ(buffer.ToString(), "-34,567,890");
    EXPECT_EQ(formatting::Format("{:L}", -34'567'890), "-34,567,890");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, 1'234'567'890);
    EXPECT_EQ(buffer.ToString(), "1,234,567,890");
    EXPECT_EQ(formatting::Format("{:L}", 1'234'567'890), "1,234,567,890");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(buffer, -1'234'567'890);
    EXPECT_EQ(buffer.ToString(), "-1,234,567,890");
    EXPECT_EQ(formatting::Format("{:L}", -1'234'567'890), "-1,234,567,890");
  }
}

} // namespace Testing