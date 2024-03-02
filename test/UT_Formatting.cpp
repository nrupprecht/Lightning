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
    FormatIntegerWithCommas(120, buffer);
    EXPECT_EQ(buffer.ToString(), "120");
    EXPECT_EQ(formatting::Format("{:L}", 120), "120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(-120, buffer);
    EXPECT_EQ(buffer.ToString(), "-120");
    EXPECT_EQ(formatting::Format("{:L}", -120), "-120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(24'998, buffer);
    EXPECT_EQ(buffer.ToString(), "24,998");
    EXPECT_EQ(formatting::Format("{:L}", 24'998), "24,998");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(-24'998, buffer);
    EXPECT_EQ(buffer.ToString(), "-24,998");
    EXPECT_EQ(formatting::Format("{:L}", -24'998), "-24,998");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(34'567'890, buffer);
    EXPECT_EQ(buffer.ToString(), "34,567,890");
    EXPECT_EQ(formatting::Format("{:L}", 34'567'890), "34,567,890");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(-34'567'890, buffer);
    EXPECT_EQ(buffer.ToString(), "-34,567,890");
    EXPECT_EQ(formatting::Format("{:L}", -34'567'890), "-34,567,890");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(1'234'567'890, buffer);
    EXPECT_EQ(buffer.ToString(), "1,234,567,890");
    EXPECT_EQ(formatting::Format("{:L}", 1'234'567'890), "1,234,567,890");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatIntegerWithCommas(-1'234'567'890, buffer);
    EXPECT_EQ(buffer.ToString(), "-1,234,567,890");
    EXPECT_EQ(formatting::Format("{:L}", -1'234'567'890), "-1,234,567,890");
  }
}

TEST(Formatting, FormatHex) {
  {
    memory::MemoryBuffer<char> buffer;
    FormatHex(0xF2F, buffer);
    EXPECT_EQ(buffer.ToString(), "0xF2F");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatHex(static_cast<uint64_t>(0xAAAA), buffer);
    EXPECT_EQ(buffer.ToString(), "0xAAAA");
  }
}

TEST(Formatting, FormatInteger_Errors) {
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_ANY_THROW(FormatInteger(":", 120, buffer));
  }
  {
    // Left alignment by default
    memory::MemoryBuffer<char> buffer;
    EXPECT_ANY_THROW(FormatInteger(":x10", 120, buffer));
  }
}

TEST(Formatting, FormatInteger_Alignment) {
  {
    // No instructions - left alignment by default
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger("", 120, buffer));
    EXPECT_EQ(buffer.ToString(), "120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger("", -120, buffer));
    EXPECT_EQ(buffer.ToString(), "-120");
  }
  {
    // Left alignment by default
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":10", 120, buffer));
    EXPECT_EQ(buffer.ToString(), "120       ");
  }
  {
    // Left alignment by default
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":10", -120, buffer));
    EXPECT_EQ(buffer.ToString(), "-120      ");
  }
  {
    // Left alignment, no padding. Useless, but legal.
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":<", 120, buffer));
    EXPECT_EQ(buffer.ToString(), "120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":<3", 120, buffer));
    EXPECT_EQ(buffer.ToString(), "120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":<3", -120, buffer));
    EXPECT_EQ(buffer.ToString(), "-120");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":<5", 120, buffer));
    EXPECT_EQ(buffer.ToString(), "120  ");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":<5", -120, buffer));
    EXPECT_EQ(buffer.ToString(), "-120 ");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":>6", 145, buffer));
    EXPECT_EQ(buffer.ToString(), "   145");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":>6", -145, buffer));
    EXPECT_EQ(buffer.ToString(), "  -145");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":^7", 145, buffer));
    EXPECT_EQ(buffer.ToString(), "  145  ");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":^7", -145, buffer));
    EXPECT_EQ(buffer.ToString(), " -145  ");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":^0", 145, buffer));
    EXPECT_EQ(buffer.ToString(), "145");
  }
}

TEST(Formatting, FormatInteger_FillChar) {
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":*^7", 145, buffer));
    EXPECT_EQ(buffer.ToString(), "**145**");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":x<7", -145, buffer));
    EXPECT_EQ(buffer.ToString(), "-145xxx");
  }
}

TEST(Formatting, FormatInteger_Separators) {
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":<9L", 10'000, buffer));
    EXPECT_EQ(buffer.ToString(), "10,000   ");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":x<9L", 14'573, buffer));
    EXPECT_EQ(buffer.ToString(), "14,573xxx");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":x<9L", -14'573, buffer));
    EXPECT_EQ(buffer.ToString(), "-14,573xx");
  }
}

} // namespace Testing