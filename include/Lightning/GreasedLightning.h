//
// Created by Nathaniel Rupprecht on 6/6/23.
//

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <utility>
#include <vector>
#include <set>
#include <type_traits>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <map>
#include <charconv>

namespace lightning {


// ==============================================================================
//  Macro definitions.
// ==============================================================================

#define NO_DISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]

//! \brief  Macro that allows you to create a type trait based on whether a statement about a type called Value_t is valid.
//!         For example, to make a type trait called can_i_stream_this_v<T> that will be true if T can be streamed into std::cout,
//!         you can write:
//!         NEW_TYPE_TRAIT(can_i_stream_this_v, std::cout << std::declval<T>());
//!
#define NEW_TYPE_TRAIT(trait_name, trait_test)                                      \
namespace detail_traits_##trait_name {                                              \
  template<typename Value_t>                                                        \
  inline auto test_##trait_name (int) -> decltype((trait_test), std::true_type{});  \
                                                                                    \
  template<typename Value_t>                                                        \
  inline auto test_##trait_name (...) -> std::false_type;                           \
                                                                                    \
  template<typename Value_t> struct trait_class_##trait_name {                      \
    static constexpr bool value = decltype(test_##trait_name<Value_t>(0))::value;   \
  };                                                                                \
}                                                                                   \
template<typename Value_t> static constexpr bool trait_name                         \
  = detail_traits_##trait_name::trait_class_##trait_name<Value_t>::value;

// ==============================================================================
//  Error checking and handling.
// ==============================================================================

#define LL_REQUIRE(condition, message) { \
  if (!(condition)) {                       \
    std::ostringstream _strm_; \
    _strm_ << message; \
    throw std::runtime_error(_strm_.str()); \
  }                                         \
}

#define LL_ASSERT(condition, message) { \
  if (!(condition)) {                       \
    std::ostringstream _strm_; \
    _strm_ << message; \
    throw std::runtime_error(_strm_.str()); \
  }                                         \
}

#define LL_FAIL(message) { \
  std::ostringstream _strm_; \
  _strm_ << message; \
  throw std::runtime_error(_strm_.str()); \
}

// ==============================================================================
//  Type traits.
// ==============================================================================

namespace typetraits { // namespace lightning::typetraits

//! \brief  Define a type trait that determines whether a type can be ostream'ed.
NEW_TYPE_TRAIT(is_ostreamable_v, std::declval<std::ostream&>() << std::declval<Value_t>());

NEW_TYPE_TRAIT(has_to_string_v, to_string(std::declval<Value_t>()));

namespace detail_always_false {

template <typename T>
struct always_false {
  static constexpr bool value = false;
};

} // detail_always_false

//! \brief  "Type trait" that is always false, useful in static_asserts, since right now, you cannot static_assert(false).
template <typename T>
inline constexpr bool always_false_v = detail_always_false::always_false<T>::value;

}; // namespace typetraits.


class ImplBase {
 public:
  class Impl {
   public:
    virtual ~Impl() = default;
  };

  template <typename Concrete_t>
  NO_DISCARD MAYBE_UNUSED bool IsType() const {
    return dynamic_cast<typename Concrete_t::Impl>(impl_.get());
  }

 protected:
  template <typename Concrete_t>
  typename Concrete_t::Impl* impl() {
    return reinterpret_cast<typename Concrete_t::Impl*>(impl_.get());
  }

  template <typename Concrete_t>
  NO_DISCARD const typename Concrete_t::Impl* impl() const {
    return reinterpret_cast<const typename Concrete_t::Impl*>(impl_.get());
  }

  std::shared_ptr<Impl> impl_ = nullptr;

  explicit ImplBase(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}
};

// ...

// ==============================================================================
//  Date / time support.
// ==============================================================================

namespace time { // namespace lightning::time

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
  static int days_in_month_[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month != 2) {
    return days_in_month_[month];
  }
  return IsLeapYear(year) ? 29 : 28;
}

enum class Month {
  January = 1, February, March, April, May, June, July, August, September, October, November, December
};

inline std::string MonthAbbreviation(Month m) {
  static std::string abbrev_[]{"", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return abbrev_[static_cast<int>(m)];
}

inline Month MonthIntToMonth(int month) {
  LL_REQUIRE(0 < month && month < 13, "month must be in the range [1, 12], not " << month);
  static Month months_[]{Month::January, Month::February, Month::March, Month::April,
                         Month::May, Month::June, Month::July, Month::August,
                         Month::September, Month::October, Month::November, Month::December};
  return months_[month - 1];
}

//! \brief  A class that represents a date and time, all the way down to the millisecond.
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
  //!         Note: localtime (and its related variants, and gmtime) are relatively slow functions. Prefer using
  //!         FastDateGenerator if you need to repeatedly generate DateTimes.
  //!
  explicit DateTime(const std::chrono::time_point<std::chrono::system_clock>& time_point) {
    auto time = std::chrono::system_clock::now();
    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    int ms = static_cast<int>((duration_cast<std::chrono::microseconds>(time.time_since_epoch()) % 1'000'000).count());
    // convert to std::time_t in order to convert to std::tm (broken time)
    auto t = std::chrono::system_clock::to_time_t(time);

    // Convert time - this is expensive.
    std::tm* now = std::localtime(&t);

    setYMD(now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, false);
    setHMSUS(now->tm_hour, now->tm_min, now->tm_sec, ms, false);
  }

  explicit DateTime(int yyyymmdd) : DateTime(yyyymmdd / 10000, (yyyymmdd / 100) % 100, yyyymmdd % 100) {}

  NO_DISCARD int AsYYYYMMDD() const {
    return GetYear() * 10000 + GetMonthInt() * 100 + GetDay();
  }

  NO_DISCARD int GetYear() const { return static_cast<int>(y_m_d_h_m_s_um_ >> shift_year_); }
  NO_DISCARD int GetMonthInt() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_month_) & month_mask_); }
  NO_DISCARD Month GetMonth() const { return MonthIntToMonth(GetMonthInt()); }
  NO_DISCARD int GetDay() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_day_) & day_mask_); }
  NO_DISCARD int GetHour() const { return static_cast<int>(y_m_d_h_m_s_um_ >> shift_hour_) & hour_mask_; }
  NO_DISCARD int GetMinute() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_minute_) & minute_mask_); }
  NO_DISCARD int GetSecond() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_second_) & second_mask_); }
  NO_DISCARD int GetMillisecond() const { return GetMicrosecond() / 1000; }
  NO_DISCARD int GetMicrosecond() const { return static_cast<int>(y_m_d_h_m_s_um_ & us_mask_); }

  //! \brief Check if the date is a non-null (empty) date.
  explicit operator bool() const { return y_m_d_h_m_s_um_ != 0; }

  //! \brief Check if two DateTime are equal.
  bool operator==(const DateTime& dt) const {
    return y_m_d_h_m_s_um_ == dt.y_m_d_h_m_s_um_;
  }

  //! \brief Check if two DateTime are not equal.
  bool operator!=(const DateTime& dt) const {
    return y_m_d_h_m_s_um_ != dt.y_m_d_h_m_s_um_;
  }

  //! \brief Check if one DateTime is less than the other.
  bool operator<(const DateTime& dt) const {
    return y_m_d_h_m_s_um_ < dt.y_m_d_h_m_s_um_;
  }

  //! \brief  Get the current clock time, in the local timezone.
  //!
  //!         Note: localtime (and its related variants, and gmtime) are relatively slow functions. Prefer using
  //!         FastDateGenerator if you need to repeatedly generate DateTimes.
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
  DateTime(bool, int year, int month, int day, int hour = 0, int minute = 0, int second = 0, int microsecond = 0) {
    setYMD(year, month, day, false);
    setHMSUS(hour, minute, second, microsecond, false);
  }

  void setYMD(int year, int month, int day, bool validate = true) {
    if (validate) validateYMD(year, month, day);
    // Zero previous y-m-d.
    y_m_d_h_m_s_um_ = (y_m_d_h_m_s_um_ << 32) >> 32;
    y_m_d_h_m_s_um_ |=
        (static_cast<long>(year) << shift_year_)
            | (static_cast<long>(month) << shift_month_)
            | (static_cast<long>(day) << shift_day_);
  }

  void setHMSUS(int hour, int minute, int second, int microseconds, bool validate = true) {
    if (validate) validateHMSUS(hour, minute, second, microseconds);
    // Zero previous h-m-s-us.
    y_m_d_h_m_s_um_ = (y_m_d_h_m_s_um_ >> 32) << 32;
    y_m_d_h_m_s_um_ |= (static_cast<long>(hour) << shift_hour_)
        | (static_cast<long>(minute) << shift_minute_)
        | (static_cast<long>(second) << shift_second_)
        | microseconds;
  }

  static void validateYMD(int year, int month, int day) {
    LL_REQUIRE(0 < year, "year must be > 0");
    LL_REQUIRE(0 < month && month <= 12, "month must be in the range [1, 12]")
    LL_REQUIRE(0 < day < DaysInMonth(month, year), "there are only " << DaysInMonth(month, year) << " days in " << year << "-" << month);
  }

  static void validateHMSUS(int hour, int minute, int second, int microseconds) {
    // I am ignoring things like the time change and leap seconds.
    LL_REQUIRE(0 <= hour && hour < 24, "hour must be in the range [0, 24)");
    LL_REQUIRE(0 <= minute && minute < 60, "minute must be in the range [0, 60)");
    LL_REQUIRE(0 <= second && second < 60, "second must be in the range [0, 60)");
    LL_REQUIRE(0 <= microseconds && microseconds < 1'000'000, "microseconds must be in the range [0, 1,000,000)");
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
  long y_m_d_h_m_s_um_{};

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

DateTime AddMicroseconds(const DateTime& time, unsigned long long microseconds) {
  int new_years = time.GetYear(), new_months = time.GetMonthInt(), new_days = time.GetDay();
  int new_hours = time.GetHour(), new_minutes = time.GetMinute(), new_seconds = time.GetSecond(), new_us = time.GetMicrosecond();

  auto new_us_overflow = time.GetMicrosecond() + microseconds;
  new_us = static_cast<int>(new_us_overflow % 1'000'000);

  auto carry_seconds = new_us_overflow / 1'000'000;
  if (carry_seconds == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  auto new_seconds_overflow = new_seconds + carry_seconds;
  new_seconds = static_cast<int>(new_seconds_overflow % 60);

  auto carry_minutes = new_seconds_overflow / 60;
  if (carry_minutes == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  auto new_minutes_overflow = new_minutes + carry_minutes;
  new_minutes = static_cast<int>(new_minutes_overflow % 60);

  auto carry_hours = new_minutes_overflow / 60;
  if (carry_hours == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  auto new_hours_overflow = new_hours + carry_hours;
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

class FastDateGenerator {
 public:
  FastDateGenerator()
      : start_time_point_(std::chrono::system_clock::now()), base_date_time_(start_time_point_) {}

  NO_DISCARD DateTime CurrentTime() const {
    auto current_time = std::chrono::system_clock::now();

    auto us = duration_cast<std::chrono::microseconds>(current_time - start_time_point_).count();
    return AddMicroseconds(base_date_time_, us);
  }

 private:

  //! \brief Keep track of when the logger was created.
  std::chrono::time_point<std::chrono::system_clock> start_time_point_;

  DateTime base_date_time_;
};

} // namespace time

// ==============================================================================
//  Formatting.
// ==============================================================================

namespace formatting { // namespace lightning::formatting

struct MessageInfo {
  //! \brief  The indentation of the start of the message within the formatted record.
  unsigned message_indentation = 0;
};

//! \brief Structure that represents part of a message coming from an attribute.
//!
struct Fmt {
  std::string attr_name{}; // The name of the attribute that should be string-ized.
  std::size_t attr_name_hash{}; // The hash of the attribute name.
  std::string attr_fmt{}; // Any formatting that should be used for the attribute.
};

//! \brief  Convert a format string into a set of formatting segments.
//!
//!         This function is defined externally so it can be tested.
//!
inline std::vector<std::variant<std::string, Fmt>> Segmentize(const std::string& fmt_string) {
  std::vector<std::variant<std::string, Fmt>> fmt_segments;

  bool is_escape = false;
  std::string segment{};
  for (auto i = 0u; i < fmt_string.size(); ++i) {
    char c = fmt_string[i];
    if (c == '{' && !is_escape && i + 1 < fmt_string.size()) {
      if (!segment.empty()) {
        fmt_segments.emplace_back(std::move(segment));
        segment.clear();
      }
      Fmt fmt;
      ++i;
      c = fmt_string[i++];
      for (; i < fmt_string.size() && c != '}'; ++i) {
        fmt.attr_name.push_back(c);
        c = fmt_string[i];
      }
      fmt.attr_name_hash = std::hash<std::string>{}(fmt.attr_name);
      fmt_segments.emplace_back(std::move(fmt));
      --i;
    }
    else if (c == '\\' && !is_escape) {
      is_escape = true;
    }
    else {
      segment.push_back(c);
    }
  }
  if (!segment.empty()) {
    fmt_segments.emplace_back(std::move(segment));
  }
  return fmt_segments;
}

} // namespace formatting

// basic_st
// 6,978,975/sec
// 7,131,692/sec
// 6,748,131/sec

// Greased lightning
// 6,310,521/sec
// 6,346,707/sec
// 6,224,098/sec

struct FormattingSettings {
  bool has_virtual_terminal_processing = true;

  //! \brief How to terminate the message, e.g. with a newline.
  std::string message_terminator = "\n";
};

//! \brief  The base class for all message segments.
//!
//!         For efficiency, each segment needs to know how much space its formatted self will take up.
struct BaseSegment {
  explicit BaseSegment() = default;
  virtual ~BaseSegment() = default;

  virtual void AddToBuffer(const FormattingSettings& settings, char* start, char* end) const = 0;
  NO_DISCARD virtual unsigned SizeRequired(const FormattingSettings& settings) const = 0;
};

//! \brief Base template for segments.
template <typename T, typename Enable = void>
struct Segment;

//! \brief Template specialization for string segments.
template <>
struct Segment<std::string> : public BaseSegment {
  explicit Segment(const std::string& s)
      : str_(s), size_required_(s.size()) {}

  void AddToBuffer(const FormattingSettings& settings, char* start, char* end) const override {
    for (auto c: str_) {
      if (start == end) {
        break;
      }
      *start = c;
      ++start;
    }
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings) const override {
    return size_required_;
  }

 private:
  const std::string& str_;
  unsigned size_required_{};
};

//! \brief Template specialization for char* segments.
template <>
struct Segment<char*> : public BaseSegment {
  explicit Segment(const char* s) : cstr_(s), size_required_(std::strlen(s)) {}

  void AddToBuffer(const FormattingSettings& settings, char* start, char* end) const override {
    for (auto i = 0u; i < size_required_; ++i, ++start) {
      if (start == end) break;
      *start = cstr_[i];
    }
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings) const override {
    return size_required_;
  }

 private:
  const char* cstr_;
  unsigned size_required_{};
};

//! \brief Template specialization for char arrays. TODO: Can I just combine this with char*?
template <std::size_t N>
struct Segment<char[N]> : public Segment<char*> {
  explicit Segment(const char s[N]) : Segment<char*>(&s[0]) {}
};

template <>
struct Segment<bool> : public BaseSegment {
  explicit Segment(bool b) : value_(b) {}

  void AddToBuffer(const FormattingSettings& settings, char* start, char* end) const override {
    if (value_) strcpy(start, "true");
    else strcpy(start, "false");
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings) const override {
    return value_ ? 4 : 5;
  }

 private:
  bool value_;
};

//! \brief Template specialization for floating point number segments.
template <typename Floating_t>
struct Segment<Floating_t, std::enable_if_t<std::is_floating_point_v<Floating_t>>> : public BaseSegment {
  explicit Segment(Floating_t number)
      : number_(number), size_required_(10) {
    // TEMPORARY
    serialized_number_ = std::to_string(number);
    size_required_ = serialized_number_.size();
  }

  void AddToBuffer(const FormattingSettings& settings, char* start, char* end) const override {
    // std::to_chars(start, end, number_, std::chars_format::fixed);
    std::copy(serialized_number_.begin(), serialized_number_.end(), start);
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings) const override {
    return size_required_;
  }

 private:
  Floating_t number_;
  unsigned size_required_{};

  // TEMPORARY
  std::string serialized_number_;
};

//! \brief Template specialization for integral value segments.
template <typename Integral_t>
struct Segment<Integral_t, std::enable_if_t<std::is_integral_v<Integral_t>>> : public BaseSegment {
  explicit Segment(Integral_t number)
      : number_(number), size_required_(neededPrecision(number_)) {}

  void AddToBuffer(const FormattingSettings& settings, char* start, char* end) const override {
    std::to_chars(start, end, number_);
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings) const override {
    return size_required_;
  }

 private:
  unsigned neededPrecision(Integral_t number) const {
    static const auto log10 = std::log(10);
    number = std::max(2, std::abs(number) + 1);
    auto precision = static_cast<unsigned>(std::ceil(std::log(number) / log10));
    if (number < 0) ++precision; // For minus sign.
    return precision;
  }

  Integral_t number_;
  unsigned size_required_{};
};

class RefBundle {
 public:
  template <typename T>
  RefBundle& operator<<(T&& obj) {
    using decay_t = std::decay_t<std::remove_cvref_t<T>>;

    // Add a formatting segment.
    AddSegment(std::make_unique<Segment < decay_t>>
    (obj));

    return *this;
  }

  explicit RefBundle(unsigned default_size = 10) {
    segments_.reserve(default_size);
  }

  void AddSegment(std::unique_ptr<BaseSegment>&& segment) {
    segments_.emplace_back(std::move(segment));
  }

  NO_DISCARD unsigned SizeRequired(const FormattingSettings& settings) const {
    unsigned size_required = 0;
    for (auto& segment: segments_) {
      size_required += segment->SizeRequired(settings);
    }
    return size_required;
  }

  char* FmtString(const FormattingSettings& settings, char* start, char* end) const {
    // Add message.
    for (auto& bundle: segments_) {
      auto sz = bundle->SizeRequired(settings);
      bundle->AddToBuffer(settings, start, start + sz);
      start += sz;
    }
    return start;
  }

 private:
  std::vector<std::unique_ptr<BaseSegment>> segments_;
};

class Attribute {
 protected:
  class Impl {
   public:
    virtual ~Impl() = default;
    NO_DISCARD virtual std::unique_ptr<Impl> Copy() const = 0;
  };

  std::unique_ptr<Impl> impl_;

 public:
  explicit Attribute(std::unique_ptr<Impl>&& impl) : impl_(std::move(impl)) {}

  Attribute(const Attribute& other) : impl_(other.impl_->Copy()) {}
};

enum class Severity {
  Debug = 0b1, Info = 0b10, Warning = 0b100, Error = 0b1000, Fatal = 0b10000
};

//! \brief  Structure storing very common attributes that a logging message will often have.
//!
//!         Additional attributes can be implemented as Attribute objects.
//!
struct BasicAttributes {
  BasicAttributes() = default;

  BasicAttributes(Severity lvl, bool do_timestamp = false)
      : level(lvl) {
    if (do_timestamp) time_stamp = time::DateTime::Now();
  }

  //! \brief The severity level of the record.
  std::optional<Severity> level{};

  //! \brief The time at which the record was created.
  std::optional<time::DateTime> time_stamp{};
};

class BasicSeverityFilter {
 public:
  NO_DISCARD bool Check(std::optional<Severity> severity) const {
    if (severity) {
      return mask_ & static_cast<int>(severity.value());
    }
    return allow_if_no_severity_;
  }

  BasicSeverityFilter& SetAcceptance(Severity severity, bool does_accept) {
    if (does_accept) mask_ |= static_cast<int>(severity);
    else mask_ &= (0b11111 & ~static_cast<int>(severity));
    return *this;
  }

 private:
  //! \brief The acceptance mask.
  int mask_ = 0b11111;

  //! \brief Whether to accept a message that does not contain the severity attribute.
  bool allow_if_no_severity_ = false;
};

//! \brief Object containing all attributes for a record.
struct RecordAttributes {
  template <typename ...Attrs_t>
  explicit RecordAttributes(BasicAttributes basic_attributes = {}, Attrs_t&& ...attrs)
      : basic_attributes(basic_attributes) {
    attributes.reserve(sizeof...(attrs));
    (attributes.emplace_back(std::move(attrs)), ...);
  }

  //! \brief  The basic record attributes. These are stored as fields, which is much faster to
  //!         create and handle than pImpl attribute objects.
  //!
  BasicAttributes basic_attributes{};

  //! \brief  Additional attributes, beyond the basic attributes.
  //!
  std::vector<Attribute> attributes;
};

namespace filter {

struct AttributeFilter {
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

  NO_DISCARD virtual bool willAccept(const std::vector<Attribute>& attributes) const {
    // TODO.
    return true;
  }

  AttributeFilter& Accept(const std::set<Severity>& acceptable) {
    for (auto sev: {Severity::Debug, Severity::Info, Severity::Warning, Severity::Error, Severity::Fatal}) {
      severity_filter_.SetAcceptance(sev, acceptable.contains(sev));
    }
    return *this;
  }

 private:

  BasicSeverityFilter severity_filter_;
};

} // namespace filter

// Forward declare core.
class Core;

//! \brief The result of logging, a collection of a message, attributes, and values.
//!
class Record {
 public:
  Record() = default;

  template <typename ...Attrs_t>
  explicit Record(BasicAttributes basic_attributes = {}, Attrs_t&& ...attrs)
      : attributes_(basic_attributes, std::forward<Attrs_t>(attrs)...) {}

  RefBundle& Bundle() {
    return bundle_;
  }

  NO_DISCARD const RefBundle& Bundle() const {
    return bundle_;
  }

  NO_DISCARD const RecordAttributes& Attributes() const {
    return attributes_;
  }

  NO_DISCARD RecordAttributes& Attributes() {
    return attributes_;
  }

  bool TryOpen(std::shared_ptr<Core> core);

  explicit operator bool() const;

  void Dispatch();

 private:
  //! \brief The log message, contained as a RefBundle.
  RefBundle bundle_{};

  //! \brief The attributes collection of the message.
  RecordAttributes attributes_{};

  //! \brief The core that the record is destined for, or null if the record is closed.
  std::shared_ptr<Core> core_{};
};

//! \brief An RAII structure that dispatches the contained record upon the destruction of the RecordDispatcher.
class RecordDispatcher {
 public:
  //! \brief Create a closed, empty record.
  RecordDispatcher() = default;

  //! \brief Wrap a record in a record handler.
  explicit RecordDispatcher(Record&& record)
      : record_(std::move(record)), uncaught_exceptions_(std::uncaught_exceptions()) {}

  //! \brief Construct a record handler, constructing the record in-place inside it.
  template <typename ...Attrs_t>
  explicit RecordDispatcher(std::shared_ptr<Core> core, BasicAttributes basic_attributes = {}, Attrs_t&& ...attrs)
      : record_(basic_attributes, std::forward<Attrs_t>(attrs)...), uncaught_exceptions_(std::uncaught_exceptions()) {
    record_.TryOpen(std::move(core));
  }

  //! \brief RecordDispatcher is an RAII structure for dispatching records.
  ~RecordDispatcher() {
    if (std::uncaught_exceptions() <= uncaught_exceptions_) {
      record_.Dispatch();
    }
  }

  //! \brief Check whether a record is open.
  NO_DISCARD bool RecordIsOpen() const {
    return static_cast<bool>(record_);
  }

  //! \brief Check whether a record is open via explicit conversion to bool.
  explicit operator bool() const {
    return RecordIsOpen();
  }

  //! \brief Get the record from the RecordDispatcher.
  Record& GetRecord() {
    return record_;
  }

 private:
  Record record_{};

  int uncaught_exceptions_{};
};

namespace formatting {

//! \brief Base class for attribute formatters, objects that know how to serialize attribute representations to strings.
class AttributeFormatter {
 public:
  virtual void AddToBuffer(const RecordAttributes& attributes, const FormattingSettings& settings, char* start, char* end) const = 0;
  NO_DISCARD virtual unsigned RequiredSize(const RecordAttributes& attributes, const FormattingSettings& settings) const = 0;
};

class SeverityAttributeFormatter : public AttributeFormatter {
 public:
  void AddToBuffer(const RecordAttributes& attributes, const FormattingSettings& settings, char* start, char* end) const override {
    if (attributes.basic_attributes.level) {
      auto& str = getString(attributes.basic_attributes.level.value());
      std::copy(str.begin(), str.end(), start);
    }
  }

  NO_DISCARD unsigned RequiredSize(const RecordAttributes& attributes, const FormattingSettings& settings) const override {
    return attributes.basic_attributes.level ? getString(attributes.basic_attributes.level.value()).size() : 0u;
  }

 private:
  NO_DISCARD const std::string& getString(Severity severity) const {
    switch (severity) {
      case Severity::Debug: return debug_;
      case Severity::Info: return info_;
      case Severity::Warning: return warn_;
      case Severity::Error: return error_;
      case Severity::Fatal: return fatal_;
      default: LL_FAIL("unrecognized severity attribute");
    }
  }

  std::string debug_ = "Debug  ";
  std::string info_ = "Info   ";
  std::string warn_ = "Warning";
  std::string error_ = "Error  ";
  std::string fatal_ = "Fatal  ";
};


class RecordFormatter {
 public:
  //! \brief The default record formatter just prints the message.
  RecordFormatter() {
    AddMsgSegment();
  }

  std::string Format(const Record& record, const FormattingSettings& sink_settings) {
    std::optional<unsigned> msg_size{};

    unsigned required_size = 0;
    for (auto i = 0u; i < formatters_.size(); ++i) {
      auto& formatter = formatters_[i];
      unsigned size = 0;
      switch (formatter.index()) {
        case 0: {
          if (!msg_size) msg_size = record.Bundle().SizeRequired(sink_settings);
          size = msg_size.value();
          break;
        }
        case 1: {
          size = std::get<1>(formatter)->RequiredSize(record.Attributes(), sink_settings);
          break;
        }
        case 2: {
          size = std::get<2>(formatter).size();
          break;
        }
      }

      required_size += size;
      required_sizes_[i] = size;
    }

    // Account for message terminator.
    required_size += sink_settings.message_terminator.size();

    std::string buffer(required_size, ' ');
    char* c = &buffer[0], * end = c + required_size;

    for (auto i = 0u; i < formatters_.size(); ++i) {
      auto& formatter = formatters_[i];
      switch (formatter.index()) {
        case 0: {
          // TODO: Update for LogNewLine type alignment.
          record.Bundle().FmtString(sink_settings, c, end);
          break;
        }
        case 1: {
          std::get<1>(formatter)->AddToBuffer(record.Attributes(), sink_settings, c, end);
          break;
        }
        case 2: {
          std::copy(std::get<2>(formatter).begin(), std::get<2>(formatter).end(), c);
          break;
        }
      }

      c += required_sizes_[i];
    }
    // Add terminator.
    std::copy(sink_settings.message_terminator.begin(), sink_settings.message_terminator.end(), c);

    // Return the formatted message.
    return buffer;
  }

  RecordFormatter& AddMsgSegment() {
    required_sizes_.emplace_back(0);
    formatters_.emplace_back(MSG{});
    return *this;
  }

  RecordFormatter& AddAttributeFormatter(std::shared_ptr<AttributeFormatter> formatter) {
    required_sizes_.emplace_back(0);
    formatters_.emplace_back(std::move(formatter));
    return *this;
  }

  RecordFormatter& AddLiteralSegment(std::string literal) {
    required_sizes_.emplace_back(literal.size());
    formatters_.emplace_back(std::move(literal));
    return *this;
  }

  RecordFormatter& ClearSegments() {
    formatters_.clear();
    required_sizes_.clear();
    return *this;
  }

  NO_DISCARD std::size_t NumSegments() const {
    return formatters_.size();
  }

 private:
  //! \brief Placeholder representing the logging message text.
  struct MSG {};
  std::vector<std::variant<MSG, std::shared_ptr<AttributeFormatter>, std::string>> formatters_;
  std::vector<unsigned> required_sizes_;
};

} // namespace formatting

class Sink {
 public:
  virtual ~Sink() = default;

  NO_DISCARD bool WillAccept(const RecordAttributes& attributes) const {
    return filter_.WillAccept(attributes);
  }

  NO_DISCARD bool WillAccept(std::optional<Severity> severity) const {
    return filter_.WillAccept(severity);
  }

  virtual void Dispatch(const Record& record) = 0;

  //! \brief Get the AttributeFilter for the sink.
  filter::AttributeFilter& GetFilter() { return filter_; }

  //! \brief Get the record formatter.
  formatting::RecordFormatter& GetFormatter() { return formatter_; }
 protected:

  FormattingSettings settings_;

  filter::AttributeFilter filter_;

  formatting::RecordFormatter formatter_;
};

class Core {
 public:
  bool WillAccept(const RecordAttributes& attributes) {
    // Core level filtering.
    if (!core_filter_.WillAccept(attributes)) {
      return false;
    }
    // Check that at least one sink will accept.
    return std::any_of(sinks_.begin(), sinks_.end(), [attributes](auto& sink) { return sink->WillAccept(attributes); });
  }

  bool WillAccept(std::optional<Severity> severity) {
    if (!core_filter_.WillAccept(severity)) {
      return false;
    }
    // Check that at least one sink will accept.
    return std::any_of(sinks_.begin(), sinks_.end(), [severity](auto& sink) { return sink->WillAccept(severity); });
  }

  //! \brief Dispatch a ref bundle to the sinks.
  void Dispatch(const Record& record) {
    for (auto& sink: sinks_) {
      sink->Dispatch(record);
    }
  }

  Core& AddSink(std::shared_ptr<Sink> sink) {
    sinks_.emplace_back(std::move(sink));
    return *this;
  }

  //! \brief Get the core level filter.
  filter::AttributeFilter& GetFilter() { return core_filter_; }

 private:

  std::vector<std::shared_ptr<Sink>> sinks_;

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
    core_ = nullptr;
  }
}

// ==============================================================================
//  Logger base class.
// ==============================================================================

class Logger {
 public:
  //! \brief Create a logger with a new core and a single sink.
  explicit Logger(const std::shared_ptr<Sink>& sink)
      : core_(std::make_shared<Core>()) {
    core_->AddSink(sink);
  }

  template <typename ...Attrs_t>
  RecordDispatcher Log(BasicAttributes basic_attributes = {}, Attrs_t&& ...attrs) {
    if (!core_) {
      return {};
    }
    if (do_time_stamp_) {
      basic_attributes.time_stamp = generator_.CurrentTime();
    }
    return RecordDispatcher(core_, basic_attributes, attrs...);
  }

  template <typename ...Attrs_t>
  RecordDispatcher Log(Severity severity, Attrs_t&& ...attrs) {
    return Log(BasicAttributes(severity), attrs...);
  }

//  template <typename ...Data_t>
//  void FmtLog(Severity severity, const char* fmt_string, const Data_t& ...data) {
//    if (!core_) {
//      return;
//    }
//
//    // TODO: For real.
//    Record record(severity);
//  }

  bool WillAccept(Severity severity) {
    if (!core_) return false;
    return core_->WillAccept(severity);
  }

  NO_DISCARD const std::shared_ptr<Core>& GetCore() const {
    return core_;
  }

  Logger& SetDoTimeStamp(bool do_timestamp) {
    do_time_stamp_ = do_timestamp;
    return *this;
  }

 private:
  bool do_time_stamp_ = true;

  //! \brief A generator to create the DateTime timestamps for records.
  time::FastDateGenerator generator_;

  //! \brief The logger's core.
  std::shared_ptr<Core> core_;

  //! \brief Keep track of when the logger was created.
  std::chrono::time_point<std::chrono::system_clock> start_time_point_;
};

//! \brief A simple sink that writes to a file via an ofstream.
class FileSink : public Sink {
 public:
  explicit FileSink(const std::string& file) : fout_(file) {}

  void Dispatch(const Record& record) override {
    auto message = formatter_.Format(record, settings_);
    fout_ << message;
  }

  std::ofstream fout_;
};


//! \brief Log with severity to a specific logger. First does a very fast check whether the
//! message would be accepted given its severity level, since this is a very common case.
//! If it will be, creates a handler, constructing the record in-place inside the handler.
#define LOG_SEV_TO(logger, severity) \
  if ((logger).WillAccept(::lightning::Severity::severity)) \
    if (auto handler = (logger).Log(::lightning::Severity::severity)) \
      handler.GetRecord().Bundle()

namespace formatting {


namespace detail {

template <std::size_t I, typename ...Args_t>
unsigned sizeRequired(const std::tuple<Segment<Args_t>...>& segments,
                      std::size_t count_placed,
                      const FormattingSettings& settings) {
  if (count_placed == I) {
    return 0u;
  }
  unsigned size = std::get<I>(segments).SizeRequired(settings);
  if constexpr (I + 1 < sizeof...(Args_t)) {
    size += sizeRequired < I + 1 > (segments, count_placed, settings);
  }
  return size;
}

template <std::size_t I, typename ...Args_t, std::size_t ...Indices>
void formatHelper(char* c,
                  const char* const* starts,
                  const char* const* ends,
                  std::size_t actual_substitutions,
                  const std::tuple<Segment<Args_t>...>& segments,
                  const FormattingSettings& settings,
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
    auto size = segment.SizeRequired(settings);
    segment.AddToBuffer(settings, c, c + size);
    c += size;
  }

  if constexpr (I < N) {
    formatHelper < I + 1 > (c, starts, ends, actual_substitutions, segments, settings, is);
  }
}

} // namespace detail


template <typename ...Args_t>
std::string Format(const FormattingSettings& settings, const char* fmt_string, const Args_t& ...args) {
  constexpr auto N = sizeof...(args);
  const char* starts[N + 1], * ends[N + 1];

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

  auto segments = std::make_tuple(Segment<std::decay_t<std::remove_cvref_t<Args_t>>>(args)...);
  unsigned format_size = detail::sizeRequired<0>(segments, count_placed, settings);
  std::string buffer(format_size + str_length, ' ');
  detail::formatHelper<0>(&buffer[0], starts, ends, count_placed, segments, settings, std::make_index_sequence<sizeof...(args)>{});
  return buffer;
}

template <typename ...Args_t>
std::string FormatTo(char* buffer, const char* end, const FormattingSettings& settings, const char* fmt_string, const Args_t& ...args) {
  LL_REQUIRE(buffer <= end, "cannot have the buffer start after the buffer end");
  constexpr auto N = sizeof...(args);
  const char* starts[N + 1], * ends[N + 1];

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

  auto segments = std::make_tuple(Segment<std::decay_t<std::remove_cvref_t<Args_t>>>(args)...);
  unsigned format_size = detail::sizeRequired<0>(segments, count_placed, settings);
  detail::formatHelper<0>(&buffer[0], starts, ends, count_placed, segments, settings, std::make_index_sequence<sizeof...(args)>{});
  return buffer;
}

template <typename ...Args_t>
std::string Format(const char* fmt_string, const Args_t& ...args) {
  return Format(FormattingSettings{}, fmt_string, args...);
}



// ==============================================================================
//  Additional AttributeFormatters that require Format().
// ==============================================================================

class DateTimeAttributeFormatter : public AttributeFormatter {
  // TODO: Allow for different formattings of the DateTime, via format string.
 public:
  void AddToBuffer(const RecordAttributes& attributes, const FormattingSettings& settings, char* start, char* end) const override {
    if (attributes.basic_attributes.time_stamp) {
      auto& dt = attributes.basic_attributes.time_stamp.value();
      // TODO: Integer padding.
      FormatTo(start, end, settings, "%-%-% %:%:%.%",
               dt.GetYear(), dt.GetMonthInt(), dt.GetDay(),
               dt.GetHour(), dt.GetMinute(), dt.GetSecond(), dt.GetMicrosecond());
    }
  }

  NO_DISCARD unsigned RequiredSize(const RecordAttributes& attributes, const FormattingSettings& settings) const override {
    return attributes.basic_attributes.time_stamp ? 26 : 0u;
  }
};

} // namespace formatting

} // namespace lightning