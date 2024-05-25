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
    auto locked_sink = sink->GetLockedSink();
    EXPECT_TRUE(sink->IsLocked());
    locked_sink->GetBackend().GetFormattingSettings().needs_formatting = false;

    ASSERT_TRUE(locked_sink->GetBackendAs<OstreamSink>());
    ASSERT_FALSE(locked_sink->GetBackendAs<FileSink>());
    locked_sink->GetBackendAs<OstreamSink>()->GetStream() << "Hello world!";
  }
  EXPECT_FALSE(sink->IsLocked());
  ASSERT_FALSE(sink->GetBackend().GetFormattingSettings().needs_formatting);
}

TEST(Sink, Clone) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = NewSink<OstreamSink>(stream);

  std::shared_ptr<Sink> cloned_sink;
  EXPECT_NO_THROW(cloned_sink = sink->Clone());

  EXPECT_NO_THROW(cloned_sink->GetBackend().CopySettings(sink->GetBackend()));
}

}  // namespace Testing