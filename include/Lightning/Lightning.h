/*
Created by Nathaniel Rupprecht on 6/6/23.

MIT License

Copyright (c) 2023 Nathaniel Rupprecht

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#pragma once

#include <algorithm>
#include <charconv>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace lightning {

// ==============================================================================
//  Macro definitions.
// ==============================================================================

#define NO_DISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]

//! \brief  Macro that allows you to create a type trait based on whether a statement about a type called
//! Value_t is valid.
//!         For example, to make a type trait called can_i_stream_this_v<T> that will be true if T can be
//!         streamed into std::cout, you can write: NEW_TYPE_TRAIT(can_i_stream_this_v, std::cout <<
//!         std::declval<T>());
//!
#define NEW_TYPE_TRAIT(trait_name, trait_test) \
  namespace detail_traits_##trait_name { \
    template<typename Value_t> \
    inline auto test_##trait_name(int) -> decltype((trait_test), std::true_type {}); \
\
    template<typename Value_t> \
    inline auto test_##trait_name(...) -> std::false_type; \
\
    template<typename Value_t> \
    struct trait_class_##trait_name { \
      static constexpr bool value = decltype(test_##trait_name<Value_t>(0))::value; \
    }; \
  } \
  template<typename Value_t> \
  static constexpr bool trait_name = detail_traits_##trait_name::trait_class_##trait_name<Value_t>::value;

// ==============================================================================
//  Error checking and handling.
// ==============================================================================

#define LL_REQUIRE(condition, message) \
  { \
    if (!(condition)) { \
      std::ostringstream _strm_; \
      _strm_ << message; \
      throw std::runtime_error(_strm_.str()); \
    } \
  }

#define LL_ASSERT(condition, message) \
  { \
    if (!(condition)) { \
      std::ostringstream _strm_; \
      _strm_ << message; \
      throw std::runtime_error(_strm_.str()); \
    } \
  }

#define LL_FAIL(message) \
  { \
    std::ostringstream _strm_; \
    _strm_ << message; \
    throw std::runtime_error(_strm_.str()); \
  }

// ==============================================================================
//  Type traits.
// ==============================================================================

namespace typetraits {  // namespace lightning::typetraits

//! \brief  Type trait that determines whether a type can be ostream'ed.
NEW_TYPE_TRAIT(is_ostreamable_v, std::declval<std::ostream&>() << std::declval<Value_t>())

//! \brief  Type trait that determines whether a type has a to_string function.
NEW_TYPE_TRAIT(has_to_string_v, to_string(std::declval<Value_t>()))

//! \brief  Type trait that determines if there is std::to_chars support for a type. Some compilers, like
//! clang,
//!         have std::to_chars for integral types, but not for doubles, so we can't just check the feature
//!         test macro __cpp_lib_to_chars.
NEW_TYPE_TRAIT(has_to_chars,
               std::to_chars(std::declval<char*>(), std::declval<char*>(), std::declval<Value_t>()))

//! \brief Define remove_cvref_t, which is not available everywhere.
template<typename T>
using remove_cvref_t = typename std::remove_cv_t<std::remove_reference_t<T>>;

namespace detail_always_false {  // namespace lightning::typetraits::detail_always_false

template<typename T>
struct always_false {
  static constexpr bool value = false;
};

}  // namespace detail_always_false

//! \brief  "Type trait" that is always false, useful in static_asserts, since right now, you cannot
//! static_assert(false).
template<typename T>
inline constexpr bool always_false_v = detail_always_false::always_false<T>::value;

namespace detail_unconst {  // namespace lightning::detail_unconst

template<typename T>
struct Unconst {
  using type = T;
};

template<typename T>
struct Unconst<const T> {
  using type = typename Unconst<T>::type;
};

template<typename T>
struct Unconst<const T*> {
  using type = typename Unconst<T>::type*;
};

template<typename T>
struct Unconst<T*> {
  using type = typename Unconst<T>::type*;
};

}  // namespace detail_unconst

//! \brief  Remove const-ness on all levels of pointers.
//!
template<typename T>
using Unconst_t = typename detail_unconst::Unconst<T>::type;

template<typename T>
constexpr inline bool IsCstrRelated_v =
    std::is_same_v<Unconst_t<std::decay_t<T>>, char*> || std::is_same_v<remove_cvref_t<T>, std::string>;

}  // namespace typetraits.

//! \brief Convenient base class for pImpl type objects.
class ImplBase {
public:
  class Impl {
  public:
    virtual ~Impl() = default;
  };

  template<typename Concrete_t>
  NO_DISCARD MAYBE_UNUSED bool IsType() const {
    return dynamic_cast<typename Concrete_t::Impl>(impl_.get());
  }

protected:
  template<typename Concrete_t>
  typename Concrete_t::Impl* impl() {
    return reinterpret_cast<typename Concrete_t::Impl*>(impl_.get());
  }

  template<typename Concrete_t>
  NO_DISCARD const typename Concrete_t::Impl* impl() const {
    return reinterpret_cast<const typename Concrete_t::Impl*>(impl_.get());
  }

  std::shared_ptr<Impl> impl_ = nullptr;

  explicit ImplBase(std::shared_ptr<Impl> impl)
      : impl_(std::move(impl)) {}
};

// ==============================================================================
//  Date / time support.
// ==============================================================================

namespace time {  // namespace lightning::time

//! \brief Compute whether a year is a leap year.
//!
inline bool IsLeapYear(int year) {
  // According to the Gregorian calendar, a year is a leap year if the year is divisible by 4
  // UNLESS the year is also divisible by 100, in which case it is not a leap year
  // UNLESS the year is also divisible by 400, in which case it is a leap year.
  if (year % 4 != 0) {
    return false;
  }
  if (year % 400 == 0) {
    return true;
  }
  return year % 100 != 0;
}

//! \brief  Get the number of days in a month in a particular year.
//!
inline int DaysInMonth(int month, int year) {
  LL_REQUIRE(0 < month && month < 13, "month must be in the range [1, 12], not " << month);
  static int days_in_month_[] = {0 /* Unused */, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month != 2) {
    return days_in_month_[month];
  }
  return IsLeapYear(year) ? 29 : 28;
}

enum class Month {
  January = 1,
  February,
  March,
  April,
  May,
  June,
  July,
  August,
  September,
  October,
  November,
  December
};

inline std::string MonthAbbreviation(Month m) {
  auto m_int = static_cast<int>(m);
  LL_REQUIRE(0 < m_int && m_int < 13, "month must be in the range [1, 12], not " << m_int);
  static std::string abbrev_[] {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return abbrev_[m_int - 1];
}

inline Month MonthIntToMonth(int month) {
  LL_REQUIRE(0 < month && month < 13, "month must be in the range [1, 12], not " << month);
  static Month months_[] {Month::January,
                          Month::February,
                          Month::March,
                          Month::April,
                          Month::May,
                          Month::June,
                          Month::July,
                          Month::August,
                          Month::September,
                          Month::October,
                          Month::November,
                          Month::December};
  return months_[month - 1];
}

//! \brief  A class that represents a date and time, down to millisecond precision.
//!
//!         This class does ignore some of the very odd bits of timekeeping, like leap-seconds.
//!
class DateTime {
public:
  DateTime() = default;
  DateTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int microsecond = 0) {
    setYMD(year, month, day);
    setHMSUS(hour, minute, second, microsecond);
  }

  //! \brief  Date time from system clock.
  //!
  //!         Note: localtime (and its related variants, and gmtime) are relatively slow functions. Prefer
  //!         using FastDateGenerator if you need to repeatedly generate DateTimes.
  //!
  explicit DateTime(const std::chrono::time_point<std::chrono::system_clock>& time_point) {
    // Get number of milliseconds for the current second (remainder after division into seconds)
    int ms = static_cast<int>(
        (std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch()) % 1'000'000)
            .count());
    // convert to std::time_t in order to convert to std::tm (broken time)
    auto t = std::chrono::system_clock::to_time_t(time_point);

    // Convert time - this is expensive.
    std::tm now {};
#if defined _MSC_VER
    localtime_s(&now, &t);
#else
    //    std::tm* now = std::localtime(&t);
    localtime_r(&t, &now);
#endif

    setYMD(now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, false);
    setHMSUS(now.tm_hour, now.tm_min, now.tm_sec, ms, false);
  }

  explicit DateTime(int yyyymmdd)
      : DateTime(yyyymmdd / 10000, (yyyymmdd / 100) % 100, yyyymmdd % 100) {}

  NO_DISCARD int AsYYYYMMDD() const { return GetYear() * 10000 + GetMonthInt() * 100 + GetDay(); }

  NO_DISCARD int GetYear() const { return static_cast<int>(y_m_d_h_m_s_um_ >> shift_year_); }
  NO_DISCARD int GetMonthInt() const {
    return static_cast<int>((y_m_d_h_m_s_um_ >> shift_month_) & month_mask_);
  }
  NO_DISCARD Month GetMonth() const { return MonthIntToMonth(GetMonthInt()); }
  NO_DISCARD int GetDay() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_day_) & day_mask_); }
  NO_DISCARD int GetHour() const { return static_cast<int>(y_m_d_h_m_s_um_ >> shift_hour_) & hour_mask_; }
  NO_DISCARD int GetMinute() const {
    return static_cast<int>((y_m_d_h_m_s_um_ >> shift_minute_) & minute_mask_);
  }
  NO_DISCARD int GetSecond() const {
    return static_cast<int>((y_m_d_h_m_s_um_ >> shift_second_) & second_mask_);
  }
  NO_DISCARD int GetMillisecond() const { return GetMicrosecond() / 1000; }
  NO_DISCARD int GetMicrosecond() const { return static_cast<int>(y_m_d_h_m_s_um_ & us_mask_); }

  //! \brief Check if the date is a non-null (empty) date.
  explicit operator bool() const { return y_m_d_h_m_s_um_ != 0; }

  //! \brief Check if two DateTime are equal.
  bool operator==(const DateTime& dt) const { return y_m_d_h_m_s_um_ == dt.y_m_d_h_m_s_um_; }

  //! \brief Check if two DateTime are not equal.
  bool operator!=(const DateTime& dt) const { return y_m_d_h_m_s_um_ != dt.y_m_d_h_m_s_um_; }

  //! \brief Check if one DateTime is less than the other.
  bool operator<(const DateTime& dt) const { return y_m_d_h_m_s_um_ < dt.y_m_d_h_m_s_um_; }

  //! \brief  Get the current clock time, in the local timezone.
  //!
  //!         Note: localtime (and its related variants, and gmtime) are relatively slow functions. Prefer
  //!         using FastDateGenerator if you need to repeatedly generate DateTimes.
  //!
  static DateTime Now() {
    auto time = std::chrono::system_clock::now();
    return DateTime(time);
  }

  static DateTime YMD_Time(int yyyymmdd, int hours, int minutes, int seconds = 0, int microsecond = 0) {
    DateTime dt(yyyymmdd);
    dt.setHMSUS(hours, minutes, seconds, microsecond);
    return dt;
  }

private:
  //! \brief Non-validating date constructor.
  DateTime(
      bool, int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int microsecond = 0) {
    setYMD(year, month, day, false);
    setHMSUS(hour, minute, second, microsecond, false);
  }

  void setYMD(int year, int month, int day, bool validate = true) {
    if (validate)
      validateYMD(year, month, day);
    // Zero previous y-m-d.
    y_m_d_h_m_s_um_ = (y_m_d_h_m_s_um_ << 32) >> 32;
    y_m_d_h_m_s_um_ |= (static_cast<std::uint64_t>(year) << shift_year_)
        | (static_cast<std::uint64_t>(month) << shift_month_)
        | (static_cast<std::uint64_t>(day) << shift_day_);
  }

  void setHMSUS(int hour, int minute, int second, int microseconds, bool validate = true) {
    if (validate)
      validateHMSUS(hour, minute, second, microseconds);
    // Zero previous h-m-s-us.
    y_m_d_h_m_s_um_ = (y_m_d_h_m_s_um_ >> 32) << 32;
    y_m_d_h_m_s_um_ |= (static_cast<std::uint64_t>(hour) << shift_hour_)
        | (static_cast<std::uint64_t>(minute) << shift_minute_)
        | (static_cast<std::uint64_t>(second) << shift_second_) | static_cast<std::uint64_t>(microseconds);
  }

  static void validateYMD(int year, int month, int day) {
    LL_REQUIRE(0 < year, "year must be > 0");
    LL_REQUIRE(0 < month && month <= 12, "month must be in the range [1, 12]")
    LL_REQUIRE(0 < day && day <= DaysInMonth(month, year),
               "there are only " << DaysInMonth(month, year) << " days in " << year << "-" << month);
  }

  static void validateHMSUS(int hour, int minute, int second, int microseconds) {
    // I am ignoring things like the time change and leap seconds.
    LL_REQUIRE(0 <= hour && hour < 24, "hour must be in the range [0, 24)");
    LL_REQUIRE(0 <= minute && minute < 60, "minute must be in the range [0, 60)");
    LL_REQUIRE(0 <= second && second < 60, "second must be in the range [0, 60)");
    LL_REQUIRE(0 <= microseconds && microseconds < 1'000'000,
               "microseconds must be in the range [0, 1,000,000)");
  }

  //! \brief  Storage for all the date-time data.
  //!         0 <= Y < 4096   => 12 bits  (shift =  52)
  //!         0 < m <= 12     => 7 bits   (shift = 45)
  //!         0 < d < 32      => 8 bits   (shift = 37)
  //!         0 <= h < 24     => 5 bits   (shift = 32)
  //!         0 <= m < 60     => 6 bits   (shift = 26)
  //!         0 <= s < 60     => 6 bits   (shift = 20)
  //!         0 <= ms < 1'000'000  => 20 bits
  //!         Total: 64 bits
  //! yyyyyyyy-yyyymmmm-mmmddddd-dddhhhhh mmmmmmss-ssssuuuu-uuuuuuuu-uuuuuuuu
  std::uint64_t y_m_d_h_m_s_um_ {};

  static constexpr int shift_second_ = 20;
  static constexpr int shift_minute_ = 26;
  static constexpr int shift_hour_ = 32;
  static constexpr int shift_day_ = 37;
  static constexpr int shift_month_ = 45;
  static constexpr int shift_year_ = 52;

  static constexpr int us_mask_ = 0b11111111111111111111;
  static constexpr int second_mask_ = 0b111111;
  static constexpr int minute_mask_ = 0b111111;
  static constexpr int hour_mask_ = 0b11111;
  static constexpr int day_mask_ = 0b11111111;
  static constexpr int month_mask_ = 0b1111111;
};

inline DateTime AddMicroseconds(const DateTime& time, unsigned long long microseconds) {
  int new_years = time.GetYear(), new_months = time.GetMonthInt(), new_days = time.GetDay();
  int new_hours = time.GetHour(), new_minutes = time.GetMinute(), new_seconds = time.GetSecond(),
      new_us = time.GetMicrosecond();

  auto new_us_overflow = static_cast<unsigned long long>(time.GetMicrosecond()) + microseconds;
  new_us = static_cast<int>(new_us_overflow % 1'000'000);

  auto carry_seconds = new_us_overflow / 1'000'000;
  if (carry_seconds == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  auto new_seconds_overflow = static_cast<unsigned long long>(new_seconds) + carry_seconds;
  new_seconds = static_cast<int>(new_seconds_overflow % 60);

  auto carry_minutes = new_seconds_overflow / 60;
  if (carry_minutes == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  auto new_minutes_overflow = static_cast<unsigned long long>(new_minutes) + carry_minutes;
  new_minutes = static_cast<int>(new_minutes_overflow % 60);

  auto carry_hours = new_minutes_overflow / 60;
  if (carry_hours == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  auto new_hours_overflow = static_cast<unsigned long long>(new_hours) + carry_hours;
  new_hours = static_cast<int>(new_hours_overflow % 24);

  auto carry_days = static_cast<int>(new_hours_overflow / 24);
  if (carry_days == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }

  // This is, relatively speaking, the hard part, have to take into account different numbers of days
  // in the month, and increment years when you go past December.
  auto which_month = time.GetMonthInt(), which_year = time.GetYear();
  while (0 < carry_days) {
    auto days = DaysInMonth(which_month, which_year);
    if (new_days + carry_days < days) {
      new_days += carry_days;
      break;
    }
    new_days = 1;
    ++which_month;
    if (which_month == 13) {
      ++which_year, which_month = 1;
    }
  }

  return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
}

//! \brief The conversion from a clock point to a local time is relatively slow, much slower than just
//! getting a time point, or subtracting time points. We can use this to quickly generate date times
//! by calculate the DateTime once, then just calculating microsecond offsets from that and adding
//! that to the original DateTime.
class FastDateGenerator {
public:
  FastDateGenerator()
      : start_time_point_(std::chrono::system_clock::now())
      , base_date_time_(start_time_point_) {}

  NO_DISCARD DateTime CurrentTime() const {
    auto current_time = std::chrono::system_clock::now();

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time_point_).count();
    return AddMicroseconds(base_date_time_, static_cast<unsigned long long>(us));
  }

private:
  //! \brief Keep track of when the logger was created.
  std::chrono::time_point<std::chrono::system_clock> start_time_point_;

  DateTime base_date_time_;
};

}  // namespace time

// ==============================================================================
//  Formatting.
// ==============================================================================

namespace formatting {  // namespace lightning::formatting

namespace detail {
//! \brief Store all powers of ten that fit in an unsigned long long.
const unsigned long long powers_of_ten[] = {
    1,
    10,
    100,
    1'000,
    10'000,
    100'000,
    1'000'000,
    10'000'000,
    100'000'000,
    1'000'000'000,
    10'000'000'000,
    100'000'000'000,
    1'000'000'000'000,
    10'000'000'000'000,
    100'000'000'000'000,
    1'000'000'000'000'000,
    10'000'000'000'000'000,
    100'000'000'000'000'000,
    1'000'000'000'000'000'000,
    10'000'000'000'000'000'000ull,
};
}  // namespace detail

//! \brief Returns how many digits the decimal representation of a ull will have.
//!        The optional bound "upper" means that the number has at most "upper" digits.
inline unsigned NumberOfDigitsULL(unsigned long long x, int upper = 19) {
  using namespace detail;
  upper = std::max(0, std::min(upper, 19));
  if (x == 0)
    return 1u;
  if (x >= 10'000'000'000'000'000'000ull)
    return 20;
  auto it = std::upper_bound(&powers_of_ten[0], &powers_of_ten[upper], x);
  return static_cast<unsigned>(std::distance(&powers_of_ten[0], it));
}

template<typename Integral_t,
         typename = std::enable_if_t<std::is_integral_v<Integral_t> && !std::is_same_v<Integral_t, bool>>>
inline unsigned NumberOfDigits(Integral_t x, int upper = 19) {
  if constexpr (!std::is_signed_v<Integral_t>) {
    return NumberOfDigitsULL(x, upper);
  }
  else {
    return NumberOfDigitsULL(static_cast<unsigned long long>(std::llabs(x)), upper);
  }
}

inline char* CopyPaddedInt(
    char* start, char* end, unsigned long long x, int width, char fill_char = '0', int max_power = 19) {
  auto nd = NumberOfDigits(x, max_power);
  auto remainder = static_cast<unsigned>(width) - nd;
  if (0 < remainder) {
    std::fill_n(start, remainder, fill_char);
  }
  return std::to_chars(start + remainder, end, x).ptr;
}

//! \brief Format a date to a buffer.
inline char* FormatDateTo(char* c, char* end_c, const time::DateTime& dt) {
  // Store all zero-padded one and two-digit numbers, allows for very fast serialization.
  static const char up_to[] =
      "00010203040506070809"
      "10111213141516171819"
      "20212223242526272829"
      "30313233343536373839"
      "40414243444546474849"
      "50515253545556575859"
      "60616263646566676869"
      "70717273747576777879"
      "80818283848586878889"
      "90919293949596979899";
  static const char* up_to_c = static_cast<const char*>(up_to);

  LL_REQUIRE(26 <= end_c - c, "need at least 26 characters to format date");

  // Year
  auto start_c = c;
  std::to_chars(c, end_c, dt.GetYear());
  *(start_c + 4) = '-';
  // Month
  auto month = dt.GetMonthInt();
  std::copy(up_to_c + 2 * month, up_to_c + 2 * month + 2, start_c + 5);
  *(start_c + 7) = '-';
  // Day
  auto day = dt.GetDay();
  std::copy(up_to_c + 2 * day, up_to_c + 2 * day + 2, start_c + 8);
  *(start_c + 10) = ' ';
  // Hour.
  auto hour = dt.GetHour();
  std::copy(up_to_c + 2 * hour, up_to_c + 2 * hour + 2, start_c + 11);
  *(start_c + 13) = ':';
  // Minute.
  auto minute = dt.GetMinute();
  std::copy(up_to_c + 2 * minute, up_to_c + 2 * minute + 2, start_c + 14);
  *(start_c + 16) = ':';
  // Second.
  auto second = dt.GetSecond();
  std::copy(up_to_c + 2 * second, up_to_c + 2 * second + 2, start_c + 17);
  *(start_c + 19) = '.';
  // Microsecond.
  auto microseconds = dt.GetMicrosecond();
  int nd = static_cast<int>(formatting::NumberOfDigits(microseconds, 6));
  std::fill_n(c + 20, 6 - nd, '0');
  std::to_chars(c + 26 - nd, end_c, microseconds);

  return c + 26;
}

struct MessageInfo {
  //! \brief The total length of the formatted string so far, inclusive of message and non-message segments.
  unsigned total_length = 0;

  //! \brief The indentation of the start of the message within the formatted record.
  unsigned message_indentation = 0;

  //! \brief The length of the message segment so far.
  unsigned message_length = 0;

  //! \brief True if the current segment or formatter is within the message segment of a formatted record.
  bool is_in_message_segment = false;

  //! \brief  If true, some formatting segment needs the message indentation to be calculated. Otherwise, the
  //!         message indentation does not need to be calculated.
  bool needs_message_indentation = false;
};

// ==============================================================================
//  Virtual terminal commands.
// ==============================================================================

enum class AnsiForegroundColor : short {
  Reset = 0,
  Default = 39,
  Black = 30,
  Red = 31,
  Green = 32,
  Yellow = 33,
  Blue = 34,
  Magenta = 35,
  Cyan = 36,
  White = 37,
  BrightBlack = 90,
  BrightRed = 91,
  BrightGreen = 92,
  BrightYellow = 93,
  BrightBlue = 94,
  BrightMagenta = 95,
  BrightCyan = 96,
  BrightWhite = 97
};

enum class AnsiBackgroundColor : short {
  Reset = 0,
  Default = 49,
  Black = 40,
  Red = 41,
  Green = 42,
  Yellow = 43,
  Blue = 44,
  Magenta = 45,
  Cyan = 46,
  White = 47,
  BrightBlack = 100,
  BrightRed = 101,
  BrightGreen = 102,
  BrightYellow = 103,
  BrightBlue = 104,
  BrightMagenta = 105,
  BrightCyan = 106,
  BrightWhite = 107
};

using Ansi256Color = unsigned char;

//! \brief Generate a string that can change the ANSI 8bit color of a terminal, if supported.
inline std::string SetAnsiColorFmt(std::optional<AnsiForegroundColor> foreground,
                                   std::optional<AnsiBackgroundColor> background = {}) {
  std::string fmt {};
  if (foreground)
    fmt += "\x1b[" + std::to_string(static_cast<short>(*foreground)) + "m";
  if (background)
    fmt += "\x1b[" + std::to_string(static_cast<short>(*background)) + "m";
  return fmt;
}

//! \brief Generate a string that can change the ANSI 256-bit color of a terminal, if supported.
inline std::string SetAnsi256ColorFmt(std::optional<Ansi256Color> foreground_color_id,
                                      std::optional<Ansi256Color> background_color_id = {}) {
  std::string fmt {};
  if (foreground_color_id)
    fmt = "\x1b[38;5;" + std::to_string(*foreground_color_id) + "m";
  if (background_color_id)
    fmt += "\x1b[48;5;" + std::to_string(*background_color_id) + "m";
  return fmt;
}

//! \brief Generate a string that can change the ANSI RGB color of a terminal, if supported.
inline std::string SetAnsiRGBColorFmt(Ansi256Color r, Ansi256Color g, Ansi256Color b) {
  return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

//! \brief Generate a string that will reset the settings of a virtual terminal
inline std::string AnsiReset() {
  return SetAnsiColorFmt(AnsiForegroundColor::Default, AnsiBackgroundColor::Default);
}

//! \brief Count the number of characters in a range that are not part of an Ansi escape sequence.
inline unsigned CountNonAnsiSequenceCharacters(const char* begin, const char* end) {
  unsigned count = 0;
  bool in_escape = false;
  for (auto it = begin; it != end; ++it) {
    if (*it == '\x1b') {
      in_escape = true;
    }
    if (!in_escape) {
      ++count;
    }
    if (*it == 'm') {
      in_escape = false;
    }
  }
  return count;
}

}  // namespace formatting

//! \brief  Structure that specifies how a message can be formatted, and what are the capabilities of the sink
//! that
//!         the formatted message will be dispatched to.
struct FormattingSettings {
  //! \brief Records whether the target can handle virtual terminal ascii codes.
  bool has_virtual_terminal_processing = false;

  //! \brief How to terminate the message, e.g. with a newline.
  std::string message_terminator = "\n";

  //! \brief Whether the sink frontend should format the record to a string and pass that to the backend.
  bool needs_formatting = true;
};

//! \brief  The base class for all message segments.
//!
//!         For efficiency, each segment needs to know how much space its formatted self will take up.
struct BaseSegment {
  explicit BaseSegment() = default;
  BaseSegment([[maybe_unused]] const char* fmt_begin, [[maybe_unused]] const char* fmt_end) {}
  virtual ~BaseSegment() = default;

  virtual char* AddToBuffer(const FormattingSettings& settings,
                            const formatting::MessageInfo& msg_info,
                            char* start,
                            char* end) const = 0;
  NO_DISCARD virtual unsigned SizeRequired(const FormattingSettings& settings,
                                           const formatting::MessageInfo& msg_info) const = 0;
  virtual void CopyTo(class SegmentStorage&) const = 0;

  //! \brief  Some formatting segments need the message indentation (the distance from the start of the
  //! message back to the last newline in the
  //!         header, counting only visible characters) to be calculated, e.g. to do an aligned newline. The
  //!         indentation is only calculated if needed, to save time, since it is rarely needed.
  NO_DISCARD virtual bool NeedsMessageIndentation() const { return false; }
};

//! \brief  Acts like a unique pointer for an object deriving from BaseSegment, but uses a SBO to put small
//! segments
//!         on the stack.
class SegmentStorage {
public:
  //! \brief Default construct an empty SegmentStorage object.
  SegmentStorage() = default;

  //! \brief Copy a segment storage.
  explicit SegmentStorage(const SegmentStorage& storage) { storage.segment_pointer_->CopyTo(*this); }

  //! \brief Move SegmentStorage.
  explicit SegmentStorage(SegmentStorage&& storage) {
    if (storage.IsUsingBuffer()) {
      // TODO: BaseSegment should have a MoveTo function, and call that.
      std::memcpy(buffer_, storage.buffer_, sizeof(buffer_));
      segment_pointer_ = reinterpret_cast<BaseSegment*>(buffer_);
    }
    else {
      segment_pointer_ = storage.segment_pointer_;
    }
    storage.segment_pointer_ = nullptr;
  }

  SegmentStorage& operator=(const SegmentStorage& storage) {
    storage.segment_pointer_->CopyTo(*this);
    return *this;
  }

  SegmentStorage& operator=(SegmentStorage&& storage) {
    if (storage.IsUsingBuffer()) {
      // TODO: BaseSegment should have a MoveTo function, and call that.
      std::memcpy(buffer_, storage.buffer_, sizeof(buffer_));
      segment_pointer_ = reinterpret_cast<BaseSegment*>(buffer_);
    }
    else {
      segment_pointer_ = storage.segment_pointer_;
    }
    storage.segment_pointer_ = nullptr;
    return *this;
  }

  ~SegmentStorage() {
    if (segment_pointer_ && !IsUsingBuffer()) {
      delete segment_pointer_;
      segment_pointer_ = nullptr;
    }
  }

  template<typename Segment_t,
           typename... Args,
           typename = std::enable_if_t<std::is_base_of_v<BaseSegment, Segment_t>>>
  SegmentStorage& Create(Args&&... args) {
    if constexpr (sizeof(Segment_t) <= sizeof(buffer_)) {
      // Use the internal buffer.
      segment_pointer_ = new (buffer_) Segment_t(std::forward<Args>(args)...);
    }
    else {
      segment_pointer_ = new Segment_t(std::forward<Args>(args)...);
    }
    return *this;
  }

  BaseSegment* Get() { return segment_pointer_; }
  NO_DISCARD const BaseSegment* Get() const { return segment_pointer_; }

  NO_DISCARD bool IsUsingBuffer() const {
    auto this_ptr = reinterpret_cast<const char*>(this);
    auto seg_ptr = reinterpret_cast<const char*>(segment_pointer_);
    return seg_ptr - this_ptr < static_cast<long>(sizeof(SegmentStorage));
  }

  NO_DISCARD bool HasData() const { return segment_pointer_; }
  static constexpr std::size_t BufferSize() { return sizeof(buffer_); }

private:
  constexpr static std::size_t default_buffer_size = 3 * sizeof(void*);

  //! \brief Pointer to the data that is either on the stack or heap. Allows for polymorphically accessing the
  //! data.
  BaseSegment* segment_pointer_ {};

  //! \brief The internal buffer for data. Note that all BaseSegments have a vptr, so that takes up
  //! sizeof(void*) bytes by itself.
  unsigned char buffer_[default_buffer_size] = {0};
};

//! \brief Formatting segment that changes the coloration of a terminal.
struct AnsiColorSegment : public BaseSegment {
  explicit AnsiColorSegment(std::optional<formatting::AnsiForegroundColor> foreground,
                            std::optional<formatting::AnsiBackgroundColor> background = {})
      : fmt_string_(SetAnsiColorFmt(foreground, background)) {}

  char* AddToBuffer(const FormattingSettings& settings,
                    const formatting::MessageInfo&,
                    char* start,
                    char*) const override {
    if (settings.has_virtual_terminal_processing) {
      std::copy(fmt_string_.begin(), fmt_string_.end(), start);
      return start + fmt_string_.size();
    }
    return start;
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings,
                                   const formatting::MessageInfo&) const override {
    return settings.has_virtual_terminal_processing ? static_cast<unsigned>(fmt_string_.size()) : 0u;
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<AnsiColorSegment>(*this); }

  void SetColors(std::optional<formatting::AnsiForegroundColor> foreground,
                 std::optional<formatting::AnsiBackgroundColor> background = {}) {
    fmt_string_ = SetAnsiColorFmt(foreground, background);
  }

private:
  std::string fmt_string_;
};

//! \brief Formatting segment that resets the style.
struct AnsiResetSegment_t : public AnsiColorSegment {
  AnsiResetSegment_t() noexcept
      : AnsiColorSegment(formatting::AnsiForegroundColor::Reset) {}
};

//! \brief  Prototypical AnsiResetSegment object. There is no customization possible, so there
//!         is no reason to explicitly create more.
const inline AnsiResetSegment_t AnsiResetSegment {};

//! \brief  How to count distance while formatting, Specifies whether the pad should be for the message
//!         part of the formatted string, or the entire length of the formatted string.
enum class FmtDistanceType : unsigned char {
  MESSAGE_LENGTH,
  TOTAL_LENGTH
};

//! \brief Inject spaces until the message length or total formatted length reaches a specified length.
struct FillUntil : public BaseSegment {
  explicit FillUntil(unsigned pad_length,
                     char fill_char,
                     FmtDistanceType pad_type = FmtDistanceType::MESSAGE_LENGTH)
      : pad_length_(pad_length)
      , fill_char_(fill_char)
      , pad_type_(pad_type) {}

  char* AddToBuffer(const FormattingSettings& settings,
                    const formatting::MessageInfo& msg_info,
                    char* start,
                    [[maybe_unused]] char* end) const override {
    auto num_chars = SizeRequired(settings, msg_info);
    std::fill_n(start, num_chars, fill_char_);
    return start + num_chars;
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings&,
                                   const formatting::MessageInfo& msg_info) const override {
    if (pad_type_ == FmtDistanceType::MESSAGE_LENGTH) {
      return pad_length_ - std::min(msg_info.message_length, pad_length_);
    }
    else {  // TOTAL_LENGTH
      return pad_length_ - std::min(msg_info.total_length, pad_length_);
    }
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<FillUntil>(*this); }

private:
  unsigned pad_length_ {};
  char fill_char_ {};
  FmtDistanceType pad_type_;
};

//! \brief Inject spaces until the message length or total formatted length reaches a specified length.
struct PadUntil : public FillUntil {
  explicit PadUntil(unsigned pad_length, FmtDistanceType pad_type = FmtDistanceType::MESSAGE_LENGTH)
      : FillUntil(pad_length, ' ', pad_type) {}

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<PadUntil>(*this); }
};

//! \brief Segment that repeats a character N times.
struct RepeatChar : public BaseSegment {
  RepeatChar(unsigned repeat_length, char c)
      : repeat_length_(repeat_length)
      , c_(c) {}

  char* AddToBuffer(const FormattingSettings&,
                    const formatting::MessageInfo&,
                    char* start,
                    char*) const override {
    std::fill_n(start, repeat_length_, c_);
    return start + repeat_length_;
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings&, const formatting::MessageInfo&) const override {
    return repeat_length_;
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<RepeatChar>(*this); }

private:
  unsigned repeat_length_ {};
  char c_ {};
};

struct NewLineIndent_t : public BaseSegment {
  char* AddToBuffer(const FormattingSettings&,
                    const formatting::MessageInfo& msg_info,
                    char* start,
                    char*) const override {
    *start = '\n';
    std::fill_n(++start, msg_info.message_indentation, ' ');
    return start + msg_info.message_indentation + 1;
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings&,
                                   const formatting::MessageInfo& msg_info) const override {
    return msg_info.message_indentation + 1;
  }

  NO_DISCARD bool NeedsMessageIndentation() const override { return true; }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<NewLineIndent_t>(); }
};

//! \brief Prototypical NewLineIndent_t object.
const inline NewLineIndent_t NewLineIndent;

// ==============================================================================
//  Ordinary data formatting segments.
// ==============================================================================

//! \brief Base template for formatting Segment<T>'s. Has a slot to allow for enable_if-ing.
template<typename T, typename Enable = void>
struct Segment;

//! \brief  Type trait that determines whether a type has a to_string function.
NEW_TYPE_TRAIT(has_segment_formatter_v,
               Segment<std::decay_t<typetraits::remove_cvref_t<Value_t>>>(
                   std::declval<std::decay_t<typetraits::remove_cvref_t<Value_t>>>(), nullptr, nullptr))

//! \brief Template specialization for string segments.
template<>
struct Segment<std::string> : public BaseSegment {
  explicit Segment(std::string s, const char* fmt_begin, const char* fmt_end)
      : BaseSegment(fmt_begin, fmt_end)
      , str_(std::move(s)) {}

  char* AddToBuffer(const FormattingSettings&,
                    const formatting::MessageInfo&,
                    char* start,
                    char* end) const override {
    for (auto c : str_) {
      if (start == end)
        break;
      *start = c;
      ++start;
    }
    return start;
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings&, const formatting::MessageInfo&) const override {
    return static_cast<unsigned>(str_.size());
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<Segment<std::string>>(*this); }

private:
  const std::string str_;
};

//! \brief Template specialization for char* segments.
template<>
struct Segment<char*> : public BaseSegment {
  explicit Segment(const char* s, const char* fmt_begin, const char* fmt_end)
      : BaseSegment(fmt_begin, fmt_end)
      , cstr_(s)
      , size_required_(static_cast<unsigned>(std::strlen(s))) {}

  char* AddToBuffer([[maybe_unused]] const FormattingSettings& settings,
                    const formatting::MessageInfo&,
                    char* start,
                    char* end) const override {
    for (auto i = 0u; i < size_required_; ++i, ++start) {
      if (start == end)
        break;
      *start = cstr_[i];
    }
    return start;
  }

  NO_DISCARD unsigned SizeRequired([[maybe_unused]] const FormattingSettings& settings,
                                   const formatting::MessageInfo&) const override {
    return size_required_;
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<Segment<char*>>(*this); }

private:
  const char* cstr_;
  unsigned size_required_ {};
};

//! \brief Template specialization for char array segments. TODO: Can I just combine this with char*?
template<std::size_t N>
struct Segment<char[N]> : public Segment<char*> {
  explicit Segment(const char s[N], const char* fmt_begin, const char* fmt_end)
      : Segment<char*>(&s[0], fmt_begin, fmt_end) {}
};

//! \brief Template specialization for bool segments.
template<>
struct Segment<bool> : public BaseSegment {
  explicit Segment(bool b)
      : value_(b) {}

  char* AddToBuffer([[maybe_unused]] const FormattingSettings& settings,
                    const formatting::MessageInfo&,
                    char* start,
                    [[maybe_unused]] char* end) const override {
    if (value_) {
#if defined _MSC_VER
      strcpy_s(start, 5, "true");
#else
      strcpy(start, "true");
#endif
      return start + 4;
    }
    else {
#if defined _MSC_VER
      strcpy_s(start, 6, "false");
#else
      strcpy(start, "false");
#endif
      return start + 5;
    }
  }

  NO_DISCARD unsigned SizeRequired([[maybe_unused]] const FormattingSettings& settings,
                                   const formatting::MessageInfo&) const override {
    return value_ ? 4u : 5u;
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<Segment<bool>>(*this); }

private:
  bool value_;
};

//! \brief Template specialization for floating point number segments.
template<typename Floating_t>
struct Segment<Floating_t, std::enable_if_t<std::is_floating_point_v<Floating_t>>> : public BaseSegment {
  explicit Segment(Floating_t number, const char* fmt_begin, const char* fmt_end)
      : BaseSegment(fmt_begin, fmt_end)
      , number_(number)
      , size_required_(10) {
    // TEMPORARY...? Wish that std::to_chars works for floating points.
    serialized_number_ = std::to_string(number);
    size_required_ = static_cast<unsigned>(serialized_number_.size());
  }

  char* AddToBuffer([[maybe_unused]] const FormattingSettings& settings,
                    const formatting::MessageInfo&,
                    char* start,
                    [[maybe_unused]] char* end) const override {
// std::to_chars(start, end, number_, std::chars_format::fixed);
#if defined _MSC_VER
    strcpy_s(start, serialized_number_.size() + 1, &serialized_number_[0]);
#else
    std::strcpy(start, &serialized_number_[0]);
#endif
    return start + serialized_number_.size();
  }

  NO_DISCARD unsigned SizeRequired([[maybe_unused]] const FormattingSettings& settings,
                                   const formatting::MessageInfo&) const override {
    return size_required_;
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<Segment<Floating_t>>(*this); }

private:
  Floating_t number_;
  unsigned size_required_ {};

  // TEMPORARY, hopefully. clang does not support std::to_chars for floating point types.
  std::string serialized_number_;
};

//! \brief Template specialization for integral value segments.
template<typename Integral_t>
struct Segment<Integral_t, std::enable_if_t<std::is_integral_v<Integral_t>>> : public BaseSegment {
  explicit Segment(Integral_t number, const char* fmt_begin, const char* fmt_end)
      : BaseSegment(fmt_begin, fmt_end)
      , number_(number)
      , size_required_(formatting::NumberOfDigits(number_) + (number < 0 ? 1 : 0)) {}

  char* AddToBuffer([[maybe_unused]] const FormattingSettings& settings,
                    const formatting::MessageInfo&,
                    char* start,
                    char* end) const override {
    std::to_chars(start, end, number_);
    return start + size_required_;
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings&, const formatting::MessageInfo&) const override {
    return size_required_;
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<Segment<Integral_t>>(*this); }

private:
  Integral_t number_;
  unsigned size_required_ {};
};

//! \brief Template specialization for DateTime segments.
template<>
struct Segment<time::DateTime> : public BaseSegment {
  explicit Segment(time::DateTime dt, const char* fmt_begin, const char* fmt_end)
      : BaseSegment(fmt_begin, fmt_end)
      , value_(dt) {}

  char* AddToBuffer([[maybe_unused]] const FormattingSettings& settings,
                    const formatting::MessageInfo&,
                    char* start,
                    [[maybe_unused]] char* end) const override {
    return formatting::FormatDateTo(start, end, value_);
  }

  NO_DISCARD unsigned SizeRequired([[maybe_unused]] const FormattingSettings& settings,
                                   const formatting::MessageInfo&) const override {
    return 26;
  }

  void CopyTo(class SegmentStorage& storage) const override {
    storage.Create<Segment<time::DateTime>>(*this);
  }

private:
  time::DateTime value_;
};

//! \brief Formatting segment that colors a single piece of data.
template<typename T>
struct AnsiColor8Bit : public BaseSegment {
  explicit AnsiColor8Bit(const T& data,
                         std::optional<formatting::AnsiForegroundColor> foreground,
                         std::optional<formatting::AnsiBackgroundColor> background = {})
      : set_formatting_string_(SetAnsiColorFmt(foreground, background))
      , segment_(data, nullptr, nullptr) {}

  char* AddToBuffer(const FormattingSettings& settings,
                    const formatting::MessageInfo& msg_info,
                    char* start,
                    [[maybe_unused]] char* end) const override {
    if (settings.has_virtual_terminal_processing) {
      std::copy(set_formatting_string_.begin(), set_formatting_string_.end(), start);
      start += set_formatting_string_.size();
    }
    start = segment_.AddToBuffer(settings, msg_info, start, end);
    start = AnsiResetSegment.AddToBuffer(settings, msg_info, start, end);
    return start;
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings,
                                   const formatting::MessageInfo& msg_info) const override {
    auto required_size =
        settings.has_virtual_terminal_processing ? static_cast<unsigned>(set_formatting_string_.size()) : 0u;
    required_size += segment_.SizeRequired(settings, msg_info);
    required_size += AnsiResetSegment.SizeRequired(settings, msg_info);
    return required_size;
  }

  void CopyTo(class SegmentStorage& storage) const override { storage.Create<AnsiColor8Bit<T>>(*this); }

private:
  //! \brief Cache the formatting string.
  std::string set_formatting_string_;
  //! \brief The object that should be colored.
  Segment<std::decay_t<typetraits::remove_cvref_t<T>>> segment_;
};

//! \brief An object that has a bundle of data, to be formatted.
class RefBundle {
public:
  //! \brief Stream data into a RefBundle.
  template<typename T>
  RefBundle& operator<<(T&& obj);

  explicit RefBundle(unsigned default_size = 10) { segments_.reserve(default_size); }

  template<typename Segment_t, typename... Args>
  void CreateSegment(Args&&... args) {
    segments_.emplace_back();
    segments_.back().Create<Segment_t>(std::forward<Args>(args)...);
  }

  SegmentStorage& AddSegment() {
    segments_.emplace_back();
    return segments_.back();
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings,
                                   formatting::MessageInfo& msg_info) const {
    // Reset message length counter.
    msg_info.message_length = 0;
    msg_info.is_in_message_segment = true;
    // Calculate message length.
    unsigned size_required = 0;
    for (auto& segment : segments_) {
      auto size = segment.Get()->SizeRequired(settings, msg_info);
      size_required += size;
      msg_info.message_length += size;
      msg_info.total_length += size;
    }
    msg_info.is_in_message_segment = false;
    return size_required;
  }

  char* FmtString(const FormattingSettings& settings,
                  char* start,
                  char* end,
                  formatting::MessageInfo& msg_info) const {
    // Reset message length counter.
    msg_info.message_length = 0;
    msg_info.is_in_message_segment = true;
    // Add message.
    for (auto& bundle : segments_) {
      auto sz = bundle.Get()->SizeRequired(settings, msg_info);
      bundle.Get()->AddToBuffer(settings, msg_info, start, std::min(start + sz, end));
      start += sz;
      msg_info.message_length += sz;
      msg_info.total_length += sz;
    }
    msg_info.is_in_message_segment = false;
    return start;
  }

  //! \brief Returns whether any segment needs the message indentation to be calculated.
  NO_DISCARD bool NeedsMessageIndentation() const {
    if (segments_.empty()) {
      return false;
    }
    return std::any_of(segments_.begin(), segments_.end(), [](const SegmentStorage& ss) {
      return ss.Get()->NeedsMessageIndentation();
    });
  }

private:
  std::vector<SegmentStorage> segments_;
};

//! \brief  Create a type trait that determines if a 'format_logstream' function has been defined for a type.
NEW_TYPE_TRAIT(has_logstream_formatter_v,
               format_logstream(std::declval<const Value_t&>(), std::declval<RefBundle&>()))

template<typename T>
RefBundle& RefBundle::operator<<(T&& obj) {
  using decay_t = std::decay_t<typetraits::remove_cvref_t<T>>;

  if constexpr (has_logstream_formatter_v<decay_t>) {
    format_logstream(obj, *this);
  }
  else if constexpr (std::is_base_of_v<BaseSegment, decay_t>) {
    obj.CopyTo(AddSegment());
  }
  else if constexpr (has_segment_formatter_v<decay_t>) {
    // Add a formatting segment.
    CreateSegment<Segment<decay_t>>(obj, nullptr, nullptr);
  }
  else if constexpr (typetraits::has_to_string_v<decay_t>) {
    operator<<(to_string(std::forward<T>(obj)));
  }
  else if constexpr (typetraits::is_ostreamable_v<decay_t>) {
    std::ostringstream str;
    str << obj;
    operator<<(str.str());
  }
  else {
    static_assert(typetraits::always_false_v<T>, "No streaming available for this type");
  }
  return *this;
}

//! \brief Base class for "freestanding" attributes that can be attached to logging messages,
//!  but are not the "permanent" record attributes like severity, time, logger name, etc.
class Attribute : public ImplBase {
public:
  class Impl : public ImplBase::Impl {
  public:
    NO_DISCARD virtual std::unique_ptr<Impl> Copy() const = 0;
  };

  NO_DISCARD Attribute Copy() const { return Attribute(impl<Attribute>()->Copy()); }

  explicit Attribute(std::unique_ptr<Impl>&& impl)
      : ImplBase(std::move(impl)) {}
  Attribute(const Attribute& other)
      : ImplBase(other.impl<Attribute>()->Copy()) {}
  Attribute& operator=(const Attribute& other) {
    impl_ = other.impl<Attribute>()->Copy();
    return *this;
  }
};

//! \brief The integer type used for severity.
using SeverityInt_t = int32_t;

enum class Severity : SeverityInt_t {
  Trace = 0b1,
  Debug = 0b10,
  Info = 0b100,
  Major = 0b1000,
  Warning = 0b10000,
  Error = 0b100000,
  Fatal = 0b1000000
};

//! \brief  Structure storing very common attributes that a logging message will often have.
//!
//!         Additional attributes can be implemented as Attribute objects.
//!
struct BasicAttributes {
  BasicAttributes() = default;

  explicit BasicAttributes(std::optional<Severity> lvl, bool do_timestamp = false)
      : level(lvl) {
    if (do_timestamp)
      time_stamp = time::DateTime::Now();
  }

  explicit BasicAttributes(std::optional<Severity> lvl,
                           const char* file_name,
                           unsigned line_number,
                           bool do_timestamp = false)
      : level(lvl)
      , file_name(file_name)
      , line_number(line_number) {
    if (do_timestamp)
      time_stamp = time::DateTime::Now();
  }

  //! \brief The severity level of the record.
  std::optional<Severity> level {};

  //! \brief The time at which the record was created.
  std::optional<time::DateTime> time_stamp {};

  //! \brief The name of the logger which sent a message.
  std::string logger_name {};

  const char* file_name {};

  std::optional<unsigned> line_number {};
};

//! \brief A filter that checks whether a record should be accepted solely based on its severity.
class BasicSeverityFilter {
public:
  NO_DISCARD bool Check(std::optional<Severity> severity) const {
    if (severity) {
      return mask_ & static_cast<int>(severity.value());
    }
    return allow_if_no_severity_;
  }

  BasicSeverityFilter& SetAcceptance(Severity severity, bool does_accept) {
    if (does_accept)
      mask_ |= static_cast<int>(severity);
    else
      mask_ &= (0b1111111 & ~static_cast<int>(severity));
    return *this;
  }

  BasicSeverityFilter& AcceptNoSeverity(bool flag) {
    allow_if_no_severity_ = flag;
    return *this;
  }

private:
  //! \brief The acceptance mask.
  int mask_ = 0xFFFF;

  //! \brief Whether to accept a message that does not contain the severity attribute.
  bool allow_if_no_severity_ = false;
};

//! \brief Object containing all attributes for a record.
struct RecordAttributes {
  template<typename... Attrs_t>
  explicit RecordAttributes(BasicAttributes basic_attributes = {}, Attrs_t&&... attrs)
      : basic_attributes(std::move(basic_attributes)) {
    attributes.reserve(sizeof...(attrs));
    (attributes.emplace_back(std::move(attrs)), ...);
  }

  //! \brief  The basic record attributes. These are stored as fields, which is much faster to
  //!         create and handle than pImpl attribute objects.
  //!
  BasicAttributes basic_attributes {};

  //! \brief  Additional attributes, beyond the basic attributes.
  //!
  std::vector<Attribute> attributes;
};

namespace filter {

class AttributeFilter {
public:
  virtual ~AttributeFilter() = default;

  NO_DISCARD bool WillAccept(const RecordAttributes& attributes) const {
    // Check basic attributes.
    if (!severity_filter_.Check(attributes.basic_attributes.level)) {
      return false;
    }
    return willAccept(attributes.attributes);
  }

  //! \brief Check if a message whose only attribute is severity of the specified level will be accepted.
  NO_DISCARD bool WillAccept(std::optional<Severity> severity) const {
    return severity_filter_.Check(severity);
  }

  //! \brief Set the severity levels that will be accepted.
  AttributeFilter& Accept(const std::set<Severity>& acceptable) {
    for (auto sev : {Severity::Debug, Severity::Info, Severity::Warning, Severity::Error, Severity::Fatal}) {
      severity_filter_.SetAcceptance(sev, acceptable.count(sev) != 0);
    }
    return *this;
  }

  //! \brief Set whether a message with no severity level should be accepted.
  AttributeFilter& AcceptNoSeverity(bool flag) {
    severity_filter_.AcceptNoSeverity(flag);
    return *this;
  }

private:
  //! \brief Private implementation of whether a message should be accepted based on its attributes.
  NO_DISCARD virtual bool willAccept([[maybe_unused]] const std::vector<Attribute>& attributes) const {
    // TODO.
    return true;
  }

  //! \brief The filter used to decide if a record should be accepted based on its severity settings.
  BasicSeverityFilter severity_filter_;
};

}  // namespace filter

// Forward declare core.
class Core;

//! \brief The result of logging, a collection of a message, attributes, and values.
//!
class Record {
public:
  Record() = default;

  template<typename... Attrs_t>
  explicit Record(BasicAttributes basic_attributes = {}, Attrs_t&&... attrs)
      : attributes_(basic_attributes, std::forward<Attrs_t>(attrs)...) {}

  //! \brief Get the message bundle from the record.
  RefBundle& Bundle() { return bundle_; }
  NO_DISCARD const RefBundle& Bundle() const { return bundle_; }

  //! \brief Get the record attributes.
  NO_DISCARD const RecordAttributes& Attributes() const { return attributes_; }
  NO_DISCARD RecordAttributes& Attributes() { return attributes_; }

  //! \brief Try to open the record for a core. Returns whether the record opened.
  inline bool TryOpen(std::shared_ptr<Core> core);

  //! \brief Test whether a record is open.
  inline explicit operator bool() const;

  //! \brief Dispatch the record to the associated core.
  inline void Dispatch();

private:
  //! \brief The log message, contained as a RefBundle.
  RefBundle bundle_ {};

  //! \brief The attributes collection of the message.
  RecordAttributes attributes_ {};

  //! \brief The core that the record is destined for, or null if the record is closed.
  std::shared_ptr<Core> core_ {};
};

//! \brief An RAII structure that dispatches the contained record upon the destruction of the
//! RecordDispatcher.
class RecordDispatcher {
public:
  //! \brief Create a closed, empty record.
  RecordDispatcher() = default;

  //! \brief Wrap a record in a record handler.
  [[maybe_unused]] explicit RecordDispatcher(Record&& record)
      : record_(std::move(record))
      , uncaught_exceptions_(std::uncaught_exceptions()) {}

  //! \brief Construct a record handler, constructing the record in-place inside it.
  template<typename... Attrs_t>
  explicit RecordDispatcher(std::shared_ptr<Core> core,
                            BasicAttributes basic_attributes = {},
                            Attrs_t&&... attrs)
      : record_(basic_attributes, std::forward<Attrs_t>(attrs)...)
      , uncaught_exceptions_(std::uncaught_exceptions()) {
    record_.TryOpen(std::move(core));
  }

  //! \brief RecordDispatcher is an RAII structure for dispatching records.
  ~RecordDispatcher() {
    if (std::uncaught_exceptions() <= uncaught_exceptions_) {
      record_.Dispatch();
    }
  }

  //! \brief Check whether a record is open.
  NO_DISCARD bool RecordIsOpen() const { return static_cast<bool>(record_); }

  //! \brief Check whether a record is open via explicit conversion to bool.
  explicit operator bool() const { return RecordIsOpen(); }

  //! \brief Get the record from the RecordDispatcher.
  Record& GetRecord() { return record_; }

  template<typename T>
  RecordDispatcher& operator<<(T&& obj) {
    record_.Bundle() << std::forward<T>(obj);
    return *this;
  }

private:
  Record record_ {};

  int uncaught_exceptions_ {};
};

namespace formatting {

//! \brief Base class for attribute formatters, objects that know how to serialize attribute representations
//! to strings.
class AttributeFormatter {
public:
  virtual ~AttributeFormatter() = default;
  virtual void AddToBuffer(const RecordAttributes& attributes,
                           const FormattingSettings& settings,
                           const formatting::MessageInfo& msg_info,
                           char* start,
                           char* end) const = 0;
  NO_DISCARD virtual unsigned RequiredSize(const RecordAttributes& attributes,
                                           const FormattingSettings& settings,
                                           const formatting::MessageInfo& msg_info) const = 0;
};

//! \brief Format the severity attribute.
class SeverityAttributeFormatter : public AttributeFormatter {
public:
  void AddToBuffer(const RecordAttributes& attributes,
                   const FormattingSettings& settings,
                   const formatting::MessageInfo& msg_info,
                   char* start,
                   char* end) const override {
    if (attributes.basic_attributes.level) {
      start =
          colorSegment(attributes.basic_attributes.level.value()).AddToBuffer(settings, msg_info, start, end);
      auto& str = getString(attributes.basic_attributes.level.value());
      std::copy(str.begin(), str.end(), start);
      start += str.size();
      AnsiResetSegment.AddToBuffer(settings, msg_info, start, end);
    }
  }

  NO_DISCARD unsigned RequiredSize(const RecordAttributes& attributes,
                                   const FormattingSettings& settings,
                                   const formatting::MessageInfo& msg_info) const override {
    if (attributes.basic_attributes.level) {
      unsigned required_size =
          colorSegment(attributes.basic_attributes.level.value()).SizeRequired(settings, msg_info);
      required_size += AnsiResetSegment.SizeRequired(settings, msg_info);
      return required_size
          + static_cast<unsigned>(getString(attributes.basic_attributes.level.value()).size());
    }
    return 0u;
  }

  SeverityAttributeFormatter& SeverityName(Severity severity, const std::string& name) {
    getString(severity) = name;
    return *this;
  }

  SeverityAttributeFormatter& SeverityFormatting(Severity severity,
                                                 std::optional<AnsiForegroundColor> foreground,
                                                 std::optional<AnsiBackgroundColor> background = {}) {
    auto& color_formatting = colorSegment(severity);
    color_formatting.SetColors(foreground, background);
    return *this;
  }

private:
  NO_DISCARD const std::string& getString(Severity severity) const {
    switch (severity) {
      case Severity::Trace:
        return trace_;
      case Severity::Debug:
        return debug_;
      case Severity::Info:
        return info_;
      case Severity::Major:
        return major_;
      case Severity::Warning:
        return warn_;
      case Severity::Error:
        return error_;
      case Severity::Fatal:
        return fatal_;
      default:
        LL_FAIL("unrecognized severity attribute");
    }
  }

  NO_DISCARD std::string& getString(Severity severity) {
    switch (severity) {
      case Severity::Trace:
        return trace_;
      case Severity::Debug:
        return debug_;
      case Severity::Info:
        return info_;
      case Severity::Major:
        return major_;
      case Severity::Warning:
        return warn_;
      case Severity::Error:
        return error_;
      case Severity::Fatal:
        return fatal_;
      default:
        LL_FAIL("unrecognized severity attribute");
    }
  }

  NO_DISCARD const AnsiColorSegment& colorSegment(Severity severity) const {
    switch (severity) {
      case Severity::Trace:
        return trace_colors_;
      case Severity::Debug:
        return debug_colors_;
      case Severity::Info:
        return info_colors_;
      case Severity::Major:
        return major_colors_;
      case Severity::Warning:
        return warn_colors_;
      case Severity::Error:
        return error_colors_;
      case Severity::Fatal:
        return fatal_colors_;
      default:
        LL_FAIL("unrecognized severity attribute");
    }
  }

  NO_DISCARD AnsiColorSegment& colorSegment(Severity severity) {
    switch (severity) {
      case Severity::Trace:
        return trace_colors_;
      case Severity::Debug:
        return debug_colors_;
      case Severity::Info:
        return info_colors_;
      case Severity::Major:
        return major_colors_;
      case Severity::Warning:
        return warn_colors_;
      case Severity::Error:
        return error_colors_;
      case Severity::Fatal:
        return fatal_colors_;
      default:
        LL_FAIL("unrecognized severity attribute");
    }
  }

  std::string trace_ = "Trace  ";
  std::string debug_ = "Debug  ";
  std::string info_ = "Info   ";
  std::string major_ = "Major  ";
  std::string warn_ = "Warning";
  std::string error_ = "Error  ";
  std::string fatal_ = "Fatal  ";

  AnsiColorSegment trace_colors_ {AnsiForegroundColor::White};
  AnsiColorSegment debug_colors_ {AnsiForegroundColor::BrightWhite};
  AnsiColorSegment info_colors_ {AnsiForegroundColor::Green};
  AnsiColorSegment major_colors_ {AnsiForegroundColor::BrightBlue};
  AnsiColorSegment warn_colors_ {AnsiForegroundColor::Yellow};
  AnsiColorSegment error_colors_ {AnsiForegroundColor::Red};
  AnsiColorSegment fatal_colors_ {AnsiForegroundColor::BrightRed};
};

class DateTimeAttributeFormatter final : public AttributeFormatter {
  // TODO: Allow for different formatting of the DateTime, via format string?
public:
  void AddToBuffer(const RecordAttributes& attributes,
                   const FormattingSettings&,
                   const formatting::MessageInfo&,
                   char* start,
                   char* end) const override {
    if (attributes.basic_attributes.time_stamp) {
      auto& dt = attributes.basic_attributes.time_stamp.value();
      formatting::FormatDateTo(start, end, dt);
    }
  }

  NO_DISCARD unsigned RequiredSize(const RecordAttributes& attributes,
                                   const FormattingSettings&,
                                   const formatting::MessageInfo&) const override {
    return attributes.basic_attributes.time_stamp ? 26 : 0u;
  }
};

class LoggerNameAttributeFormatter final : public AttributeFormatter {
public:
  void AddToBuffer(const RecordAttributes& attributes,
                   const FormattingSettings&,
                   const formatting::MessageInfo&,
                   char* start,
                   char*) const override {
    std::copy(attributes.basic_attributes.logger_name.begin(),
              attributes.basic_attributes.logger_name.end(),
              start);
  }

  NO_DISCARD unsigned RequiredSize(const RecordAttributes& attributes,
                                   const FormattingSettings&,
                                   const formatting::MessageInfo&) const override {
    return static_cast<unsigned>(attributes.basic_attributes.logger_name.size());
  }
};

//! \brief Formatter for the file name attribute.
class FileNameAttributeFormatter final : public AttributeFormatter {
public:
  explicit FileNameAttributeFormatter(bool only_file_name = false)
      : only_file_name_(only_file_name) {}

  void AddToBuffer(const RecordAttributes& attributes,
                   const FormattingSettings& settings,
                   const formatting::MessageInfo& info,
                   char* start,
                   char*) const override {
    if (attributes.basic_attributes.file_name) {
      if (only_file_name_) {
        auto [first, last] = getRange(attributes.basic_attributes.file_name);
        std::copy(attributes.basic_attributes.file_name + first,
                  attributes.basic_attributes.file_name + last,
                  start);
      }
      else {
        auto size = RequiredSize(attributes, settings, info);
        std::copy(attributes.basic_attributes.file_name, attributes.basic_attributes.file_name + size, start);
      }
    }
  }

  NO_DISCARD unsigned RequiredSize(const RecordAttributes& attributes,
                                   const FormattingSettings&,
                                   const formatting::MessageInfo&) const override {
    if (attributes.basic_attributes.file_name) {
      if (only_file_name_) {
        auto [first, last] = getRange(attributes.basic_attributes.file_name);
        return static_cast<unsigned>(last - first);
      }
      else {
        return static_cast<unsigned>(std::strlen(attributes.basic_attributes.file_name));
      }
    }
    return 0;
  }

private:
  std::pair<std::size_t, std::size_t> getRange(const char* str) const {
    std::size_t first = 0, last = 0;
    for (auto ptr = str; *ptr != '\0'; ++ptr, ++last) {
      if (only_file_name_ && (*ptr == '/' || *ptr == '\\'))
        first = last + 1;
    }
    return {first, last};
  }

  //! \brief If true, only the file name is displayed, not the full path.
  bool only_file_name_ = true;
};

//! \brief Formatter for the file name attribute.
class FileLineAttributeFormatter final : public AttributeFormatter {
public:
  void AddToBuffer(const RecordAttributes& attributes,
                   [[maybe_unused]] const FormattingSettings& settings,
                   [[maybe_unused]] const formatting::MessageInfo& info,
                   char* start,
                   char* end) const override {
    if (attributes.basic_attributes.line_number) {
      std::to_chars(start, end, *attributes.basic_attributes.line_number);
    }
  }

  NO_DISCARD unsigned RequiredSize(const RecordAttributes& attributes,
                                   const FormattingSettings&,
                                   const formatting::MessageInfo&) const override {
    if (attributes.basic_attributes.line_number) {
      return formatting::NumberOfDigits(*attributes.basic_attributes.line_number);
    }
    return 0;
  }
};

//! \brief  Function to calculate how far the start of the message is from the last newline in the header,
//! counting only visible
//!         (non ansi virtual terminal) characters.
inline unsigned CalculateMessageIndentation(char* buffer_end, const MessageInfo& msg_info) {
  if (msg_info.total_length == 0) {
    return 0;
  }
  auto c = buffer_end - 1;
  for (; c != buffer_end - msg_info.total_length; --c) {
    if (*c == '\n') {
      ++c;
      break;
    }
  }
  return CountNonAnsiSequenceCharacters(c, buffer_end);
}

//! \brief  Base class for message formatters, objects capable of taking a record and formatting it into a
//! string,
//!         according to the formatting settings.
class BaseMessageFormatter {
public:
  virtual ~BaseMessageFormatter() = default;
  NO_DISCARD virtual std::string Format(const Record& record,
                                        const FormattingSettings& sink_settings) const = 0;
  NO_DISCARD virtual std::unique_ptr<BaseMessageFormatter> Copy() const = 0;
};

//! \brief Object used to represent a placeholder for the logging message.
struct MSG_t {};

//! \brief Global prototypical MSG_t object.
constexpr inline MSG_t MSG;

template<typename... Types>
class MsgFormatter : public BaseMessageFormatter {
private:
  static_assert(((std::is_base_of_v<AttributeFormatter, Types> || std::is_same_v<MSG_t, Types>)&&...),
                "All types must be AttributeFormatters or a MSG tag.");

public:
  explicit MsgFormatter(const std::string& fmt_string, const Types&... types)
      : formatters_(types...) {
    // Find the segments, ensure that there are the right number of arguments.
    auto count_slots = 0;
    literals_.emplace_back();
    for (auto i = 0u; i < fmt_string.size();) {
      if (fmt_string[i] == '{') {
        // Always advance i, since it is either an escaped '{' or we need to find the end of the "{...}"
        if (i + 1 < fmt_string.size() && fmt_string[++i] != '{') {
          // Start of a slot, end of the literal.
          literals_.emplace_back();  // start a new literal
          ++count_slots;
          // Find the closing '}' TODO: Capture formatting string?
          for (; i < fmt_string.size() && fmt_string[i] != '}'; ++i)
            ;
          ++i;
          continue;
        }
      }
      literals_.back().push_back(fmt_string[i]);
      ++i;
    }

    // Make sure there are the right number of slots.
    LL_ASSERT(count_slots == sizeof...(Types),
              "mismatch in the number of slots (" << count_slots << ") and the number of formatters ("
                                                  << sizeof...(Types) << ")");
    LL_ASSERT(literals_.size() == sizeof...(Types) + 1,
              "not the right number of literals, needed " << sizeof...(Types) + 1 << ", but had "
                                                          << literals_.size());
  }

  NO_DISCARD std::string Format(const Record& record,
                                const FormattingSettings& sink_settings) const override {
    auto needs_message_indentation = record.Bundle().NeedsMessageIndentation();

    MessageInfo msg_info {};
    msg_info.needs_message_indentation = needs_message_indentation;

    auto required_size = getRequiredSize<0>(record, sink_settings, msg_info);
    required_size += static_cast<unsigned>(sink_settings.message_terminator.size());
    std::string buffer(required_size, ' ');

    // Reset message info.
    msg_info = MessageInfo {};
    msg_info.needs_message_indentation = needs_message_indentation;

    if (0 < required_size) {
      auto c = &buffer[0];
      // Format all the segments.
      c = format<0>(c, record, sink_settings, msg_info);
      // Add the terminator.
      std::copy(sink_settings.message_terminator.begin(), sink_settings.message_terminator.end(), c);
      c += sink_settings.message_terminator.size();

      // Erase any unused bits at the end.
      auto actual_length = std::distance(&buffer[0], c);
      LL_ASSERT(actual_length <= required_size, "formatted message overflowed buffer");
      buffer.resize(static_cast<unsigned long>(actual_length));
    }

    return buffer;
  }

  NO_DISCARD std::unique_ptr<BaseMessageFormatter> Copy() const override {
    return std::unique_ptr<BaseMessageFormatter>(new MsgFormatter(*this));
  }

private:
  template<std::size_t N>
  NO_DISCARD unsigned getRequiredSize([[maybe_unused]] const Record& record,
                                      [[maybe_unused]] const FormattingSettings& sink_settings,
                                      [[maybe_unused]] MessageInfo& msg_info) const {
    if constexpr (N == sizeof...(Types)) {
      auto usize = static_cast<unsigned>(literals_[N].size());
      msg_info.total_length += usize;
      return usize;
    }
    else {
      unsigned required_size {};
      auto usize = static_cast<unsigned>(literals_[N].size());
      msg_info.total_length += usize;

      if constexpr (std::is_same_v<MSG_t, std::tuple_element_t<N, decltype(formatters_)>>) {
        if (msg_info.needs_message_indentation) {
          // Can't calculate the number of invisible characters (e.g. from a virtual terminal control
          // sequence), or if there were newlines in the header, so conservatively estimate that the
          // indentation is the message length so far.
          msg_info.message_indentation = msg_info.total_length;
        }
        else {
          msg_info.message_indentation = 0;
        }

        // Bundle::SizeRequired handles incrementing the MessageInfo data, and sets the is_in_message_segment
        // variable.
        required_size = record.Bundle().SizeRequired(sink_settings, msg_info);
      }
      else {
        required_size = std::get<N>(formatters_).RequiredSize(record.Attributes(), sink_settings, msg_info);
        msg_info.total_length += required_size;
      }
      return usize + required_size + getRequiredSize<N + 1>(record, sink_settings, msg_info);
    }
  }

  template<std::size_t N>
  char* format(char*& buffer,
               const Record& record,
               const FormattingSettings& sink_settings,
               MessageInfo& msg_info) const {
    // First, the literal.
    buffer = std::copy(literals_[N].begin(), literals_[N].end(), buffer);
    msg_info.total_length += static_cast<unsigned>(literals_[N].size());

    if constexpr (N != sizeof...(Types)) {
      // Then the formatter from the slot.
      if constexpr (std::is_same_v<MSG_t, std::tuple_element_t<N, decltype(formatters_)>>) {
        // Since this calculation can take some time, only calculate if needed.
        if (msg_info.needs_message_indentation) {
          // At this point, we can calculate the true message indentation by looking for the last newline
          // (if any), and not counting escape characters.
          msg_info.message_indentation = CalculateMessageIndentation(buffer, msg_info);
        }
        else {
          msg_info.message_indentation = 0;
        }

        auto required_size = record.Bundle().SizeRequired(sink_settings, msg_info);
        buffer = record.Bundle().FmtString(sink_settings, buffer, buffer + required_size, msg_info);
      }
      else {
        auto required_size =
            std::get<N>(formatters_).RequiredSize(record.Attributes(), sink_settings, msg_info);
        std::get<N>(formatters_)
            .AddToBuffer(record.Attributes(), sink_settings, msg_info, buffer, buffer + required_size);
        msg_info.total_length += required_size;
        buffer += required_size;
      }
      return format<N + 1>(buffer, record, sink_settings, msg_info);
    }
    else {
      return buffer;
    }
  }

  std::tuple<Types...> formatters_;
  std::vector<std::string> literals_;
};

//! \brief Helper function to create a unique pointer to a MsgFormatter
template<typename... Types>
auto MakeMsgFormatter(const std::string& fmt_string, const Types&... types) {
  return std::unique_ptr<BaseMessageFormatter>(new MsgFormatter(fmt_string, types...));
}

inline auto MakeStandardFormatter() {
  return MakeMsgFormatter("[{}] [{}] {}",
                          formatting::SeverityAttributeFormatter {},
                          formatting::DateTimeAttributeFormatter {},
                          formatting::MSG);
}

class RecordFormatter : public BaseMessageFormatter {
public:
  //! \brief The default record formatter just prints the message.
  RecordFormatter() { AddMsgSegment(); }

  NO_DISCARD std::string Format(const Record& record,
                                const FormattingSettings& sink_settings) const override {
    std::optional<unsigned> msg_size {};
    MessageInfo msg_info;

    unsigned required_size = 0;
    for (const auto& formatter : formatters_) {
      unsigned size = 0;
      switch (formatter.index()) {
        case 0: {
          if (!msg_size)
            msg_size = record.Bundle().SizeRequired(sink_settings, msg_info);
          size = msg_size.value();
          break;
        }
        case 1: {
          size = std::get<1>(formatter)->RequiredSize(record.Attributes(), sink_settings, msg_info);
          break;
        }
        case 2: {
          size = static_cast<unsigned>(std::get<2>(formatter).size());
          break;
        }
      }

      required_size += size;
    }

    // Account for message terminator.
    required_size += static_cast<unsigned>(sink_settings.message_terminator.size());

    std::string buffer(required_size, ' ');
    char *c = &buffer[0], *end = c + required_size;

    // Reset the message info.
    msg_info = MessageInfo {};
    for (const auto& formatter : formatters_) {
      unsigned segment_size = 0;
      switch (formatter.index()) {
        case 0: {
          // TODO: Update for LogNewLine type alignment.
          segment_size = record.Bundle().SizeRequired(sink_settings, msg_info);
          record.Bundle().FmtString(sink_settings, c, end, msg_info);
          break;
        }
        case 1: {
          segment_size = std::get<1>(formatter)->RequiredSize(record.Attributes(), sink_settings, msg_info);
          std::get<1>(formatter)->AddToBuffer(record.Attributes(), sink_settings, msg_info, c, end);
          break;
        }
        case 2: {
          segment_size = static_cast<unsigned>(std::get<2>(formatter).size());
          std::copy(std::get<2>(formatter).begin(), std::get<2>(formatter).end(), c);
          break;
        }
      }

      c += segment_size;
    }
    // Add terminator.
    std::copy(sink_settings.message_terminator.begin(), sink_settings.message_terminator.end(), c);

    // Return the formatted message.
    return buffer;
  }

  NO_DISCARD std::unique_ptr<BaseMessageFormatter> Copy() const override {
    return std::unique_ptr<BaseMessageFormatter>(new RecordFormatter(*this));
  }

  RecordFormatter& AddMsgSegment() {
    formatters_.emplace_back(MSG);
    return *this;
  }

  RecordFormatter& AddAttributeFormatter(std::shared_ptr<AttributeFormatter> formatter) {
    formatters_.emplace_back(std::move(formatter));
    return *this;
  }

  RecordFormatter& AddLiteralSegment(std::string literal) {
    formatters_.emplace_back(std::move(literal));
    return *this;
  }

  RecordFormatter& ClearSegments() {
    formatters_.clear();
    return *this;
  }

  NO_DISCARD std::size_t NumSegments() const { return formatters_.size(); }

private:
  std::vector<std::variant<MSG_t, std::shared_ptr<AttributeFormatter>, std::string>> formatters_;
};

}  // namespace formatting

namespace flush {

//! \brief Base class for objects that determine whether a sink should be flushed.
class FlushHandler : public ImplBase {
public:
  class Impl : public ImplBase::Impl {
  public:
    virtual bool DoFlush(const Record& record) = 0;
  };

  bool DoFlush(const Record& record) { return impl<FlushHandler>()->DoFlush(record); }
  explicit FlushHandler(const std::shared_ptr<Impl>& impl)
      : ImplBase(impl) {}
};

//! \brief Flush after every message.
class AutoFlush : public FlushHandler {
public:
  class Impl : public FlushHandler::Impl {
  public:
    bool DoFlush(const Record&) override { return true; }
  };

  AutoFlush()
      : FlushHandler(std::make_shared<Impl>()) {}
};

//! \brief Flush after every N messages.
class FlushEveryN : public FlushHandler {
public:
  class Impl : public FlushHandler::Impl {
  public:
    explicit Impl(std::size_t N)
        : N_(N) {
      LL_REQUIRE(0 < N, "N cannot be 0");
    }

    bool DoFlush(const Record&) override {
      ++count_;
      count_ %= N_;
      return count_ == 0;
    }

  private:
    std::size_t count_ {}, N_;
  };

  explicit FlushEveryN(std::size_t N)
      : FlushHandler(std::make_shared<Impl>(N)) {}
};

class DisjunctionFlushHandler : public FlushHandler {
public:
  class Impl : public FlushHandler::Impl {
  public:
    Impl(FlushHandler lhs, FlushHandler rhs)
        : lhs_(std::move(lhs))
        , rhs_(std::move(rhs)) {}
    bool DoFlush(const Record& record) override { return lhs_.DoFlush(record) || rhs_.DoFlush(record); }

  private:
    FlushHandler lhs_, rhs_;
  };
  DisjunctionFlushHandler(FlushHandler lhs, FlushHandler rhs)
      : FlushHandler(std::make_shared<Impl>(std::move(lhs), std::move(rhs))) {}
};

class ConjunctionFlushHandler : public FlushHandler {
public:
  class Impl : public FlushHandler::Impl {
  public:
    Impl(FlushHandler lhs, FlushHandler rhs)
        : lhs_(std::move(lhs))
        , rhs_(std::move(rhs)) {}
    bool DoFlush(const Record& record) override { return lhs_.DoFlush(record) && rhs_.DoFlush(record); }

  private:
    FlushHandler lhs_, rhs_;
  };
  ConjunctionFlushHandler(FlushHandler lhs, FlushHandler rhs)
      : FlushHandler(std::make_shared<Impl>(std::move(lhs), std::move(rhs))) {}
};

inline DisjunctionFlushHandler operator||(const FlushHandler& lhs, const FlushHandler& rhs) {
  return {lhs, rhs};
}

inline ConjunctionFlushHandler operator&&(const FlushHandler& lhs, const FlushHandler& rhs) {
  return {lhs, rhs};
}

}  // namespace flush

//! \brief  Base class for sink backends, which are responsible for actually handling the record and
//!         doing something with it.
//!         Not responsible for thread synchronization, this is the job of the frontend.
class SinkBackend {
public:
  //! \brief Flush the sink upon deletion.
  virtual ~SinkBackend() { Flush(); }

  //! \brief Dispatch a record.
  void Dispatch(std::optional<std::string>&& formatted_message, const Record& record) {
    dispatch(std::move(formatted_message), record);
    if (flush_handler_ && flush_handler_->DoFlush(record)) {
      Flush();
    }
  }

  //! \brief Flush the sink. This is implementation defined.
  SinkBackend& Flush() {
    flush();
    return *this;
  }

  //! \brief Get the sink formatting settings.
  NO_DISCARD const FormattingSettings& GetFormattingSettings() const { return settings_; }

  template<typename FlushHandler_t, typename... Args_t>
  SinkBackend& CreateFlushHandler(Args_t&&... args) {
    flush_handler_ = FlushHandler_t(std::forward<Args_t>(args)...);
    return *this;
  }

  SinkBackend& SetFlushHandler(flush::FlushHandler&& flush_handler) {
    flush_handler_ = std::move(flush_handler);
    return *this;
  }

  SinkBackend& ClearFlushHandler() {
    flush_handler_ = std::nullopt;
    return *this;
  }

protected:
  virtual void dispatch(std::optional<std::string>&& formatted_message, const Record& record) = 0;

  //! \brief Protected implementation of flushing the sink.
  virtual void flush() {}

  //! \brief The sink formatting settings.
  FormattingSettings settings_;

  std::optional<flush::FlushHandler> flush_handler_ {};

  //! \brief Whether to automatically flush the sink after each message.
  bool auto_flush_ = false;
};

//! \brief  Base class for sink frontends. These are responsible for common tasks, like filtering
//!         and controlling access to the sink backend, i.e. synchronization.
class Sink {
public:
  //! \brief
  explicit Sink(std::unique_ptr<SinkBackend>&& backend)
      : sink_backend_(std::move(backend))
      , formatter_(formatting::MakeStandardFormatter()) {}

  virtual ~Sink() = default;

  NO_DISCARD bool WillAccept(const RecordAttributes& attributes) const {
    return filter_.WillAccept(attributes);
  }

  NO_DISCARD bool WillAccept(std::optional<Severity> severity) const { return filter_.WillAccept(severity); }

  //! \brief Get the AttributeFilter for the sink.
  filter::AttributeFilter& GetFilter() { return filter_; }

  //! \brief Dispatch a record.
  virtual void Dispatch(const Record& record) { dispatch(record); }

  // ==============================================================================================
  //  Pass-through methods to the backend.
  // ==============================================================================================

  //! \brief Get the record formatter.
  formatting::BaseMessageFormatter& GetFormatter() { return *formatter_; }

  //! \brief Set the sink's formatter.
  void SetFormatter(std::unique_ptr<formatting::BaseMessageFormatter>&& formatter) {
    formatter_ = std::move(formatter);
  }

  //! \brief 'Flush' the sink. This is implementation defined.
  Sink& Flush() {
    sink_backend_->Flush();
    return *this;
  }

  SinkBackend& GetBackend() { return *sink_backend_; }

protected:
  virtual void dispatch(const Record& record) = 0;

  //! \brief The sink backend, to which the frontend feeds record.
  std::unique_ptr<SinkBackend> sink_backend_ {};

  //! \brief The Sink's attribute filters.
  filter::AttributeFilter filter_;

  //! \brief The sink's formatter.
  std::unique_ptr<formatting::BaseMessageFormatter> formatter_;

  std::optional<flush::FlushHandler> flush_handler_ {};
};

//! \brief  Sink frontend that uses no synchronization methods.
class UnlockedSink : public Sink {
public:
  explicit UnlockedSink(std::unique_ptr<SinkBackend>&& backend)
      : Sink(std::move(backend)) {}

  template<typename SinkBackend_t, typename... Args_t>
  static std::shared_ptr<UnlockedSink> From(Args_t&&... args) {
    static_assert(std::is_base_of_v<SinkBackend, SinkBackend_t>, "sink type must be a child of SinkBackend");
    return std::make_shared<UnlockedSink>(std::make_unique<SinkBackend_t>(std::forward<Args_t>(args)...));
  }

private:
  void dispatch(const Record& record) override {
    auto message = formatter_->Format(record, sink_backend_->GetFormattingSettings());
    sink_backend_->Dispatch(std::move(message), record);
  }
};

//! \brief  Sink frontend that uses a mutex to control access.
class SynchronousSink : public Sink {
public:
  explicit SynchronousSink(std::unique_ptr<SinkBackend>&& backend)
      : Sink(std::move(backend)) {}

  template<typename SinkBackend_t, typename... Args_t>
  static std::shared_ptr<SynchronousSink> From(Args_t&&... args) {
    static_assert(std::is_base_of_v<SinkBackend, SinkBackend_t>, "sink type must be a child of SinkBackend");
    return std::make_shared<SynchronousSink>(std::make_unique<SinkBackend_t>(std::forward<Args_t>(args)...));
  }

private:
  void dispatch(const Record& record) override {
    auto message = formatter_->Format(record, sink_backend_->GetFormattingSettings());
    {
      std::lock_guard guard(lock_);
      sink_backend_->Dispatch(std::move(message), record);
    }
  }

  std::mutex lock_;
};

//! \brief  Create a new sink frontend / backend pair.
template<typename Frontend_t, typename Backend_t, typename... Args_t>
std::shared_ptr<Sink> NewSink(Args_t&&... args) {
  return std::make_shared<Frontend_t>(std::make_unique<Backend_t>(std::forward<Args_t>(args)...));
}

//! \brief  Object that can receive records from multiple loggers, and dispatches them to multiple sinks.
//!         The core has its own filter, which is checked before any of the individual sinks' filters.
class Core {
public:
  bool WillAccept(const RecordAttributes& attributes) {
    // If there are no sinks, there are no things that *can* accept.
    if (sinks_.empty()) {
      return false;
    }
    // Core level filtering.
    if (!core_filter_.WillAccept(attributes)) {
      return false;
    }
    // Check that at least one sink will accept.
    return std::any_of(
        sinks_.begin(), sinks_.end(), [attributes](auto& sink) { return sink->WillAccept(attributes); });
  }

  bool WillAccept(std::optional<Severity> severity) {
    if (!core_filter_.WillAccept(severity)) {
      return false;
    }
    // Check that at least one sink will accept.
    return std::any_of(
        sinks_.begin(), sinks_.end(), [severity](auto& sink) { return sink->WillAccept(severity); });
  }

  //! \brief Dispatch a ref bundle to the sinks.
  void Dispatch(const Record& record) {
    for (auto& sink : sinks_) {
      sink->Dispatch(record);
    }
  }

  //! \brief Add a sink to the core.
  Core& AddSink(std::shared_ptr<Sink> sink) {
    sinks_.emplace_back(std::move(sink));
    return *this;
  }

  //! \brief Set the formatter for every sink the core points at.
  Core& SetAllFormatters(const std::unique_ptr<formatting::BaseMessageFormatter>& formatter) {
    for (auto& sink : sinks_) {
      sink->SetFormatter(formatter->Copy());
    }
    return *this;
  }

  //! \brief Get the core level filter.
  filter::AttributeFilter& GetFilter() { return core_filter_; }

  NO_DISCARD const std::vector<std::shared_ptr<Sink>>& GetSinks() const { return sinks_; }

  // TODO: Unit test.
  template<typename Func>
  Core& ApplyToAllSink(Func&& func) {
    std::for_each(sinks_.begin(), sinks_.end(), [f = std::forward<Func>(func)](auto& sink) { f(*sink); });
    return *this;
  }

  //! \brief Flush all sinks.
  void Flush() const {
    std::for_each(sinks_.begin(), sinks_.end(), [](auto& sink) { sink->Flush(); });
  }

private:
  //! \brief All sinks the core will dispatch messages to.
  std::vector<std::shared_ptr<Sink>> sinks_;

  //! \brief Core-level attribute filters.
  filter::AttributeFilter core_filter_;
};

// ==============================================================================
//  Definitions of Record functions that have to go after Core is defined.
// ==============================================================================

bool Record::TryOpen(std::shared_ptr<Core> core) {
  if (core->WillAccept(attributes_)) {
    core_ = std::move(core);
    return true;
  }
  return false;
}

Record::operator bool() const {
  return core_ != nullptr;
}

void Record::Dispatch() {
  if (core_) {
    core_->Dispatch(*this);
    core_ = nullptr;  // One time use.
  }
}

// ==============================================================================
//  Logger base class.
// ==============================================================================

//! \brief Structure that serves as a tag that a Logger should not be created with a core.
struct NoCore_t {};

//! \brief Prototypical NoCore_t object.
inline constexpr NoCore_t NoCore;

//! \brief Base logger class. Capable of creating logging records which route messages
//! to the logger's logging core.
class Logger {
public:
  //! \brief Create a logger with a new core.
  Logger()
      : core_(std::make_shared<Core>()) {}

  //! \brief Create a logger without a logging core.
  explicit Logger(NoCore_t)
      : core_(nullptr) {}

  //! \brief Create a logger with a new core and a single sink.
  explicit Logger(const std::shared_ptr<Sink>& sink)
      : core_(std::make_shared<Core>()) {
    core_->AddSink(sink);
  }

  //! \brief Create a logger with the specified logging core.
  explicit Logger(std::shared_ptr<Core> core)
      : core_(std::move(core)) {}

  ~Logger() {
    Flush();  // Make sure all sinks that the logger is connected to is flushed.
  }

  template<typename... Attrs_t>
  RecordDispatcher Log(BasicAttributes basic_attributes = {}, Attrs_t&&... attrs) const {
    if (!core_) {
      return {};
    }
    if (do_time_stamp_) {
      basic_attributes.time_stamp = generator_.CurrentTime();
    }
    if (!logger_name_.empty()) {
      basic_attributes.logger_name = logger_name_;
    }
    return RecordDispatcher(core_, basic_attributes, attrs...);
  }

  template<typename... Attrs_t>
  RecordDispatcher Log(std::optional<Severity> severity, Attrs_t&&... attrs) const {
    return Log(BasicAttributes(severity), attrs...);
  }

  //! \brief  Create a record dispatcher, setting severity, file name, line number,
  //!         and any other attributes.
  template<typename... Attrs_t>
  RecordDispatcher LogWithLocation(std::optional<Severity> severity,
                                   const char* file_name,
                                   unsigned line_number,
                                   Attrs_t&&... attrs) {
    return Log(BasicAttributes(severity, file_name, line_number), attrs...);
  }

  NO_DISCARD bool WillAccept(std::optional<Severity> severity) const {
    if (!core_)
      return false;
    return core_->WillAccept(severity);
  }

  NO_DISCARD const std::shared_ptr<Core>& GetCore() const { return core_; }

  Logger& SetCore(std::shared_ptr<Core> core) {
    core_ = std::move(core);
    return *this;
  }

  Logger& SetDoTimeStamp(bool do_timestamp) {
    do_time_stamp_ = do_timestamp;
    return *this;
  }

  Logger& SetName(const std::string& logger_name) {
    logger_name_ = logger_name;
    return *this;
  }

  //! \brief Notify the core to flush all of its sinks.
  void Flush() const {
    if (core_)
      core_->Flush();
  }

private:
  //! \brief Whether to generate a time stamp for the logs.
  bool do_time_stamp_ = true;

  //! \brief A generator to create the DateTime timestamps for records.
  time::FastDateGenerator generator_;

  //! \brief A name for the logger.
  std::string logger_name_ {};

  //! \brief The logger's core.
  std::shared_ptr<Core> core_;

  //! \brief Attributes associated with a specific logger.
  std::vector<Attribute> logger_attributes_;
};

//! \brief A sink, used for testing, that does nothing.
class EmptySink : public SinkBackend {
public:
  EmptySink() { settings_.needs_formatting = false; }

private:
  void dispatch(std::optional<std::string>&&, const Record&) override {}
};

//! \brief A sink, used for testing, that formats a record, but does not stream the result anywhere.
//!
//! Primarily for timing and testing.
class TrivialDispatchSink : public SinkBackend {
private:
  void dispatch([[maybe_unused]] std::optional<std::string>&& formatted_message,
                [[maybe_unused]] const Record& record) override {}
};

//! \brief A simple sink that writes to a file via an ofstream.
class FileSink : public SinkBackend {
public:
  explicit FileSink(const std::string& file)
      : fout_(file) {}
  ~FileSink() override { fout_.flush(); }

private:
  void dispatch([[maybe_unused]] std::optional<std::string>&& formatted_message,
                [[maybe_unused]] const Record& record) override {
    if (formatted_message) {
      fout_.write(formatted_message->c_str(), static_cast<std::streamsize>(formatted_message->size()));
    }
  }

  void flush() override { fout_.flush(); }

  std::ofstream fout_;
};

//! \brief A sink that writes to any ostream.
class OstreamSink : public SinkBackend {
public:
  ~OstreamSink() override { out_.flush(); }

  explicit OstreamSink(std::ostringstream& stream)
      : out_(stream) {
    // By default, string streams do not support vterm.
    settings_.has_virtual_terminal_processing = false;
  }

  explicit OstreamSink(std::ostream& stream = std::cout)
      : out_(stream) {
    settings_.has_virtual_terminal_processing = true;  // Default this to true
  }

private:
  void dispatch([[maybe_unused]] std::optional<std::string>&& formatted_message,
                [[maybe_unused]] const Record& record) override {
    if (formatted_message) {
      out_.write(formatted_message->c_str(), static_cast<std::streamsize>(formatted_message->size()));
    }
  }

  void flush() override { out_.flush(); }

  //! \brief A reference to the stream to write to.
  std::ostream& out_;
};

// ==============================================================================
//  Global logger.
// ==============================================================================

//! \brief  The global interface for logging.
//!
class Global {
public:
  Global() = delete;

  static std::shared_ptr<Core> GetCore() {
    if (!global_core_) {
      global_core_ = std::make_shared<Core>();
    }
    return global_core_;
  }

  //! \brief  Get the global logger.
  //!
  static Logger& GetLogger() {
    if (!logger_) {
      logger_ = Logger(GetCore());
    }
    return *logger_;
  }

private:
  inline static std::shared_ptr<Core> global_core_ = std::shared_ptr<Core>();
  inline static std::optional<Logger> logger_ {};
};

// ==============================================================================
//  Logging macros.
// ==============================================================================

//! \brief Log with severity to a specific logger. First does a very fast check whether the
//! message would be accepted given its severity level, since this is a very common case.
//! If it will be, creates a handler, constructing the record in-place inside the handler.
#define LOG_SEV_TO(logger, severity) \
  if ((logger).WillAccept(::lightning::Severity::severity)) \
    if (auto handler = (logger).LogWithLocation(::lightning::Severity::severity, __FILE__, __LINE__)) \
  handler.GetRecord().Bundle()

//! \brief Log with a severity attribute to the global logger.
#define LOG_SEV(severity) LOG_SEV_TO(::lightning::Global::GetLogger(), severity)

#define LOG_TO(logger) \
  if ((logger).WillAccept(::std::nullopt)) \
    if (auto handler = (logger).LogWithLocation(::std::nullopt, __FILE__, __LINE__)) \
  handler.GetRecord().Bundle()

#define LOG() LOG_TO(::lightning::Global::GetLogger())

namespace formatting {

namespace detail {

template<std::size_t I, typename... Args_t>
unsigned sizeRequired(const std::tuple<Segment<Args_t>...>& segments,
                      std::size_t count_placed,
                      const FormattingSettings& settings,
                      MessageInfo& msg_info) {
  if (count_placed == I) {
    return 0u;
  }
  msg_info.message_length = 0;
  unsigned size = std::get<I>(segments).SizeRequired(settings, msg_info);
  if constexpr (I + 1 < sizeof...(Args_t)) {
    size += sizeRequired<I + 1>(segments, count_placed, settings, msg_info);
    msg_info.message_length += size;
  }
  return size;
}

template<std::size_t I, typename... Args_t, std::size_t... Indices>
void formatHelper(char* c,
                  const char* const* starts,
                  const char* const* ends,
                  std::size_t actual_substitutions,
                  const std::tuple<Segment<Args_t>...>& segments,
                  const FormattingSettings& settings,
                  MessageInfo& msg_info,
                  std::index_sequence<Indices...> is) {
  constexpr auto N = sizeof...(Indices);
  // Write formatting segment, then arg segment, then call formatHelper again.
  if (starts[I] < ends[I]) {
    c = std::copy(starts[I], ends[I], c);
  }
  if (I == actual_substitutions) {
    return;
  }
  if constexpr (I < sizeof...(Args_t)) {
    auto& segment = std::get<I>(segments);
    auto size = segment.SizeRequired(settings, msg_info);
    segment.AddToBuffer(settings, msg_info, c, c + size);
    msg_info.message_length += size;
    c += size;
  }
  if constexpr (I < N) {
    formatHelper<I + 1>(c, starts, ends, actual_substitutions, segments, settings, msg_info, is);
  }
}

}  // namespace detail

template<typename... Args_t>
std::string Format(const FormattingSettings& settings, const char* fmt_string, const Args_t&... args) {
  constexpr auto N = sizeof...(args);
  const char *starts[N + 1], *ends[N + 1];

  unsigned count_placed = 0, str_length = 0;
  starts[0] = fmt_string;
  auto c = fmt_string;
  for (; *c != 0; ++c) {
    if (*c == '%' && count_placed < N) {
      ends[count_placed++] = c;
      starts[count_placed] = c + 1;
    }
    else {
      ++str_length;
    }
  }
  ends[count_placed] = c;

  auto segments =
      std::make_tuple(Segment<std::decay_t<typetraits::remove_cvref_t<Args_t>>>(args, nullptr, nullptr)...);
  MessageInfo msg_info;
  unsigned format_size = detail::sizeRequired<0>(segments, count_placed, settings, msg_info);
  std::string buffer(format_size + str_length, ' ');
  detail::formatHelper<0>(&buffer[0],
                          starts,
                          ends,
                          count_placed,
                          segments,
                          settings,
                          msg_info,
                          std::make_index_sequence<sizeof...(args)> {});
  return buffer;
}

template<typename... Args_t>
std::string FormatTo(char* buffer,
                     const char* end,
                     const FormattingSettings& settings,
                     const char* fmt_string,
                     const Args_t&... args) {
  LL_REQUIRE(buffer <= end, "cannot have the buffer start after the buffer end");
  constexpr auto N = sizeof...(args);
  const char *starts[N + 1], *ends[N + 1];

  unsigned count_placed = 0, str_length = 0;
  starts[0] = fmt_string;
  auto c = fmt_string;
  for (; *c != 0; ++c) {
    if (*c == '%' && count_placed < N) {
      ends[count_placed++] = std::min(c, end);
      starts[count_placed] = std::min(c + 1, end);
    }
    else {
      ++str_length;
    }
  }
  ends[count_placed] = std::min(c, end);

  auto segments =
      std::make_tuple(Segment<std::decay_t<typetraits::remove_cvref_t<Args_t>>>(args, nullptr, nullptr)...);
  detail::formatHelper<0>(&buffer[0],
                          starts,
                          ends,
                          count_placed,
                          segments,
                          settings,
                          std::make_index_sequence<sizeof...(args)> {});
  return buffer;
}

template<typename... Args_t>
std::string Format(const char* fmt_string, const Args_t&... args) {
  return Format(FormattingSettings {}, fmt_string, args...);
}

}  // namespace formatting

}  // namespace lightning