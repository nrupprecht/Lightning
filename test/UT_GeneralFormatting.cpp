//
// Created by Nathaniel Rupprecht on 7/1/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace std::string_literals;

namespace Testing {
TEST(GeneralFormatting, Basic) {
  auto formatter = formatting::MsgFormatter("{}", formatting::MSG);
  FormattingSettings sink_settings;
  sink_settings.message_terminator = "";

  {
    Record record;
    record.Bundle() << 12;
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "12");
  }
  {
    Record record;
    record.Bundle() << -12;
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "-12");
  }
  {
    Record record;
    record.Bundle() << 12'235'938'546'972'340'928ull;
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "12235938546972340928");
  }
  {
    Record record;
    record.Bundle() << static_cast<short>(12);
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "12");
  }
  {
    Record record;
    record.Bundle() << std::string("Hi there my friends");
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "Hi there my friends");
  }
  {
    Record record;
    record.Bundle() << time::DateTime::YMD_Time(2023'01'01, 12, 30, 56, 12'134);
    memory::MemoryBuffer<char> buffer;
    formatter.Format(record, sink_settings, buffer);
    EXPECT_EQ(buffer.ToString(), "2023-01-01 12:30:56.012134");
  }
}

TEST(GeneralFormatting, CalculateIndentation) {
  formatting::MessageInfo msg_info;

  {
    char message[] = "[Info  ] ";
    auto buffer = message + 9;
    msg_info.total_length = 9;
    EXPECT_EQ(CalculateMessageIndentation(buffer, msg_info), 9u);
  }
  {
    char message[] = "[Info  ]\n>> ";
    auto buffer = message + 12;
    msg_info.total_length = 12;
    EXPECT_EQ(CalculateMessageIndentation(buffer, msg_info), 3u);
  }
}
}
