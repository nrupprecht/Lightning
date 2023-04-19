//
// Created by Nathaniel Rupprecht on 4/16/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"
#include <string>

using namespace lightning;

namespace {

struct Nonstreamable {};

struct ToStringable {};

inline std::string to_string(const ToStringable) { return ""; }

} // namespace <unnamed>


namespace UnitTest {

TEST(Lightning, IsOstreamable) {
  EXPECT_TRUE(lightning::typetraits::is_ostreamable_v<std::string>);
  EXPECT_TRUE(lightning::typetraits::is_ostreamable_v<char[14]>);
  EXPECT_TRUE(lightning::typetraits::is_ostreamable_v<bool>);

  EXPECT_FALSE(lightning::typetraits::is_ostreamable_v<Nonstreamable>);
}

TEST(Lightning, Unconst_t) {
  EXPECT_TRUE((std::is_same_v<formatting::Unconst_t<const char*>, char*>));
  EXPECT_TRUE((std::is_same_v<formatting::Unconst_t<char*>, char*>));
  EXPECT_TRUE((std::is_same_v<formatting::Unconst_t<char const* const>, char*>));
  EXPECT_TRUE((std::is_same_v<formatting::Unconst_t<const double>, double>));
}

TEST(Lightning, IsCstrRelated_v) {
  EXPECT_TRUE(formatting::IsCstrRelated_v<char*>);
  EXPECT_TRUE(formatting::IsCstrRelated_v<const char*>);
  EXPECT_TRUE(formatting::IsCstrRelated_v<char[5]>);
  EXPECT_TRUE(formatting::IsCstrRelated_v<char[]>);

  EXPECT_FALSE(formatting::IsCstrRelated_v<int>);
  EXPECT_FALSE(formatting::IsCstrRelated_v<double>);
}

TEST(Lightning, HasToString) {
//  EXPECT_TRUE(lightning::typetraits::has_to_string_v<double>);
//  EXPECT_TRUE(lightning::typetraits::has_to_string_v<bool>);
  EXPECT_TRUE(lightning::typetraits::has_to_string_v<ToStringable>);

  EXPECT_FALSE(lightning::typetraits::has_to_string_v<std::string>);
  EXPECT_FALSE(lightning::typetraits::has_to_string_v<char[14]>);
  EXPECT_FALSE(lightning::typetraits::has_to_string_v<Nonstreamable>);
}

TEST(Lightning, OstreamSink) {
  std::ostringstream stream;

  auto sink = std::make_unique<OstreamSink>(stream);

  lightning::Logger logger;
  logger.GetCore()->AddSink<SimpleFrontend>(std::move(sink));

  logger() << "Hello world!";
  EXPECT_EQ(stream.str(), "Hello world!\n");
  stream.str("");
}

TEST(Lightning, Segmentize) {
  auto segments = formatting::Segmentize("[{Severity}]: {Message}");
  EXPECT_EQ(segments.size(), 4u);
  EXPECT_EQ(segments[0].index(), 0);
  EXPECT_EQ(segments[1].index(), 1);
  EXPECT_EQ(segments[2].index(), 0);
  EXPECT_EQ(segments[3].index(), 1);

  EXPECT_EQ(std::get<0>(segments[0]), "[");
  EXPECT_EQ(std::get<1>(segments[1]).attr_name, "Severity");
  EXPECT_EQ(std::get<0>(segments[2]), "]: ");
  EXPECT_EQ(std::get<1>(segments[3]).attr_name, "Message");
}

TEST(Lightning, SeverityLogger) {
  std::ostringstream stream;

  auto sink = MakeSink<SimpleFrontend, OstreamSink>(stream);

  EXPECT_TRUE(sink->SetFormatFrom("[{Severity}]: {Message}"));

  SeverityLogger logger;
  logger.GetCore()->AddSink(std::move(sink));

  logger(Severity::Debug) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Debug  ]: Goodbye, my friends.\n");
  stream.str("");

  logger(Severity::Info) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Info   ]: Goodbye, my friends.\n");
  stream.str("");

  logger(Severity::Warning) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Warning]: Goodbye, my friends.\n");
  stream.str("");

  logger(Severity::Error) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Error  ]: Goodbye, my friends.\n");
  stream.str("");

  logger(Severity::Fatal) << "Goodbye, my friends.";
  EXPECT_EQ(stream.str(), "[Fatal  ]: Goodbye, my friends.\n");
  stream.str("");
}

} // namespace Testing