//
// Created by Nathaniel Rupprecht on 4/23/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"
#include <string>

using namespace lightning;


namespace Testing {

TEST(Core, AddSink) {
  std::ostringstream stream1;
  auto sink1 = std::make_shared<OstreamSink>(stream1);

  std::ostringstream stream2;
  auto sink2 = std::make_shared<OstreamSink>(stream2);

  auto core = std::make_shared<Core>();
  core->AddSink(sink1);
  core->AddSink(sink2);

  EXPECT_EQ(core->GetSinks().size(), 2ull);
}

} // namespace Testing