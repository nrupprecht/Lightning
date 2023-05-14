# Lightning logging

A small but powerful header-only logging library.

Easy to use, easy to customize (that's the goal at least!).

Still a work in progress.

## Quick start

The simplest possible way to create a logger that writes to std::cout:

    // Create a simple logger with no attributes and default formatting.
    Logger logger;
    logger.GetCore()->AddSink<UnsynchronizedFrontend, OstreamSink>();

    logger() << "Hello world";

A somewhat more complex example that uses a severity logger.

    // Create a simple logger with no attributes and default formatting.
    SeverityLogger logger;
    logger.GetCore()->
        AddSink<UnsynchronizedFrontend, OstreamSink>()
        .SetAllFormats("[{Severity}]: {Message}");
    
    logger(Severity::Info) << "Hi there, friend.";

Global logging can be set up by accessing the global logger and core,

    // Add sink to the global logger, set formatting.
    lightning::Global::GetCore()
      ->AddSink<UnsynchronizedFrontend, OstreamSink>()
      .SetAllFormats("[{Severity}][{DateTime}]: {Message}");

    // Add an attributes and formatters to the global logger.
    Global::GetLogger()
        .AddAttribute(attribute::DateTimeAttribute{}, attribute::DateTimeFormatter{})
        .AddLoggerAttributeFormatter(attribute::SeverityFormatter{});

Global logging is most easily used via the logging macros LOG() and LOG_SEV(severity),

    LOG() << "Starting run...";

    LOG_SEV(Info) << "Done with workflow. Processing next item with name " << item.GetName() << ".";

As usual, whatever is streamed into the logging macros (most importantly, function calls) will only be evaluated if the
logging record opens, which occurs if the set of core-level filters allow the message (given its attributes), and at
least one sink accepts the message (again, given its attributes).

Streaming into record handlers (including macros like LOG_SEV) will work with (in order of precedence) Attributes,
objects whose classes are children of the DispatchTimeFormatting class, objects that have an ADL-findable
format_logstream(T&&, RecordHandler&) function, objects that have a ostreaming operator, and objects that have an
ADL-findable to_string(T) function.

The most powerful customization point of these are format_logstream functions. Since the function will be passes
the  (mutable) record handler itself, this allows you to execute operations on the object to log and add dispatch-time
formatting whenever an object of type T is streamed into a RecordHandler. Dispatch-time-formatting objects allow for
decisions about the formatting to be made based on sink-specific settings, at the time that the message is *dispatched*
to an actual sink (and is reevaluated for every sink). For example, the built in AnsiColor8Bit formatting object will
use ansi escape sequences to color test *if* the sink that is dispatching the message supports color. Another example is
the NewLineIndent formatting object, which knows how much to indent a message based on the width of the log "header" so
that the message aligns with the start of the message on the first line. The width of the log header depends on the
formatting for the specific sink, so it cannot be decided until dispatch time.

For example, writing a format logstream function for std::exception like this:

[comment]: <> (![Alt text]&#40;./images/format-logstream-exception.png&#41;)
    namespace std {
    
    void format_logstream(const exception& ex, lightning::RecordHandler& handler) {
        using namespace lightning::formatting;
        
        handler << NewLineIndent << AnsiColor8Bit(R"(""")", AnsiForegroundColor::Red)
                << StartAnsiColor8Bit(AnsiForegroundColor::Yellow); // Exception in yellow.
        const char* begin = ex.what(), * end = ex.what();
        while (*end) {
            for (; *end && *end != '\n'; ++end); // Find next newline.
            handler << NewLineIndent << string_view(begin, end - begin);
            while (*end && *end == '\n') ++end;  // Pass any number of newlines.
            begin = end;
        }
        handler << ResetStreamColor() << NewLineIndent // Reset colors to default.
                << AnsiColor8Bit(R"(""")", AnsiForegroundColor::Red);
    }
    
    } // namespace std

and throwing an exception like this

    try {
        throw std::runtime_error("oh wow, this is a big problem\nand I don't know what to do");
    }
    catch (const std::exception& ex) {
        LOG_SEV(Fatal) << "Caught 'unexpected' exception: " << ex;
        return 0;
    }

results in formatting that looks like this
![Alt text](./images/formatted-exception.png)
Note that this uses both color formatting and NewLineIndent formatting.

# Overview

Lightning Logging is based on a source-core-sink model. Sources (loggers) are what receive input (logs), and are
responsible for generating a record and attaching source-specific attributes to a record. Cores are responsible for
receiving messages from loggers (there may be many loggers with the same core), doing core-level filtering of messages,
determining if any sink in a core will actually accept a record, and eventually dispatching records to sinks. Sinks are
responsible for receiving records and doing something useful with them, for example, actually logging the message
somewhere.

## Attributes

When a record is opened by a logger, the logger will generate and attach all logger-specific attributes to the record.
For example, a logger might attach to each record that it opens an attribute that represents the function that the
logger was used in, the level of indentation that should be used in formatting the logging message, the time at which
the record was created, the thread ID of the process that logged the message, etc. Note that attributes can be passed in
upon creation of a record and are also added, i.e. not every attribute that you want to attach to a message must be a
source-specific attribute.

Attributes are implemented as children of the lightning::attributes::Attributes class. Every attribute class has a name,
which allows the attribute to be identified and referenced by its name. Attribute classes must optionally provide a "
Generate" method that creates (potentially) a new copy of that type of attribute. This is important since if you add a "
DateTime" attribute to a logger, you want the DateTime to be current at the time that the record is created (the time at
with the logging occurs), not the time at which the DateTimeAttribute object was created and added to the class. If no
Generate method is provided, the object itself will be returned. This can be appropriate for truly immutable attributes,
but in general, it is good to implement the Generate function.

The "dual" class to Attributes is AttributeFormatters. These are classes that know how to format an instance of an
Attribute into a string. An AttributeFormatter knows the name of the Attribute class that it knows how to handle.
Specific sinks (specifically, SinkFrontends) can be given AttributeFormatters that will allow them to format attributes
in the records that they receive. However, since it can be cumbersome to add AttributeFormatters to every sink that
might need a specific type of formatter, there is another, often easier option. Since attributes often come from
specific loggers, you can also add AttributeFormatters to specific loggers. These "additional formatters" will be
available to any sink that tries to format the message from the record, and if the sink lacks the correctly named
attribute formatter, it will try to find it in the additional formatters. So for example, a severity logger has a
SeverityAttributeFormatter automatically, which avoids the need to add a SeverityAttributeFormatter to every sink that
might receive a message from a severity logger.

## Cores

Cores are managers, they possess a collection of sinks and multiplex messages from loggers into sinks that are willing
to receive records (based on some filtering criteria on the attributes of the records). Cores also can perform
core-level filtering [Work in progress]. When a record is opened by a logger, the newly opened record, with all its
attributes, but not yet with any message or values, is checked by the core to see if it will clear the core-level
filtering and if at least one sink will accept it. As is typical of logging libraries, the built-in logging macros (LOG
and LOG_SEV) are set up to only execute any streaming into a logger, and any computation that that might entail, if the
record can be opened. So if you have an expensive bit of logging like so:

    LOG_SEV(Debug) << "Number of penguins: " << calculateNumberOfPenguins(); // Very expensive function

and none of the sinks will accept a message of severity level "Debug," the function to calculate the number of penguins
will never be called.

## Sinks

TODO

### Sink Frontends

TODO

### Sink Backends

TODO
