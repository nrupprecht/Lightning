#include <iostream>
#include <fstream>
#include <string>
#include <optional>
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
template<typename T>
inline constexpr bool always_false_v = detail_always_false::always_false<T>::value;

}; // namespace typetraits.


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

class Attribute {
 protected:
  class Impl {
   public:
    explicit Impl(const std::string& name) : attr_name(name) {}
    virtual ~Impl() = default;
    //! \brief The name of the attribute.
    const std::string attr_name;
  };

  template<typename T> class ImplConcrete : public Impl {
   public:
    explicit ImplConcrete(const std::string& name) : Impl(name) {}
    virtual T GetAttribute() = 0;
  };

  std::shared_ptr<Impl> impl_ = nullptr;

 public:
  explicit Attribute(std::shared_ptr<Impl>&& impl) : impl_(std::move(impl)) {}

  NO_DISCARD const std::string& GetAttributeName() const { return impl_->attr_name; }

  template<typename T>
  NO_DISCARD std::optional<T> GetAttribute() const {
    if (auto ptr = dynamic_cast<ImplConcrete<T>*>(impl_.get())) {
      return ptr->GetAttribute();
    }
    return {};
  }
};


//! \brief  Base class for classes that take attributes and format them in some way, which is then used
//!         to construct a formatted logging message.
//!
class AttributeFormatter {
 protected:
  class Impl {
   public:
    explicit Impl(const std::string& name) : attr_name(name) {}
    virtual ~Impl() = default;
    //! \brief The name of the attribute this extractor formats.
    const std::string attr_name;
    NO_DISCARD virtual std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings& sink_settings) const = 0;
  };

  std::unique_ptr<Impl> impl_ = nullptr;
 public:
  explicit AttributeFormatter(std::unique_ptr<Impl>&& impl) : impl_(std::move(impl)) {}

  NO_DISCARD const std::string& GetAttributeName() const { return impl_->attr_name; }

  NO_DISCARD std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings& sink_settings) const {
    return impl_->FormatAttribute(attribute, sink_settings);
  }
};

}; // namespace attribute


// ==============================================================================
//  Formatting.
// ==============================================================================

namespace formatting { // namespace lightning::formatting

//! \brief  Create a type trait that determines if a 'format_logstream' function has been defined for a type.
NEW_TYPE_TRAIT(has_logstream_formatter_v, format_logstream(std::declval<Value_t>()));

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
class DispatchTimeFormatting {
 protected:
  class Impl {
   public:
    virtual void AddWithFormatting(std::string& message,
                                   const MessageInfo& message_info,
                                   const settings::SinkSettings& sink_settings) = 0;
  };

  //! \brief  Pointer to the implementation of the formatting object.
  std::shared_ptr<Impl> impl_;

 public:
  void AddWithFormatting(std::string& message,
                         const MessageInfo& message_info,
                         const settings::SinkSettings& sink_settings) const {
    impl_->AddWithFormatting(message, message_info, sink_settings);
  }
};


//! \brief  A message composed of several segments and dispatch-time formatting objects.
//!
class FormattedMessageSegments {
 public:
  //! \brief  Convenience method to add additional segments to a FormattedMessageSegments.
  //!
  template <typename T>
  FormattedMessageSegments& operator<<(const T& object) {
    if constexpr (std::is_base_of_v<FormattedMessageSegments, T>) {
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

class MessageFormatter {
 public:
  virtual std::string operator()(const Record& record,
                                 const settings::SinkSettings&,
                                 const std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>>& additional_formatters) = 0;
  void AddAttributeFormatter(attribute::AttributeFormatter&& formatter) { attribute_formatters_.emplace(formatter.GetAttributeName(), std::move(formatter)); }
 protected:
  NO_DISCARD const attribute::AttributeFormatter* getExtractor(const std::string& name) const {
    if (auto it = attribute_formatters_.find(name); it != attribute_formatters_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  std::map<std::string, attribute::AttributeFormatter> attribute_formatters_{};
};

//! \brief  Parameterized class of objects that represent the ability to have formatting controlled by accepting an object of type
//!         FmtObject_t. This allows us to have a general (template) SetFormatFrom(T) function on the SinkFrontend (below), which
//!         is able to set the format of the MessageFormatter if the message formatter accepts formatting from objects of this type.
//!
template<typename FmtObject_t>
struct FmtFrom {
  bool SetFormatFrom(const FmtObject_t& fmt) { setFormatFrom(fmt); return true; }
 private:
  virtual void setFormatFrom(const FmtObject_t&) = 0;
};


namespace detail_unconst {
template <typename T>
struct Unconst {
  using type = T;
};

template<typename T>
struct Unconst<const T> {
  using type = typename Unconst<T>::type;
};

template<typename T>
struct Unconst<const T*> {
  using type = typename Unconst<T>::type *;
};

template<typename T>
struct Unconst<T*> {
  using type = typename Unconst<T>::type *;
};
} // namespace detail_unconst

template <typename T>
using Unconst_t = typename detail_unconst::Unconst<T>::type;

template<typename T>
constexpr inline bool IsCstrRelated_v = std::is_same_v<Unconst_t<std::decay_t<T>>, char*> || std::is_same_v<std::remove_cvref_t<T>, std::string>;


//! \brief  Map any c-string-like type (char*, char[N], etc.) to std::string, everything else to its own type.
template<typename RawFormatType_t>
using FormatType_t = std::conditional_t<IsCstrRelated_v<RawFormatType_t>, std::string, std::remove_cvref<RawFormatType_t>>;

//! \brief Structure that represents part of a message coming from an attribute.
//!
struct Fmt {
  std::string attr_name;
  std::string attr_fmt{};
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
      fmt_segments.emplace_back(std::move(segment));
      segment.clear();

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

}// namespace formatting


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

  void AddAttributes(const std::vector<attribute::Attribute>& attributes) {
    for (auto& attr : attributes) {
      AddAttribute(attr);
    }
  }

  void AddAttribute(const attribute::Attribute& attribute) {
    attributes_.emplace(attribute.GetAttributeName(), attribute);
  }

  void SetAdditionalFormatters(std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>> formatters) {
    additional_formatters_ = std::move(formatters);
  }

 private:
  std::optional<formatting::FormattedMessageSegments> message_{};

  const class Core* core_ = nullptr;

  //! \brief Attribute formatters that, in general, come from the logger that created this record.
  //!
  std::weak_ptr<std::map<std::string, attribute::AttributeFormatter>> additional_formatters_;

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
      : record_(record)
      , initial_uncaught_exception_(std::uncaught_exceptions())
  {}

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
    }
  }

  template<typename T>
  RecordHandler& operator<<(const T& input) {

    // TODO Handle values.

    // TODO Handle attributes.

    if constexpr (std::is_base_of_v<formatting::DispatchTimeFormatting, T>) {
      ensureRecordHasMessage();
      *record_.message_ << input;
    }
    else if constexpr (formatting::has_logstream_formatter_v<T>) {
      format_logstream(input, *this);
    }
    else if constexpr (typetraits::is_ostreamable_v<T>) {
      ensureRecordHasMessage();
      *record_.message_ << input;
    }

    else if constexpr (typetraits::has_to_string_v<T>) {
      // Fall-back on a to_string method.
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

 private:
  void ensureRecordHasMessage() {
    if (!record_.message_) {
      record_.message_ = formatting::FormattedMessageSegments{};
    }
  }

  //! \brief  The record being constructed by the record handler.
  //!
  Record record_;

  //! \brief  The number of uncaught exceptions at the time that the record handler is created.
  //!
  int initial_uncaught_exception_{};
};


//! \brief  An object that is used to filter acceptance of record.
//!
//!         Implemented as a pImpl.
//!
class Filter {
 public:
  virtual ~Filter() = default;
  NO_DISCARD bool Check(const Record& record) const {
    return impl_->Check(record);
  }

 protected:
  class Impl {
   public:
    NO_DISCARD virtual bool Check(const Record& record) const = 0;
  };

  std::shared_ptr<Impl> impl_;
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
      : backend_(std::move(backend))
      , formatter_(std::make_shared<formatting::StandardMessageFormatter>())
  {}

  virtual ~SinkFrontend() = default;

  SinkFrontend& AddAttributeFormatter(attribute::AttributeFormatter&& formatter) {
    if (formatter_) {
      formatter_->AddAttributeFormatter(std::move(formatter));
    }
    return *this;
  }

  bool WillAccept(const Record& record) {
    for (auto& filter : sink_level_filters_) {
      if (!filter->Check(record)) {
        return false;
      }
    }
    return true;
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
    else if (auto segments = record.GetMessage()){
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

  const std::shared_ptr<formatting::MessageFormatter> GetFormatter() const {
    return formatter_;
  }

  template<typename FormatObj_t>
  bool SetFormatFrom(const FormatObj_t fmt_obj) {
    if constexpr (formatting::IsCstrRelated_v<FormatObj_t>) {
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

 protected:
  virtual void dispatch(const std::optional<std::string>& formatted_message) = 0;

  //! \brief  A protected helper function that SinkFrontend implements since it is a friend of SinkBackend.
  //!         Tells the backend to accept a message. This will only be called after the frontend has verified
  //!         that the record should be accepted.
  void sendToBackend(const std::optional<std::string>& formatted_message) {
    backend_->accept(formatted_message);
  }

  //! \brief  The backend of the sink.
  //!
  std::unique_ptr<SinkBackend> backend_;

  //! \brief  And filters that should be used to determine if records should be accepted by the sink.
  //!
  std::vector<std::unique_ptr<Filter>> sink_level_filters_;

  std::shared_ptr<formatting::MessageFormatter> formatter_ = nullptr;
};

//! \brief  Convenience function to create a full front/back sink, forwarding the arguments to the sink backend's constructor.
//!
template<typename Frontend_t, typename Backend_t, typename ...Args>
std::unique_ptr<Frontend_t> MakeSink(Args&& ...args) {
  return std::make_unique<Frontend_t>(std::make_unique<Backend_t>(std::forward<Args>(args)...));
}

//! \brief  Object that manages a set of sinks and maintains a set of general filters for messages.
//!
class Core {
 public:
  Core& AddSink(const std::shared_ptr<SinkFrontend>& sink) {
    sinks_.push_back(sink);
    return *this;
  }

  template<typename Sink_t, typename ...Args>
  // requires std::is_base_of_v<SinkFrontend, Sink_t>
  Core& AddSink(Args&& ...args) {
    sinks_.push_back(std::make_unique<Sink_t>(std::forward<Args>(args)...));
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
    for (auto& filter : core_level_filters_) {
      if (!filter.Check(record)) {
        return false;
      }
    }

    // At least one sink must accept the record for the core to accept the record.
    return std::any_of(sinks_.begin(), sinks_.end(), [record](auto& s) { return s->WillAccept(record); });
  }

  bool TryAccept(Record& record) const {
    if (WillAccept(record)) {
      record.core_ = this;
      return true;
    }
    return false;
  }

  void Dispatch(Record&& record) const {
    for (auto& sink : sinks_) {
      sink->TryDispatch(record);
    }
  }

 private:
  std::vector<Filter> core_level_filters_;

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
  Logger() : Logger(std::make_shared<Core>()) {}

  //! \brief  Create a logger with a specific logging core.
  //!
  explicit Logger(std::shared_ptr<Core> core)
      : core_(std::move(core)), attribute_formatters_(std::make_shared<std::map<std::string, attribute::AttributeFormatter>>()) {}

  //! \brief  Explicitly create the logger without a logging core.
  //!
  explicit Logger(NoCore_t) : Logger(nullptr) {}

  NO_DISCARD const std::shared_ptr<Core>& GetCore() const {
    return core_;
  }

  NO_DISCARD RecordHandler operator()() const {
    return RecordHandler(createRecord());
  }

  void AddNamedAttribute(const std::string& name, const attribute::Attribute& attribute) {
    logger_named_attributes_.emplace(name, attribute);
  }

  void AddLoggerAttributeFormatter(attribute::AttributeFormatter&& attribute_formatter) {
    attribute_formatters_->emplace(attribute_formatter.GetAttributeName(), std::move(attribute_formatter));
  }

  NO_DISCARD attribute::Attribute* GetNamedAttribute(const std::string& name) {
    if (auto it = logger_named_attributes_.find(name); it != logger_named_attributes_.end()) {
      return &it->second;
    }
    return nullptr;
  }

 protected:
  NO_DISCARD Record createRecord(const std::vector<attribute::Attribute>& attributes = {}) const {
    Record record{};

    if (!core_) {
      return record;
    }

    std::for_each(logger_named_attributes_.begin(), logger_named_attributes_.end(), [&](auto& pr) {
      record.AddAttribute(pr.second);
    });
    record.AddAttributes(attributes);
    record.SetAdditionalFormatters(attribute_formatters_);

    core_->TryAccept(record);
    return record;
  }

  //! \brief  The logging core for the logger.
  //!
  std::shared_ptr<Core> core_ = nullptr;

  std::map<std::string, attribute::Attribute> logger_named_attributes_;

  //! \brief  Attribute formatters are passed to all the sinks via the record. If a sink does not have an attribute formatter
  //!         that can handle an attribute, it tries to find one in the sink-specific attribute formatters.
  //!
  std::shared_ptr<std::map<std::string, attribute::AttributeFormatter>> attribute_formatters_{};
};

inline void Record::Dispatch() {
  core_->Dispatch(std::move(*this));
}

inline std::string formatting::StandardMessageFormatter::operator()(const Record& record,
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
          if (auto extractor = getExtractor(fmt.attr_name)) {
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

class ThreadAttribute : public attribute::Attribute {
 protected:
  class Impl : public Attribute::ImplConcrete<std::thread::id> {
   public:
    Impl() : ImplConcrete("Thread"), thread_id(std::this_thread::get_id()) {}
    std::thread::id GetAttribute() override { return thread_id; }
    std::thread::id thread_id;
  };
 public:
  ThreadAttribute() : Attribute(std::make_unique<Impl>()) {}
};

class ThreadAttributeFormatter : public attribute::AttributeFormatter {
 protected:
  class Impl : public AttributeFormatter::Impl {
   public:
    Impl() : AttributeFormatter::Impl("Thread") {}
    NO_DISCARD std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings& sink_settings) const override {
      if (auto thread_id = attribute.GetAttribute<std::thread::id>()) {
        std::ostringstream stream;
        stream << *thread_id; // to_string does not work on thread::id.
        return stream.str();
      }
      return "";
    }
  };
 public:
  ThreadAttributeFormatter() : AttributeFormatter(std::make_unique<Impl>()) {}
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
 private:
  class Impl : public Attribute::ImplConcrete<Severity> {
   public:
    explicit Impl(Severity severity) : ImplConcrete("Severity"), severity_(severity) {}
    Severity GetAttribute() override { return severity_; }
   private:
    Severity severity_;
  };
 public:
  explicit SeverityAttribute(Severity severity) : Attribute(std::make_shared<Impl>(severity)) {}
};

class SeverityFormatter final : public AttributeFormatter {
 private:
  class Impl : public AttributeFormatter::Impl {
   public:
    Impl() : AttributeFormatter::Impl("Severity") {}
    NO_DISCARD virtual std::string FormatAttribute(const Attribute& attribute, const settings::SinkSettings& sink_settings) const override {
      if (auto sev = attribute.GetAttribute<Severity>()) {
        switch (*sev) {
          case Severity::Debug:   return "Debug  ";
          case Severity::Info:    return "Info   ";
          case Severity::Warning: return "Warning";
          case Severity::Error:   return "Error  ";
          case Severity::Fatal:   return "Fatal  ";
        }
      }
      return "";
    }
  };
 public:
  SeverityFormatter() : AttributeFormatter(std::make_unique<Impl>()) {}
};

} // namespace attribute

class SeverityLogger : public Logger {
 public:
  SeverityLogger() : Logger() {
    AddLoggerAttributeFormatter(attribute::SeverityFormatter{});
  }

  RecordHandler operator()(Severity severity) {
    return RecordHandler(createRecord({attribute::SeverityAttribute(severity)}));
  }
};


// ==============================================================================
//  Specific SinkFrontend implementations.
// ==============================================================================

//! \brief  A basic frontend that does no synchronization.
//!
class SimpleFrontend : public SinkFrontend {
 public:
  explicit SimpleFrontend(std::unique_ptr<SinkBackend>&& backend)
      : SinkFrontend(std::move(backend))
  {}

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

class OstreamSink : public SinkBackend {
 public:
  explicit OstreamSink(std::ostream& stream = std::cout) : stream_(stream) {}

 private:
  void dispatch(const std::optional<std::string>& formatted_message) override {
    if (formatted_message) stream_ << *formatted_message;
  }

  std::ostream& stream_;
};


} // namespace lightning
