//
// Created by Nathaniel Rupprecht on 12/24/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"
#include <string>

using namespace lightning;
using namespace lightning::memory;

namespace Testing {
TEST(MemoryBuffer, MemoryBuffer) {
  MemoryBuffer<char, 10> buffer;
  AppendBuffer(buffer, "Hello world, how are you?");
  EXPECT_EQ(buffer.ToString(), "Hello world, how are you?");
  EXPECT_EQ(buffer.Size(), 25);

  EXPECT_EQ(buffer.Data()[buffer.Size()], '\0');
}

TEST(MemoryBuffer, StringMemoryBuffer) {
  StringMemoryBuffer buffer;
  AppendBuffer(buffer, "Hello world, how are you?");
  EXPECT_EQ(buffer.Size(), 25);

  auto ptr = buffer.Data();
  auto str = buffer.MoveString();
  EXPECT_EQ(ptr, str.data());
  EXPECT_EQ(str, "Hello world, how are you?");
  EXPECT_EQ(str.size(), 25);
  EXPECT_EQ(buffer.Size(), 0);

  buffer.PushBack('A');
  EXPECT_EQ(buffer.Size(), 1);
  buffer.PushBack('N');
  EXPECT_EQ(buffer.Size(), 2);
  buffer.PushBack('D');
  EXPECT_EQ(buffer.Size(), 3);
}
}
