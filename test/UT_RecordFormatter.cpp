//
// Created by Nathaniel Rupprecht on 6/22/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace std::string_literals;

namespace Testing {

TEST(RecordFormatter, Basic) {
  formatting::RecordFormatter record_formatter;
  EXPECT_EQ(record_formatter.NumSegments(), 1u);
  record_formatter.ClearSegments();
  EXPECT_EQ(record_formatter.NumSegments(), 0u);

  Record record;
  record.Attributes().basic_attributes.level = Severity::Info;

  record.Bundle() << "Hello, world!";
  {
    memory::MemoryBuffer<char> buffer;
    record_formatter.Format(record, {}, buffer);
    EXPECT_EQ(buffer.ToString(), "\n");
  }

  record_formatter.AddMsgSegment();
  EXPECT_EQ(record_formatter.NumSegments(), 1u);
  {
    memory::MemoryBuffer<char> buffer;
    record_formatter.Format(record, {}, buffer);
    EXPECT_EQ(buffer.ToString(), "Hello, world!\n");
  }

  record_formatter.ClearSegments();
  record_formatter.AddLiteralSegment("[");
  record_formatter.AddAttributeFormatter(std::make_shared<formatting::SeverityAttributeFormatter>());
  record_formatter.AddLiteralSegment("] ");
  record_formatter.AddMsgSegment();
  EXPECT_EQ(record_formatter.NumSegments(), 4u);

  { // With colors
    FormattingSettings settings;
    settings.has_virtual_terminal_processing = true;
    memory::MemoryBuffer<char> buffer;
    record_formatter.Format(record, settings, buffer);
    EXPECT_EQ(buffer.ToString(), "[\x1B[32mInfo   \x1B[0m] Hello, world!\n");
  }
  { // No colors
    FormattingSettings settings;
    settings.has_virtual_terminal_processing = false;
    memory::MemoryBuffer<char> buffer;
    record_formatter.Format(record, settings, buffer);
    EXPECT_EQ(buffer.ToString(), "[Info   ] Hello, world!\n");
  }
}


TEST(RecordFormatter, ManySegments) {
  // Make sure that the vector resizing does not cause any issues with the segments, which will have to be moved around

  // Stream colored string segments into the logger.
  RefBundle bundle;
  bundle
      << AnsiColor8Bit(0, formatting::AnsiForegroundColor::Green)
      << " = 1 "
      << AnsiColor8Bit(2, formatting::AnsiForegroundColor::Green)
      << " = 3 "
      << AnsiColor8Bit(4, formatting::AnsiForegroundColor::Green)
      << " = 5 "
      << AnsiColor8Bit(6, formatting::AnsiForegroundColor::Green)
      << " = 7 "
      << AnsiColor8Bit(8, formatting::AnsiForegroundColor::Green)
      << " = 9 "
      << AnsiColor8Bit(10, formatting::AnsiForegroundColor::Green)
      << " = 11 "
      << AnsiColor8Bit(12, formatting::AnsiForegroundColor::Green)
      << " = 13 "
      << AnsiColor8Bit(12, formatting::AnsiForegroundColor::Green)
      << " = 15 "
      << AnsiColor8Bit(16, formatting::AnsiForegroundColor::Green)
      << " = 17 ";
  EXPECT_FALSE(bundle.NeedsMessageIndentation());
}

}