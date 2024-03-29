//
// Created by Nathaniel Rupprecht on 6/23/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace std::string_literals;

namespace Testing {

TEST(MsgFormatter, Basic) {
  auto formatter = formatting::MsgFormatter("[{}] [{}] {}",
                                            formatting::SeverityAttributeFormatter{},
                                            formatting::DateTimeAttributeFormatter{},
                                            formatting::MSG);

  Record record;
  record.Attributes().basic_attributes.level = Severity::Info;
  record.Attributes().basic_attributes.time_stamp = time::DateTime(2023, 12, 31, 12, 49, 30, 100'000);
  record.Bundle() << "Hello world!";

  FormattingSettings sink_settings;
  {
    // No colors
    sink_settings.has_virtual_terminal_processing = false;
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "[Info   ] [2023-12-31 12:49:30.100000] Hello world!\n");
  }
  {
    // With colors.
    sink_settings.has_virtual_terminal_processing = true;
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "[\x1B[32mInfo   \x1B[0m] [2023-12-31 12:49:30.100000] Hello world!\n");
  }
}

TEST(MsgFormatter, Default) {
  auto formatter = formatting::MakeStandardFormatter();

  Record record;
  record.Attributes().basic_attributes.level = Severity::Info;
  record.Attributes().basic_attributes.time_stamp = time::DateTime(2023, 12, 31, 12, 49, 30, 100'000);
  record.Bundle() << "Hello world!";

  FormattingSettings sink_settings;
  {
    // No colors
    sink_settings.has_virtual_terminal_processing = false;
    memory::MemoryBuffer<char> buffer;
    formatter->Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "[Info   ] [2023-12-31 12:49:30.100000] Hello world!\n");
  }
}

} // namespace Testing
