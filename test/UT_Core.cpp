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
  auto stream1 = std::make_shared<std::ostringstream>();
  auto sink1 = UnlockedSink::From<OstreamSink>(stream1);

  auto stream2 = std::make_shared<std::ostringstream>();
  auto sink2 = UnlockedSink::From<OstreamSink>(stream2);

  auto core = std::make_shared<Core>();
  core->AddSink(sink1);
  core->AddSink(sink2);

  EXPECT_EQ(core->GetSinks().size(), 2ull);
}

} // namespace Testing