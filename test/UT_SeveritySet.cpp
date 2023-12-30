//
// Created by Nathaniel Rupprecht on 12/29/23.
//

#include <gtest/gtest.h>
// Other files.
#include "Lightning/Lightning.h"

using namespace lightning;

namespace Testing {

TEST(SeveritySet, Construction) {
  {
    SeveritySet set(true);
    for (auto severity : ALL_SEVERITIES) {
      EXPECT_TRUE(set(severity));
    }
  }
  {
    SeveritySet set(false);
    for (auto severity : ALL_SEVERITIES) {
      EXPECT_FALSE(set(severity));
    }
  }
  {
    SeveritySet set({Severity::Info, Severity::Fatal});
    for (auto severity : ALL_SEVERITIES) {
      if (severity == Severity::Info || severity == Severity::Fatal) {
        EXPECT_TRUE(set(severity));
      }
      else {
        EXPECT_FALSE(set(severity));
      }
    }
  }
}

TEST(SeveritySet, LessThan) {
  auto set = LoggingSeverity < Severity::Info;
  EXPECT_TRUE(set(Severity::Trace));
  EXPECT_TRUE(set(Severity::Debug));
  EXPECT_FALSE(set(Severity::Info));
  EXPECT_FALSE(set(Severity::Major));
  EXPECT_FALSE(set(Severity::Warning));
  EXPECT_FALSE(set(Severity::Error));
  EXPECT_FALSE(set(Severity::Fatal));
}

TEST(SeveritySet, LessThanOrEqual) {
  auto set = LoggingSeverity <= Severity::Info;
  EXPECT_TRUE(set(Severity::Trace));
  EXPECT_TRUE(set(Severity::Debug));
  EXPECT_TRUE(set(Severity::Info));
  EXPECT_FALSE(set(Severity::Major));
  EXPECT_FALSE(set(Severity::Warning));
  EXPECT_FALSE(set(Severity::Error));
  EXPECT_FALSE(set(Severity::Fatal));
}

TEST(SeveritySet, GreaterThan) {
  auto set = LoggingSeverity > Severity::Info;
  EXPECT_FALSE(set(Severity::Trace));
  EXPECT_FALSE(set(Severity::Debug));
  EXPECT_FALSE(set(Severity::Info));
  EXPECT_TRUE(set(Severity::Major));
  EXPECT_TRUE(set(Severity::Warning));
  EXPECT_TRUE(set(Severity::Error));
  EXPECT_TRUE(set(Severity::Fatal));
}

TEST(SeveritySet, GreaterThanOrEqual) {
  auto set = LoggingSeverity >= Severity::Info;
  EXPECT_FALSE(set(Severity::Trace));
  EXPECT_FALSE(set(Severity::Debug));
  EXPECT_TRUE(set(Severity::Info));
  EXPECT_TRUE(set(Severity::Major));
  EXPECT_TRUE(set(Severity::Warning));
  EXPECT_TRUE(set(Severity::Error));
  EXPECT_TRUE(set(Severity::Fatal));
}

TEST(SeveritySet, Equal) {
  auto set = LoggingSeverity == Severity::Info;
  EXPECT_FALSE(set(Severity::Trace));
  EXPECT_FALSE(set(Severity::Debug));
  EXPECT_TRUE(set(Severity::Info));
  EXPECT_FALSE(set(Severity::Major));
  EXPECT_FALSE(set(Severity::Warning));
  EXPECT_FALSE(set(Severity::Error));
  EXPECT_FALSE(set(Severity::Fatal));
}

TEST(SeveritySet, NotEqual) {
  auto set = LoggingSeverity != Severity::Info;
  EXPECT_TRUE(set(Severity::Trace));
  EXPECT_TRUE(set(Severity::Debug));
  EXPECT_FALSE(set(Severity::Info));
  EXPECT_TRUE(set(Severity::Major));
  EXPECT_TRUE(set(Severity::Warning));
  EXPECT_TRUE(set(Severity::Error));
  EXPECT_TRUE(set(Severity::Fatal));
}

TEST(SeveritySet, Union) {
  auto set = LoggingSeverity <= Severity::Info || LoggingSeverity >= Severity::Fatal;
  EXPECT_TRUE(set(Severity::Trace));
  EXPECT_TRUE(set(Severity::Debug));
  EXPECT_TRUE(set(Severity::Info));
  EXPECT_FALSE(set(Severity::Major));
  EXPECT_FALSE(set(Severity::Warning));
  EXPECT_FALSE(set(Severity::Error));
  EXPECT_TRUE(set(Severity::Fatal));
}

TEST(SeveritySet, Intersection) {
  auto set1 = LoggingSeverity <= Severity::Info;
  auto set2 = Severity::Debug <= LoggingSeverity;
  auto set3 = set1 && set2;
  EXPECT_FALSE(set3(Severity::Trace));
  EXPECT_TRUE(set3(Severity::Debug));
  EXPECT_TRUE(set3(Severity::Info));
  EXPECT_FALSE(set3(Severity::Major));
  EXPECT_FALSE(set3(Severity::Warning));
  EXPECT_FALSE(set3(Severity::Error));
  EXPECT_FALSE(set3(Severity::Fatal));
}

TEST(SeveritySet, Difference) {
  auto set1 = LoggingSeverity <= Severity::Info;
  auto set2 = LoggingSeverity >= Severity::Warning;
  auto set3 = set1 && !set2;
  EXPECT_TRUE(set3(Severity::Trace));
  EXPECT_TRUE(set3(Severity::Debug));
  EXPECT_TRUE(set3(Severity::Info));
  EXPECT_FALSE(set3(Severity::Major));
  EXPECT_FALSE(set3(Severity::Warning));
  EXPECT_FALSE(set3(Severity::Error));
  EXPECT_FALSE(set3(Severity::Fatal));
}

} // namespace Testing