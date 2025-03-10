//
// Created by Nathaniel Rupprecht on 5/5/23.
//

#include <gtest/gtest.h>

// Includes
#include "Lightning/Lightning.h"

using namespace lightning;
using namespace lightning::time;


namespace Testing {

TEST(DateTime, BasicDate) {
  {
    DateTime d1(2011'12'01);
    EXPECT_EQ(d1.GetYear(), 2011);
    EXPECT_EQ(d1.GetMonthInt(), 12);
    EXPECT_EQ(d1.GetMonth(), Month::December);
    EXPECT_EQ(d1.GetDay(), 1);
    EXPECT_EQ(d1.AsYYYYMMDD(), 2011'12'01);
  }
  {
    DateTime d1(2023'08'03);
    EXPECT_EQ(d1.GetYear(), 2023);
    EXPECT_EQ(d1.GetMonthInt(), 8);
    EXPECT_EQ(d1.GetMonth(), Month::August);
    EXPECT_EQ(d1.GetDay(), 3);
    EXPECT_EQ(d1.AsYYYYMMDD(), 2023'08'03);
  }
}

TEST(DateTime, AddMicroseconds) {
  auto dt = DateTime::YMD_Time(2023'01'01, 0, 0, 0, 0);

  EXPECT_EQ(AddMicroseconds(dt, 1000ull), DateTime::YMD_Time(2023'01'01, 0, 0, 0, 1000));
  EXPECT_EQ(AddMicroseconds(dt, 1'000'000ull), DateTime::YMD_Time(2023'01'01, 0, 0, 1, 0));
  EXPECT_EQ(AddMicroseconds(dt, 1'000'012ull), DateTime::YMD_Time(2023'01'01, 0, 0, 1, 12));
  EXPECT_EQ(AddMicroseconds(dt, 60'000'012ull), DateTime::YMD_Time(2023'01'01, 0, 1, 0, 12));
  //
}

TEST(DateTime, AddMicroseconds_2) {
  {
    // Test going to next month
    DateTime dt(2023'01'31);
    // Add one day
    EXPECT_EQ(AddMicroseconds(dt, 24 * 60 * 60 * 1'000'000ull), DateTime(2023'02'01));
    // Add two days
    EXPECT_EQ(AddMicroseconds(dt, 24 * 60 * 60 * 2'000'000ull), DateTime(2023'02'02));
    // Add 30 days
    EXPECT_EQ(AddMicroseconds(dt, 24 * 60 * 60 * 30'000'000ull), DateTime(2023'03'02));
  }
  {
    // Leap year

    DateTime dt(20240228);
    EXPECT_EQ(AddMicroseconds(dt, 24 * 60 * 60 * 1'000'000ull), DateTime(20240229));

    // Add two days
    EXPECT_EQ(AddMicroseconds(dt, 24 * 60 * 60 * 2'000'000ull), DateTime(20240301));
  }
}

TEST(DateTime, Streaming) {
  auto dt = DateTime::YMD_Time(2023'01'01, 12, 30, 30, 1000);
  
  std::ostringstream stream;
  stream << dt;
  EXPECT_EQ(stream.str(), "2023-01-01 12:30:30.001000");
}

//TEST(DateTime, Formatting) {
//  auto dt = DateTime::YMD_Time(2023'05'06, 13, 01, 15, 532000);
//
//  EXPECT_EQ(Format(dt, "%Y-%m-%d"), "2023-05-06");
//  EXPECT_EQ(Format(dt, "%m/%d/%y"), "05/06/23");
//  EXPECT_EQ(Format(dt, "%d-%b-%y"), "06-May-23");
//  EXPECT_EQ(Format(dt, "%I:%M:%S %p"), "01:01:15 PM");
//}

} // namespace Testing