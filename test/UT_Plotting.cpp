//
// Created by Nathaniel Rupprecht on 3/17/24.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Plotting.h"

using namespace lightning;
using namespace std::string_literals;

namespace Testing {

TEST(Plotting, BasicSanity) {
  auto figure = std::make_unique<MatplotlibSerializingFigure>(10, 8, "direct-to-ree");

  EXPECT_EQ(figure->GetWidth(), 10);
  EXPECT_EQ(figure->GetHeight(), 8);
  EXPECT_EQ(figure->GetWriteDirectory().string(), "direct-to-ree");
}

}  // namespace Testing