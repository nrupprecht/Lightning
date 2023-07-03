//
// Created by Nathaniel Rupprecht on 5/4/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/GreasedLightning.h"
#include <string>

using namespace lightning;
using namespace lightning::formatting;

namespace Testing {

TEST(Lightning, NewLineIndent) {
  // Set up logger.
  std::ostringstream stream;
  auto sink = std::make_shared<OstreamSink>(stream);
  lightning::Logger logger({sink});

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

  EXPECT_EQ(stream.str(), "[Top]: A is for apple.\n       B is for bear.\n[Repeated]: A is for apple.\n            B is for bear.\n");
}

}