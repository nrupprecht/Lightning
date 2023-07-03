//
// Created by Nathaniel Rupprecht on 7/1/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/GreasedLightning.h"

using namespace lightning;
using namespace std::string_literals;


namespace Testing {

TEST(SegmentStorage, StackVsHeap) {
  formatting::MessageInfo msg_info{};

  { // Object small enough to go on the stack.
    SegmentStorage container;
    EXPECT_FALSE(container.HasData());
    container.Create<Segment<int>>(15, nullptr, nullptr);
    EXPECT_TRUE(container.HasData());
    EXPECT_TRUE(container.IsUsingBuffer());

    FormattingSettings sink_settings;
    EXPECT_EQ(container.Get()->SizeRequired(sink_settings, msg_info), 2u);
  }
  { // Object too large to go on the stack.
    SegmentStorage container;
    container.Create<AnsiColor8Bit<int>>(55, formatting::AnsiForegroundColor::Green);

    EXPECT_FALSE(container.IsUsingBuffer());

    EXPECT_EQ(sizeof(AnsiColor8Bit<int>), 48u);

    FormattingSettings sink_settings;
    sink_settings.has_virtual_terminal_processing = true;
    EXPECT_EQ(container.Get()->SizeRequired(sink_settings, msg_info), 11u);
  }
}

} // namespace Testing
