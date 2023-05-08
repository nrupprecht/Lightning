//
// Created by Nathaniel Rupprecht on 5/5/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace lightning::time;


namespace Testing {

TEST(DateTime, Basic) {
  {
    DateTime d1(2011'12'01);
    EXPECT_EQ(d1.GetYear(), 2011);
    EXPECT_EQ(d1.GetMonthInt(), 12);
    EXPECT_EQ(d1.GetDay(), 1);
    EXPECT_EQ(d1.AsYYYYMMDD(), 2011'12'01);
  }
  {
    DateTime d1(2023'08'03);
    EXPECT_EQ(d1.GetYear(), 2023);
    EXPECT_EQ(d1.GetMonthInt(), 8);
    EXPECT_EQ(d1.GetDay(), 3);
    EXPECT_EQ(d1.AsYYYYMMDD(), 2023'08'03);
  }
}

TEST(DateTime, Formatting) {
  auto dt = DateTime::YMD_Time(2023'05'06, 13, 01, 15, 532000);

  EXPECT_EQ(Format(dt, "%Y-%m-%d"), "2023-05-06");
  EXPECT_EQ(Format(dt, "%m/%d/%y"), "05/06/23");
  EXPECT_EQ(Format(dt, "%d-%b-%y"), "06-May-23");
  EXPECT_EQ(Format(dt, "%I:%M:%S %p"), "01:01:15 PM");
}

} // namespace Testing