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

TEST(Lightning, NewLineIndent) {
  // Set up logger.
  std::ostringstream stream;
  auto sink = MakeSink<UnsynchronizedFrontend, OstreamSink>(stream);
  lightning::Logger logger({sink});

  // Usual type of formatting, header is on one line, contains a single {Message}.
  logger.GetCore()->SetAllFormats("[Ready:] {Message}");
  logger() << "Bop." << NewLineIndent << "Should align.";
  EXPECT_EQ(stream.str(), "[Ready:] Bop.\n         Should align.\n");
  stream.str("");

  // Message formatting is multiple lines long.
  logger.GetCore()->SetAllFormats("[First]\n[Second] {Message}");
  logger() << "Bop." << NewLineIndent << "Should align.";
  EXPECT_EQ(stream.str(), "[First]\n[Second] Bop.\n         Should align.\n");
  stream.str("");

  // Header contains multiple lines and {Message} parts.
  logger.GetCore()->SetAllFormats("[Top]: {Message}\n[Repeated]: {Message}");
  logger() << "A is for apple." << NewLineIndent << "B is for bear.";

  EXPECT_EQ(stream.str(), "[Top]: A is for apple.\n       B is for bear.\n[Repeated]: A is for apple.\n            B is for bear.\n");
}

}