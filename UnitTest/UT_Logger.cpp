//
// Created by Nathaniel Rupprecht on 4/16/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/GreasedLightning.h"
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
  auto sink = std::make_shared<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

  lightning::Logger logger(sink);

  LOG_SEV_TO(logger, Info) << ToStringable{'h'};

  EXPECT_EQ(stream.str(), "<h>\n");
}

TEST(Logger, OstreamSink) {
  std::ostringstream stream;
  auto sink = std::make_shared<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

  lightning::Logger logger;
  logger.GetCore()->AddSink(std::move(sink));

  LOG_SEV_TO(logger, Info) << "Hello world!";
  EXPECT_EQ(stream.str(), "Hello world!\n");
  stream.str("");
}

TEST(Logger, Segmentize_1) {
  auto segments = formatting::Segmentize("[{Severity}]: {Message}");
  EXPECT_EQ(segments.size(), 4u);
  EXPECT_EQ(segments[0].index(), 0u);
  EXPECT_EQ(segments[1].index(), 1u);
  EXPECT_EQ(segments[2].index(), 0u);
  EXPECT_EQ(segments[3].index(), 1u);

  EXPECT_EQ(std::get<0>(segments[0]), "[");
  EXPECT_EQ(std::get<1>(segments[1]).attr_name, "Severity");
  EXPECT_EQ(std::get<0>(segments[2]), "]: ");
  EXPECT_EQ(std::get<1>(segments[3]).attr_name, "Message");
}

TEST(Logger, Segmentize_2) {
  auto segments = formatting::Segmentize("{First}==<>=={Time}{Attitude}:{Weather}({Thread}): {Message}");
  EXPECT_EQ(segments.size(), 10u);
  EXPECT_EQ(segments[0].index(), 1u);
  EXPECT_EQ(segments[1].index(), 0u);
  EXPECT_EQ(segments[2].index(), 1u);
  EXPECT_EQ(segments[3].index(), 1u);
  EXPECT_EQ(segments[4].index(), 0u);
  EXPECT_EQ(segments[5].index(), 1u);
  EXPECT_EQ(segments[6].index(), 0u);
  EXPECT_EQ(segments[7].index(), 1u);
  EXPECT_EQ(segments[8].index(), 0u);
  EXPECT_EQ(segments[9].index(), 1u);

  EXPECT_EQ(std::get<1>(segments[0]).attr_name, "First");
  EXPECT_EQ(std::get<0>(segments[1]), "==<>==");
  EXPECT_EQ(std::get<1>(segments[2]).attr_name, "Time");
  EXPECT_EQ(std::get<1>(segments[3]).attr_name, "Attitude");
  EXPECT_EQ(std::get<0>(segments[4]), ":");
  EXPECT_EQ(std::get<1>(segments[5]).attr_name, "Weather");
  EXPECT_EQ(std::get<0>(segments[6]), "(");
  EXPECT_EQ(std::get<1>(segments[7]).attr_name, "Thread");
  EXPECT_EQ(std::get<0>(segments[8]), "): ");
  EXPECT_EQ(std::get<1>(segments[9]).attr_name, "Message");
}

TEST(Logger, SeverityLogger) {
  std::ostringstream stream;
  auto sink = std::make_shared<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("[{}]: {}",
                                    formatting::SeverityAttributeFormatter{},
                                    formatting::MSG));

  Logger logger;
  logger.GetCore()->AddSink(std::move(sink));

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

TEST(Logger, BlockAttributes) {
  std::ostringstream stream;
  auto sink = std::make_shared<OstreamSink>(stream);
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