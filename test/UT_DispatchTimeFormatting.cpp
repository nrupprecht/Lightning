//
// Created by Nathaniel Rupprecht on 5/4/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"
#include <string>

using namespace lightning;
using namespace lightning::formatting;

namespace Testing {
TEST(DispatchTimeFormatting, NewLineIndent) {
  // Set up logger.
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  Logger logger(sink);

  EXPECT_TRUE((std::is_base_of_v<BaseSegment, decltype(NewLineIndent)>));

  // Usual type of formatting, header is on one line, contains a single {Message}.
  sink->SetFormatter(MakeMsgFormatter("[Ready:] {}", formatting::MSG));

  LOG_SEV_TO(logger, Info) << "Bop." << NewLineIndent_t{} << "Should align.";
  EXPECT_EQ(stream.str(), "[Ready:] Bop.\n         Should align.\n");
  stream.str("");

  // Message formatting is multiple lines long.
  sink->SetFormatter(MakeMsgFormatter("[First]\n[Second] {}", formatting::MSG));
  LOG_SEV_TO(logger, Info) << "Bop." << NewLineIndent_t{} << "Should align.";
  EXPECT_EQ(stream.str(), "[First]\n[Second] Bop.\n         Should align.\n");
  stream.str("");

  // Header contains multiple lines and {Message} parts.
  sink->SetFormatter(MakeMsgFormatter("[Top]: {}\n[Repeated]: {}", formatting::MSG, formatting::MSG));
  LOG_SEV_TO(logger, Info) << "A is for apple." << NewLineIndent_t{} << "B is for bear.";

  EXPECT_EQ(stream.str(),
            "[Top]: A is for apple.\n       B is for bear.\n[Repeated]: A is for apple.\n            B is for bear.\n");
}

TEST(DispatchTimeFormatting, PadTill) {
  // Set up logger.
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));
  Logger logger(sink);

  sink->GetFilter().AcceptNoSeverity(true);
  logger.GetCore()->GetFilter().AcceptNoSeverity(true);

  {
    LOG_TO(logger) << PadUntil(10) << "X";
    EXPECT_EQ(stream.str(), "          X\n");
    stream.str("");
  }
  {
    LOG_TO(logger) << "1234" << PadUntil(10) << "X";
    EXPECT_EQ(stream.str(), "1234      X\n");
    stream.str("");
  }
  {
    LOG_TO(logger) << "1234567" << PadUntil(10) << "X";
    EXPECT_EQ(stream.str(), "1234567   X\n");
    stream.str("");
  }
  {
    LOG_TO(logger) << "123456789AB" << PadUntil(10) << "X";
    EXPECT_EQ(stream.str(), "123456789ABX\n");
    stream.str("");
  }
}

TEST(DispatchTimeFormatting, RepeatChar) {
  // Set up logger.
  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(MakeMsgFormatter("{}", MSG));
  Logger logger(sink);

  sink->GetFilter().AcceptNoSeverity(true);
  logger.GetCore()->GetFilter().AcceptNoSeverity(true);

  {
    LOG_TO(logger) << RepeatChar(5, 'a') << "X";
    EXPECT_EQ(stream.str(), "aaaaaX\n");
    stream.str("");
  }
  {
    LOG_TO(logger) << "A" << RepeatChar(5, 'x') << "B" << RepeatChar(3, 'c');
    EXPECT_EQ(stream.str(), "AxxxxxBccc\n");
    stream.str("");
  }
}

} // namespace Testing
