//
// Created by Nathaniel Rupprecht on 1/14/24.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace std::string_literals;


namespace Testing {

TEST(Sink, GetWrappedSink) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = NewSink<OstreamSink>(stream);
  sink->GetBackend().GetFormattingSettings().needs_formatting = true;

  {
    auto locked_backend = sink->GetLockedBackend();
    EXPECT_TRUE(sink->IsLocked());
    locked_backend->GetFormattingSettings().needs_formatting = false;

    ASSERT_TRUE(locked_backend.As<OstreamSink>());
    ASSERT_FALSE(locked_backend.As<FileSink>());
    locked_backend.As<OstreamSink>()->GetStream() << "Hello world!";
  }
  EXPECT_FALSE(sink->IsLocked());
  ASSERT_FALSE(sink->GetBackend().GetFormattingSettings().needs_formatting);
}

}  // namespace Testing