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

  // Escaped '{' character
  EXPECT_NO_THROW(EXPECT_EQ(formatting::Format("One {{space} {}", 1), "One {space} 1"));
}

TEST(Formatting, String) {
  EXPECT_EQ(formatting::Format("{} there.", "Hello"), "Hello there.");
  EXPECT_EQ(formatting::Format("{} there.", std::string("Hello")), "Hello there.");
  EXPECT_EQ(formatting::Format("Richard {} York {} battle {} {}", "of", "gave", "in", "vain"),
            "Richard of York gave battle in vain");
  EXPECT_EQ(formatting::Format("{} there", "Hello"), "Hello there");
  EXPECT_EQ(formatting::Format("{:?} there", "Hello"), "\"Hello\" there");
  EXPECT_EQ(formatting::Format("{:_^7} there", "Hello"), "_Hello_ there");
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

TEST(Formatting, Colors) {
  EXPECT_EQ(formatting::Format("When in {@RED}Rome{@RESET}, do as the {@GREEN}Greeks{@RESET} do."),
            "When in \033[31mRome\033[0m, do as the \033[32mGreeks\033[0m do.");

  // Looks like using a formatter, but is not actually in all places.
  EXPECT_EQ(formatting::Format("When in {@REDR}Rome{@RRESET}, do as the {@GREEN}Greeks{@RESET} do."),
            "When in {@REDR}Rome{@RRESET}, do as the \033[32mGreeks\033[0m do.");

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
  {
    memory::MemoryBuffer<char> buffer;
    FormatHex(static_cast<uint64_t>(0xABAB), buffer, false);
    EXPECT_EQ(buffer.ToString(), "0xabab");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatHex(static_cast<uint64_t>(0xABAB), buffer, false, PrefixFmtType::Upper);
    EXPECT_EQ(buffer.ToString(), "0Xabab");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatHex(static_cast<uint64_t>(0xABAB), buffer, false, PrefixFmtType::None);
    EXPECT_EQ(buffer.ToString(), "abab");
  }
  {
    memory::MemoryBuffer<char> buffer;
    FormatHex(static_cast<uint64_t>(0xABAB), buffer, true, PrefixFmtType::None);
    EXPECT_EQ(buffer.ToString(), "ABAB");
  }
}

TEST(Formatting, FormatInteger_Errors) {
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_ANY_THROW(FormatInteger("<", 120, buffer));
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
    // No options.
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":", 120, buffer));
    EXPECT_EQ(buffer.ToString(), "120");
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
  {
    // NOTE: I don't yet support using custom separators in the formatting, but you can specify it
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":x<9L:,", -14'573, buffer));
    EXPECT_EQ(buffer.ToString(), "-14,573xx");
  }
  {
    // NOTE: I don't yet support using custom separators in the formatting, but you can specify it
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatInteger(":x<9L:_", -14'573, buffer));
    // When implemented, this should be "-14_573xx"
    EXPECT_EQ(buffer.ToString(), "-14,573xx");
  }
}

TEST(Formatting, FormatString) {
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString("", "Hello", buffer));
    EXPECT_EQ(buffer.ToString(), "Hello");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":", std::string("Hello"), buffer));
    EXPECT_EQ(buffer.ToString(), "Hello");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":10", "Hello", buffer));
    EXPECT_EQ(buffer.ToString(), "Hello     ");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":>10", "Hello", buffer));
    EXPECT_EQ(buffer.ToString(), "     Hello");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":^10", "Hello", buffer));
    EXPECT_EQ(buffer.ToString(), "  Hello   ");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":*>10", "Hello", buffer));
    EXPECT_EQ(buffer.ToString(), "*****Hello");
  }
}

TEST(Formatting, FormatString_Debug) {
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":?", "Hello", buffer));
    EXPECT_EQ(buffer.ToString(), "\"Hello\"");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":?", "Hello\n\tWorld", buffer));
    EXPECT_EQ(buffer.ToString(), "\"Hello\\n\\tWorld\"");
  }
  {
    memory::MemoryBuffer<char> buffer;
    EXPECT_NO_THROW(FormatString(":x^20?", "Hello\n\tWorld", buffer));
    EXPECT_EQ(buffer.ToString(), "xx\"Hello\\n\\tWorld\"xx");
  }
}

} // namespace Testing