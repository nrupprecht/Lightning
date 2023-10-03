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

struct Nonstreamable {};

struct ToStringable { char c; };

inline std::string to_string(const ToStringable x) {
  return std::string{'<', x.c, '>'};
}

} // namespace <unnamed>


namespace Testing {

TEST(Logger, RecordHandler_Streaming) {
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

  lightning::Logger logger(sink);

  LOG_SEV_TO(logger, Info) << ToStringable{'h'};

  EXPECT_EQ(stream.str(), "<h>\n");
}

TEST(Logger, OstreamSink) {
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

  lightning::Logger logger;
  logger.GetCore()->AddSink(std::move(sink));

  LOG_SEV_TO(logger, Info) << "Hello world!";
  EXPECT_EQ(stream.str(), "Hello world!\n");
  stream.str("");
}

TEST(Logger, SeverityLogger) {
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("[{}]: {}",
                                    formatting::SeverityAttributeFormatter{},
                                    formatting::MSG));

  Logger logger(std::move(sink));

  LOG_SEV_TO(logger, Debug) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Debug  ]: Goodbye, my friends.\n");
  stream.str("");

  LOG_SEV_TO(logger, Info) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Info   ]: Goodbye, my friends.\n");
  stream.str("");

  LOG_SEV_TO(logger, Warning) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Warning]: Goodbye, my friends.\n");
  stream.str("");

  LOG_SEV_TO(logger, Error) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Error  ]: Goodbye, my friends.\n");
  stream.str("");

  LOG_SEV_TO(logger, Fatal) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Fatal  ]: Goodbye, my friends.\n");
  stream.str("");
}

TEST(GeneralFormatting, MoreThanInitialBuffer) {
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(formatting::MakeMsgFormatter("{}", formatting::MSG));
  Logger logger(sink);

  LOG_SEV_TO(logger, Info) << "1" << "2" << "3" << "4" << "5" << "6" << "7" << "8" << "9" << "10" << "11" << "12" << "13" << "14" << "15";
  EXPECT_EQ(stream.str(), "123456789101112131415\n");
}

TEST(Logger, BlockAttributes) {
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  Logger logger(sink);
  /*
  logger
      .AddLoggerAttributeFormatter(attribute::BlockIndentationFormatter{})
      .AddAttribute(attribute::BlockLevelAttribute{});

  EXPECT_TRUE(sink->SetFormatFrom("[{Severity}]: {BlockLevel}>> {Message}"));

  logger(Severity::Info) << "Hi there.";
  {
    controllers::BlockLevel indent1(logger);
    logger(Severity::Info) << "Indented message.";
    {
      controllers::BlockLevel indent2(logger);
      logger(Severity::Warning) << "Even more indented??";
    }
    logger(Severity::Error) << "This has gone too far.";
  }
  logger(Severity::Fatal) << "Death approaches.";

  EXPECT_EQ(stream.str(), "[Info   ]: >> Hi there.\n"
                          "[Info   ]:   >> Indented message.\n"
                          "[Warning]:     >> Even more indented??\n"
                          "[Error  ]:   >> This has gone too far.\n"
                          "[Fatal  ]: >> Death approaches.\n"
                          "");
                          */
}

} // namespace Testing