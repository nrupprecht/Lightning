//
// Created by Nathaniel Rupprecht on 12/14/23.
//


#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;

namespace Testing {

TEST(HybridVector, Basic) {

  memory::HybridVector<int, 3> hvector{};

  EXPECT_EQ(hvector.Size(), 0);
  EXPECT_TRUE(hvector.Empty());

  hvector.PushBack(1);
  EXPECT_EQ(hvector.Size(), 1);
  EXPECT_EQ(hvector.HeapSize(), 0);
  EXPECT_FALSE(hvector.Empty());
  EXPECT_EQ(hvector[0], 1);
  EXPECT_EQ(hvector.Back(), 1);

  hvector.PushBack(3);
  EXPECT_EQ(hvector.Size(), 2);
  EXPECT_EQ(hvector.HeapSize(), 0);
  EXPECT_FALSE(hvector.Empty());
  EXPECT_EQ(hvector[1], 3);
  EXPECT_EQ(hvector.Back(), 3);

  hvector.PushBack(5);
  EXPECT_EQ(hvector.Size(), 3);
  EXPECT_EQ(hvector.HeapSize(), 0);
  EXPECT_FALSE(hvector.Empty());
  EXPECT_EQ(hvector[2], 5);
  EXPECT_EQ(hvector.Back(), 5);

  hvector.PushBack(7);
  EXPECT_EQ(hvector.Size(), 4);
  EXPECT_EQ(hvector.HeapSize(), 1);
  EXPECT_FALSE(hvector.Empty());
  EXPECT_EQ(hvector[3], 7);
  EXPECT_EQ(hvector.Back(), 7);
}

}