#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <optional>
#include <utility>
#include <vector>
#include <type_traits>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>

namespace lightning {

// Forward declarations.
class Record;

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
  const typename Concrete_t::Impl* impl() const {
    return reinterpret_cast<const typename Concrete_t::Impl*>(impl_.get());
  }

  std::shared_ptr<Impl> impl_ = nullptr;

  explicit ImplBase(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}
};

// ==============================================================================
//  Settings and configurations.
// ==============================================================================

namespace settings { // namespace lightning::settings

//! \brief  Structure that stores information about how a sink (backend) is configured and what properties
//!         it has.
struct SinkSettings {
  bool has_color_support = false;
  bool include_newline = true;
};

} // namespace settings



// ==============================================================================
//  Attributes.
// ==============================================================================

namespace attribute { // namespace lightning::attribute

class Attribute : public ImplBase {
  friend class ImplBase; // So impl<>() function works.
 protected:
  class Impl : public ImplBase::Impl {
   public:
    explicit Impl(std::string  name) : attr_name(std::move(name)) {}
    virtual std::optional<Attribute> Generate() const = 0;
    //! \brief The name of the attribute.
    const std::string attr_name;
  };

  template <typename T>
  class ImplConcrete : public Impl {
   public:
    explicit ImplConcrete(const std::string& name) : Impl(name) {}
    virtual T GetAttribute() = 0;
  };

 public:
  explicit Attribute(std::shared_ptr<Impl>&& impl) : ImplBase(std::move(impl)) {}

  NO_DISCARD const std::string& GetAttributeName() const { return impl<Attribute>()->attr_name; }

  //! \brief  Some attributes, like timestamps, need to be generated every time an attribute is attached to
  //!         a record. Others might not. The impl decides if the attaching an attribute should generate a
  //!         new copy of the attribute, potentially with newly generated information inside it, or if
  //!         this attribute should just be used.
  NO_DISCARD Attribute Generate() const {
    if (auto attr = impl<Attribute>()->Generate()) {
      return *attr;
    }
    return *this;
  }

  template <typename T>
  NO_DISCARD std::optional<T> GetAttribute() const {
    if (auto ptr = dynamic_cast<ImplConcrete<T>*>(impl_.get())) {
      return ptr->GetAttribute();
    }
    return {};
  }
};

template <typename Concrete_t>
Concrete_t reinterpret_impl_cast(Attribute& attribute_type) {
  return *reinterpret_cast<Concrete_t*>(&attribute_type);
}

//! \brief  Base class for classes that take attributes and format them in some way, which is then used
//!         to construct a formatted logging message.
//!
class AttributeFormatter : public ImplBase {
  friend class ImplBase; // So impl<>() function works.
 protected:
  class Impl : public ImplBase::Impl {
   public:
    explicit Impl(std::string name) : attr_name(std::move(name)) {}
    //! \brief The name of the attribute this extractor formats.
    const std::string attr_name;
    NO_DISCARD virtual std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings& sink_settings) const = 0;
  };

 public:
  explicit AttributeFormatter(const std::shared_ptr<Impl>& impl) : ImplBase(impl) {}

  NO_DISCARD const std::string& GetAttributeName() const { return impl<AttributeFormatter>()->attr_name; }

  NO_DISCARD std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings& sink_settings) const {
    return impl<AttributeFormatter>()->FormatAttribute(attribute, sink_settings);
  }

};

template <typename Concrete_t>
Concrete_t reinterpret_impl_cast(AttributeFormatter& attribute_type) {
  return *reinterpret_cast<Concrete_t*>(&attribute_type);
}

}; // namespace attribute


// ==============================================================================
//  Formatting.
// ==============================================================================

namespace formatting { // namespace lightning::formatting

struct MessageInfo {
  //! \brief  The indentation of the start of the message within the formatted record.
  unsigned message_indentation = 0;

  // TODO: Pointers to the attributes and values vectors.
};

//! \brief  A way to format messages in sink-backend specific ways that delay the formatting choices until
//!         record-dispatch time.
//!
//!         This object is implemented as a pImpl.
//!
class DispatchTimeFormatting : public ImplBase {
  friend class ImplBase; // So impl<>() function works.
 protected:
  class Impl : public ImplBase::Impl {
   public:
    virtual void AddWithFormatting(std::string& message, const MessageInfo& message_info,
                                   const settings::SinkSettings& sink_settings) const = 0;
  };
 public:
  void AddWithFormatting(std::string& message, const MessageInfo& message_info,
                         const settings::SinkSettings& sink_settings) const {
    impl<DispatchTimeFormatting>()->AddWithFormatting(message, message_info, sink_settings);
  }
  explicit DispatchTimeFormatting(const std::shared_ptr<Impl>& impl) : ImplBase(impl) {}
};

//! \brief  A message composed of several segments and dispatch-time formatting objects.
//!
class FormattedMessageSegments {
 public:
  //! \brief  Convenience method to add additional segments to a FormattedMessageSegments.
  //!
  template <typename T>
  FormattedMessageSegments& operator<<(const T& object) {
    if constexpr (std::is_base_of_v<DispatchTimeFormatting, T>) {
      segments_.emplace_back(object);
    }
    else {
      // Just some other object to stream.
      std::ostringstream stream;
      stream << object;
      transferString(stream.str());
    }
    return *this;
  }

  //! \brief  Create a message from the segments.
  //!
  NO_DISCARD std::optional<std::string> MakeMessage(const MessageInfo& message_info,
                                                    const settings::SinkSettings& sink_settings) const {
    if (segments_.empty()) {
      return {};
    }

    std::string message;
    for (auto& segment: segments_) {
      if (segment.index() == 0) {
        std::get<0>(segment).AddWithFormatting(message, message_info, sink_settings);
      }
      else {
        message += std::get<1>(segment);
      }
    }
    return message;
  }

 private:
  void transferString(const std::string& str) {
    if (segments_.empty() || segments_.back().index() == 0) {
      // If there are no segments, or the last segment was a formatting segment, put a string segment on the vector.
      segments_.emplace_back(str);
    }
    else {
      // Add to the string segment.
      std::get<1>(segments_.back()) += str;
    }
  }

  //! \brief  The segments of the message, which are either formatting segments or ordinary string segments.
  std::vector<std::variant<DispatchTimeFormatting, std::string>> segments_{};
};

//! \brief  An object that can format a message from a record.
//!
class MessageFormatter {
 public:
  virtual ~MessageFormatter() = default;
  virtual std::string operator()(const Record& record,
                                 const settings::SinkSettings&,
                                 const std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>>& additional_formatters) = 0;
  void AddAttributeFormatter(attribute::AttributeFormatter&& formatter) { attribute_formatters_.emplace(formatter.GetAttributeName(), std::move(formatter)); }
  void AddAttributeFormatter(const attribute::AttributeFormatter& formatter) { attribute_formatters_.emplace(formatter.GetAttributeName(), formatter); }
  attribute::AttributeFormatter* GetAttributeFormatter(const std::string& formatter_name) {
    return getAttributeFormatter(formatter_name);
  }
 protected:
  NO_DISCARD const attribute::AttributeFormatter* getAttributeFormatter(const std::string& name) const {
    if (auto it = attribute_formatters_.find(name); it != attribute_formatters_.end()) return &it->second;
    return nullptr;
  }
  NO_DISCARD attribute::AttributeFormatter* getAttributeFormatter(const std::string& name) {
    if (auto it = attribute_formatters_.find(name); it != attribute_formatters_.end()) return &it->second;
    return nullptr;
  }

  std::map<std::string, attribute::AttributeFormatter> attribute_formatters_{};
};

//! \brief  Parameterized class of objects that represent the ability to have formatting controlled by accepting an object of type
//!         FmtObject_t. This allows us to have a general (template) SetFormatFrom(T) function on the SinkFrontend (below), which
//!         is able to set the format of the MessageFormatter if the message formatter accepts formatting from objects of this type.
//!
template <typename FmtObject_t>
struct FmtFrom {
  bool SetFormatFrom(const FmtObject_t& fmt) {
    setFormatFrom(fmt);
    return true;
  }
 private:
  virtual void setFormatFrom(const FmtObject_t&) = 0;
};

namespace detail_unconst {
template <typename T>
struct Unconst { using type = T; };
template <typename T>
struct Unconst<const T> { using type = typename Unconst<T>::type; };
template <typename T>
struct Unconst<const T*> { using type = typename Unconst<T>::type*; };
template <typename T>
struct Unconst<T*> { using type = typename Unconst<T>::type*; };
} // namespace detail_unconst

//! \brief  Remove const-ness on all levels of pointers.
//!
template <typename T>
using Unconst_t = typename detail_unconst::Unconst<T>::type;

template <typename T>
constexpr inline bool IsCstrRelated_v = std::is_same_v<Unconst_t<std::decay_t<T>>, char*> || std::is_same_v<std::remove_cvref_t<T>, std::string>;

//! \brief Structure that represents part of a message coming from an attribute.
//!
struct Fmt {
  std::string attr_name{}; // The name of the attribute that should be string-ized.
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

//! \brief  The default message formatter. Uses a format string and attribute extractors.
//!
class StandardMessageFormatter : public MessageFormatter, public FmtFrom<std::string> {
 public:
  explicit StandardMessageFormatter(const std::string& fmt_string = "{Message}") {
    SetFormatFrom(fmt_string);
  }

  std::string operator()(const Record& record,
                         const settings::SinkSettings& sink_settings,
                         const std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>>& additional_formatters) override;

 private:
  //! \brief  Specify how to format the message, based on formatting string.
  //!
  //!         This function calculates the formatting segments based on the formatting string.
  //!
  void setFormatFrom(const std::string& fmt_string) override {
    fmt_segments_ = Segmentize(fmt_string);
  }

  std::vector<std::variant<std::string, Fmt>> fmt_segments_;
};

// For a nice summary of ansi commands, see https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797.
// See also https://en.wikipedia.org/wiki/ANSI_escape_code.

enum class AnsiForegroundColor : short {
  Reset = 0,
  Default = 39,
  Black = 30, Red = 31, Green = 32, Yellow = 33, Blue = 34, Magenta = 35, Cyan = 36, White = 37,
  BrightBlack = 90, BrightRed = 91, BrightGreen = 92, BrightYellow = 93, BrightBlue = 94, BrightMagenta = 95, BrightCyan = 96, BrightWhite = 97
};

enum class AnsiBackgroundColor : short {
  Reset = 0,
  Default = 49,
  Black = 40, Red = 41, Green = 42, Yellow = 43, Blue = 44, Magenta = 45, Cyan = 46, White = 47,
  BrightBlack = 100, BrightRed = 101, BrightGreen = 102, BrightYellow = 103, BrightBlue = 104, BrightMagenta = 105, BrightCyan = 106, BrightWhite = 107
};

using Ansi256Color = unsigned char;

inline std::string StartAnsiColorFmt(std::optional<AnsiForegroundColor> foreground, std::optional<AnsiBackgroundColor> background = {}) {
  std::string fmt{};
  if (foreground) fmt += "\x1b[" + std::to_string(static_cast<short>(*foreground)) + "m";
  if (background) fmt += "\x1b[" + std::to_string(static_cast<short>(*background)) + "m";
  return fmt;
}

inline std::string ResetColors() { return StartAnsiColorFmt(AnsiForegroundColor::Default, AnsiBackgroundColor::Default); }

inline std::string StartAnsi256ColorFmt(std::optional<Ansi256Color> foreground_color_id, std::optional<Ansi256Color> background_color_id = {}) {
  std::string fmt{};
  if (foreground_color_id) fmt = "\x1b[38;5;" + std::to_string(*foreground_color_id) + "m";
  if (background_color_id) fmt += "\x1b[48;5;" + std::to_string(*background_color_id) + "m";
  return fmt;
}

inline std::string StartAnsiRGBColorFmt(Ansi256Color r, Ansi256Color g, Ansi256Color b) {
  return "\x1b[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

inline std::string PotentiallyAnsiColor(const std::string& message, bool do_color, std::optional<AnsiForegroundColor> foreground, std::optional<AnsiBackgroundColor> background = {}) {
  return do_color ? StartAnsiColorFmt(foreground, background) + message + ResetColors() : message;
}

inline std::string PotentiallyAnsiRGBColor(const std::string& message, bool do_color, Ansi256Color r, Ansi256Color g, Ansi256Color b) {
  return do_color ? StartAnsiRGBColorFmt(r, g, b) + message + ResetColors() : message;
}

//! \brief  A dispatch time formatting that colors a string using Ansi RGB colors (if the sink supports colors).
//!
class AnsiColor8Bit : public DispatchTimeFormatting {
  friend class ImplBase; // So impl<>() function works.
 protected:
  class Impl : public DispatchTimeFormatting::Impl {
   public:
    Impl(std::string msg, std::optional<AnsiForegroundColor> foreground_color, std::optional<AnsiBackgroundColor> background_color)
        : msg_(std::move(msg)), foreground_color_(foreground_color), background_color_(background_color) {}
    void AddWithFormatting(std::string& message, const MessageInfo&, const settings::SinkSettings& sink_settings) const override {
      message += PotentiallyAnsiColor(msg_, sink_settings.has_color_support, foreground_color_, background_color_);
    }
   private:
    const std::string msg_;
    std::optional<AnsiForegroundColor> foreground_color_;
    std::optional<AnsiBackgroundColor> background_color_;
  };
 public:
  AnsiColor8Bit(const std::string& msg, std::optional<AnsiForegroundColor> foreground_color, std::optional<AnsiBackgroundColor> background_color = {})
      : DispatchTimeFormatting(std::make_shared<Impl>(msg, foreground_color, background_color)) {}
};

//! \brief  A dispatch time formatting that colors a string using Ansi RGB colors (if the sink supports colors).
//!
class AnsiColorRGB : public DispatchTimeFormatting {
 protected:
  class Impl : public DispatchTimeFormatting::Impl {
   public:
    Impl(std::string msg, Ansi256Color r, Ansi256Color g, Ansi256Color b)
        : msg_(std::move(msg)), r_(r), g_(g), b_(b) {}
    void AddWithFormatting(std::string& message, const MessageInfo&, const settings::SinkSettings& sink_settings) const override {
      message += PotentiallyAnsiRGBColor(msg_, sink_settings.has_color_support, r_, g_, b_);
    }
   private:
    std::string msg_{};
    Ansi256Color r_{}, g_{}, b_{};
  };
 public:
  AnsiColorRGB(const std::string& msg, Ansi256Color r, Ansi256Color g, Ansi256Color b) : DispatchTimeFormatting(std::make_shared<Impl>(msg, r, g, b)) {}
};

//! \brief  A dispatch time formatting that adds a newline to the logging message, and then indents the line to match the start of the message.
//!
class NewLineIndent_t : public DispatchTimeFormatting {
 protected:
  class Impl : public DispatchTimeFormatting::Impl {
   public:
    void AddWithFormatting(std::string& message, const MessageInfo& message_info, const settings::SinkSettings&) const override {
      message += "\n" + std::string(message_info.message_indentation, ' ');
    }
  };
 public:
  NewLineIndent_t() noexcept : DispatchTimeFormatting(std::make_shared<Impl>()) {}
};

const inline NewLineIndent_t NewLineIndent{};

};// namespace formatting


// ==============================================================================
//  Values.
// ==============================================================================

namespace values { // namespace lightning::values

// TODO: Do we really need separate values and attributes?
class Value {
 public:
  const std::string name;

};

}; // namespace values


// ==============================================================================
//  General logging objects.
// ==============================================================================

//! \brief  Class that records a log message, attributes, values, and other properties of the message.
//!
class Record {
  friend class SinkFrontend;
  friend class RecordHandler;
  friend class Core;
 public:
  NO_DISCARD bool IsOpen() const {
    return core_ != nullptr;
  }

  void Close() {
    core_ = nullptr;
  }

  void Dispatch();

  NO_DISCARD const std::optional<formatting::FormattedMessageSegments>& GetMessage() const {
    return message_;
  }

  NO_DISCARD const attribute::Attribute* GetAttribute(const std::string& name) const {
    if (auto it = attributes_.find(name); it != attributes_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  Record& AddAttributes(const std::vector<attribute::Attribute>& attributes) {
    for (auto& attr: attributes) {
      AddAttribute(attr);
    }
    return *this;
  }

  Record& AddAttribute(const attribute::Attribute& attribute) {
    attributes_.emplace(attribute.GetAttributeName(), attribute);
    return *this;
  }

  Record& SetAdditionalFormatters(std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>> formatters) {
    additional_formatters_ = std::move(formatters);
    return *this;
  }

 private:
  std::optional<formatting::FormattedMessageSegments> message_{};

  const class Core* core_ = nullptr;

  //! \brief Attribute formatters that, in general, come from the logger that created this record.
  //!
  std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>> additional_formatters_{};

  // TODO: Add values.
  std::map<std::string, attribute::Attribute> attributes_{};

  //! \brief  If false, this is a request to not use the standard formatter during the dispatch of a record to
  //!         the sink backend.
  bool do_standard_formatting_ = true;
};

//! \brief  Class that knows how to construct a record via streaming data into it.
//!
class RecordHandler {
 public:
  explicit RecordHandler(Record&& record)
      : record_(record), initial_uncaught_exception_(std::uncaught_exceptions()) {}

  //! \brief  The record handler's destruction causes the record to be dispatched to its core.
  ~RecordHandler() {
    if (record_.IsOpen()) {
      // Make sure the RecordHandler is not being destructed because of stack unwinding. It is fine if the logging is occurring
      // *during* stack unwinding, we just want to avoid an exception during logging from causing a half-processed message from
      // being dispatched.
      if (std::uncaught_exceptions() != initial_uncaught_exception_) {
        return;
      }

      record_.Dispatch();
      record_.Close();
    }
  }

  NO_DISCARD bool IsOpen() const { return record_.IsOpen(); }

  template <typename T>
  RecordHandler& operator<<(const T& input);

 private:
  void ensureRecordHasMessage() {
    if (!record_.message_) record_.message_ = formatting::FormattedMessageSegments{};
  }

  //! \brief  The record being constructed by the record handler.
  //!
  Record record_;

  //! \brief  The number of uncaught exceptions at the time that the record handler is created.
  //!
  int initial_uncaught_exception_{};
};

namespace formatting { // namespace lightning::formatting

//! \brief  Create a type trait that determines if a 'format_logstream' function has been defined for a type.
NEW_TYPE_TRAIT(has_logstream_formatter_v, format_logstream(std::declval<Value_t>(), std::declval<RecordHandler&>()));

} // namespace formatting


template <typename T>
RecordHandler& RecordHandler::operator<<(const T& input) {
  // TODO Handle values.
  if constexpr (std::is_base_of_v<attribute::Attribute, T>) {
    // Add an attribute via streaming.
    record_.AddAttribute(input);
  }
  else if constexpr (std::is_base_of_v<formatting::DispatchTimeFormatting, T>) {
    ensureRecordHasMessage();
    *record_.message_ << input;
  }
  else if constexpr (formatting::has_logstream_formatter_v < T >) {
    // If a formatting function has been provided, use it.
    format_logstream(input, *this);
  }
  else if constexpr (typetraits::is_ostreamable_v<T>) {
    ensureRecordHasMessage();
    *record_.message_ << input;
  }
  else if constexpr (typetraits::has_to_string_v<T>) {
    // Fall-back on a to_string method.
    ensureRecordHasMessage();
    using std::to_string; // Allow for a customization point.
    *record_.message_ << to_string(input);
  }
  else {
    // Static fail.
    static_assert(typetraits::always_false_v<T>,
                  "No logging for this type, define a format_logstream or to_string function in the same namespace as this type.");
  }

  return *this;
}

//! \brief  An object that is used to filter acceptance of record.
//!
//!         Implemented as a pImpl.
//!
class Filter : public ImplBase {
  friend class ImplBase; // So impl<>() function works.
 public:
  virtual ~Filter() = default;
  NO_DISCARD bool Check(const Record& record) const {
    return impl<Filter>()->Check(record);
  }

 protected:
  class Impl : public ImplBase::Impl {
   public:
    NO_DISCARD virtual bool Check(const Record& record) const = 0;
  };
};

//! \brief  Base class for sink backends. These are the objects that are actually responsible for taking
//!         a record an doing something with its message and values.
class SinkBackend {
  friend class SinkFrontend;
 public:
  SinkBackend() : sink_settings_(std::make_unique<settings::SinkSettings>()) {}
  explicit SinkBackend(std::unique_ptr<settings::SinkSettings>&& settings) : sink_settings_(std::move(settings)) {}

  virtual ~SinkBackend() = default;

  NO_DISCARD const settings::SinkSettings& GetSettings() const { return *sink_settings_; }
  NO_DISCARD settings::SinkSettings& GetSettings() { return *sink_settings_; }

 private:
  void accept(const std::optional<std::string>& formatted_message) {
    // Any work that needs to be done pre-message-dispatch.
    preMessage();

    // TODO: Handle values.

    // Actually consume the message.
    dispatch(formatted_message);

    // Any work post-message-dispatch, e.g. flushing the sink.
    postMessage();
  }

  virtual void preMessage() {}
  virtual void dispatch(const std::optional<std::string>& formatted_message) = 0;
  virtual void postMessage() {}

  //! \brief  The settings for the sink. This is a pointer so we can inherit different settings for different sinks.
  //!
  std::unique_ptr<settings::SinkSettings> sink_settings_;
};

//! \brief  Base class for sink frontends. These are classes responsible for formatting messages, doing common
//!         filtering tasks, and feeding data to the sink backend, possible controlling things like thread
//!         synchronization.
class SinkFrontend {
 public:
  explicit SinkFrontend(std::unique_ptr<SinkBackend>&& backend)
      : backend_(std::move(backend)), formatter_(std::make_shared<formatting::StandardMessageFormatter>()) {}

  virtual ~SinkFrontend() = default;

  SinkFrontend& AddAttributeFormatter(attribute::AttributeFormatter&& formatter) {
    if (formatter_) {
      formatter_->AddAttributeFormatter(std::move(formatter));
    }
    return *this;
  }

  SinkFrontend& AddAttributeFormatter(const attribute::AttributeFormatter& formatter) {
    if (formatter_) {
      formatter_->AddAttributeFormatter(formatter);
    }
    return *this;
  }

  //! \brief  Look for an attribute formatter, returning a pointer to it if it exists, otherwise a nullptr.
  //!
  //!         If there is no formatter, a nullptr is returned in this case as well.
  //! \param formatter_name
  //! \return
  attribute::AttributeFormatter* GetAttributeFormatter(const std::string& formatter_name) {
    return formatter_->GetAttributeFormatter(formatter_name);
  }

  bool WillAccept(const Record& record) {
    return std::all_of(sink_level_filters_.begin(), sink_level_filters_.end(), [record](auto& filter) {
      return filter.Check(record);
    });
  }

  void TryDispatch(const Record& record) {
    if (!WillAccept(record)) {
      return;
    }

    // Format any message.
    std::optional<std::string> formatted_message;
    auto& settings = backend_->GetSettings();
    if (record.do_standard_formatting_ && formatter_) {
      formatted_message = (*formatter_)(record, settings, record.additional_formatters_);
    }
    else if (auto segments = record.GetMessage()) {
      formatted_message = (*segments).MakeMessage(formatting::MessageInfo{}, settings);
    }

    // Potentially add newline.
    if (formatted_message && settings.include_newline) {
      *formatted_message += "\n";
    }

    // Front-end specific dispatching.
    dispatch(formatted_message);
  }

  void SetFormatter(std::shared_ptr<formatting::MessageFormatter> formatter) {
    formatter_ = std::move(formatter);
  }

  NO_DISCARD const std::shared_ptr<formatting::MessageFormatter>& GetFormatter() const {
    return formatter_;
  }

  template <typename FormatObj_t>
  bool SetFormatFrom(const FormatObj_t fmt_obj) {
    if constexpr (formatting::IsCstrRelated_v < FormatObj_t >) {
      if (auto ptr = dynamic_cast<formatting::FmtFrom<std::string>*>(formatter_.get())) {
        ptr->SetFormatFrom(std::string(fmt_obj));
        return true;
      }
    }
    else {
      if (auto ptr = dynamic_cast<formatting::FmtFrom<FormatObj_t>*>(formatter_.get())) {
        ptr->SetFormatFrom(fmt_obj);
        return true;
      }
    }
    return false;
  }

  template <typename Backend_t>
  NO_DISCARD Backend_t* TryGetBackendAs() const {
    return dynamic_cast<Backend_t>(backend_.get());
  }

  NO_DISCARD const SinkBackend& GetBackend() const {
    return *backend_;
  }

 protected:
  virtual void dispatch(const std::optional<std::string>& formatted_message) = 0;

  //! \brief  A protected helper function that SinkFrontend implements since it is a friend of SinkBackend.
  //!         Tells the backend to accept a message. This will only be called after the frontend has verified
  //!         that the record should be accepted.
  void sendToBackend(const std::optional<std::string>& formatted_message) {
    backend_->accept(formatted_message);
  }

  //! \brief  The backend of the sink. Unique pointer, since each backend should be owned by exactly one frontend.
  //!
  std::unique_ptr<SinkBackend> backend_;

  //! \brief  And filters that should be used to determine if records should be accepted by the sink.
  //!
  std::vector<Filter> sink_level_filters_;

  //! \brief  The formatting object for a message.
  //!
  std::shared_ptr<formatting::MessageFormatter> formatter_ = nullptr;
};

//! \brief  Convenience function to create a full front/back sink, forwarding the arguments to the sink backend's constructor.
//!
template <typename Frontend_t, typename Backend_t, typename ...Args>
requires std::is_base_of_v<SinkFrontend, Frontend_t> && std::is_base_of_v<SinkBackend, Backend_t>
std::shared_ptr<Frontend_t> MakeSink(Args&& ...args) {
  return std::make_shared<Frontend_t>(std::make_unique<Backend_t>(std::forward<Args>(args)...));
}

//! \brief  Object that manages a set of sinks and maintains a set of general filters for messages.
//!
class Core {
 public:
  Core& AddSink(const std::shared_ptr<SinkFrontend>& sink) {
    sinks_.push_back(sink);
    return *this;
  }

  template <typename Frontend_t, typename Backend_t, typename ...Args>
  requires std::is_base_of_v<SinkFrontend, Frontend_t> && std::is_base_of_v<SinkBackend, Backend_t>
  Core& AddSink(Args&& ... args) {
    sinks_.push_back(MakeSink<Frontend_t, Backend_t>(std::forward<Args>(args)...));
    return *this;
  }

  Core& AddFilter(const Filter& filter) {
    core_level_filters_.push_back(filter);
    return *this;
  }

  std::vector<std::shared_ptr<SinkFrontend>>& GetSinks() {
    return sinks_;
  }

  NO_DISCARD const std::vector<std::shared_ptr<SinkFrontend>>& GetSinks() const {
    return sinks_;
  }

  NO_DISCARD bool WillAccept(const Record& record) const {
    // Make sure the record will not be filtered out by core-level filters.
    for (auto& filter: core_level_filters_) {
      if (!filter.Check(record)) return false;
    }

    // At least one sink must accept the record for the core to accept the record.
    return std::any_of(sinks_.begin(), sinks_.end(), [record](auto& s) { return s->WillAccept(record); });
  }

  Record& TryAccept(Record& record) const {
    record.core_ = WillAccept(record) ? this : nullptr;
    return record;
  }

  void Dispatch(Record&& record) const {
    std::for_each(sinks_.begin(), sinks_.end(), [record](auto sink) { sink->TryDispatch(record); });
  }

  //! \brief  Get the first sink backend of the specified type, if one exists. Otherwise, returns nullptr.
  //!
  template <typename Backend_t>
  Backend_t* GetFirstSink() const {
    for (auto& frontend: sinks_) {
      if (auto backend = frontend->TryGetBackendAs<Backend_t>()) {
        return backend;
      }
    }
    return nullptr;
  }

  template <typename Func_t>
  void ForEachBackend(const Func_t func) const {
    std::for_each(sinks_.begin(), sinks_.end(), [func](auto& sink) {
      func(*sink->GetBackend());
    });
  }

  template <typename Func_t>
  void ForEachFrontend(const Func_t func) const {
    std::for_each(sinks_.begin(), sinks_.end(), [func](auto& sink) {
      func(*sink);
    });
  }

  template <typename FormatObj_t>
  Core& SetAllFormats(const FormatObj_t& fmt_obj) {
    std::for_each(sinks_.begin(), sinks_.end(), [fmt_obj](auto& sink) { sink->SetFormatFrom(fmt_obj); });
    return *this;
  }

  Core& AddFormatterToAll(const attribute::AttributeFormatter& formatter) {
    std::for_each(sinks_.begin(), sinks_.end(), [&](auto& sink) { sink->AddAttributeFormatter(formatter); });
    return *this;
  }

 private:
  //! \brief  Filters that apply to any message routed through the core.
  //!
  std::vector<Filter> core_level_filters_;

  //! \brief  All the sinks that the core multiplexes.
  //!
  std::vector<std::shared_ptr<SinkFrontend>> sinks_;
};

//! \brief  A structure that is used to signal that an object should be created without a logging core.
//!
struct NoCore_t {};

//! \brief  The prototypical NoCore_t object.
//!
constexpr inline NoCore_t NoCore;

//! \brief  Base class for logging objects.
//!
class Logger {
 public:
  //! \brief  Create a logger with a new logging core.
  //!
  Logger() noexcept : Logger(std::make_shared<Core>()) {}

  //! \brief  Create a logger with a specific logging core.
  //!
  explicit Logger(std::shared_ptr<Core> core) noexcept
      : core_(std::move(core)), attribute_formatters_(std::make_shared<std::map<std::string, attribute::AttributeFormatter>>()) {}

  //! \brief  Create a logger and add some sinks to the loggers (newly created) core.
  //!
  explicit Logger(const std::vector<std::shared_ptr<SinkFrontend>>& sinks) noexcept
      : Logger() {
    for (auto& sink: sinks) {
      GetCore()->AddSink(sink);
    }
  }

  //! \brief  Explicitly create the logger without a logging core.
  //!
  explicit Logger(NoCore_t) noexcept : Logger(nullptr) {}

  NO_DISCARD const std::shared_ptr<Core>& GetCore() const {
    return core_;
  }

  NO_DISCARD RecordHandler operator()(const std::vector<attribute::Attribute>& attributes = {}) const {
    return RecordHandler(CreateRecord(attributes));
  }

  Logger& AddNamedAttribute(const std::string& name, const attribute::Attribute& attribute) {
    logger_named_attributes_.emplace(name, attribute);
    return *this;
  }

  Logger& AddAttribute(const attribute::Attribute& attribute) {
    logger_named_attributes_.emplace(attribute.GetAttributeName(), attribute);
    return *this;
  }

  Logger& AddLoggerAttributeFormatter(attribute::AttributeFormatter&& attribute_formatter) {
    attribute_formatters_->emplace(attribute_formatter.GetAttributeName(), std::move(attribute_formatter));
    return *this;
  }

  NO_DISCARD attribute::Attribute* GetNamedAttribute(const std::string& name) {
    if (auto it = logger_named_attributes_.find(name); it != logger_named_attributes_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  NO_DISCARD Record CreateRecord(const std::vector<attribute::Attribute>& attributes = {}) const {
    if (!core_) return {};

    Record record{};
    std::for_each(logger_named_attributes_.begin(), logger_named_attributes_.end(), [&](auto& pr) {
      record.AddAttribute(pr.second.Generate()); // Generate an attribute from every attribute of the logger.
    });
    record.AddAttributes(attributes).SetAdditionalFormatters(attribute_formatters_);
    return core_->TryAccept(record);
  }

 protected:
  //! \brief  The logging core for the logger.
  //!
  std::shared_ptr<Core> core_ = nullptr;

  //! \brief  Attributes that are attached to every message.
  //!
  std::map<std::string, attribute::Attribute> logger_named_attributes_;

  //! \brief  Attribute formatters are passed to all the sinks via the record. If a sink does not have an attribute formatter
  //!         that can handle an attribute, it tries to find one in the sink-specific attribute formatters.
  //!
  std::shared_ptr<std::map<std::string, attribute::AttributeFormatter>> attribute_formatters_{};
};

inline void Record::Dispatch() {
  core_->Dispatch(std::move(*this));
}

//! \brief Count the number of characters in a range that are not part of an Ansi escape sequence.
inline std::size_t CountNonAnsiSequenceCharacters(std::string::iterator begin, std::string::iterator end) {
  std::size_t count = 0;
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

inline std::string formatting::StandardMessageFormatter::operator()(
    const Record& record,
    const settings::SinkSettings& sink_settings,
    const std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>>& additional_formatters) {
  MessageInfo message_info{};

  std::string message{};
  for (const auto& fmt_seg: fmt_segments_) {
    if (fmt_seg.index() == 0) {
      // Just a literal segment of the format string.
      message += std::get<0>(fmt_seg);
    }
    else {
      // Format an attribute.
      auto& fmt = std::get<1>(fmt_seg);

      // Check if this is where the message should be printed.
      if (fmt.attr_name == "Message") {
        if (auto& msg = record.GetMessage()) {
          // Calculate message indentation, calculate it from the last newline.
          auto it = std::find(message.rbegin(), message.rend(), '\n');
          message_info.message_indentation = CountNonAnsiSequenceCharacters(it.base(), message.end());
          if (auto opt = (*msg).MakeMessage(message_info, sink_settings)) {
            message += *opt;
          }
        }
      }
      else {
        // Format some other attribute.
        if (auto attr = record.GetAttribute(fmt.attr_name)) {
          // NOTE/TODO?(Nate):  This could be further sped up by having a pointer to the extractor
          //                    in the segments instead of the name, which we then have to look up.
          //                    But we would have to recalculate the fmt segments whenever a new
          //                    extractor is added.
          if (auto extractor = getAttributeFormatter(fmt.attr_name)) {
            message += extractor->FormatAttribute(*attr, sink_settings);
          }
          else if (!additional_formatters.expired()) {
            auto additional = additional_formatters.lock();
            if (auto it = additional->find(fmt.attr_name); it != additional->end()) {
              message += it->second.FormatAttribute(*attr, sink_settings);
            }
          }
        }
      }
    }
  }
  return message;
}


// ==============================================================================
//  Specific Attribute implementations.
// ==============================================================================

namespace attribute { // namespace lightning::attribute

//! \brief  Attribute that stores the thread that a message came from.
//!
class ThreadAttribute : public attribute::Attribute {
  friend class ImplBase;
 protected:
  class Impl : public Attribute::ImplConcrete<std::thread::id> {
   public:
    Impl() : ImplConcrete("Thread"), thread_id(std::this_thread::get_id()) {}
    std::optional<Attribute> Generate() const override { return ThreadAttribute(); }
    std::thread::id GetAttribute() override { return thread_id; }
    std::thread::id thread_id;
  };
 public:
  ThreadAttribute() : Attribute(std::make_shared<Impl>()) {}
};

class ThreadAttributeFormatter : public attribute::AttributeFormatter {
  friend class ImplBase;
 protected:
  class Impl : public AttributeFormatter::Impl {
   public:
    Impl() : AttributeFormatter::Impl("Thread") {}
    NO_DISCARD std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings&) const override {
      if (auto thread_id = attribute.GetAttribute<std::thread::id>()) {
        std::ostringstream stream;
        stream << *thread_id; // to_string does not work on thread::id.
        return stream.str();
      }
      return "";
    }
  };
 public:
  ThreadAttributeFormatter() : AttributeFormatter(std::make_shared<Impl>()) {}
};

} // namespace attribute

// ==============================================================================
//  Specific logger implementations.
// ==============================================================================

enum class Severity {
  Debug, Info, Warning, Error, Fatal,
};

namespace attribute { // namespace lightning::attribute

class SeverityAttribute final : public Attribute {
  friend class ImplBase;
 private:
  class Impl : public Attribute::ImplConcrete<Severity> {
   public:
    explicit Impl(Severity severity) : ImplConcrete("Severity"), severity_(severity) {}
    Severity GetAttribute() override { return severity_; }
    std::optional<Attribute> Generate() const override { return SeverityAttribute(severity_); }
   private:
    Severity severity_;
  };
 public:
  explicit SeverityAttribute(Severity severity) : Attribute(std::make_shared<Impl>(severity)) {}
};

class SeverityFormatter final : public AttributeFormatter {
  friend class ImplBase;
 private:
  class Impl : public AttributeFormatter::Impl {
   public:
    Impl() : AttributeFormatter::Impl("Severity") {}
    NO_DISCARD std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings& sink_settings) const override {
      if (auto sev = attribute.GetAttribute<Severity>()) {
        switch (*sev) {
          case Severity::Debug:   return formatting::PotentiallyAnsiColor("Debug  ", sink_settings.has_color_support, formatting::AnsiForegroundColor::BrightBlack);
          case Severity::Info:    return formatting::PotentiallyAnsiColor("Info   ", sink_settings.has_color_support, formatting::AnsiForegroundColor::Green);
          case Severity::Warning: return formatting::PotentiallyAnsiColor("Warning", sink_settings.has_color_support, formatting::AnsiForegroundColor::Yellow);
          case Severity::Error:   return formatting::PotentiallyAnsiColor("Error  ", sink_settings.has_color_support, formatting::AnsiForegroundColor::Red);
          case Severity::Fatal:   return formatting::PotentiallyAnsiColor("Fatal  ", sink_settings.has_color_support, formatting::AnsiForegroundColor::BrightRed);
        }
      }
      return "";
    }
  };
 public:
  SeverityFormatter() : AttributeFormatter(std::make_shared<Impl>()) {}
};

//! \brief  Attribute that indicates a level of indentation.
//!
class BlockLevelAttribute : public Attribute {
  friend class ImplBase;
 private:
  class Impl : public Attribute::ImplConcrete<unsigned> {
   public:
    explicit Impl(unsigned level) : ImplConcrete("BlockLevel"), level_(level) {}
    unsigned GetAttribute() override { return level_; }
    void SetIndentation(unsigned level) { level_ = level; }
    void IncrementIndentation() { ++level_; }
    void DecrementIndentation() { if (0 < level_) --level_; }
    std::optional<Attribute> Generate() const override { return BlockLevelAttribute(level_); }
   private:
    unsigned level_;
  };
 public:
  explicit BlockLevelAttribute(unsigned level = 0) : Attribute(std::make_shared<Impl>(level)) {}
  void SetIndentation(unsigned level) { impl<BlockLevelAttribute>()->SetIndentation(level); }
  void IncrementIndentation() { impl<BlockLevelAttribute>()->IncrementIndentation(); }
  void DecrementIndentation() { impl<BlockLevelAttribute>()->DecrementIndentation(); }
};

//! \brief  Attribute formatter that reads the block level and injects spaces based on the BlockLevelAttribute attribute.
//!
class BlockIndentationFormatter final : public AttributeFormatter {
  friend class ImplBase;
 private:
  class Impl : public AttributeFormatter::Impl {
   public:
    explicit Impl(unsigned spaces_per_level) : AttributeFormatter::Impl("BlockLevel"), spaces_per_level_(spaces_per_level) {}
    NO_DISCARD std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings&) const override {
      return std::string(spaces_per_level_ * *attribute.GetAttribute<unsigned>(), ' ');
    }
   private:
    unsigned spaces_per_level_;
  };
 public:
  explicit BlockIndentationFormatter(unsigned spaces_per_level = 2) : AttributeFormatter(std::make_shared<Impl>(spaces_per_level)) {}
};

} // namespace attribute

namespace controllers { // namespace lightning::controllers

//! \brief  RAII wrapper that increments a block attribute upon creation, and decrements it upon destruction.
//!
class BlockLevel {
 public:
  explicit BlockLevel(Logger& logger) {
    if (auto attr = logger.GetNamedAttribute("BlockLevel")) {
      block_attribute_ = attribute::reinterpret_impl_cast<attribute::BlockLevelAttribute>(*attr);
      block_attribute_->IncrementIndentation();
    }
  }
  ~BlockLevel() {
    if (block_attribute_) block_attribute_->DecrementIndentation();
  }

 private:
  std::optional<attribute::BlockLevelAttribute> block_attribute_;
};

} // namespace controllers

class SeverityLogger : public Logger {
 public:
  SeverityLogger() : Logger() {
    AddLoggerAttributeFormatter(attribute::SeverityFormatter{});
  }

  explicit SeverityLogger(const std::vector<std::shared_ptr<SinkFrontend>>& sinks) : Logger(sinks) {
    AddLoggerAttributeFormatter(attribute::SeverityFormatter{});
  }

  RecordHandler operator()(Severity severity) {
    return RecordHandler(CreateRecord({attribute::SeverityAttribute(severity)}));
  }
};


// ==============================================================================
//  Specific SinkFrontend implementations.
// ==============================================================================

//! \brief  A basic frontend that does no synchronization.
//!
class UnsynchronizedFrontend : public SinkFrontend {
 public:
  explicit UnsynchronizedFrontend(std::unique_ptr<SinkBackend>&& backend)
      : SinkFrontend(std::move(backend)) {}

 private:
  void dispatch(const std::optional<std::string>& formatted_message) override {
    // Just send the message to the backend.
    sendToBackend(formatted_message);
  }
};

//! \brief  A frontend that uses a mutex to feed messages one at a time to the backend.
//!
class SynchronizedFrontend : public SinkFrontend {
 public:
  explicit SynchronizedFrontend(std::unique_ptr<SinkBackend>&& backend) : SinkFrontend(std::move(backend)) {}

 private:
  void dispatch(const std::optional<std::string>& formatted_message) override {
    std::lock_guard guard(dispatch_mutex_);
    sendToBackend(formatted_message);
  }

  std::mutex dispatch_mutex_;
};


// ==============================================================================
//  Specific SinkBackend implementations.
// ==============================================================================

//! \brief  Sink backend that writes output to an ostream.
//!
class OstreamSink : public SinkBackend {
 public:
  explicit OstreamSink(std::ostringstream& stream) : stream_(stream) {
    GetSettings().has_color_support = false;
  }

  explicit OstreamSink(std::ostream& stream = std::cout) : stream_(stream) {
    GetSettings().has_color_support = true; // Default this to true... TODO: Detect?
  }

 private:
  void dispatch(const std::optional<std::string>& formatted_message) override {
    if (formatted_message) stream_ << *formatted_message;
  }

  std::ostream& stream_;
};


// ==============================================================================
//  Global logger.
// ==============================================================================

//! \brief  The global interface for logging.
//!
class Global {
 public:
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
  inline static std::optional<Logger> logger_{};
};

#define LOG() \
  if (auto record = ::lightning::Global::GetLogger().CreateRecord(); record.IsOpen()) \
    ::lightning::RecordHandler(std::move(record))

#define LOG_SEV(severity) \
  if (auto record = ::lightning::Global::GetLogger().CreateRecord({ \
    ::lightning::attribute::SeverityAttribute(::lightning::Severity::severity) \
  }); record.IsOpen()) \
    ::lightning::RecordHandler(std::move(record))

} // namespace lightning
