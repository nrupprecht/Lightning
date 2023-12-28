//
// Created by Nathaniel Rupprecht on 12/27/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"
#include <string>

using namespace lightning;
using namespace lightning::formatting;

namespace Testing {

TEST(FormatterBySeverity, Basic) {
  auto formatter = std::make_unique<FormatterBySeverity>();
  auto ptr = formatter.get();

  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(std::move(formatter));
  Logger logger(sink);

  // No formatters, so nothing should be printed.
  EXPECT_NO_THROW(LOG_SEV_TO(logger, Info) << "A");
  EXPECT_EQ(stream.str(), "");
  stream.str("");

  ptr->SetFormatterForSeverity(Severity::Info, MakeMsgFormatter("{}", formatting::MSG));
  EXPECT_NO_THROW(LOG_SEV_TO(logger, Info) << "A");
  EXPECT_EQ(stream.str(), "A\n");
  stream.str("");


  ptr->SetFormatterForSeverity(Severity::Major, MakeMsgFormatter("MAJOR: {}", formatting::MSG));
  EXPECT_NO_THROW(LOG_SEV_TO(logger, Info) << "A");
  EXPECT_EQ(stream.str(), "A\n");
  stream.str("");
  EXPECT_NO_THROW(LOG_SEV_TO(logger, Major) << "B");
  EXPECT_EQ(stream.str(), "MAJOR: B\n");
  stream.str("");
}

TEST(FormatterBySeverity, DefaultFormatter) {
  auto formatter = std::make_unique<FormatterBySeverity>();
  formatter->SetDefaultFormatter(MakeMsgFormatter("DEFAULT: {}", formatting::MSG));
  auto ptr = formatter.get();

  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(std::move(formatter));
  Logger logger(sink);

  // No formatters, so nothing should be printed.
  EXPECT_NO_THROW(LOG_SEV_TO(logger, Info) << "A");
  EXPECT_EQ(stream.str(), "DEFAULT: A\n");
  stream.str("");

  ptr->SetFormatterForSeverity(Severity::Info, MakeMsgFormatter("INFO: {}", formatting::MSG));

  EXPECT_NO_THROW(LOG_SEV_TO(logger, Info) << "A");
  EXPECT_EQ(stream.str(), "INFO: A\n");
  stream.str("");
  EXPECT_NO_THROW(LOG_SEV_TO(logger, Major) << "B");
  EXPECT_EQ(stream.str(), "DEFAULT: B\n");
  stream.str("");
}

TEST(FormatterBySeverity, NoSeverity) {
  auto formatter = std::make_unique<FormatterBySeverity>();
  formatter->SetDefaultFormatter(MakeMsgFormatter("DEFAULT: {}", formatting::MSG));
  auto ptr = formatter.get();

  std::ostringstream stream;
  auto sink = UnlockedSink::From<OstreamSink>(stream);
  sink->SetFormatter(std::move(formatter)).GetFilter().AcceptNoSeverity(true);
  Logger logger(sink);
  logger.GetCore()->GetFilter().AcceptNoSeverity(true);

  // No formatters, so nothing should be printed.
  EXPECT_NO_THROW(LOG_TO(logger) << "A");
  EXPECT_EQ(stream.str(), "DEFAULT: A\n");
  stream.str("");
  EXPECT_NO_THROW(LOG_SEV_TO(logger, Info) << "B");
  EXPECT_EQ(stream.str(), "DEFAULT: B\n");

  // Make sure Copy works.
  EXPECT_NO_THROW(ptr->Copy());
}

}