//
// Created by Nathaniel Rupprecht on 7/1/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace std::string_literals;


namespace Testing {

TEST(SegmentStorage, StackVsHeap) {
  { // Object small enough to go on the stack.
    SegmentStorage container;
    EXPECT_FALSE(container.HasData());
    container.Create<Segment<int>>(15);
    EXPECT_TRUE(container.HasData());
    EXPECT_TRUE(container.IsUsingBuffer());
  }
  { // Object too large to go on the stack.
    SegmentStorage container;
    container.Create<AnsiColor8Bit<int>>(55, formatting::AnsiForegroundColor::Green);

    EXPECT_FALSE(container.IsUsingBuffer());
  }
}

} // namespace Testing
