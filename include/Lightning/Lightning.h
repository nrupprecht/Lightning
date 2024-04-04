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
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>
#include <string_view>
#include <cstring> // For std::strlen, std::memcpy, etc.

#ifdef __cpp_lib_span
#include <span>
#endif  // __cpp_lib_span

namespace lightning {
// ==============================================================================
//  Macro definitions.
// ==============================================================================

#define NO_DISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]

#define LL_ENABLE_IF(...) typename = std::enable_if_t<(__VA_ARGS__)>

//! \brief  Macro that allows you to create a type trait based on whether a statement about a type called
//! Value_t is valid.
//!
//! For example, to make a type trait called can_i_stream_this_v<T> that will be true if T can be streamed into
//! std::cout, you can write: NEW_TYPE_TRAIT(can_i_stream_this_v, std::cout << std::declval<T>());
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
//  Current function.
// ==============================================================================

// Current function, from BOOST, taken from https://www.boost.org/doc/libs/1_78_0/boost/current_function.hpp
namespace detail {

//  boost/current_function.hpp - LL_CURRENT_FUNCTION
//
//  Copyright 2002-2018 Peter Dimov
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//
//  http://www.boost.org/libs/assert
[[maybe_unused]] inline void current_function_helper() {
#if defined( LL_DISABLE_CURRENT_FUNCTION )
# define LL_CURRENT_FUNCTION "(unknown)"
#elif defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__) || defined(__clang__)
# define LL_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__DMC__) && (__DMC__ >= 0x810)
# define LL_CURRENT_FUNCTION __PRETTY_FUNCTION__
#elif defined(__FUNCSIG__)
# define LL_CURRENT_FUNCTION __FUNCSIG__
#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))
# define LL_CURRENT_FUNCTION __FUNCTION__
#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)
# define LL_CURRENT_FUNCTION __FUNC__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
# define LL_CURRENT_FUNCTION __func__
#elif defined(__cplusplus) && (__cplusplus >= 201103)
# define LL_CURRENT_FUNCTION __func__
#else
# define LL_CURRENT_FUNCTION "(unknown)"
#endif
}

} // namespace detail
// NOTE: End of code from boost.

// ==============================================================================
//  Error checking and handling.
// ==============================================================================

//! \brief Exception class for Lightning.
class LightningException : public std::runtime_error {
 public:
  explicit LightningException(const std::string &message, const std::string &file, const std::string &function,
                              std::size_t line)
      : std::runtime_error(formatMessage(message, file, function, line)),
        message_(formatMessage(message, file, function, line)),
        file_(file),
        function_(function),
        line_(line) {}

  const char *what() const noexcept override {
    return message_.c_str();
  }

  const std::string &GetMessage() const noexcept {
    return message_;
  }

  const std::string &GetFile() const noexcept {
    return file_;
  }

  std::size_t GetLine() const noexcept {
    return line_;
  }

 private:
  NO_DISCARD static std::string formatMessage(const std::string &message,
                                              const std::string &file,
                                              const std::string &function,
                                              std::size_t line) {
    std::ostringstream strm;
    strm << "exception from " << file << ":" << line << "\nin function " << function << "\n\"" << message << "\"";
    return strm.str();
  }

  std::string message_;
  std::string file_;
  std::string function_;
  std::size_t line_;
};

#define THROW(message) throw ::lightning::LightningException(message, __FILE__, LL_CURRENT_FUNCTION, __LINE__)

#define LL_REQUIRE(condition, message) \
  do { \
    if (!(condition)) { \
      std::ostringstream _strm_; \
      _strm_ << message; \
      THROW(_strm_.str()); \
    } \
  } while (false)

#define LL_ASSERT(condition, message) \
  do { \
    if (!(condition)) { \
      std::ostringstream _strm_; \
      _strm_ << message; \
      THROW(_strm_.str()); \
    } \
  } while (false)

#define LL_FAIL(message) \
  do { \
    std::ostringstream _strm_; \
    _strm_ << message; \
    THROW(_strm_.str()); \
  } while (false)

// ==============================================================================
//  Type traits.
// ==============================================================================

namespace typetraits {
//! \brief  Type trait that determines whether a type can be ostream'ed.
NEW_TYPE_TRAIT(is_ostreamable_v, std::declval<std::ostream &>() << std::declval<Value_t>())

//! \brief  Type trait that determines whether a type has an overload of the std::to_string function.
NEW_TYPE_TRAIT(has_std_to_string_v, std::to_string(std::declval<Value_t>()))
//! \brief Type trait that determines whether a type has an ADL findable to_string function.
NEW_TYPE_TRAIT(has_adl_to_string_v, to_string(std::declval<Value_t>()))

//! \brief A type is "to-stringable" if it has either a std::to_string or an ADL to_string.
template <typename T>
inline constexpr bool has_to_string_v = has_std_to_string_v<T> || has_adl_to_string_v<T>;

//! \brief  Type trait that determines if there is std::to_chars support for a type. Some compilers, like clang,
//! have std::to_chars for integral types, but not for doubles, so we can't just check the feature test macro
//! __cpp_lib_to_chars.
NEW_TYPE_TRAIT(has_to_chars,
               std::to_chars(std::declval<char *>(), std::declval<char *>(), std::declval<Value_t>()))

//! \brief Define remove_cvref_t, which is not available everywhere.
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

namespace detail_always_false {
template <typename T>
struct always_false {
  static constexpr bool value = false;
};
} // namespace detail_always_false

//! \brief  "Type trait" that is always false, useful in static_asserts, since right now (meaning in C++17),
//! you cannot static_assert(false).
template <typename T>
inline constexpr bool always_false_v = detail_always_false::always_false<T>::value;

namespace detail_unconst {
template <typename T>
struct Unconst {
  using type = T;
};

template <typename T>
struct Unconst<const T> {
  using type = typename Unconst<T>::type;
};

template <typename T>
struct Unconst<const T *> {
  using type = typename Unconst<T>::type *;
};

template <typename T>
struct Unconst<T *> {
  using type = typename Unconst<T>::type *;
};
} // namespace detail_unconst

//! \brief  Remove const-ness on all levels of pointers.
template <typename T>
using Unconst_t = typename detail_unconst::Unconst<T>::type;

//! \brief Type trait that checks if a type is a c-style string or a std::string, no matter what const-ness qualifies T.
template <typename T>
constexpr inline bool IsCstrRelated_v =
    std::is_same_v<Unconst_t<std::decay_t<T>>, char *> || std::is_same_v<remove_cvref_t<T>, std::string>;
} // namespace typetraits.

//! \brief Convenient base class for pImpl type objects. Note that here, we use PImpl to give value semantics to objects
//! that would otherwise need to be stored as shared pointers to base objects. This is in contrast to why PImpl is
//! usually used, as a way to hide implementation details and a compilation firewall.
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
  typename Concrete_t::Impl *impl() {
    return reinterpret_cast<typename Concrete_t::Impl *>(impl_.get());
  }

  template <typename Concrete_t>
  NO_DISCARD const typename Concrete_t::Impl *impl() const {
    return reinterpret_cast<const typename Concrete_t::Impl *>(impl_.get());
  }

  std::shared_ptr<Impl> impl_ = nullptr;

  explicit ImplBase(std::shared_ptr<Impl> impl)
      : impl_(std::move(impl)) {}
};

namespace memory {

//! \brief Basic contiguous memory buffer. Not thread safe.
//!
template <typename T>
class BasicMemoryBuffer {
 public:
  explicit BasicMemoryBuffer(bool needs_normalization = true)
      : needs_normalization_(needs_normalization) {}

  virtual ~BasicMemoryBuffer() = default;

  //! \brief Returns a pointer to the beginning of the current buffer.
  //!
  //! NOTE that this is not *guaranteed* to be the beginning of all the data loaded into the buffer, allowing for cases
  //! where the buffer flushes to some other storage periodically. It is, however, the beginning of the current buffer.
  NO_DISCARD T *Data() { return data_; }
  NO_DISCARD const T *Data() const { return data_; }

  //! \brief Returns a pointer to the end (one past the last element) of the buffer.
  [[maybe_unused]] NO_DISCARD const T *End() const { return data_ + size_; }
  [[maybe_unused]] NO_DISCARD T *End() { return data_ + size_; }

#ifdef __cpp_lib_span
  //! \brief Get a span of the data.
  std::span<T> Span() { return noexcept {data_, size_}; }

  //! \brief Get a constant span of the data.
  std::span<const T> Span() const noexcept { return {data_, size_}; }
#endif

  //! \brief begin() function, for compatibility with range-based for loops.
  NO_DISCARD const T *begin() const { return data_; }
  NO_DISCARD T *begin() { return data_; }
  //! \brief end() function, for compatibility with range-based for loops.
  NO_DISCARD const T *end() const { return data_ + size_; }
  NO_DISCARD T *end() { return data_ + size_; }

  //! \brief Add an element to the buffer, first resizing if necessary.
  void PushBack(const T &value) {
    reserve(size_ + 1);
    data_[size_] = value;
    increaseSize(1);
  }

  //! \brief Append a range of values to the buffer.
  void Append(const T *begin, const T *end) {
    auto count = std::distance(begin, end);
    reserve(size_ + static_cast<std::size_t>(count));
    std::uninitialized_copy(begin, end, data_ + size_);
    increaseSize(static_cast<std::size_t>(count));
  }

  //! \brief Append the contents of another buffer to this buffer.
  void Append(const BasicMemoryBuffer<T>& other) {
    Append(other.data_, other.data_ + other.size_);
  }

#ifdef __cpp_lib_span
  //! \brief Append a span of values to the buffer.
  template <typename S = T>
  void Append(std::span<S> span) {
    Append(span.data(), span.data() + span.size());
  }
#endif

  //! \brief Append n_copies copies of an object to the buffer.
  void AppendN(const T &object, std::size_t n_copies) {
    reserve(size_ + n_copies);
    std::uninitialized_fill_n(data_ + size_, n_copies, object);
    increaseSize(n_copies);
  }

  //! \brief Clear the buffer, resetting the size to zero.
  //!
  //! How memory is managed is implementation dependent, but by default, we set the size to zero and normalize.
  virtual void Clear() {
    size_ = 0;
    normalize();
  }

  //! \brief Get the size of the buffer.
  NO_DISCARD std::size_t Size() const { return size_; }

  //! \brief Get the capacity of the buffer.
  [[maybe_unused]] NO_DISCARD std::size_t Capacity() const { return capacity_; }

  //! \brief Checks whether the buffer is empty.
  NO_DISCARD bool Empty() const { return size_ == 0; }

  //! \brief Make sure the capacity is enough for the given size plus some additional amount of space.
  [[maybe_unused]] void ReserveAdditional(std::size_t additional) {
    reserve(size_ + additional);
  }

  //! \brief Allocate an additional chunk of storage and return pointers to the beginning and end (element after last
  //!        valid element) of the chunk.
  std::pair<T*, T*> Allocate(std::size_t additional) {
    reserve(size_ + additional);
    auto begin = End();
    increaseSize(additional);
    return {begin, End()};
  }

  //! \brief If the storage type is char, return a string copy of the data.
  template <typename S = T, LL_ENABLE_IF(std::is_same_v<S, char>)>
  NO_DISCARD std::string ToString() const {
    return std::string(Data(), End());
  }

  //! \brief If the storage type is char, return a string_view of the data.
  template <typename S = T, LL_ENABLE_IF(std::is_same_v<S, char>)>
  [[maybe_unused]] NO_DISCARD std::string_view ToView() const {
    return std::string_view(Data(), Size());
  }

 protected:
  //! \brief Make sure the capacity is at least as much as new_capacity.
  void reserve(std::size_t new_capacity) {
    // Note: only makes a virtual call if the capacity is too small.
    // Note: If normalization is needed we always need at least one more element, for the null terminator.
    auto normalization_capacity = needs_normalization_ ? new_capacity + 1 : new_capacity;
    if (capacity_ < normalization_capacity) {
      allocate(normalization_capacity);
    }
  }

  //! \brief Allocate the buffer to contain at least this much space.
  virtual void allocate(std::size_t size) = 0;

  //! \brief Perform any normalization that is needed after the buffer is modified.
  void normalize() {
    // If the data type is char, we want the buffer to act like a c-style string, so we need to null terminate.
    if constexpr (std::is_same_v<T, char>) {
      if (needs_normalization_) {
        // If normalization is true, we always make sure that we have capacity of at least size + 1, so this is valid.
        data_[size_] = '\0';
      }
    }
  }

  //! \brief Increase the size by the requested amount and call normalize(). There must be enough capacity.
  void increaseSize(std::size_t increase_by) {
    size_ += increase_by;
    normalize();
  }

  //! \brief Pointer to the data that is being used.
  T *data_{};
  //! \brief The current size of the data stored in the buffer (number of entries).
  std::size_t size_{};
  //! \brief The current capacity of the buffer.
  std::size_t capacity_{};
  //! \brief Whether the buffer needs to be "normalized." This means, we need to add a null terminator if the
  //! data type is char, making the buffer compatible with c-style string functions.
  const bool needs_normalization_ = true;
};

//! \brief Basic memory buffer that has a predetermined amount of stack storage and a concrete method for allocating
//! new data on the heap.
//!
template <typename T, std::size_t stack_size_v = 256>
class MemoryBuffer final : public BasicMemoryBuffer<T> {
 public:
  MemoryBuffer() {
    this->data_ = buffer_;
    this->capacity_ = stack_size_v;
  }

  //! \brief Clean up by deallocating any heap memory.
  ~MemoryBuffer() override { deallocate(); }

 private:
  void allocate(std::size_t size) override {
    auto trial_capacity = this->capacity_ + this->capacity_ / 2;
    trial_capacity = std::max(size, trial_capacity);
    auto old_data = this->data_;
    this->data_ = new T[trial_capacity];
    std::copy(old_data, old_data + this->size_, this->data_);
    if (old_data != buffer_) {
      delete[] old_data;
    }
    this->capacity_ = trial_capacity;
  }

  void deallocate() {
    if (this->data_ != buffer_) {
      delete[] this->data_;
    }
  }

  //! \brief The internal data buffer.
  T buffer_[stack_size_v];
};

//! \brief A memory buffer that manages a string. This is useful if you want to build up a string in a buffer
//! and then return it as a string.
class StringMemoryBuffer final : public BasicMemoryBuffer<char> {
 public:
  explicit StringMemoryBuffer(std::size_t initial_capacity = 256) : BasicMemoryBuffer<char>(false) {
    allocateBuffer(initial_capacity);
  }

  //! \brief Move the string buffer out of the memory buffer.
  std::string &&MoveString() {
    buffer_.resize(size_);
    data_ = nullptr;
    size_ = 0;
    capacity_ = 0;
    return std::move(buffer_);
  }

 private:
  void allocate(std::size_t size) override {
    allocateBuffer(size);
  }

  //! \brief Reallocate the string. This is a separate function so that it can be called from the constructor.
  void allocateBuffer(std::size_t size) {
    auto trial_capacity = this->capacity_ + this->capacity_ / 2;
    trial_capacity = std::max(size, trial_capacity);
    buffer_.resize(trial_capacity);
    data_ = buffer_.data();
  }

  //! \brief The internal string buffer.
  std::string buffer_{};
};

//! \brief Helper function to append a string to a memory buffer of chars.
inline void AppendBuffer(BasicMemoryBuffer<char> &buffer, const std::string &str) {
  buffer.Append(str.data(), str.data() + str.size());
}

//! \brief Helper function to append a string view to a memory buffer of chars.
inline void AppendBuffer(BasicMemoryBuffer<char> &buffer, const std::string_view &str) {
  buffer.Append(str.data(), str.data() + str.size());
}

//! \brief Helper function to append a c-style string to a memory buffer of chars.
inline void AppendBuffer(BasicMemoryBuffer<char> &buffer, const char *str) {
  for (auto c = str; *c != '\0'; ++c) {
    buffer.PushBack(*c);
  }
}

//! \brief Helper function to append a range of characters to a buffer.
inline void AppendBuffer(BasicMemoryBuffer<char> &buffer, const char *start, const char *end) {
  LL_REQUIRE(start <= end, "start of buffer must not be after end of buffer");
  for (auto c = start; c != end; ++c) {
    buffer.PushBack(*c);
  }
}

//! \brief A vector (-like class) that has a predetermined amount of stack storage and uses a vector for the
//! rest of the storage.
//!
//! Note: This is very similar to a MemoryBuffer, but differs in a couple ways. First, it is not a contiguous
//! buffer, so it does not have a Data() (or related) methods. Second, it allows for direct indexing. I am not
//! allowing MemoryBuffer to index, since there are certain use cases I might want to explore where you would
//! not want to allow indexing. For example, you could create a BasicMemoryBuffer child that wraps around an
//! ostream, and flushes the fixed buffer to the ostream whenever it is full, and then resets. Obviously, you
//! cannot index into an arbitrary place in a class like this.
//! So for now, despite their similarities, I am keeping these classes distinct and unrelated.
template <typename T, std::size_t stack_size_v>
class HybridVector {
 public:
  //! \brief Move an element onto the back of the hybrid vector.
  void PushBack(T &&x) {
    if (stack_size_ == stack_size_v) {
      heap_buffer_.push_back(std::move(x));
    }
    else {
      stack_buffer_[stack_size_++] = std::move(x);
    }
  }

  //! \brief Construct an element on the back of the hybrid vector.
  template <typename... Args_t>
  void EmplaceBack(Args_t &&... args) {
    if (stack_size_ == stack_size_v) {
      heap_buffer_.emplace_back(std::forward<Args_t>(args)...);
    }
    else {
      // Use placement new to allocate an item on the stack buffer.
      new(stack_buffer_ + stack_size_++) T(std::forward<Args_t>(args)...);
    }
  }

  //! \brief Get a reference to the last element in the hybrid vector.
  T &Back() {
    if (!heap_buffer_.empty()) {
      return heap_buffer_.back();
    }
    else {
      return stack_buffer_[stack_size_ - 1];
    }
  }

  //! \brief Access the index'th element of the hybrid vector.
  T &operator[](std::size_t index) {
    if (index < stack_size_v) {
      return stack_buffer_[index];
    }
    return heap_buffer_[index - stack_size_v];
  }

  const T &operator[](std::size_t index) const {
    if (index < stack_size_v) {
      return stack_buffer_[index];
    }
    return heap_buffer_[index - stack_size_v];
  }

  //! \brief Get the total size (stack and heap) of the hybrid vector.
  NO_DISCARD std::size_t Size() const { return stack_size_ + heap_buffer_.size(); }

  //! \brief Get the heap size of the hybrid vector.
  NO_DISCARD std::size_t HeapSize() const { return heap_buffer_.size(); }

  //! \brief Check whether the hybrid vector is empty.
  NO_DISCARD bool Empty() const { return stack_size_ == 0; }

 private:
  //! \brief Stack storage space.
  T stack_buffer_[stack_size_v];
  //! \brief Regular vector, for the remaining storage.
  std::vector<T> heap_buffer_;
  //! \brief The utilized size of the stack storage.
  std::size_t stack_size_{};
};

//! \brief Create a string view from a range of characters.
inline std::string_view MakeStringView(const char *begin, const char *end) {
  LL_REQUIRE(begin <= end, "begin must not be after end");
  return {begin, static_cast<std::size_t>(std::distance(begin, end))};
}

} // namespace memory

// ==============================================================================
//  Date / time support.
// ==============================================================================

namespace time {

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

//! \brief Enumeration for the months of the year.
//!
//! Starting january as 1 is enough to make the others 2 - 12, but to be explicit that this numbering is important (we
//! use it in functions, casting month to int), we set them all.
enum class Month {
  January = 1,
  February = 2,
  March = 3,
  April = 4,
  May = 5,
  June = 6,
  July = 7,
  August = 8,
  September = 9,
  October = 10,
  November = 11,
  December = 12
};

inline const char *MonthAbbreviation(Month m) {
  const auto m_int = static_cast<int>(m);
  LL_REQUIRE(0 < m_int && m_int < 13, "month must be in the range [1, 12], not " << m_int);
  static const char *abbrev_[]{
      "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  return abbrev_[m_int - 1];
}

inline Month MonthIntToMonth(int month) {
  LL_REQUIRE(0 < month && month < 13, "month must be in the range [1, 12], not " << month);
  static Month months_[]{Month::January,
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
//! The year must satisfy 0 <= year < 4096.
//! Note: This class does ignore some of the very odd bits of timekeeping, like leap-seconds. It also doesn't deal with
//! timezones and time changes, so it is up to the user to make sure that the time is in the correct timezone.
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
  //! Note: localtime (and its related variants, and gmtime) are relatively slow functions. Prefer using
  //! FastDateGenerator if you need to repeatedly generate DateTimes.
  //!
  explicit DateTime(const std::chrono::time_point<std::chrono::system_clock> &time_point) {
    // Get number of milliseconds for the current second (remainder after division into seconds)
    const int ms = static_cast<int>(
        (std::chrono::duration_cast<std::chrono::microseconds>(time_point.time_since_epoch()) % 1'000'000)
            .count());
    // Convert to std::time_t in order to convert to std::tm (broken time)
    auto t = std::chrono::system_clock::to_time_t(time_point);

    // Convert time - this is expensive. See FastDateGenerator for a faster alternative.
    std::tm now{};
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

  //! \brief Get an integer representation of the date, in the form yyyymmdd.
  NO_DISCARD int AsYYYYMMDD() const { return GetYear() * 10000 + GetMonthInt() * 100 + GetDay(); }

  //! \brief Get the year from the date.
  NO_DISCARD int GetYear() const { return static_cast<int>(y_m_d_h_m_s_um_ >> shift_year_); }

  //! \brief Get the month of the date, as an integer.
  NO_DISCARD int GetMonthInt() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_month_) & month_mask_); }

  //! \brief Get the month of the date.
  NO_DISCARD Month GetMonth() const { return MonthIntToMonth(GetMonthInt()); }

  //! \brief Get the day of the month from the date.
  NO_DISCARD int GetDay() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_day_) & day_mask_); }

  //! \brief Get the hour from the datetime.
  NO_DISCARD int GetHour() const { return static_cast<int>(y_m_d_h_m_s_um_ >> shift_hour_) & hour_mask_; }

  //! \brief Get the minute from the datetime.
  NO_DISCARD int GetMinute() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_minute_) & minute_mask_); }

  //! \brief Get the second from the datetime.
  NO_DISCARD int GetSecond() const { return static_cast<int>((y_m_d_h_m_s_um_ >> shift_second_) & second_mask_); }

  //! \brief Get the millisecond from the datetime.
  NO_DISCARD int GetMillisecond() const { return GetMicrosecond() / 1000; }

  //! \brief Get the microsecond from the datetime.
  NO_DISCARD int GetMicrosecond() const { return static_cast<int>(y_m_d_h_m_s_um_ & us_mask_); }

  //! \brief Check if the date is a non-null (empty) date.
  explicit operator bool() const { return y_m_d_h_m_s_um_ != 0; }

  //! \brief Check if two DateTime are equal.
  bool operator==(const DateTime &dt) const { return y_m_d_h_m_s_um_ == dt.y_m_d_h_m_s_um_; }

  //! \brief Check if two DateTime are not equal.
  bool operator!=(const DateTime &dt) const { return y_m_d_h_m_s_um_ != dt.y_m_d_h_m_s_um_; }

  //! \brief Check if one DateTime is less than the other.
  bool operator<(const DateTime &dt) const { return y_m_d_h_m_s_um_ < dt.y_m_d_h_m_s_um_; }

  //! \brief  Get the current clock time, in the local timezone.
  //!
  //! Note: localtime (and its related variants, and gmtime) are relatively slow functions. Prefer using
  //! FastDateGenerator if you need to repeatedly generate DateTimes.
  static DateTime Now() {
    const auto time = std::chrono::system_clock::now();
    return DateTime(time);
  }

  //! \brief Construct a DateTime out of a yyyymmdd integer, hours, minutes and optional seconds and microseconds.
  static DateTime YMD_Time(int yyyymmdd, int hours, int minutes, int seconds = 0, int microsecond = 0) {
    DateTime dt(yyyymmdd);
    dt.setHMSUS(hours, minutes, seconds, microsecond);
    return dt;
  }

 private:
  //! \brief Non-validating date constructor.
  DateTime(
      bool, // Just a flag, so that we can distinguish this constructor from the validating one.
      int year,
      int month,
      int day,
      int hour = 0,
      int minute = 0,
      int second = 0,
      int microsecond = 0) {
    setYMD(year, month, day, false);
    setHMSUS(hour, minute, second, microsecond, false);
  }

  //! \brief Set the year, month, and day of the date, potentially validating beforenhand.
  void setYMD(int year, int month, int day, bool validate = true) {
    if (validate)
      validateYMD(year, month, day);
    // Zero previous y-m-d.
    y_m_d_h_m_s_um_ = (y_m_d_h_m_s_um_ << 32) >> 32;
    y_m_d_h_m_s_um_ |= (static_cast<std::uint64_t>(year) << shift_year_)
        | (static_cast<std::uint64_t>(month) << shift_month_)
        | (static_cast<std::uint64_t>(day) << shift_day_);
  }

  //! \brief Set the hour, minute, second, and microsecond of the date, potentially validating beforehand.
  void setHMSUS(int hour, int minute, int second, int microseconds, bool validate = true) {
    if (validate)
      validateHMSUS(hour, minute, second, microseconds);
    // Zero previous h-m-s-us.
    y_m_d_h_m_s_um_ = (y_m_d_h_m_s_um_ >> 32) << 32;
    y_m_d_h_m_s_um_ |= (static_cast<std::uint64_t>(hour) << shift_hour_)
        | (static_cast<std::uint64_t>(minute) << shift_minute_)
        | (static_cast<std::uint64_t>(second) << shift_second_) |
        static_cast<std::uint64_t>(microseconds);
  }

  static void validateYMD(int year, int month, int day) {
    LL_REQUIRE(0 < year, "year must be > 0");
    LL_REQUIRE(0 < month && month <= 12, "month must be in the range [1, 12]");
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
  std::uint64_t y_m_d_h_m_s_um_{};

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

//! \brief Add some number of microseconds to a DateTime, returning the new DateTime.
inline DateTime AddMicroseconds(const DateTime &time, unsigned long long microseconds) {
  int new_years = time.GetYear(), new_months = time.GetMonthInt(), new_days = time.GetDay();
  int new_hours = time.GetHour(), new_minutes = time.GetMinute(), new_seconds = time.GetSecond();
  int new_us = time.GetMicrosecond();

  const auto new_us_overflow = static_cast<unsigned long long>(new_us) + microseconds;
  new_us = static_cast<int>(new_us_overflow % 1'000'000);

  const auto carry_seconds = new_us_overflow / 1'000'000;
  if (carry_seconds == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  const auto new_seconds_overflow = static_cast<unsigned long long>(new_seconds) + carry_seconds;
  new_seconds = static_cast<int>(new_seconds_overflow % 60);

  const auto carry_minutes = new_seconds_overflow / 60;
  if (carry_minutes == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  const auto new_minutes_overflow = static_cast<unsigned long long>(new_minutes) + carry_minutes;
  new_minutes = static_cast<int>(new_minutes_overflow % 60);

  const auto carry_hours = new_minutes_overflow / 60;
  if (carry_hours == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }
  const auto new_hours_overflow = static_cast<unsigned long long>(new_hours) + carry_hours;
  new_hours = static_cast<int>(new_hours_overflow % 24);

  const auto carry_days = static_cast<int>(new_hours_overflow / 24);
  if (carry_days == 0) {
    return {new_years, new_months, new_days, new_hours, new_minutes, new_seconds, new_us};
  }

  // This is, relatively speaking, the hard part, have to take into account different numbers of days
  // in the month, and increment years when you go past December.
  auto which_month = time.GetMonthInt(), which_year = time.GetYear();
  while (0 < carry_days) {
    const auto days = DaysInMonth(which_month, which_year);
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
      : start_time_point_(std::chrono::system_clock::now()), base_date_time_(start_time_point_) {}

  NO_DISCARD DateTime CurrentTime() const {
    const auto current_time = std::chrono::system_clock::now();
    const auto dt = current_time - start_time_point_;
    const auto us = std::chrono::duration_cast<std::chrono::microseconds>(dt).count();
    return AddMicroseconds(base_date_time_, static_cast<unsigned long long>(us));
  }

 private:
  //! \brief Keep track of when the logger was created.
  std::chrono::time_point<std::chrono::system_clock> start_time_point_;
  //! \brief The DateTime at the time that the generator was created.
  DateTime base_date_time_;
};

} // namespace time

// ==============================================================================
//  Formatting.
// ==============================================================================

namespace formatting {
namespace detail {

//! \brief Store all powers of ten that fit in an unsigned long long.
constexpr unsigned long long powers_of_ten[] = {
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

//! \brief The largest power of ten that fits in an unsigned long long.
constexpr unsigned long long max_ull_power_of_ten = 10'000'000'000'000'000'000ull;

//! \brief The largest power of ten that fits in an unsigned long long is (10 ^ log10_max_ull_power_of_ten).
constexpr int log10_max_ull_power_of_ten = 19;

} // namespace detail

//! \brief Returns how many digits the decimal representation of a ull will have.
//! The optional bound "upper" means that the number has at most "upper" digits.
inline unsigned NumberOfDigitsULL(unsigned long long x, int upper = detail::log10_max_ull_power_of_ten) {
  upper = std::max(0, std::min(upper, 19));
  if (x == 0) return 1u;
  if (x >= detail::max_ull_power_of_ten) return 20;
  auto it = std::upper_bound(&detail::powers_of_ten[0], &detail::powers_of_ten[upper], x);
  return static_cast<unsigned>(std::distance(&detail::powers_of_ten[0], it));
}

//! \brief Get the number of digits that the base 10 representation of an integer will have. Note that this does not
//! count the sign character - it is the number of *digits*, not the number of characters needed to represent the
//! integer.
template <typename Integral_t,
    typename = std::enable_if_t<std::is_integral_v<Integral_t> && !std::is_same_v<Integral_t, bool>>>
unsigned NumberOfDigits(Integral_t x, int upper = detail::log10_max_ull_power_of_ten) {
  if constexpr (!std::is_signed_v<Integral_t>) {
    return NumberOfDigitsULL(x, upper);
  }
  else {
    return NumberOfDigitsULL(static_cast<unsigned long long>(std::llabs(x)), upper);
  }
}

//! \brief Copy an integer into a buffer, with optional padding.
inline char *CopyPaddedInt(
    char *start,
    char *end,
    unsigned long long x,
    int width,
    char fill_char = '0',
    int max_power = detail::log10_max_ull_power_of_ten) {
  const auto nd = NumberOfDigits(x, max_power);
  const auto remainder = static_cast<unsigned>(width) - nd;
  if (0 < remainder) {
    std::fill_n(start, remainder, fill_char);
  }
  return std::to_chars(start + remainder, end, x).ptr;
}

//! \brief Formatting enum that represents alignment.
enum class Alignment : uint8_t { Left, Right, Center };

//! \brief Structure that encodes all formatting options for a segment.
struct FmtData {
  //! \brief The requested width of the segment.
  unsigned width = 0;

  //! \brief The alignment of data within the segment. Default is left aligned.
  Alignment alignment = Alignment::Left;

  //! \brief Whether to use number separators (e.g. commas in large numbers).
  bool use_separators = false;

  //! \brief The fill character, if padding is requested.
  char fill_char = ' ';

  //! \brief The type of formatting to do. For example, an integer may be formatted as a decimal, hex, etc.
  char type = '\0';

  //! \brief A specific separator character can be specified, if use_separators is true. If separator_char is '\0',
  //! the default separator character will be used.
  char separator_char = '\0';
};

namespace detail {

//! \brief Format an integer into a buffer, with commas every three digits.
inline void formatIntegerWithCommas(unsigned long long x, memory::BasicMemoryBuffer<char> &buffer) {
  const auto num_digits = NumberOfDigitsULL(x);

  // Compute the number of commas that will be needed.
  const auto num_commas = (num_digits - 1) / 3;
  const auto num_chars = num_digits + num_commas;

  auto [begin, end] = buffer.Allocate(num_chars);
  std::to_chars(begin, end, x);
  if (num_commas == 0) {
    return;
  }

  // Copy characters in groups of threes to the far side of the buffer and insert commas.
  //
  // Example: input X = 123456789, desired output 123,456,789
  // Initial:
  // 1  2  3  4  5  6  7  8  9  x  x
  //                         ^A    ^B
  // After one step:
  // 1  2  3  4  5  6  U  ,  7  8  9
  //                ^A ^B
  // After two steps:
  // 1  2  3  ,  4  5  6  ,  7  8  9
  //       ^AB
  // -> Done
  auto it_a = begin + num_digits - 1; // Last char written by to_chars
  auto it_b = end - 1; // Last space in the total buffer
  for (auto i = 0u; i < num_commas; ++i) {
    *it_b = *it_a;
    --it_a, --it_b;
    *it_b = *it_a;
    --it_a, --it_b;
    *it_b = *it_a;
    --it_a, --it_b;
    *it_b = ',';
    --it_b;
  }
}

//! \brief Format a string into a buffer, quoting it and escaping special characters.
inline void formatDebugString(std::string_view str, memory::BasicMemoryBuffer<char> &buffer) {
  // Debug formatting.
  buffer.PushBack('"');
  for (auto c : str) {
    switch (c) {
      case '"':
        buffer.PushBack('\\');
        buffer.PushBack('"');
        break;
      case '\\':
        buffer.PushBack('\\');
        buffer.PushBack('\\');
        break;
      case '\n':
        buffer.PushBack('\\');
        buffer.PushBack('n');
        break;
      case '\r':
        buffer.PushBack('\\');
        buffer.PushBack('r');
        break;
      case '\t':
        buffer.PushBack('\\');
        buffer.PushBack('t');
        break;
      default:
        buffer.PushBack(c);
    }
  }
  buffer.PushBack('"');
}

//! \brief Helper function to get arrays of upper- or lower-case hex digits.
inline const char *getHexDigits(bool upper_case) {
  static const char *upper_hex_digits = "0123456789ABCDEF";
  static const char *lower_hex_digits = "0123456789abcdef";
  return upper_case ? upper_hex_digits : lower_hex_digits;
}

//! \brief Extract formatting options from the interior of a formatting segment, i.e. the section between the '{'
//! and '}' in a formatting string.
//!
//! The formatting options are modeled as closely as possible after https://fmt.dev/latest/syntax.html, which is in turn
//! the source for C++20 std::format's specification (https://en.cppreference.com/w/cpp/utility/format/spec).
//! The formatting options are:
//!
//! format_spec ::=  [[fill]align][sign]["#"]["0"][width]["." precision]["L"][type]
//! fill        ::=  <a character other than '{' or '}'>
//! align       ::=  "<" | ">" | "^"
//! sign        ::=  "+" | "-" | " "
//! width       ::=  integer | "{" [arg_id] "}"
//! precision   ::=  integer | "{" [arg_id] "}"
//! type        ::=  "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" |
//!                  "o" | "p" | "s" | "x" | "X" | "?"
//!
//!  I am not supporting all of these options, at least right now. And I am allowing for at least one more option,
//!  the use of a explicit separator characters, that fmt does not support.
//!
//! \param fmt The formatting segment, what would go inside the "{}" in a usual format string.
//! \param fmt_data Formatting data to fill. Some defaults may have been filled in by a calling function, e.g. what the
//!     default formatting type is, so we fill this, instead of returning this.
inline void extractFormatting(std::string_view fmt, FmtData &fmt_data) {
  if (fmt.empty()) {
    return;
  }
  LL_REQUIRE(fmt[0] == ':', "invalid format string for integer segment '" << fmt << "'");
  // It is legal for the format string to just be "{:}" - useless, but legal.
  if (fmt.size() == 1) {
    return;
  }
  std::size_t index = 1;

  // If the next char is an alignment, this means the preceding char was a fill character.
  if (2 < fmt.size() && (fmt[2] == '<' || fmt[2] == '^' || fmt[2] == '>')) {
    fmt_data.fill_char = fmt[1];
    ++index;
  }

  // Determine if any padding and alignment is needed.
  if (fmt[index] == '<') {
    fmt_data.alignment = Alignment::Left;
    ++index;
  }
  else if (fmt[index] == '>') {
    fmt_data.alignment = Alignment::Right;
    ++index;
  }
  else if (fmt[index] == '^') {
    fmt_data.alignment = Alignment::Center;
    ++index;
  }

  // Get the buffer width
  if (index < fmt.size() && std::isdigit(fmt[index])) {
    // Small buffer for the digits, do not allow more than 9999 characters in a padded output.
    char digits[4];
    // Get digits.
    unsigned num_digits = 0;
    while (index < fmt.size() && std::isdigit(fmt[index])) {
      LL_ASSERT(num_digits < 4, "cannot format to a width greater than 9999");
      digits[num_digits] = fmt[index++];
      ++num_digits;
    }
    std::from_chars(digits, digits + num_digits, fmt_data.width);
  }

  // Check if number separators should be used.
  if (index < fmt.size() && fmt[index] == 'L') {
    ++index;
    fmt_data.use_separators = true;
  }
  // User specified a separator character.
  if (index < fmt.size() && fmt[index] == ':') {
    ++index;
    LL_REQUIRE(index < fmt.size(), "invalid format string for integer segment '" << fmt << "', index " << index);
    fmt_data.separator_char = fmt[index++];
  }

  // TODO: Implement different integer formatting.
  [[maybe_unused]] char type = 'd'; // Format as a decimal.
  if (index < fmt.size()) {
    fmt_data.type = fmt[index++];
  }

  // Make sure we are at the end.
  LL_REQUIRE(index == fmt.size(), "invalid format string for integer segment '" << fmt << "'");
}

} // namespace detail

//! \brief Enum to define how to format an integer's prefix, when not in decimal format.
//! For example, when formatting an integer in hex, the prefix is "0x" when Lower, "0X" when upper, and not present when
//! None.
enum class PrefixFmtType { Upper, Lower, None };

template <typename T, LL_ENABLE_IF(std::is_integral_v<T> && !std::is_same_v<T, bool>)>
void FormatIntegerWithCommas(T x, memory::BasicMemoryBuffer<char> &buffer) {
  if constexpr (std::is_signed_v<T>) {
    if (x < 0) {
      buffer.PushBack('-');
    }
    detail::formatIntegerWithCommas(static_cast<unsigned long long>(std::llabs(x)), buffer);
  }
  else {
    detail::formatIntegerWithCommas(static_cast<unsigned long long>(x), buffer);
  }
}

template <typename Integral_t, LL_ENABLE_IF(std::is_integral_v<Integral_t> && !std::is_same_v<Integral_t, bool>)>
void FormatHex(Integral_t x,
               memory::BasicMemoryBuffer<char> &buffer,
               bool use_uppercase = true,
               PrefixFmtType prefix_fmt_type = PrefixFmtType::Lower,
               bool pad_zeros = false) {
  if constexpr (std::is_signed_v<Integral_t>) {
    if (x < 0) {
      buffer.PushBack('-');
    }
    x = static_cast<Integral_t>(std::abs(x));
  }

  // x will take up 8 characters, then two for "0x".
  switch (prefix_fmt_type) {
    case PrefixFmtType::Upper:
      buffer.PushBack('0');
      buffer.PushBack('X');
      break;
    case PrefixFmtType::Lower:
      buffer.PushBack('0');
      buffer.PushBack('x');
      break;
    default: {
    } // Pass
  }
  const char *hex_digits = detail::getHexDigits(use_uppercase);

  // Maximum number of characters that would be needed.
  char internal_buffer[2 * sizeof(Integral_t)];

  std::size_t count = 0;
  while (x) {
    internal_buffer[count] = hex_digits[x & 0xF];
    x >>= 4;
    ++count;
  }
  if (count == 0) {
    internal_buffer[count] = '0';
    ++count;
  }
  if (pad_zeros) {
    std::fill(internal_buffer + count, internal_buffer + 2 * sizeof(Integral_t), '0');
    count = 2 * sizeof(Integral_t);
  }
  // Reverse buffer.
  for (std::size_t i = 0; i < count; ++i) {
    buffer.PushBack(internal_buffer[count - i - 1]);
  }
}

template <typename Integral_t, LL_ENABLE_IF(std::is_integral_v<Integral_t> && !std::is_same_v<Integral_t, bool>)>
void FormatBinary(Integral_t x,
                  memory::BasicMemoryBuffer<char>& buffer,
                  PrefixFmtType prefix_fmt_type = PrefixFmtType::Lower,
                  bool pad_zeros = false) {
  if constexpr (std::is_signed_v<Integral_t>) {
    if (x < 0) {
      buffer.PushBack('-');
    }
    x = static_cast<Integral_t>(std::abs(x));
  }

  // x will take up 8 characters, then two for "0x".
  switch (prefix_fmt_type) {
    case PrefixFmtType::Upper:
      buffer.PushBack('0');
    buffer.PushBack('B');
    break;
    case PrefixFmtType::Lower:
      buffer.PushBack('0');
    buffer.PushBack('b');
    break;
    default: {
    } // Pass
  }
  char binary_digits[] = "01";

  // Maximum number of characters that would be needed.
  char internal_buffer[8 * sizeof(Integral_t)];

  std::size_t count = 0;
  while (x) {
    internal_buffer[count] = binary_digits[x & 0x1];
    x >>= 1;
    ++count;
  }
  if (count == 0) {
    internal_buffer[count] = '0';
    ++count;
  }
  if (pad_zeros) {
    std::fill(internal_buffer + count, internal_buffer + 8 * sizeof(Integral_t), '0');
    count = 8 * sizeof(Integral_t);
  }
  // Reverse buffer.
  for (std::size_t i = 0; i < count; ++i) {
    buffer.PushBack(internal_buffer[count - i - 1]);
  }
}

//! \brief Format an integral type to a buffer.
//!
//! Features taken from here https://fmt.dev/latest/syntax.html. Not all feature are implemented.
//!
//! \param fmt Format string. What would go inside the "{}" in a usual format string.
//! \param number The integer to format.
//! \param buffer The buffer to format the integer into.
template <typename Integral_t, LL_ENABLE_IF(std::is_integral_v<Integral_t> && !std::is_same_v<Integral_t, bool>)>
void FormatInteger(std::string_view fmt, Integral_t number, memory::BasicMemoryBuffer<char> &buffer) {
  // Check if there are formatting instructions.
  if (fmt.empty()) {
    auto num_digits = formatting::NumberOfDigits(number);
    if constexpr (std::is_signed_v<Integral_t>) {
      if (number < 0) {
        ++num_digits;
      }
    }
    auto [start, end] = buffer.Allocate(num_digits);
    std::to_chars(start, end, number);
    return;
  }
  FmtData fmt_data;
  // Default type for integers is 'd' (decimal).
  fmt_data.type = 'd';
  detail::extractFormatting(fmt, fmt_data);

  // ==============================================================================
  //  All formatting discovered, do the formatting.
  // ==============================================================================

  // Format the data.

  memory::MemoryBuffer<char> temp_buffer;
  switch (fmt_data.type) {
  case 'd':
    if (fmt_data.use_separators) {
      formatting::FormatIntegerWithCommas(number, temp_buffer);
    }
    else {
      auto num_digits = formatting::NumberOfDigits(number);
      num_digits += (number < 0) ? 1 : 0;
      auto [start, end] = temp_buffer.Allocate(num_digits);
      std::to_chars(start, end, number);
    }
    break;
  case 'b':
    FormatBinary(number, temp_buffer, PrefixFmtType::Lower, false);
    break;
  case 'B':
    FormatBinary(number, temp_buffer, PrefixFmtType::Upper, false);
    break;
  case 'x':
    FormatHex(number, temp_buffer, true, PrefixFmtType::Lower, false);
    break;
  case 'X':
    FormatHex(number, temp_buffer, true, PrefixFmtType::Upper, false);
    break;
  default:
    LL_FAIL("unrecognized / unhandled formatting option '" << fmt_data.type << "' for integer segment '" << fmt << "'");
  }

  auto num_digits = static_cast<unsigned>(temp_buffer.Size());
  auto total_width = std::max(num_digits, fmt_data.width);

  std::size_t alignment_offset = 0, right_width = 0;
  if (fmt_data.alignment == Alignment::Right) {
    alignment_offset = total_width - num_digits;
  }
  else if (fmt_data.alignment == Alignment::Center) {
    alignment_offset = (total_width - num_digits) / 2;
    right_width = total_width - alignment_offset - num_digits;
  }
  else {  // Alignment::Left
    right_width = total_width - num_digits;
  }

  if (alignment_offset != 0) {
    auto [start, end] = buffer.Allocate(alignment_offset);
    std::fill_n(start, alignment_offset, fmt_data.fill_char);
  }

  // Add the formatted data.
  buffer.Append(temp_buffer);
  // Right padding.
  if (right_width != 0) {
      auto [start, end] = buffer.Allocate(right_width);
      std::fill_n(start, right_width, fmt_data.fill_char);
  }
}

//! \brief Format a string to a buffer, applying formatting options.
inline void FormatString(std::string_view fmt, std::string_view str, memory::BasicMemoryBuffer<char> &buffer) {
  if (fmt.empty()) {
    memory::AppendBuffer(buffer, str);
    return;
  }
  FmtData fmt_data;
  // Default string formatting is 's'
  fmt_data.type = 's';
  detail::extractFormatting(fmt, fmt_data);

  // TODO: Checks that invalid formatting options were not used.
  LL_REQUIRE(fmt_data.type == 's' || fmt_data.type == '?',
             "invalid format string for string segment '" << fmt << "', illegal formatting type '" << fmt_data.type
                                                          << "'");
  LL_REQUIRE(!fmt_data.use_separators, "cannot specify use separators ('L') for formatting a string");

  // ==============================================================================
  //  All formatting discovered, do the formatting.
  // ==============================================================================

  auto num_chars = static_cast<unsigned>(str.size());
  unsigned extra_debug_chars = 0;
  if (fmt_data.type == '?') {
    extra_debug_chars += 2; // For the quotes.
    // Count characters that need to be escaped.
    for (auto c : str) {
      if (c == '"' || c == '\\' || c == '\n' || c == '\r' || c == '\t') {
        ++extra_debug_chars;
      }
    }
  }
  num_chars += extra_debug_chars;
  auto total_width = std::max(num_chars, fmt_data.width);

  std::size_t alignment_offset = 0, right_width = 0;
  if (fmt_data.alignment == Alignment::Right) {
    alignment_offset = total_width - num_chars;
  }
  else if (fmt_data.alignment == Alignment::Center) {
    alignment_offset = (total_width - num_chars) / 2;
    right_width = total_width - alignment_offset - num_chars;
  }
  else {  // Alignment::Left
    right_width = total_width - num_chars;
  }

  if (alignment_offset != 0) {
    auto [start, end] = buffer.Allocate(alignment_offset);
    std::fill_n(start, alignment_offset, fmt_data.fill_char);
  }

  if (extra_debug_chars == 0) {
    // Standard formatting.
    memory::AppendBuffer(buffer, str);
  }
  else {
    // Debug formatting.
    detail::formatDebugString(str, buffer);
  }

  if (right_width != 0) {
    auto [start, end] = buffer.Allocate(right_width);
    std::fill_n(start, right_width, fmt_data.fill_char);
  }
}

//! \brief Format a date to a character range in 'YYYY-mm-dd hh:mm:ss.uuuuuu' format.
inline char *FormatDateTo(char *c, char *end_c, const time::DateTime &dt) {
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
  LL_REQUIRE(26 <= end_c - c, "need at least 26 characters to format date");

  // Year
  const auto start_c = c;
  std::to_chars(c, end_c, dt.GetYear());
  *(start_c + 4) = '-';
  // Month
  const auto month = dt.GetMonthInt();
  std::copy(up_to + 2 * month, up_to + 2 * month + 2, start_c + 5);
  *(start_c + 7) = '-';
  // Day
  const auto day = dt.GetDay();
  std::copy(up_to + 2 * day, up_to + 2 * day + 2, start_c + 8);
  *(start_c + 10) = ' ';
  // Hour.
  const auto hour = dt.GetHour();
  std::copy(up_to + 2 * hour, up_to + 2 * hour + 2, start_c + 11);
  *(start_c + 13) = ':';
  // Minute.
  const auto minute = dt.GetMinute();
  std::copy(up_to + 2 * minute, up_to + 2 * minute + 2, start_c + 14);
  *(start_c + 16) = ':';
  // Second.
  const auto second = dt.GetSecond();
  std::copy(up_to + 2 * second, up_to + 2 * second + 2, start_c + 17);
  *(start_c + 19) = '.';
  // Microsecond.
  const auto microseconds = dt.GetMicrosecond();
  const int nd = static_cast<int>(formatting::NumberOfDigits(microseconds, 6));
  std::fill_n(c + 20, 6 - nd, '0');
  std::to_chars(c + 26 - nd, end_c, microseconds);

  return c + 26;
}

//! \brief Information about a formatted logging message. Used to keep track of various information about the message
//! that is being formatted to a string.
struct MessageInfo {
  //! \brief The total length of the formatted string so far, inclusive of message and non-message segments.
  unsigned total_length = 0;

  //! \brief The indentation of the start of the message within the formatted record. This is only filled in
  //! if needs_message_indentation is requested (true), and is only valid if is_in_message_segment is true.
  std::optional<unsigned> message_indentation{};

  //! \brief The length of the message segment so far.
  unsigned message_length = 0;

  //! \brief True if the current segment or formatter is within the message segment of a formatted record.
  bool is_in_message_segment = false;

  //! \brief If true, some formatting segment needs the message indentation to be calculated. Otherwise, the message
  //! indentation does not need to be calculated.
  bool needs_message_indentation = false;
};

// ==============================================================================
//  Virtual terminal commands.
// ==============================================================================

//! \brief All ansi foreground colors.
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

//! \brief All ansi background colors.
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
  std::string fmt{};
  if (foreground)
    fmt += "\x1b[" + std::to_string(static_cast<short>(*foreground)) + "m";
  if (background)
    fmt += "\x1b[" + std::to_string(static_cast<short>(*background)) + "m";
  return fmt;
}

//! \brief Generate a string that can change the ANSI 256-bit color of a terminal, if supported.
inline std::string SetAnsi256ColorFmt(std::optional<Ansi256Color> foreground_color_id,
                                      std::optional<Ansi256Color> background_color_id = {}) {
  std::string fmt{};
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
inline unsigned CountNonAnsiSequenceCharacters(const char *begin, const char *end) {
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

} // namespace formatting

namespace time {

//! \brief Streaming operator for DateTime. Put in the time namespace so it can be found by ADL.
inline std::ostream &operator<<(std::ostream &stream, const DateTime &dt) {
  char buffer[26];
  formatting::FormatDateTo(buffer, buffer + 26, dt);
  stream.write(buffer, 26);
  return stream;
}

}  // namespace time

//! \brief  Structure that specifies how a message can be formatted, and what are the capabilities of the sink
//! that the formatted message will be dispatched to.
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
//! For efficiency, each segment needs to know how much space its formatted self will take up.
struct BaseSegment {
  explicit BaseSegment() = default;
  virtual ~BaseSegment() = default;

  //! \brief Add the segment to the supplied buffer, using the given formatting settings.
  void AddToBuffer(const FormattingSettings &settings,
                   const formatting::MessageInfo &msg_info,
                   memory::BasicMemoryBuffer<char> &buffer,
                   const std::string_view &fmt) const {
    addToBuffer(settings, msg_info, buffer, fmt);
  }

  //! \brief Add the segment to the buffer, not using a string format, but using the other formatting settings.
  void AddToBuffer(const FormattingSettings &settings,
                   const formatting::MessageInfo &msg_info,
                   memory::BasicMemoryBuffer<char> &buffer) const {
    AddToBuffer(settings, msg_info, buffer, {});
  }

  //! \brief Copy the segment to the supplied storage.
  virtual void CopyTo(class SegmentStorage &) const = 0;

  //! \brief  Some formatting segments need the message indentation (the distance from the start of the
  //! message back to the last newline in the header, counting only visible characters) to be calculated, e.g.
  //! to do an aligned newline. The indentation is only calculated if needed, to save time, since it is rarely
  //! needed.
  NO_DISCARD virtual bool NeedsMessageIndentation() const { return false; }

 private:
  //! \brief Add the segment to the supplied buffer, using the given formatting settings.
  //! We implement this as a private method so we don't have to do a using BaseSegment::AddToBuffer in every class when
  //! we override this method.
  virtual void addToBuffer(const FormattingSettings &settings,
                           const formatting::MessageInfo &msg_info,
                           memory::BasicMemoryBuffer<char> &buffer,
                           const std::string_view &fmt) const = 0;
};

//! \brief  Acts like a unique pointer for an object deriving from BaseSegment, but uses a SBO to put small
//! segments on the stack.
class SegmentStorage {
 public:
  //! \brief Default construct an empty SegmentStorage object.
  SegmentStorage() = default;

  //! \brief Move SegmentStorage.
  SegmentStorage(SegmentStorage &&storage) noexcept {
    moveFrom(std::move(storage));
  }

  SegmentStorage &operator=(SegmentStorage &&storage) noexcept {
    moveFrom(std::move(storage));
    return *this;
  }

  ~SegmentStorage() {
    if (segment_pointer_ && !IsUsingBuffer()) {
      delete segment_pointer_;
      segment_pointer_ = nullptr;
    }
  }

  template <typename Segment_t,
      typename... Args,
      LL_ENABLE_IF(std::is_base_of_v<BaseSegment, Segment_t>)>
  SegmentStorage &Create(Args &&... args) {
    if constexpr (sizeof(Segment_t) <= sizeof(buffer_)) {
      // Use the internal buffer.
      segment_pointer_ = new(buffer_) Segment_t(std::forward<Args>(args)...);
    }
    else {
      // Have to allocate on the heap.
      segment_pointer_ = new Segment_t(std::forward<Args>(args)...);
    }
    return *this;
  }

  BaseSegment *Get() {
    if (segment_pointer_ == nullptr) { // In the buffer.
      return reinterpret_cast<BaseSegment *>(buffer_);
    }
    return segment_pointer_;
  }

  NO_DISCARD const BaseSegment *Get() const {
    if (segment_pointer_ == nullptr) { // In the buffer.
      return reinterpret_cast<const BaseSegment*>(buffer_);
    }
    return segment_pointer_;
  }

  NO_DISCARD bool IsUsingBuffer() const {
    const auto this_ptr = reinterpret_cast<const char *>(this);
    const auto seg_ptr = reinterpret_cast<const char *>(segment_pointer_);
    auto difference = seg_ptr - this_ptr;
    // We can probably just check if this and seg pointer are equal.
    return 0 <= difference && difference < static_cast<long long>(sizeof(SegmentStorage));
  }

  NO_DISCARD bool HasData() const { return segment_pointer_; }

  [[maybe_unused]] static constexpr std::size_t BufferSize() { return sizeof(buffer_); }

 private:
  void moveFrom(SegmentStorage &&storage) {
    if (!storage.segment_pointer_) {
      segment_pointer_ = nullptr;
    }
    else {
      if (storage.IsUsingBuffer()) {
        // TODO: BaseSegment should have a MoveTo function, and call that.
        std::memcpy(buffer_, storage.buffer_, sizeof(buffer_));
        segment_pointer_ = reinterpret_cast<BaseSegment *>(buffer_);
      }
      else {
        segment_pointer_ = storage.segment_pointer_;
      }
      storage.segment_pointer_ = nullptr;
    }
  }

  //! \brief The size of the internal buffer.
  constexpr static std::size_t default_buffer_size = 3 * sizeof(void *);

  //! \brief Pointer to the data that is either on the stack or heap. Allows for polymorphically accessing the
  //! data.
  BaseSegment *segment_pointer_{};

  //! \brief The internal buffer for data. Note that all BaseSegments have a vptr, so that takes up
  //! sizeof(void*) bytes by itself.
  unsigned char buffer_[default_buffer_size] = {0};
};

//! \brief Formatting segment that changes the coloration of a terminal.
struct AnsiColorSegment : public BaseSegment {
  explicit AnsiColorSegment(std::optional<formatting::AnsiForegroundColor> foreground,
                            std::optional<formatting::AnsiBackgroundColor> background = {})
      : fmt_string_(SetAnsiColorFmt(foreground, background)) {}

  void CopyTo(class SegmentStorage &storage) const override { storage.Create<AnsiColorSegment>(*this); }

  void SetColors(std::optional<formatting::AnsiForegroundColor> foreground,
                 std::optional<formatting::AnsiBackgroundColor> background = {}) {
    fmt_string_ = SetAnsiColorFmt(foreground, background);
  }

 private:
  void addToBuffer(const FormattingSettings &settings,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if (settings.has_virtual_terminal_processing) {
      AppendBuffer(buffer, fmt_string_);
    }
  }

  std::string fmt_string_;
};

//! \brief Formatting segment that resets the style.
struct AnsiResetSegment_t : public AnsiColorSegment {
  AnsiResetSegment_t() noexcept
      : AnsiColorSegment(formatting::AnsiForegroundColor::Reset) {}
};

//! \brief Prototypical AnsiResetSegment object. There is no customization possible, so there is no reason
//! to explicitly create more.
const inline AnsiResetSegment_t AnsiResetSegment{};

//! \brief How to count distance while formatting, Specifies whether the pad should be for the message
//! part of the formatted string, or the entire length of the formatted string.
enum class FmtDistanceType : unsigned char {
  MESSAGE_LENGTH,
  TOTAL_LENGTH,
};

//! \brief Inject spaces until the message length or total formatted length reaches a specified length.
struct FillUntil : public BaseSegment {
  explicit FillUntil(unsigned pad_length,
                     char fill_char,
                     FmtDistanceType pad_type = FmtDistanceType::MESSAGE_LENGTH)
      : pad_length_(pad_length), fill_char_(fill_char), pad_type_(pad_type) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<FillUntil>(*this); }

 private:
  void addToBuffer(const FormattingSettings &settings,
                   const formatting::MessageInfo &msg_info,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    const auto num_chars = numToFill(settings, msg_info);
    buffer.AppendN(fill_char_, num_chars);
  }

  NO_DISCARD unsigned numToFill(const FormattingSettings &,
                                const formatting::MessageInfo &msg_info) const {
    if (pad_type_ == FmtDistanceType::MESSAGE_LENGTH) {
      return pad_length_ - std::min(msg_info.message_length, pad_length_);
    }
    // Otherwise, TOTAL_LENGTH
    return pad_length_ - std::min(msg_info.total_length, pad_length_);
  }

  unsigned pad_length_{};
  char fill_char_{};
  FmtDistanceType pad_type_;
};

//! \brief Inject spaces until the message length or total formatted length reaches a specified length.
struct PadUntil : public FillUntil {
  explicit PadUntil(unsigned pad_length, FmtDistanceType pad_type = FmtDistanceType::MESSAGE_LENGTH)
      : FillUntil(pad_length, ' ', pad_type) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<PadUntil>(*this); }
};

//! \brief Segment that repeats a character N times.
struct RepeatChar : public BaseSegment {
  RepeatChar(unsigned repeat_length, char c)
      : repeat_length_(repeat_length), c_(c) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<RepeatChar>(*this); }

 private:
  void addToBuffer(const FormattingSettings &,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    buffer.AppendN(c_, repeat_length_);
  }

  unsigned repeat_length_{};
  char c_{};
};

struct NewLineIndent_t : public BaseSegment {
  NO_DISCARD bool NeedsMessageIndentation() const override { return true; }

  void CopyTo(SegmentStorage &storage) const override { storage.Create<NewLineIndent_t>(); }

 private:
  void addToBuffer(const FormattingSettings &,
                   const formatting::MessageInfo &msg_info,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    buffer.PushBack('\n');
    if (msg_info.message_indentation) {
      buffer.AppendN(' ', *msg_info.message_indentation);
    }
  }
};

//! \brief Prototypical NewLineIndent_t object.
const inline NewLineIndent_t NewLineIndent;

// ==============================================================================
//  Ordinary data formatting segments.
// ==============================================================================

//! \brief Base template for formatting Segment<T>'s. Has a slot to allow for enable_if-ing.
template <typename T, typename Enable = void>
struct Segment;

//! \brief  Type trait that determines whether a type has a to_string function.
NEW_TYPE_TRAIT(has_segment_formatter_v,
               Segment<std::decay_t<typetraits::remove_cvref_t<Value_t>>>(
                   std::declval<std::decay_t<typetraits::remove_cvref_t<Value_t>>>()))

//! \brief Template specialization for string segments.
template <>
struct Segment<std::string> : public BaseSegment {
  explicit Segment(std::string s)
      : str_(std::move(s)) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<Segment>(*this); }

 private:
  void addToBuffer(const FormattingSettings &,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if (!fmt.empty()) {
      formatting::FormatString(fmt, str_, buffer);
    }
    else {
      memory::AppendBuffer(buffer, str_);
    }
  }

  const std::string str_;
};

//! \brief Template specialization for string-view segments.
template <>
struct Segment<std::string_view> : public BaseSegment {
  explicit Segment(std::string_view str_view)
      : str_view_(str_view) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<Segment>(*this); }

 private:
  void addToBuffer(const FormattingSettings &,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if (!fmt.empty()) {
      formatting::FormatString(fmt, str_view_, buffer);
    }
    else {
      memory::AppendBuffer(buffer, str_view_);
    }
  }

  const std::string_view str_view_;
};

//! \brief Template specialization for char* segments.
template <>
struct Segment<char *> : public BaseSegment {
  explicit Segment(const char *s)
      : cstr_(s), size_required_(static_cast<unsigned>(std::strlen(s))) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<Segment>(*this); }

 private:
  void addToBuffer([[maybe_unused]] const FormattingSettings &settings,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if (!fmt.empty()) {
      formatting::FormatString(fmt, std::string_view{cstr_, size_required_}, buffer);
    }
    else {
      buffer.Append(cstr_, cstr_ + size_required_);
    }
  }

  const char *cstr_;
  unsigned size_required_{};
};

//! \brief Template specialization for char array segments. TODO: Can I just combine this with char*?
template <std::size_t N>
struct Segment<char[N]> : public Segment<char *> {
  explicit Segment(const char s[N])
      : Segment<char *>(&s[0]) {}
};

//! \brief Template specialization for bool segments.
template <>
struct Segment<bool> : public BaseSegment {
  explicit Segment(bool b)
      : value_(b) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<Segment>(*this); }

 private:
  void addToBuffer([[maybe_unused]] const FormattingSettings &settings,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if (value_) {
      AppendBuffer(buffer, "true");
    }
    else {
      AppendBuffer(buffer, "false");
    }
  }

  bool value_;
};

//! \brief Template specialization for floating point number segments.
template <typename Floating_t>
struct Segment<Floating_t, std::enable_if_t<std::is_floating_point_v<Floating_t>>> : public BaseSegment {
  explicit Segment(Floating_t number)
      : number_(number) {}

  void CopyTo(SegmentStorage& storage) const override { storage.Create<Segment<Floating_t>>(*this); }

 private:
  void addToBuffer([[maybe_unused]] const FormattingSettings &settings,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if constexpr (typetraits::has_to_chars<Floating_t>) {
      constexpr std::size_t buffer_size = 64;
      char serialization_buffer[buffer_size];
      auto result = std::to_chars(serialization_buffer, serialization_buffer + buffer_size, number_);
      auto string_length = static_cast<std::string_view::size_type>(result.ptr - serialization_buffer);
      memory::AppendBuffer(buffer, std::string_view(serialization_buffer, string_length));
    }
    else {
      std::ostringstream stream;
      stream << number_;
      memory::AppendBuffer(buffer, stream.str());
    }
  }

  Floating_t number_;
};

//! \brief Template specialization for integral value segments.
template <>
struct Segment<char>
    : public BaseSegment {
  explicit Segment(char c)
      : c_(c) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<Segment>(*this); }

 private:
  void addToBuffer([[maybe_unused]] const FormattingSettings &settings,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    buffer.PushBack(c_);
  }

  char c_;
};

//! \brief Template specialization for integral value segments.
template <typename Integral_t>
struct Segment<Integral_t,
               std::enable_if_t<std::is_integral_v<Integral_t> && !std::is_same_v<Integral_t, char>>>
    : public BaseSegment {
  explicit Segment(Integral_t number)
      : number_(number), size_required_(formatting::NumberOfDigits(number_) + (number < 0 ? 1 : 0)) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<Segment>(*this); }

 private:
  void addToBuffer([[maybe_unused]] const FormattingSettings &settings,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if (!fmt.empty()) {
      formatting::FormatInteger(fmt, number_, buffer);
    }
    else {
      auto [start, end] = buffer.Allocate(size_required_);
      std::to_chars(start, end, number_);
    }
  }

  Integral_t number_;
  unsigned size_required_{};
};

//! \brief Template specialization for DateTime segments.
template <>
struct Segment<time::DateTime> : public BaseSegment {
  explicit Segment(time::DateTime dt)
      : value_(dt) {}

  void CopyTo(class SegmentStorage &storage) const override {
    storage.Create<Segment>(*this);
  }

 private:
  void addToBuffer([[maybe_unused]] const FormattingSettings &settings,
                   const formatting::MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    // FormatDateTo needs 26 characters.
    auto [start, end] = buffer.Allocate(26);
    formatting::FormatDateTo(start, end, value_);
  }

  time::DateTime value_;
};

//! \brief Formatting segment that colors a single piece of data.
template <typename T>
struct AnsiColor8Bit : public BaseSegment {
  explicit AnsiColor8Bit(const T &data,
                         std::optional<formatting::AnsiForegroundColor> foreground,
                         std::optional<formatting::AnsiBackgroundColor> background = {})
      : set_formatting_string_(SetAnsiColorFmt(foreground, background)), segment_(data) {}

  void CopyTo(SegmentStorage &storage) const override { storage.Create<AnsiColor8Bit>(*this); }

 private:
  void addToBuffer(const FormattingSettings &settings,
                   const formatting::MessageInfo &msg_info,
                   memory::BasicMemoryBuffer<char> &buffer,
                   [[maybe_unused]] const std::string_view &fmt) const override {
    if (settings.has_virtual_terminal_processing) {
      AppendBuffer(buffer, set_formatting_string_);
    }
    segment_.AddToBuffer(settings, msg_info, buffer);
    AnsiResetSegment.AddToBuffer(settings, msg_info, buffer, fmt);
  }

  //! \brief Cache the formatting string.
  std::string set_formatting_string_;
  //! \brief The object that should be colored.
  Segment<std::decay_t<typetraits::remove_cvref_t<T>>> segment_;
};

//! \brief An object that has a bundle of data, to be formatted.
class RefBundle {
 public:
  //! \brief Stream data into a RefBundle.
  template <typename T>
  RefBundle &operator<<(T &&obj);

  template <typename Segment_t, typename... Args>
  void CreateSegment(Args &&... args) {
    segments_.EmplaceBack();
    segments_.Back().Create<Segment_t>(std::forward<Args>(args)...);
  }

  SegmentStorage &AddSegment() {
    segments_.EmplaceBack();
    return segments_.Back();
  }

  void FmtString(const FormattingSettings &settings,
                 memory::BasicMemoryBuffer<char> &buffer,
                 formatting::MessageInfo &msg_info) const {
    // Reset message length counter.
    msg_info.message_length = 0;
    msg_info.is_in_message_segment = true;
    // Add message.
    for (auto i = 0u; i < segments_.Size(); ++i) {
      auto &bundle = segments_[i];
      const auto size_before = buffer.Size();
      bundle.Get()->AddToBuffer(settings, msg_info, buffer);
      const auto size_after = buffer.Size();
      msg_info.message_length += static_cast<unsigned>(size_after - size_before);
      msg_info.total_length = static_cast<unsigned>(buffer.Size());
    }
    msg_info.is_in_message_segment = false;
  }

  //! \brief Returns whether any segment needs the message indentation to be calculated.
  NO_DISCARD bool NeedsMessageIndentation() const {
    if (segments_.Empty()) {
      return false;
    }

    for (auto i = 0u; i < segments_.Size(); ++i) {
      if (segments_[i].Get()->NeedsMessageIndentation()) {
        return true;
      }
    }
    return false;
  }

 private:
  //! \brief Segment storage, use a stack size of 10.
  memory::HybridVector<SegmentStorage, 10> segments_;
};

//! \brief  Create a type trait that determines if a 'format_logstream' function has been defined for a type.
NEW_TYPE_TRAIT(has_logstream_formatter_v,
               format_logstream(std::declval<const Value_t &>(), std::declval<RefBundle &>()))

template <typename T>
RefBundle &RefBundle::operator<<(T &&obj) {
  using decay_t = std::decay_t<typetraits::remove_cvref_t<T>>;

  if constexpr (has_logstream_formatter_v < decay_t >) {
    format_logstream(obj, *this);
  }
  else if constexpr (std::is_base_of_v<BaseSegment, decay_t>) {
    obj.CopyTo(AddSegment());
  }
  else if constexpr (has_segment_formatter_v < decay_t >) {
    // Add a formatting segment.
    CreateSegment < Segment < decay_t >> (obj);
  }
  else if constexpr (typetraits::has_to_string_v<decay_t>) {
    using std::to_string;  // Enable ADL.
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
//!
//! NOTE(Nate): I have not put a lot of time or attention into this class, so it does not do much right now,
//!  but it is here for future expansion.
class Attribute : public ImplBase {
 public:
  class Impl : public ImplBase::Impl {
   public:
    NO_DISCARD virtual std::unique_ptr<Impl> Copy() const = 0;
  };

  NO_DISCARD Attribute Copy() const { return Attribute(impl<Attribute>()->Copy()); }

  explicit Attribute(std::unique_ptr<Impl> &&impl)
      : ImplBase(std::move(impl)) {}

  Attribute(const Attribute &other)
      : ImplBase(other.impl<Attribute>()->Copy()) {}

  Attribute &operator=(const Attribute &other) {
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

static std::vector ALL_SEVERITIES{
    Severity::Trace,
    Severity::Debug,
    Severity::Info,
    Severity::Major,
    Severity::Warning,
    Severity::Error,
    Severity::Fatal
};

//! \brief Return the index of a severity level in the ALL_SEVERITIES vector.
inline int SeverityIndex(Severity severity) {
  switch (severity) {
    case Severity::Trace:
      return 0;
    case Severity::Debug:
      return 1;
    case Severity::Info:
      return 2;
    case Severity::Major:
      return 3;
    case Severity::Warning:
      return 4;
    case Severity::Error:
      return 5;
    case Severity::Fatal:
      return 6;
    default:
      return -1;
  }
}

//! \brief A "severity set," which acts as a filter of which severities are "acceptable." This is encoded as a bit mask.
//! Boolean operations can be performed on SeveritySets to create more complex filters, and the set can be used to check
//! whether a severity is acceptable.
class SeveritySet {
 public:
  explicit SeveritySet(bool all_or_none) { severity_mask_ = all_or_none ? 0xFFFF : 0; }
  explicit SeveritySet(const std::set<Severity> &severities) : severity_mask_(0) {
    for (auto severity : ALL_SEVERITIES) {
      if (severities.count(severity) != 0) severity_mask_ |= static_cast<int>(severity);
    }
  }

  NO_DISCARD bool Check(Severity severity) const { return severity_mask_ & static_cast<int>(severity); }
  NO_DISCARD bool operator()(Severity severity) const { return Check(severity); }
  NO_DISCARD int GetMask() const { return severity_mask_; }

  SeveritySet &SetAcceptance(Severity severity, bool does_accept) {
    if (does_accept) severity_mask_ |= static_cast<int>(severity);
    else severity_mask_ &= (0b1111111 & ~static_cast<int>(severity));
    return *this;
  }

  friend SeveritySet operator||(SeveritySet lhs, SeveritySet rhs) { return SeveritySet{lhs.GetMask() | rhs.GetMask()}; }
  friend SeveritySet operator&&(SeveritySet lhs, SeveritySet rhs) { return SeveritySet{lhs.GetMask() & rhs.GetMask()}; }
  friend SeveritySet operator!(SeveritySet set) { return SeveritySet{~set.GetMask()}; }
 private:
  explicit SeveritySet(int mask) : severity_mask_(mask) {}
  //! \brief The acceptance mask.
  int severity_mask_ = 0xFFFF;
};

// ==============================================================================================
//  SeveritySet operators.
// ==============================================================================================

//! \brief A type that can be used to specify the severity level of a logging message, for creating SeveritySets using boolean operators.
struct LoggingSeverity_t {};
//! \brief Prototypical LoggingSeverity_t object.
constexpr inline LoggingSeverity_t LoggingSeverity;

inline SeveritySet operator>(Severity severity, LoggingSeverity_t) {
  SeveritySet severity_set(false);
  for (auto sev : ALL_SEVERITIES) {
    if (static_cast<int>(sev) < static_cast<int>(severity)) severity_set.SetAcceptance(sev, true);
  }
  return severity_set;
}

inline SeveritySet operator>=(Severity severity, LoggingSeverity_t) {
  return (severity > LoggingSeverity).SetAcceptance(severity, true);
}

inline SeveritySet operator<(Severity severity, LoggingSeverity_t) { return !(severity >= LoggingSeverity); }
inline SeveritySet operator<=(Severity severity, LoggingSeverity_t) {
  return !(severity > LoggingSeverity);
}
inline SeveritySet operator==(Severity severity, LoggingSeverity_t) {
  return SeveritySet({severity});
}
inline SeveritySet operator!=(Severity severity, LoggingSeverity_t) { return !(severity == LoggingSeverity); }

// Order reversed operators.
inline SeveritySet operator>(LoggingSeverity_t, Severity severity) { return severity < LoggingSeverity; }
inline SeveritySet operator>=(LoggingSeverity_t, Severity severity) { return severity <= LoggingSeverity; }
inline SeveritySet operator<(LoggingSeverity_t, Severity severity) { return severity > LoggingSeverity; }
inline SeveritySet operator<=(LoggingSeverity_t, Severity severity) { return severity >= LoggingSeverity; }
inline SeveritySet operator==(LoggingSeverity_t, Severity severity) { return severity == LoggingSeverity; }
inline SeveritySet operator!=(LoggingSeverity_t, Severity severity) { return severity != LoggingSeverity; }

namespace detail {

inline std::string ThreadIDAsString() {
  std::ostringstream stream;
  stream << std::this_thread::get_id();
  return stream.str();
}

} // namespace detail

//! \brief Get a const reference to the thread ID string.
inline const std::string &GetThreadID() {
  static thread_local auto thread_id = detail::ThreadIDAsString();
  return thread_id;
}

//! \brief Structure storing very common attributes that a logging message will often have.
//!
//! Additional attributes can be implemented as Attribute objects.
//!
struct BasicAttributes {
  BasicAttributes() = default;

  //! \brief Create a basic attributes with a particular severity level.
  //!
  //! Note that we usually set do_timestamp off, since loggers create their own timestamps with their fast
  //! datetime generators.
  explicit BasicAttributes(std::optional<Severity> lvl, bool do_timestamp = false)
      : level(lvl) {
    if (do_timestamp) time_stamp = time::DateTime::Now();
  }

  //! \brief Create a basic attributes with all possible data.
  //!
  //! Note that we usually set do_timestamp off, since loggers create their own timestamps with their fast datetime generators.
  explicit BasicAttributes(std::optional<Severity> lvl,
                           const char *file_name,
                           const char *function_name,
                           unsigned line_number,
                           bool do_timestamp = false)
      : level(lvl), file_name(file_name), function_name(function_name), line_number(line_number) {
    if (do_timestamp) time_stamp = time::DateTime::Now();
  }

  //! \brief The severity level of the record.
  std::optional<Severity> level{};

  //! \brief String representation of the thread ID.
  const std::string &thread_id = GetThreadID();

  //! \brief The time at which the record was created.
  std::optional<time::DateTime> time_stamp{};

  //! \brief A string view of the name of the logger which sent a message.
  std::string_view logger_name{};

  //! \brief Const char* to the file name.
  const char *file_name{};

  //! \brief Const char* to the function name.
  const char *function_name{};

  //! \brief Optionally, the line number that the log is from.
  std::optional<unsigned> line_number{};
};

//! \brief A filter that checks whether a record should be accepted solely based on its severity.
//!
//! Essentially, this is a wrapper around a SeveritySet, plus an option to accept messages that do not have a
//! severity attribute.
class BasicSeverityFilter {
 public:
  NO_DISCARD bool Check(std::optional<Severity> severity) const {
    if (severity) return filter_(*severity);
    return allow_if_no_severity_;
  }

  BasicSeverityFilter &SetAcceptance(Severity severity, bool does_accept) {
    filter_.SetAcceptance(severity, does_accept);
    return *this;
  }

  BasicSeverityFilter &SetAcceptance(SeveritySet acceptable) {
    filter_ = acceptable;
    return *this;
  }

  BasicSeverityFilter &AcceptNoSeverity(bool flag) {
    allow_if_no_severity_ = flag;
    return *this;
  }

 private:
  //! \brief The severity acceptance filter.
  SeveritySet filter_{true};

  //! \brief Whether to accept a message that does not contain the severity attribute.
  bool allow_if_no_severity_ = true;
};

//! \brief Object containing all attributes for a record.
struct RecordAttributes {
  template <typename... Attrs_t>
  explicit RecordAttributes(BasicAttributes basic_attributes = {}, Attrs_t &&... attrs)
      : basic_attributes(basic_attributes) {
    attributes.reserve(sizeof...(attrs));
    (attributes.emplace_back(std::move(attrs)), ...);
  }

  //! \brief The basic record attributes. These are stored as fields, which is much faster to
  //! create and handle than pImpl attribute objects.
  BasicAttributes basic_attributes{};

  //! \brief Additional attributes, beyond the basic attributes.
  std::vector<Attribute> attributes;
};

namespace filter {

//! \brief Class that can be configured to test whether a record should be accepted based on its attributes.
class AttributeFilter {
 public:
  virtual ~AttributeFilter() = default;

  NO_DISCARD bool WillAccept(const RecordAttributes &attributes) const {
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
  AttributeFilter &Accept(const std::set<Severity> &acceptable) {
    for (auto sev : ALL_SEVERITIES) {
      severity_filter_.SetAcceptance(sev, acceptable.count(sev) != 0);
    }
    return *this;
  }

  AttributeFilter &Accept(SeveritySet acceptable) {
    severity_filter_.SetAcceptance(acceptable);
    return *this;
  }

  //! \brief Shortcut for accepting all severities.
  AttributeFilter &Accept(LoggingSeverity_t) {
    severity_filter_.SetAcceptance(SeveritySet(true));
    return *this;
  }

  //! \brief Set whether a message with no severity level should be accepted.
  AttributeFilter &AcceptNoSeverity(bool flag) {
    severity_filter_.AcceptNoSeverity(flag);
    return *this;
  }

  //! \brief Reset the filter.
  AttributeFilter &Clear() {
    severity_filter_ = BasicSeverityFilter();
    return *this;
  }

 private:
  //! \brief Private implementation of whether a message should be accepted based on its attributes.
  NO_DISCARD virtual bool willAccept([[maybe_unused]] const std::vector<Attribute> &attributes) const {
    // TODO.
    return true;
  }

  //! \brief The filter used to decide if a record should be accepted based on its severity settings.
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

  template <typename... Attrs_t>
  explicit Record(BasicAttributes basic_attributes = {}, Attrs_t &&... attrs)
      : attributes_(basic_attributes, std::forward<Attrs_t>(attrs)...) {}

  //! \brief Get the message bundle from the record.
  RefBundle &Bundle() { return bundle_; }

  //! \brief Get a const ref to the RefBundle of the record.
  NO_DISCARD const RefBundle &Bundle() const { return bundle_; }

  //! \brief Get the record attributes.
  NO_DISCARD const RecordAttributes &Attributes() const { return attributes_; }

  NO_DISCARD RecordAttributes &Attributes() { return attributes_; }

  //! \brief Try to open the record for a core. Returns whether the record opened.
  inline bool TryOpen(std::shared_ptr<Core> core);

  //! \brief Test whether a record is open.
  inline explicit operator bool() const;

  //! \brief Dispatch the record to the associated core.
  inline void Dispatch();

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
  [[maybe_unused]] explicit RecordDispatcher(Record &&record)
      : record_(std::move(record)), uncaught_exceptions_(std::uncaught_exceptions()) {}

  //! \brief Construct a record handler, constructing the record in-place inside it.
  template <typename... Attrs_t>
  explicit RecordDispatcher(std::shared_ptr<Core> core,
                            BasicAttributes basic_attributes = {},
                            Attrs_t &&... attrs)
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
  NO_DISCARD bool RecordIsOpen() const { return static_cast<bool>(record_); }

  //! \brief Check whether a record is open via explicit conversion to bool.
  explicit operator bool() const { return RecordIsOpen(); }

  //! \brief Get the record from the RecordDispatcher.
  Record &GetRecord() { return record_; }

  template <typename T>
  RecordDispatcher &operator<<(T &&obj) {
    record_.Bundle() << std::forward<T>(obj);
    return *this;
  }

 private:
  Record record_{};
  int uncaught_exceptions_{};
};

namespace formatting {

//! \brief Base class for attribute formatters, objects that know how to serialize attribute representations
//! to strings.
class AttributeFormatter {
 public:
  virtual ~AttributeFormatter() = default;

  virtual void AddToBuffer(const RecordAttributes &attributes,
                           const FormattingSettings &settings,
                           const formatting::MessageInfo &msg_info,
                           memory::BasicMemoryBuffer<char> &buffer) const = 0;
};

//! \brief Format the severity attribute.
class SeverityAttributeFormatter : public AttributeFormatter {
 public:
  explicit SeverityAttributeFormatter(bool aligned_names = true) {
    if (aligned_names) setAlignedNames();
    else setUnalignedNames();
  }

  void AddToBuffer(const RecordAttributes &attributes,
                   const FormattingSettings &settings,
                   const formatting::MessageInfo &msg_info,
                   memory::BasicMemoryBuffer<char> &buffer) const override {
    if (attributes.basic_attributes.level) {
      colorSegment(attributes.basic_attributes.level.value()).AddToBuffer(settings, msg_info, buffer);
      auto &str = getString(attributes.basic_attributes.level.value());
      AppendBuffer(buffer, str);
      AnsiResetSegment.AddToBuffer(settings, msg_info, buffer);
    }
  }

  SeverityAttributeFormatter &SeverityName(Severity severity, const std::string &name) {
    getString(severity) = name;
    return *this;
  }

  SeverityAttributeFormatter &SeverityFormatting(Severity severity,
                                                 std::optional<AnsiForegroundColor> foreground,
                                                 std::optional<AnsiBackgroundColor> background = {}) {
    auto &color_formatting = colorSegment(severity);
    color_formatting.SetColors(foreground, background);
    return *this;
  }

 private:
  void setAlignedNames() {
    trace_ = "Trace  ";
    debug_ = "Debug  ";
    info_ = "Info   ";
    major_ = "Major  ";
    warn_ = "Warning";
    error_ = "Error  ";
    fatal_ = "Fatal  ";
  }

  void setUnalignedNames() {
    trace_ = "Trace";
    debug_ = "Debug";
    info_ = "Info";
    major_ = "Major";
    warn_ = "Warning";
    error_ = "Error";
    fatal_ = "Fatal";
  }

  NO_DISCARD const std::string &getString(Severity severity) const {
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

  NO_DISCARD std::string &getString(Severity severity) {
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

  NO_DISCARD const AnsiColorSegment &colorSegment(Severity severity) const {
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

  NO_DISCARD AnsiColorSegment &colorSegment(Severity severity) {
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

  AnsiColorSegment trace_colors_{AnsiForegroundColor::White};
  AnsiColorSegment debug_colors_{AnsiForegroundColor::BrightWhite};
  AnsiColorSegment info_colors_{AnsiForegroundColor::Green};
  AnsiColorSegment major_colors_{AnsiForegroundColor::BrightBlue};
  AnsiColorSegment warn_colors_{AnsiForegroundColor::Yellow};
  AnsiColorSegment error_colors_{AnsiForegroundColor::Red};
  AnsiColorSegment fatal_colors_{AnsiForegroundColor::BrightRed};
};

class DateTimeAttributeFormatter final : public AttributeFormatter {
  // TODO: Allow for different formatting of the DateTime, via format string?
 public:
  void AddToBuffer(const RecordAttributes &attributes,
                   const FormattingSettings &,
                   const MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer) const override {
    if (attributes.basic_attributes.time_stamp) {
      auto &dt = attributes.basic_attributes.time_stamp.value();
      auto [start, end] = buffer.Allocate(26);
      formatting::FormatDateTo(start, end, dt);
    }
  }
};

class LoggerNameAttributeFormatter final : public AttributeFormatter {
 public:
  void AddToBuffer(const RecordAttributes &attributes,
                   const FormattingSettings &,
                   const MessageInfo &,
                   memory::BasicMemoryBuffer<char> &buffer) const override {
    AppendBuffer(buffer, attributes.basic_attributes.logger_name);
  }
};

//! \brief Formatter for the file name attribute.
class FileNameAttributeFormatter final : public AttributeFormatter {
 public:
  explicit FileNameAttributeFormatter(bool only_file_name = false)
      : only_file_name_(only_file_name) {}

  void AddToBuffer(const RecordAttributes &attributes,
                   [[maybe_unused]] const FormattingSettings &settings,
                   [[maybe_unused]] const MessageInfo &info,
                   memory::BasicMemoryBuffer<char> &buffer) const override {
    if (attributes.basic_attributes.file_name) {
      if (only_file_name_) {
        auto [first, last] = getRange(attributes.basic_attributes.file_name);
        buffer.Append(attributes.basic_attributes.file_name + first,
                      attributes.basic_attributes.file_name + last);
      }
      else {
        AppendBuffer(buffer, attributes.basic_attributes.file_name);
      }
    }
  }

 private:
  std::pair<std::size_t, std::size_t> getRange(const char *str) const {
    std::size_t first = 0, last = 0;
    for (auto ptr = str; *ptr != '\0'; ++ptr, ++last) {
      if (only_file_name_ && (*ptr == '/' || *ptr == '\\'))
        first = last + 1;
    }
    return {first, last};
  }

  //! \brief If true, only the file name is displayed, not the full path.
  bool only_file_name_;
};

//! \brief Formatter for the function name attribute.
class FunctionNameAttributeFormatter final : public AttributeFormatter {
 public:
  void AddToBuffer(const RecordAttributes &attributes,
                   [[maybe_unused]] const FormattingSettings &settings,
                   [[maybe_unused]] const MessageInfo &info,
                   memory::BasicMemoryBuffer<char> &buffer) const override {
    if (attributes.basic_attributes.function_name) {
      AppendBuffer(buffer, attributes.basic_attributes.function_name);
    }
  }
};

//! \brief Formatter for the file name attribute.
class FileLineAttributeFormatter final : public AttributeFormatter {
 public:
  void AddToBuffer(const RecordAttributes &attributes,
                   [[maybe_unused]] const FormattingSettings &settings,
                   [[maybe_unused]] const MessageInfo &info,
                   memory::BasicMemoryBuffer<char> &buffer) const override {
    if (attributes.basic_attributes.line_number) {
      // Calculate the length, base 10, of line_number
      auto size = NumberOfDigits(*attributes.basic_attributes.line_number);
      // Reserve this much additional size
      auto [start, end] = buffer.Allocate(size);
      std::to_chars(start, end, *attributes.basic_attributes.line_number);
    }
  }
};

//! \brief Formatter for the file name attribute.
class ThreadAttributeFormatter final : public AttributeFormatter {
 public:
  void AddToBuffer(const RecordAttributes &attributes,
                   [[maybe_unused]] const FormattingSettings &settings,
                   [[maybe_unused]] const MessageInfo &info,
                   memory::BasicMemoryBuffer<char> &buffer) const override {
    AppendBuffer(buffer, attributes.basic_attributes.thread_id);
  }
};

//! \brief  Function to calculate how far the start of the message is from the last newline in the header,
//! counting only visible (non ansi virtual terminal) characters.
inline unsigned CalculateMessageIndentation(char *buffer_end, const MessageInfo &msg_info) {
  if (msg_info.total_length == 0) {
    return 0;
  }
  auto c = buffer_end - 1;
  auto rend = c - msg_info.total_length;
  for (; c != rend && *c != '\n'; --c);
  ++c; // Start after the '\n' or char-before-begin.
  return CountNonAnsiSequenceCharacters(c, buffer_end);
}

//! \brief  Base class for message formatters, objects capable of taking a record and formatting it into a
//! string,
//!         according to the formatting settings.
class BaseMessageFormatter {
 public:
  virtual ~BaseMessageFormatter() = default;

  virtual void Format(const Record &record,
                      const FormattingSettings &sink_settings,
                      memory::BasicMemoryBuffer<char> &buffer) const = 0;

  NO_DISCARD virtual std::unique_ptr<BaseMessageFormatter> Copy() const = 0;
};

//! \brief Object used to represent a placeholder for the logging message.
struct MSG_t {};

//! \brief Global prototypical MSG_t object.
constexpr inline MSG_t MSG;

//! \brief Currently the main and most useful message formatter, this formatter is templated by the types of
//! the segments that it contains. It is defined with a string that contains literal segments and segments
//! that are replaced by the values of the attributes. There are several helper functions, below, that make
//! creating a MsgFormatter easier.
template <typename... Types>
class MsgFormatter : public BaseMessageFormatter {
  static_assert(((std::is_base_of_v<AttributeFormatter, Types> || std::is_same_v<MSG_t, Types>) && ...),
                "All types must be AttributeFormatters or a MSG tag.");

 public:
  explicit MsgFormatter(const std::string &fmt_string, const Types &... types)
      : formatters_(types...) {
    // Find the segments, ensure that there are the right number of arguments.
    auto count_slots = 0;
    literals_.emplace_back();
    for (auto i = 0u; i < fmt_string.size();) {
      if (fmt_string[i] == '{') {
        // Always advance i, since it is either an escaped '{' or we need to find the end of the "{...}"
        if (i + 1 < fmt_string.size() && fmt_string[++i] != '{') {
          // Start of a slot, end of the literal.
          literals_.emplace_back(); // start a new literal
          ++count_slots;
          // Find the closing '}' TODO: Capture a formatting string?
          for (; i < fmt_string.size() && fmt_string[i] != '}'; ++i);
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

  void Format(const Record &record,
              const FormattingSettings &sink_settings,
              memory::BasicMemoryBuffer<char> &buffer) const override {
    MessageInfo msg_info{};
    msg_info.needs_message_indentation = record.Bundle().NeedsMessageIndentation();

    // Format all the segments.
    format<0>(buffer, record, sink_settings, msg_info);
    // Add the terminator.
    AppendBuffer(buffer, sink_settings.message_terminator);
  }

  NO_DISCARD std::unique_ptr<BaseMessageFormatter> Copy() const override {
    return std::unique_ptr<BaseMessageFormatter>(new MsgFormatter(*this));
  }

 private:
  template <std::size_t N>
  void format(memory::BasicMemoryBuffer<char> &buffer,
              [[maybe_unused]] const Record &record,
              [[maybe_unused]] const FormattingSettings &sink_settings,
              MessageInfo &msg_info) const {
    // First, the next literal segment.
    AppendBuffer(buffer, literals_[N]);
    msg_info.total_length = static_cast<unsigned>(buffer.Size());

    if constexpr (N != sizeof...(Types)) {
      // Then the formatter from the slot.
      if constexpr (std::is_same_v<MSG_t, std::tuple_element_t<N, decltype(formatters_)>>) {
        // Since this calculation can take some time, only calculate if needed.
        if (msg_info.needs_message_indentation) {
          // At this point, we can calculate the true message indentation by looking for the last newline
          // (if any), and not counting escape characters.
          msg_info.message_indentation = CalculateMessageIndentation(buffer.End(), msg_info);
        }
        else {
          msg_info.message_indentation = 0u;
        }
        // Bundle's FmtString function will set msg_info.is_in_message_segment to true, so we don't need to here.
        record.Bundle().FmtString(sink_settings, buffer, msg_info);
      }
      else {
        std::get<N>(formatters_).AddToBuffer(record.Attributes(), sink_settings, msg_info, buffer);
        msg_info.total_length = static_cast<unsigned>(buffer.Size());
      }
      // Recursively format the next literal and (if it does not terminate) segment.
      format < N + 1 > (buffer, record, sink_settings, msg_info);
    }
  }

  std::tuple<Types...> formatters_;
  std::vector<std::string> literals_;
};

//! \brief Helper function to create a unique pointer to a MsgFormatter
template <typename... Types>
auto MakeMsgFormatter(const std::string &fmt_string, const Types &... types) {
  return std::unique_ptr<BaseMessageFormatter>(new MsgFormatter<Types...>(fmt_string, types...));
}

//! \brief Helper function that makes the "standard" MsgFormatter. What is standard may change, and only be
//! "standard" in the eye of the beholder, but I am the beholder, and these are the basic things I want in
//! a simple logging formatter.
inline auto MakeStandardFormatter() {
  return MakeMsgFormatter("[{}] [{}] {}",
                          formatting::SeverityAttributeFormatter{},
                          formatting::DateTimeAttributeFormatter{},
                          formatting::MSG);
}

//! \brief A message formatter that delegates formatting to a different formatter based on the severity of the record.
class FormatterBySeverity : public BaseMessageFormatter {
 public:
  void Format(const Record &record,
              const FormattingSettings &sink_settings,
              memory::BasicMemoryBuffer<char> &buffer) const override {
    // Format all the segments.
    auto formatter = getFormatter(record.Attributes().basic_attributes.level);
    if (formatter) {
      MessageInfo msg_info{};
      msg_info.needs_message_indentation = record.Bundle().NeedsMessageIndentation();
      formatter->Format(record, sink_settings, buffer);
    }
  }

  NO_DISCARD std::unique_ptr<BaseMessageFormatter> Copy() const override {
    auto formatter = std::make_unique<FormatterBySeverity>();
    for (auto i = 0u; i < 7; ++i) {
      formatter->formatters_[i] = formatters_[i] ? formatters_[i]->Copy() : nullptr;
    }
    formatter->default_formatter_ = default_formatter_ ? default_formatter_->Copy() : nullptr;
    return formatter;
  }

  FormatterBySeverity& SetFormatterForSeverity(Severity severity, std::unique_ptr<BaseMessageFormatter>&& formatter) {
    if (auto index = SeverityIndex(severity); index != -1) {
      formatters_[index] = std::move(formatter);
      return *this;
    }
    LL_FAIL("unrecognized severity");
  }
  FormatterBySeverity &SetFormatterForSeverity(Severity severity, const BaseMessageFormatter &formatter) {
    if (auto index = SeverityIndex(severity); index != -1) {
      formatters_[index] = formatter.Copy();
      return *this;
    }
    LL_FAIL("unrecognized severity");
  }

  FormatterBySeverity &SetFormatterForSeverity(SeveritySet condition, const BaseMessageFormatter &formatter) {
    for (auto severity : ALL_SEVERITIES) {
      if (condition(severity)) formatters_[SeverityIndex(severity)] = formatter.Copy();
    }
    return *this;
  }

  FormatterBySeverity& SetDefaultFormatter(std::unique_ptr<BaseMessageFormatter>&& formatter) {
    default_formatter_ = std::move(formatter);
    return *this;
  }
  FormatterBySeverity &SetDefaultFormatter(BaseMessageFormatter &formatter) {
    default_formatter_ = formatter.Copy();
    return *this;
  }

 private:
  //! \brief A function that, given the severity level, returns the formatter for that level.
  //! If the severity is not recognized, returns the default formatter.
  NO_DISCARD const BaseMessageFormatter* getFormatter(std::optional<Severity> severity) const {
    if (severity) {
      if (auto index = SeverityIndex(*severity); index != -1) {
        if (formatters_[index]) {
          return formatters_[index].get();
        }
        return default_formatter_.get();
      }
      // Severity provided but it is ill-formed.
      LL_FAIL("unrecognized severity");
    }
    return default_formatter_.get();
  }

  //! \brief The formatters for each severity level.
  std::unique_ptr<BaseMessageFormatter> formatters_[7];
  std::unique_ptr<BaseMessageFormatter> default_formatter_;
};

//! \brief Another type of BaseMessageFormatter, this one can be created and added to without being templated by
//! the segment types, but is generally lower performance.
class RecordFormatter : public BaseMessageFormatter {
 public:
  //! \brief The default record formatter just prints the message.
  RecordFormatter() { AddMsgSegment(); }

  void Format(const Record &record,
              const FormattingSettings &sink_settings,
              memory::BasicMemoryBuffer<char> &buffer) const override {
    MessageInfo msg_info{};
    for (const auto &formatter : formatters_) {
      switch (formatter.index()) {
        case 0: {
          // TODO: Update for LogNewLine type alignment.
          record.Bundle().FmtString(sink_settings, buffer, msg_info);
          break;
        }
        case 1: {
          std::get<1>(formatter)->AddToBuffer(record.Attributes(), sink_settings, msg_info, buffer);
          break;
        }
        case 2: {
          AppendBuffer(buffer, std::get<2>(formatter));
          break;
        }
      }
    }
    // Add terminator.
    AppendBuffer(buffer, sink_settings.message_terminator);
  }

  NO_DISCARD std::unique_ptr<BaseMessageFormatter> Copy() const override {
    return std::unique_ptr<BaseMessageFormatter>(new RecordFormatter(*this));
  }

  RecordFormatter &AddMsgSegment() {
    formatters_.emplace_back(MSG);
    return *this;
  }

  RecordFormatter &AddAttributeFormatter(std::shared_ptr<AttributeFormatter> formatter) {
    formatters_.emplace_back(std::move(formatter));
    return *this;
  }

  RecordFormatter &AddLiteralSegment(std::string literal) {
    formatters_.emplace_back(std::move(literal));
    return *this;
  }

  RecordFormatter &ClearSegments() {
    formatters_.clear();
    return *this;
  }

  NO_DISCARD std::size_t NumSegments() const { return formatters_.size(); }

 private:
  std::vector<std::variant<MSG_t, std::shared_ptr<AttributeFormatter>, std::string>> formatters_;
};

} // namespace formatting

namespace flush {

//! \brief Base class for objects that determine whether a sink should be flushed.
class FlushHandler : public ImplBase {
 public:
  class Impl : public ImplBase::Impl {
   public:
    virtual bool DoFlush(const Record &record) = 0;
    NO_DISCARD virtual std::shared_ptr<Impl> Clone() const = 0;
  };

  bool DoFlush(const Record &record) { return impl<FlushHandler>()->DoFlush(record); }
  NO_DISCARD FlushHandler Clone() const { return FlushHandler(impl<FlushHandler>()->Clone()); }

  explicit FlushHandler(const std::shared_ptr<Impl> &impl)
      : ImplBase(impl) {}
};

//! \brief Flush after every message.
class AutoFlush : public FlushHandler {
 public:
  class Impl : public FlushHandler::Impl {
   public:
    bool DoFlush(const Record &) override { return true; }
    NO_DISCARD std::shared_ptr<FlushHandler::Impl> Clone() const override { return std::make_shared<Impl>(); }
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

    bool DoFlush(const Record &) override {
      ++count_;
      count_ %= N_;
      return count_ == 0;
    }

    NO_DISCARD std::shared_ptr<FlushHandler::Impl> Clone() const override { return std::make_shared<Impl>(N_); }

   private:
    std::size_t count_{}, N_;
  };

  explicit FlushEveryN(std::size_t N)
      : FlushHandler(std::make_shared<Impl>(N)) {}
};

class DisjunctionFlushHandler : public FlushHandler {
 public:
  class Impl : public FlushHandler::Impl {
   public:
    Impl(FlushHandler lhs, FlushHandler rhs)
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    bool DoFlush(const Record &record) override { return lhs_.DoFlush(record) || rhs_.DoFlush(record); }

    NO_DISCARD std::shared_ptr<FlushHandler::Impl> Clone() const override {
      return std::make_shared<Impl>(lhs_.Clone(), rhs_.Clone());
    }
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
        : lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

    bool DoFlush(const Record &record) override { return lhs_.DoFlush(record) && rhs_.DoFlush(record); }

    NO_DISCARD std::shared_ptr<FlushHandler::Impl> Clone() const override {
      return std::make_shared<Impl>(lhs_.Clone(), rhs_.Clone());
    }
   private:
    FlushHandler lhs_, rhs_;
  };

  ConjunctionFlushHandler(FlushHandler lhs, FlushHandler rhs)
      : FlushHandler(std::make_shared<Impl>(std::move(lhs), std::move(rhs))) {}
};

inline DisjunctionFlushHandler operator||(const FlushHandler &lhs, const FlushHandler &rhs) {
  return {lhs, rhs};
}

inline ConjunctionFlushHandler operator&&(const FlushHandler &lhs, const FlushHandler &rhs) {
  return {lhs, rhs};
}

} // namespace flush

//! \brief  Base class for sink backends, which are responsible for actually handling the record and
//! doing something with it. Not responsible for thread synchronization, this is the job of the frontend.
class SinkBackend {
 public:
  //! \brief Flush the sink upon deletion.
  virtual ~SinkBackend() { Flush(); }

  //! \brief Dispatch a record.
  void Dispatch(memory::BasicMemoryBuffer<char> &buffer, const Record &record) {
    preMessage();
    dispatch(buffer, record);
    postMessage();
    if (auto_flush_ || (flush_handler_ && flush_handler_->DoFlush(record))) {
      Flush();
    }
  }

  //! \brief Flush the sink. This is implementation defined.
  SinkBackend &Flush() {
    flush();
    return *this;
  }

  //! \brief Get the sink formatting settings.
  NO_DISCARD const FormattingSettings &GetFormattingSettings() const { return settings_; }
  NO_DISCARD FormattingSettings &GetFormattingSettings() { return settings_; }

  template <typename FlushHandler_t, typename... Args_t>
  SinkBackend &CreateFlushHandler(Args_t &&... args) {
    flush_handler_ = FlushHandler_t(std::forward<Args_t>(args)...);
    return *this;
  }

  SinkBackend &SetFlushHandler(flush::FlushHandler &&flush_handler) {
    flush_handler_ = std::move(flush_handler);
    return *this;
  }

  SinkBackend &ClearFlushHandler() {
    flush_handler_ = std::nullopt;
    return *this;
  }

  SinkBackend &SetAutoFlush(bool auto_flush) {
    auto_flush_ = auto_flush;
    return *this;
  }

  NO_DISCARD virtual std::unique_ptr<SinkBackend> Clone() const = 0;

  void CopySettings(const SinkBackend &other) {
    settings_ = other.settings_;
    flush_handler_ = other.flush_handler_;
    auto_flush_ = other.auto_flush_;
  }
 protected:
  virtual void dispatch(memory::BasicMemoryBuffer<char> &buffer, const Record &record) = 0;

  //! \brief Protected implementation of flushing the sink.
  virtual void flush() {}

  NO_DISCARD std::optional<flush::FlushHandler> copyFlushHandler() const {
    return flush_handler_ ? std::optional(flush_handler_->Clone()) : std::nullopt;
  }

  virtual void preMessage() {}
  virtual void postMessage() {}

  //! \brief The sink formatting settings.
  FormattingSettings settings_;

  //! \brief An optional flush handler, which determines whether the sink should be flushed based on the record.
  std::optional<flush::FlushHandler> flush_handler_{};

  //! \brief Whether to automatically flush the sink after each message.
  bool auto_flush_ = false;
};

// ==============================================================================================
//  Synchronization wrappers
// ==============================================================================================

//! \brief Base class for object wrappers. These, in general, potentially lock the object while it exists, keeping
//! other threads from accessing it while the wrapper is alive.
//! Sink frontends are responsible for thread synchronization, and only frontend sinks that use a mutex will return a
//! truly "locked" sink, others will just return a SinkWrapper.
template <typename Object_t>
class ObjectWrapper : public ImplBase {
  friend class ImplBase;

 protected:
  struct Impl : public ImplBase::Impl {
    explicit Impl(Object_t *backend) : backend(backend) {}
    Object_t *backend{};
  };

  explicit ObjectWrapper(const std::shared_ptr<Impl> &impl) : ImplBase(impl) {}
 public:
  explicit ObjectWrapper(Object_t *backend) : ImplBase(std::make_shared<Impl>(backend)) {}
  Object_t *operator->() { return impl<ObjectWrapper>()->backend; }
  explicit operator bool() { return static_cast<bool>(impl<ObjectWrapper>()->backend); }

  //! \brief Try casting the backend to a specific type.
  template <typename OtherObject_t>
  OtherObject_t *As() { return dynamic_cast<OtherObject_t *>(impl<ObjectWrapper>()->backend); }
};

//! \brief A locked object wrapper that controlls access via a mutex.
template <typename Object_t>
class LockedObject : public ObjectWrapper<Object_t> {
 protected:
  struct Impl : public ObjectWrapper<Object_t>::Impl {
    explicit Impl(Object_t *backend, std::mutex &mutex)
        : ObjectWrapper<Object_t>::Impl(backend), lock(mutex) {}
    std::lock_guard<std::mutex> lock;
  };

 public:
  explicit LockedObject(Object_t *backend, std::mutex &mutex)
      : ObjectWrapper<Object_t>(std::make_shared<Impl>(backend, mutex)) {}
};

//! \brief A locked sink that uses a mutex to lock the sink, controlling access.
using LockedSink = LockedObject<SinkBackend>;

// ==============================================================================================
//  Sinks
// ==============================================================================================

//! \brief Base class for sink frontends. These are responsible for common tasks, like filtering
//! and controlling access to the sink backend, i.e. synchronization.
class Sink {
 public:
  //! \brief Construct a sink around a specific backend.
  explicit Sink(std::unique_ptr<SinkBackend> &&backend)
      : sink_backend_(std::move(backend)), formatter_(formatting::MakeStandardFormatter()) {}

  virtual ~Sink() = default;

  //! \brief Check if a sink, given the record's attributes, will accept the record.
  NO_DISCARD bool WillAccept(const RecordAttributes &attributes) const { return filter_.WillAccept(attributes); }

  //! \brief Check if a sink, given the record's severity, will accept the record.
  NO_DISCARD bool WillAccept(std::optional<Severity> severity) const { return filter_.WillAccept(severity); }

  //! \brief Get the AttributeFilter for the sink.
  filter::AttributeFilter &GetFilter() { return filter_; }

  //! \brief Dispatch a record.
  virtual void Dispatch(const Record &record) { dispatch(record); }

  // ==============================================================================================
  //  Pass-through methods to the backend.
  // ==============================================================================================

  //! \brief Get the record formatter.
  formatting::BaseMessageFormatter &GetFormatter() { return *formatter_; }

  //! \brief Set the severity filter.
  Sink &SetFilter(SeveritySet severity_set) {
    filter_.Accept(severity_set);
    return *this;
  }

  //! \brief Set the filter back to default.
  Sink &ClearFilters() {
    filter_ = filter::AttributeFilter{};
    return *this;
  }

  //! \brief Set the sink's formatter.
  Sink& SetFormatter(std::unique_ptr<formatting::BaseMessageFormatter> &&formatter) {
    formatter_ = std::move(formatter);
    return *this;
  }

  //! \brief 'Flush' the sink. This is implementation defined.
  Sink &Flush() {
    sink_backend_->Flush();
    return *this;
  }

  //! \brief Get the sink backend.
  SinkBackend &GetBackend() { return *sink_backend_; }
  NO_DISCARD const SinkBackend &GetBackend() const { return *sink_backend_; }

  NO_DISCARD ObjectWrapper<SinkBackend> GetLockedBackend() { return getLockedBackend(); }

  //! \brief Get the sink backend, cast to a specific type.
  //!
  //! returns nullptr if the type was not correct.
  template <typename SinkBackend_t>
  SinkBackend_t *GetBackendAs() {
    return dynamic_cast<SinkBackend_t *>(sink_backend_.get());
  }

  NO_DISCARD std::shared_ptr<Sink> Clone() const {
    auto sink = clone();
    sink->filter_ = filter_;
    sink->formatter_ = formatter_->Copy();
    return sink;
  }

 protected:
  //! \brief Private virtual record dispatch method.
  virtual void dispatch(const Record &record) = 0;

  //! \brief Private virtual clone method.
  NO_DISCARD virtual std::shared_ptr<Sink> clone() const = 0;

  //! \brief Get the sink backend, wrapped in a SinkWrapper. The default implementation does not lock the sink, since
  //! the Sink base class has no mutex.
  virtual ObjectWrapper<SinkBackend> getLockedBackend() {
    return ObjectWrapper<SinkBackend>(sink_backend_.get());
  }

  //! \brief The sink backend, to which the frontend feeds record.
  std::unique_ptr<SinkBackend> sink_backend_{};

  //! \brief The Sink's attribute filters.
  filter::AttributeFilter filter_;

  //! \brief The sink's formatter.
  std::unique_ptr<formatting::BaseMessageFormatter> formatter_;
};

//! \brief Sink frontend that uses no synchronization methods.
class UnlockedSink : public Sink {
 public:
  explicit UnlockedSink(std::unique_ptr<SinkBackend> &&backend)
      : Sink(std::move(backend)) {}

  template <typename SinkBackend_t, typename... Args_t>
  static std::shared_ptr<UnlockedSink> From(Args_t &&... args) {
    static_assert(std::is_base_of_v<SinkBackend, SinkBackend_t>, "sink type must be a child of SinkBackend");
    return std::make_shared<UnlockedSink>(std::make_unique<SinkBackend_t>(std::forward<Args_t>(args)...));
  }

 private:
  void dispatch(const Record &record) override {
    memory::MemoryBuffer<char> buffer;
    if (sink_backend_->GetFormattingSettings().needs_formatting) {
      formatter_->Format(record, sink_backend_->GetFormattingSettings(), buffer);
    }
    sink_backend_->Dispatch(buffer, record);
  }

  NO_DISCARD std::shared_ptr<Sink> clone() const override { return std::make_shared<UnlockedSink>(sink_backend_->Clone()); }
};

//! \brief Sink frontend that uses a mutex to control access.
class SynchronousSink : public Sink {
 public:
  explicit SynchronousSink(std::unique_ptr<SinkBackend> &&backend)
      : Sink(std::move(backend)) {}

  template <typename SinkBackend_t, typename... Args_t>
  static std::shared_ptr<SynchronousSink> From(Args_t &&... args) {
    static_assert(std::is_base_of_v<SinkBackend, SinkBackend_t>, "sink type must be a child of SinkBackend");
    return std::make_shared<SynchronousSink>(std::make_unique<SinkBackend_t>(std::forward<Args_t>(args)...));
  }

  bool IsLocked() const {
    if (lock_.try_lock()) {
      lock_.unlock();
      return false;
    }
    return true;
  }
 private:
  void dispatch(const Record &record) override {
    memory::MemoryBuffer<char> buffer;
    // Technically, there could be some small asynchrony issue here with needs formatting being changed by another thread,
    // but not only is it unlikely, it cannot cause any deadlocks.
    if (sink_backend_->GetFormattingSettings().needs_formatting) {
      formatter_->Format(record, sink_backend_->GetFormattingSettings(), buffer);
    }
    {
      std::lock_guard guard(lock_);
      sink_backend_->Dispatch(buffer, record);
    }
  }

  NO_DISCARD ObjectWrapper<SinkBackend> getLockedBackend() override { return LockedSink(sink_backend_.get(), lock_); }

  NO_DISCARD std::shared_ptr<Sink> clone() const override {
    return std::make_shared<SynchronousSink>(sink_backend_->Clone());
  }

  //! \brief The mutex that controls access to the sink.
  mutable std::mutex lock_;
};

//! \brief  Create a new sink frontend / backend pair.
template <typename Backend_t, typename Frontend_t = SynchronousSink, typename... Args_t>
std::shared_ptr<Frontend_t> NewSink(Args_t &&... args) {
  return std::make_shared<Frontend_t>(std::make_unique<Backend_t>(std::forward<Args_t>(args)...));
}

//! \brief Object that can receive records from multiple loggers, and dispatches them to multiple sinks.
//!        The core has its own filter, which is checked before any of the individual sinks' filters.
class Core {
 public:
  bool WillAccept(const RecordAttributes &attributes) {
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
        sinks_.begin(),
        sinks_.end(),
        [attributes](auto &sink) { return sink->WillAccept(attributes); });
  }

  bool WillAccept(std::optional<Severity> severity) {
    if (!core_filter_.WillAccept(severity)) {
      return false;
    }
    // Check that at least one sink will accept.
    return std::any_of(
        sinks_.begin(),
        sinks_.end(),
        [severity](auto &sink) { return sink->WillAccept(severity); });
  }

  //! \brief Dispatch a ref bundle to the sinks.
  void Dispatch(const Record &record) const {
    for (auto &sink : sinks_) {
      if (sink->WillAccept(record.Attributes())) {
        sink->Dispatch(record);
      }
    }
  }

  //! \brief Add a sink to the core.
  Core &AddSink(std::shared_ptr<Sink> sink) {
    sinks_.emplace_back(std::move(sink));
    return *this;
  }

  //! \brief Get the number of sinks the core points at.
  NO_DISCARD std::size_t GetNumSinks() const { return sinks_.size(); }

  //! \brief Set the formatter for every sink the core points at.
  Core &SetAllFormatters(const formatting::BaseMessageFormatter &formatter) {
    for (const auto &sink : sinks_) {
      sink->SetFormatter(formatter.Copy());
    }
    return *this;
  }

  //! \brief Set the formatter for every sink the core points at.
  Core &SetAllFormatters(const std::unique_ptr<formatting::BaseMessageFormatter> &formatter) {
    return SetAllFormatters(*formatter);
  }

  //! \brief Get the core level filter.
  filter::AttributeFilter &GetFilter() { return core_filter_; }

  //! \brief Reset the core's filters.
  Core &ClearFilters() {
    core_filter_.Clear();
    return *this;
  }

  //! \brief Get the vector of all sinks.
  NO_DISCARD const std::vector<std::shared_ptr<Sink>> &GetSinks() const { return sinks_; }

  //! \brief Apply a function to all sinks.
  template <typename Func_t>
  Core &ApplyToAllSink(Func_t &&func) {
    std::for_each(sinks_.begin(), sinks_.end(), [f = std::forward<Func_t>(func)](auto &sink) { f(*sink); });
    return *this;
  }

  //! \brief Remove all sinks from the core.
  Core &ClearSinks() {
    sinks_.clear();
    return *this;
  }

  //! \brief Flush all sinks.
  //!
  //! Note: This function MAY discard.
  const Core &Flush() const {
    std::for_each(sinks_.begin(), sinks_.end(), [](auto &sink) { sink->Flush(); });
    return *this;
  }

  //! \brief Make a deep copy of the core, including deep copies of all sinks.
  NO_DISCARD std::shared_ptr<Core> Clone() const {
    auto core = std::make_shared<Core>();
    core->core_filter_ = core_filter_;
    for (const auto &sink : sinks_) {
      core->sinks_.emplace_back(sink->Clone());
    }
    return core;
  }

  //! \brief Get a locked handle to the core.
  NO_DISCARD LockedObject<Core> Lock() { return LockedObject<Core>(this, lock_); }

  bool IsLocked() const {
    if (lock_.try_lock()) {
      lock_.unlock();
      return false;
    }
    return true;
  }
 private:
  //! \brief All sinks the core will dispatch messages to.
  std::vector<std::shared_ptr<Sink>> sinks_;

  //! \brief Core-level attribute filters.
  filter::AttributeFilter core_filter_;

  //! \brief Mutex to control access to the core.
  mutable std::mutex lock_;
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
    core_ = nullptr; // One time use.
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
//!        to the logger's logging core.
class Logger {
 public:
  //! \brief Create a logger with a new core.
  Logger()
      : core_(std::make_shared<Core>()) {}

  //! \brief Create a logger without a logging core.
  //!
  //! Note that the constructor is *not* explicit on purpose, so parameters can be defaulted like
  //! `const Logger &logger = {NoCore};`.
  Logger(NoCore_t)
      : core_(nullptr) {}

  //! \brief Create a logger with a new core and a single sink.
  explicit Logger(const std::shared_ptr<Sink> &sink)
      : core_(std::make_shared<Core>()) {
    core_->AddSink(sink);
  }

  //! \brief Create a logger with the specified logging core.
  explicit Logger(std::shared_ptr<Core> core)
      : core_(std::move(core)) {}

  ~Logger() {
    Flush(); // Make sure all sinks that the logger is connected to is flushed.
  }

  template <typename... Attrs_t>
  RecordDispatcher Log(BasicAttributes basic_attributes = {}, Attrs_t &&... attrs) const {
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

  template <typename... Attrs_t>
  RecordDispatcher Log(std::optional<Severity> severity, Attrs_t &&... attrs) const {
    return Log(BasicAttributes(severity), attrs...);
  }

  //! \brief Create a record dispatcher, setting severity, file name, line number, and any other attributes.
  template <typename... Attrs_t>
  RecordDispatcher LogWithLocation(std::optional<Severity> severity,
                                   const char *file_name,
                                   const char *function_name,
                                   unsigned line_number,
                                   Attrs_t &&... attrs) const {
    return Log(BasicAttributes(severity, file_name, function_name, line_number), attrs...);
  }

  //! \brief Check whether the logger will accept a record with the specified severity (or lack of severity).
  NO_DISCARD bool WillAccept(std::optional<Severity> severity) const {
    if (!core_)
      return false;
    return core_->WillAccept(severity);
  }

  //! \brief Get the logger's core.
  NO_DISCARD const std::shared_ptr<Core> &GetCore() const { return core_; }

  //! \brief Check if the logger has a core.
  NO_DISCARD bool HasCore() const { return static_cast<bool>(core_); }

  //! \brief Replace the logger's current core with a new core.
  Logger &SetCore(std::shared_ptr<Core> core) {
    core_ = std::move(core);
    return *this;
  }

  //! \brief Set whether the logger should generate a time stamp for the logs.
  Logger &SetDoTimeStamp(bool do_timestamp) {
    do_time_stamp_ = do_timestamp;
    return *this;
  }

  //! \brief Set the name of the logger. This attribute is attached to every record.
  Logger &SetName(const std::string &logger_name) {
    logger_name_ = logger_name;
    return *this;
  }

  //! \brief Notify the core to flush all of its sinks.
  void Flush() const {
    if (core_) core_->Flush();
  }

  //! \brief Get all sinks that have the specified type of backend.
  template <typename SinkBackend_t>
  std::vector<std::pair<Sink *, SinkBackend_t *>> GetSinks() const {
    if (!HasCore()) {
      return {};
    }
    std::vector<std::pair<Sink *, SinkBackend_t *>> output;
    for (auto &sink : GetCore()->GetSinks()) {
      if (auto back_ptr = dynamic_cast<SinkBackend_t *>(&sink->GetBackend())) {
        output.emplace_back(sink.get(), back_ptr);
      }
    }
    return output;
  }

  //! \brief Map a function on every sink of the specified type that is in the logger's core.
  template <typename SinkBackend_t, typename Func_t>
  void MapOnSinks(Func_t &&func) const {
    auto &&sinks = GetSinks<SinkBackend_t>();
    for (auto &[frontend, backend] : sinks) {
      if (auto backend_ptr = dynamic_cast<SinkBackend_t *>(backend)) {
        func(*frontend, *backend_ptr);
      }
    }
  }

  //! \brief Set the formatters of all sinks in the logger's core.
  //!
  //! Warning: this may be ill-advised if many loggers share the same core.
  void SetAllFormats(const std::unique_ptr<formatting::BaseMessageFormatter> &formatter) const {
    if (auto core = GetCore()) {
      core->SetAllFormatters(*formatter);
    }
  }
  void SetAllFormats(const formatting::BaseMessageFormatter &formatter) const {
    if (auto core = GetCore()) {
      core->SetAllFormatters(formatter);
    }
  }

  //! \brief Create a clone of the logger. This not only copies the logger, but clones the core and all sinks.
  Logger Clone() {
    return Logger(*this).SetCore(core_->Clone());
  }

 private:
  //! \brief Whether to generate a time stamp for the logs.
  bool do_time_stamp_ = true;

  //! \brief A generator to create the DateTime timestamps for records.
  time::FastDateGenerator generator_;

  //! \brief A name for the logger.
  std::string logger_name_{};

  //! \brief The logger's core.
  std::shared_ptr<Core> core_;

  //! \brief Attributes associated with a specific logger.
  std::vector<Attribute> logger_attributes_;
};

//! \brief A sink, used for testing, that does nothing.
class EmptySink : public SinkBackend {
 public:
  EmptySink() { settings_.needs_formatting = false; }
  NO_DISCARD std::unique_ptr<SinkBackend> Clone() const override { return std::make_unique<EmptySink>(); }
 private:
  void dispatch(memory::BasicMemoryBuffer<char> &, const Record &) override {}
};

//! \brief A sink, used for testing, that formats a record, but does not stream the result anywhere.
//!
//! Primarily for timing and testing.
class TrivialDispatchSink : public SinkBackend {
 public:
  NO_DISCARD std::unique_ptr<SinkBackend> Clone() const override { return std::make_unique<TrivialDispatchSink>(); }
 private:
  void dispatch([[maybe_unused]] memory::BasicMemoryBuffer<char> &buffer,
                [[maybe_unused]] const Record &record) override {}
};

//! \brief A simple sink that writes to a file via an ofstream.
class FileSink : public SinkBackend {
 public:
  explicit FileSink(const std::string &file_path)
      : fout_(file_path), filename_(file_path) {}

  ~FileSink() override { fout_.flush(); }

  NO_DISCARD std::unique_ptr<SinkBackend> Clone() const override { return std::make_unique<FileSink>(filename_); }
 private:
  void dispatch([[maybe_unused]] memory::BasicMemoryBuffer<char> &buffer,
                [[maybe_unused]] const Record &record) override {
    if (!buffer.Empty()) {
      fout_.write(buffer.Data(), static_cast<std::streamsize>(buffer.Size()));
    }
  }

  void flush() override { fout_.flush(); }

  std::ofstream fout_;
  std::string filename_;
};

//! \brief A sink that writes to std::cout.
class StdoutSink : public SinkBackend {
 public:
  StdoutSink() {
    settings_.has_virtual_terminal_processing = true;
  }
  ~StdoutSink() override { std::cout.flush(); }

  NO_DISCARD std::unique_ptr<SinkBackend> Clone() const override { return std::make_unique<StdoutSink>(); }
 private:
  void dispatch([[maybe_unused]] memory::BasicMemoryBuffer<char> &buffer,
                [[maybe_unused]] const Record &record) override {
    if (!buffer.Empty()) {
      std::cout.write(buffer.Data(), static_cast<std::streamsize>(buffer.Size()));
    }
  }

  void flush() override { std::cout.flush(); }
};

//! \brief A sink that writes to an ostream.
//!
//! We require this ostream be a shared pointer so that we don't leave dangling references to locally created streams.
class OstreamSink : public SinkBackend {
 public:
  ~OstreamSink() override { out_->flush(); }

  explicit OstreamSink(std::shared_ptr<std::ostream> stream)
      : out_(std::move(stream)) {
    // By default, arbitrary streams are not assumed to support virtual terminal processing.
    settings_.has_virtual_terminal_processing = false;
  }

  NO_DISCARD std::unique_ptr<SinkBackend> Clone() const override { return std::make_unique<OstreamSink>(out_); }
  std::ostream& GetStream() { return *out_; }
 private:
  void dispatch([[maybe_unused]] memory::BasicMemoryBuffer<char> &buffer,
                [[maybe_unused]] const Record &record) override {
    if (!buffer.Empty()) {
      out_->write(buffer.Data(), static_cast<std::streamsize>(buffer.Size()));
    }
  }

  void flush() override { out_->flush(); }

  //! \brief A reference to the stream to write to.
  std::shared_ptr<std::ostream> out_;
};

// ==============================================================================
//  Global logger.
// ==============================================================================

//! \brief  The global interface for logging.
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
  static Logger &GetLogger() {
    if (!logger_) {
      logger_ = Logger(GetCore());
    }
    return *logger_;
  }

  //! \brief Flush all sinks associated with the global core.
  static void Flush() {
    auto &&core = GetCore();
    core->Flush();
  }

 private:
  inline static std::shared_ptr<Core> global_core_ = std::shared_ptr<Core>();
  inline static std::optional<Logger> logger_{};
};

// ==============================================================================
//  Logging macros.
// ==============================================================================

//! \brief Log with severity to a specific logger. First does a very fast check whether the
//!        message would be accepted given its severity level, since this is a very common case.
//!        If it will be, creates a handler, constructing the record in-place inside the handler.
#define LOG_SEV_TO(logger, severity) \
  if ((logger).WillAccept(::lightning::Severity::severity)) \
    if (auto handler = (logger).LogWithLocation(::lightning::Severity::severity, __FILE__, LL_CURRENT_FUNCTION, __LINE__)) \
      handler.GetRecord().Bundle()

//! \brief Log with a severity attribute to the global logger.
#define LOG_SEV(severity) LOG_SEV_TO(::lightning::Global::GetLogger(), severity)

//! \brief Log to a specific logger, without severity.
#define LOG_TO(logger) \
  if ((logger).WillAccept(::std::nullopt)) \
    if (auto handler = (logger).LogWithLocation(::std::nullopt, __FILE__, LL_CURRENT_FUNCTION, __LINE__)) \
      handler.GetRecord().Bundle()

//! \brief Log, without severity, to the global logger.
#define LOG() LOG_TO(::lightning::Global::GetLogger())

//! \brief Get a logging handler for a specific logger.
#define LOG_HANDLER_FOR(logger, severity) \
  (logger).LogWithLocation(::lightning::Severity::severity, __FILE__, LL_CURRENT_FUNCTION, __LINE__)

// ==============================================================================
//  Formatting functions.
// ==============================================================================

namespace formatting {
namespace detail {

inline std::pair<bool, std::string> getSpecialFormatter(std::string_view fmt) {
  static std::map<std::string_view, AnsiForegroundColor> color_map = {
      {"DEFAULT", AnsiForegroundColor::Default},
      {"RED", AnsiForegroundColor::Red},
      {"BRED", AnsiForegroundColor::BrightRed},
      {"GREEN", AnsiForegroundColor::Green},
      {"BGREEN", AnsiForegroundColor::BrightGreen},
      {"BLUE", AnsiForegroundColor::Blue},
      {"BBLUE", AnsiForegroundColor::BrightBlue},
      {"YELLOW", AnsiForegroundColor::Yellow},
      {"BYELLOW", AnsiForegroundColor::BrightYellow},
      {"CYAN", AnsiForegroundColor::Cyan},
      {"BCYAN", AnsiForegroundColor::BrightCyan},
      {"BLACK", AnsiForegroundColor::Black},
      {"BBLACK", AnsiForegroundColor::BrightBlack},
      {"WHITE", AnsiForegroundColor::White},
      {"BWHITE", AnsiForegroundColor::BrightWhite},
      {"MAGENTA", AnsiForegroundColor::Magenta},
      {"BMAGENTA", AnsiForegroundColor::BrightMagenta},
      {"RESET", AnsiForegroundColor::Reset},
  };

  if (auto it = color_map.find(fmt); it != color_map.end()) {
    return {true, SetAnsiColorFmt(it->second)};
  }
  return {false, ""};
}

inline void formatLiteralSegment(std::string_view segment, memory::BasicMemoryBuffer<char> &buffer) {
  for (auto c = segment.begin(); c < segment.end(); ++c) {
    switch (*c) {
      // Start of an escaped '{' or a special formatter.
      case '{':
        ++c;
        if (c == segment.end()) {
          buffer.PushBack('{');
          return;
        }
        else if (*c == '@') {
          // Get characters up to the next '}'
          auto start = c + 1;
          for (; *c != '}'; ++c) {
            // If this is not formatted as a special formatter, that is fine, just treat it as characters.
            if (c == segment.end()) {
              memory::AppendBuffer(buffer, "{@");
              memory::AppendBuffer(buffer,
                                   std::string_view{&(*start), static_cast<std::string_view::size_type>(c - start)});
              return;
            }
          }
          // Determine the special format string.
          std::string_view view(&(*start), static_cast<std::string_view::size_type>(c - start));
          auto [was_special, special_formatting] = getSpecialFormatter(view);
          if (was_special) {
            memory::AppendBuffer(buffer, special_formatting);
          }
          else {
            // Was not actually formatting!
            memory::AppendBuffer(buffer, "{@");
            memory::AppendBuffer(buffer, view);
            buffer.PushBack('}');
          }
        }
        else if (*c == '{') {
          buffer.PushBack(*c);

        }
        else {
          buffer.PushBack('{');
          buffer.PushBack(*c);
        }
        break;
      default:
        buffer.PushBack(*c);
    }
  }
}

//! \brief Helper function to write the I-th segment of a format string to the buffer, along with any string segment.
template <std::size_t I, typename... Args_t, std::size_t... Indices>
void formatHelper(memory::BasicMemoryBuffer<char> &buffer,
                  const char *const *starts,
                  const char *const *ends,
                  const std::string_view *const fmts,
                  std::size_t actual_substitutions,
                  const std::tuple<Segment<Args_t>...> &segments,
                  const FormattingSettings &settings,
                  MessageInfo &msg_info,
                  std::index_sequence<Indices...> is) {
  constexpr auto N = sizeof...(Indices);
  // Write formatting segment, then arg segment, then call formatHelper again.
  if (starts[I] < ends[I]) {
    formatLiteralSegment(std::string_view(starts[I], static_cast<std::string_view::size_type>(ends[I] - starts[I])),
                         buffer);
  }
  if (I == actual_substitutions) {
    return;
  }
  if constexpr (I < sizeof...(Args_t)) {
    auto &segment = std::get<I>(segments);
    segment.AddToBuffer(settings, msg_info, buffer, fmts[I]);
    msg_info.message_length += static_cast<unsigned>(buffer.Size());
  }
  if constexpr (I < N) {
    formatHelper < I + 1 > (buffer,
        starts,
        ends,
        fmts,
        actual_substitutions,
        segments,
        settings,
        msg_info,
        is);
  }
}

template <typename... Args_t>
void formatTo(memory::BasicMemoryBuffer<char> &buffer,
              const FormattingSettings &settings,
              std::string_view fmt,
              const Args_t &... args) {
  // Note that N != 0
  constexpr auto N = sizeof...(args);
  static_assert(N != 0, "N cannot be 0 in formatTo");

  const char *starts[N + 1], *ends[N + 1];
  std::string_view fmts[N];

  unsigned count_placed = 0;
  starts[0] = &fmt[0];
  auto c = &fmt[0];
  for (; *c != 0; ++c) {
    // Look out for formatting segments...
    if (*c == '{' && count_placed < N) {
      // ... but make sure it's not actually an escaped '{' or special formatter.
      if (*(c + 1) != 0 && (*(c + 1) == '{' || *(c + 1) == '@')) {
        ++c;
      }
      else {
        ends[count_placed++] = c;
        // Find the end of the format string.
        auto fmt_begin = c + 1;
        for (; *c != '}'; ++c) {
          LL_ASSERT(*c != 0, "unterminated format string");
        }
        fmts[count_placed - 1] = std::string_view(fmt_begin, static_cast<std::string_view::size_type>(c - fmt_begin));
        starts[count_placed] = c + 1;
      }
    }
  }
  ends[count_placed] = c; // Will be null the terminator.

  // If is not an error for the number of arguments to differ from the number of slots.
  auto segments =
      std::make_tuple(Segment<std::decay_t<typetraits::remove_cvref_t<Args_t>>>(args)...);
  MessageInfo msg_info;

  detail::formatHelper<0>(buffer,
                          starts,
                          ends,
                          fmts,
                          count_placed,
                          segments,
                          settings,
                          msg_info,
                          std::make_index_sequence<sizeof...(args)>{});
}
} // namespace detail

//! \brief Format data to a memory buffer. This is the main formatting function.
//!
//! This is all very similar to, but less powerful than, std::format or fmt lib.
template <typename... Args_t>
void FormatTo(memory::BasicMemoryBuffer<char> &buffer,
              const FormattingSettings &settings,
              std::string_view fmt,
              const Args_t &... args) {
  if constexpr (constexpr auto N = sizeof...(args); N == 0) {
    // If there are no arguments, just append the string to the buffer.
    detail::formatLiteralSegment(fmt, buffer);
  }
  else {
    detail::formatTo(buffer, settings, fmt, args...);
  }
}

//! \brief Format data to a string.
template <typename... Args_t>
std::string Format(const FormattingSettings &settings, std::string_view fmt, const Args_t &... args) {
  memory::StringMemoryBuffer buffer;
  FormatTo(buffer, settings, fmt, args...);
  return buffer.ToString();
}

//! \brief Format data to a string with default formatting settings.
template <typename... Args_t>
std::string Format(std::string_view fmt, const Args_t &... args) {
  FormattingSettings settings{};
  return Format(settings, fmt, args...);
}

} // namespace formatting

} // namespace lightning
