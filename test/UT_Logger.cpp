//
// Created by Nathaniel Rupprecht on 4/16/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"
#include <string>

using namespace lightning;
using namespace std::string_literals;

namespace {

struct ToStringable { char c; };

std::string to_string(const ToStringable x) {
  return std::string{'<', x.c, '>'};
}

} // namespace <unnamed>


namespace Testing {

TEST(Logger, CloneNoCore) {
  Logger logger(NoCore);
  EXPECT_NO_THROW(auto other_logger = logger.Clone());
}

TEST(Logger, RecordHandler_Streaming) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

  Logger logger(sink);

  LOG_SEV_TO(logger, Info) << ToStringable{'h'};

  EXPECT_EQ(stream->str(), "<h>\n");
}

TEST(Logger, OstreamSink) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

  Logger logger;
  logger.GetCore()->AddSink(std::move(sink));

  LOG_SEV_TO(logger, Info) << "Hello world!";
  EXPECT_EQ(stream->str(), "Hello world!\n");
  stream->str("");
}

TEST(Logger, SeverityLogger) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("[{}]: {}",
                                    formatting::SeverityAttributeFormatter{},
                                    formatting::MSG));

  Logger logger(std::move(sink));

  LOG_SEV_TO(logger, Debug) << "Goodbye, my friends.";
  EXPECT_EQ(stream->str(), "[Debug  ]: Goodbye, my friends.\n");
  stream->str("");

  LOG_SEV_TO(logger, Info) << "Goodbye, my friends.";
  EXPECT_EQ(stream->str(), "[Info   ]: Goodbye, my friends.\n");
  stream->str("");

  LOG_SEV_TO(logger, Warning) << "Goodbye, my friends.";
  EXPECT_EQ(stream->str(), "[Warning]: Goodbye, my friends.\n");
  stream->str("");

  LOG_SEV_TO(logger, Error) << "Goodbye, my friends.";
  EXPECT_EQ(stream->str(), "[Error  ]: Goodbye, my friends.\n");
  stream->str("");

  LOG_SEV_TO(logger, Fatal) << "Goodbye, my friends.";
  EXPECT_EQ(stream->str(), "[Fatal  ]: Goodbye, my friends.\n");
  stream->str("");
}

TEST(Logger, MapOnSinks) {
  Logger logger;
  logger.GetCore()->AddSink(UnlockedSink::From<StdoutSink>());
  logger.GetCore()->AddSink(UnlockedSink::From<TrivialDispatchSink>());

  // Map only on ostream sinks.
  int count = 0;
  logger.MapOnSinks<StdoutSink>([&count]([[maybe_unused]] auto& frontend, [[maybe_unused]] auto& backend) {
    ++count;
  });
  EXPECT_EQ(count, 1);
}

TEST(Logger, MoreThanInitialBuffer) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(formatting::MakeMsgFormatter("{}", formatting::MSG));
  Logger logger(sink);

  LOG_SEV_TO(logger, Info) << "1" << "2" << "3" << "4" << "5"
                           << "6" << "7" << "8" << "9" << "10"
                           << "11" << "12" << "13" << "14" << "15";
  EXPECT_EQ(stream->str(), "123456789101112131415\n");
}

TEST(Logger, LogHandler) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(formatting::MakeMsgFormatter("{}", formatting::MSG));
  Logger logger(sink);

  {
    auto handle = LOG_HANDLER_FOR(logger, Info);
    ASSERT_TRUE(handle);
    handle.GetRecord().Bundle() << "ABC";
    EXPECT_EQ(stream->str(), "");
  }
  EXPECT_EQ(stream->str(), "ABC\n");
}

TEST(Logger, LogStringView) {
  auto stream = std::make_shared<std::ostringstream>();
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(formatting::MakeMsgFormatter("{}", formatting::MSG));
  Logger logger(sink);

  char buffer[] = "Hello, world!";

  LOG_SEV_TO(logger, Info) << std::string_view{buffer, 13};
  EXPECT_EQ(stream->str(), "Hello, world!\n");
}

} // namespace Testing