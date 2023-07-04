# Lightning logging

A small but powerful header-only logging library. Attempts to be a very fast streaming logging library without
sacrificing too much performance.

Easy to use, easy to customize, makes pretty logs (those are the goals, at least!).

Still a work in progress.

## Quick start

The simplest possible way to create a logger that writes to std::cout:
```C++
// Create a simple logger with no attributes and default formatting.
Logger logger(std::make_shared<OstreamSink>());

LOG_SEV_TO(logger, Info) << "Hello world";
```

Global logging can be set up by accessing the global logger and core,
```C++
// Add sink to the global logger, set formatting.
auto sink = std::make_shared<OstreamSink>();
Global::GetCore()->AddSink(sink)
  .SetAllFormatters(formatting::MakeMsgFormatter("[{}] [{}] {}",
                                                 formatting::SeverityAttributeFormatter{},
                                                 formatting::DateTimeAttributeFormatter{},
                                                 formatting::MSG));
```
Global logging is most easily used via the logging macros LOG() and LOG_SEV(severity),
```C++
LOG() << "Starting run...";

LOG_SEV(Info) << "Done with workflow. Processing next item with name " << item.GetName() << ".";
```

As usual, whatever is streamed using the logging macros (most importantly, function calls) will only be evaluated if the
logging record opens, which occurs if the set of core-level filters allow the message (given its attributes), and at
least one sink accepts the message (again, given its attributes). Lightning has very fast message rejection time, in my
benchmarks, if the message is rejected by its severity at the core level, it can reject around 1.5 billion messages a
second, and if the message is accepted by the core, but rejected by (the single) sink, it can reject around 800 - 900
million messages a second.

Streaming into record handlers (in particular, using the macros like **LOG_SEV**) allows for formatting customization
points, if you defined an ADL-findable *format_logstream(T&&, lightning::RefBundle&)* function, this will be used to
format your object. If the type does not have a format-logstream function, similar to std::format, Lightning relies on
formatting "segments," all deriving from the **BaseSegment** class to create a string representation of the logging
message. If a format_logstream function cannot be found for a type **T** being streamed, it will be checked if a
specialization of the template class **Segment** for the type. If this cannot be found, a **to_string** will be searched
for (via ADL). If this does not work and **T** has a ostreaming operator available, this will be used to format the
object into a string. If none of the aforementioned are available, streaming will fail at compile time.

The most powerful customization point of these are format_logstream functions. Since the function will be passes the
(mutable) record handler itself, this allows you to execute operations on the object to log and add dispatch-time
formatting whenever an object of type T is streamed into a RefBundle. Dispatch-time-formatting objects allow for
decisions about the formatting to be made based on sink-specific settings, at the time that the message is *dispatched*
to an actual sink (and is reevaluated for every sink). For example, the built in **AnsiColor8Bit** formatting type will
use ansi escape sequences to color test *if* the sink that is dispatching the message supports color. Another example is
the **NewLineIndent** formatting object, which knows how much to indent a message based on the width of the log "header"
so that the message aligns with the start of the message on the first line. The width of the log header depends on the
formatting for the specific sink, so it cannot be decided until dispatch time.

For example, writing a format logstream function for std::exception like this:
```C++
namespace std {

void format_logstream(const exception& ex, lightning::RefBundle& handler) {
  using namespace lightning::formatting;

  handler << NewLineIndent
          << AnsiColor8Bit(R"(""")", AnsiForegroundColor::Red)
          << AnsiColorSegment(AnsiForegroundColor::Yellow); // Exception in yellow.
  const char* begin = ex.what(), *end = ex.what();
  while (*end) {
    for (; *end && *end != '\n'; ++end); // Find next newline.
    handler << NewLineIndent << string_view(begin, end - begin);
    for (; *end && *end == '\n'; ++end); // Pass any number of newlines.
    begin = end;
  }
  handler << AnsiResetSegment
          << NewLineIndent // Reset colors to default.
          << AnsiColor8Bit(R"(""")", AnsiForegroundColor::Red);
}

} // namespace std
```
and throwing an exception like this
```C++
try {
    throw std::runtime_error("oh wow, this is a big problem\nand I don't know what to do");
}
catch (const std::exception& ex) {
    LOG_SEV(Fatal) << "Caught 'unexpected' exception: " << ex;
    return 0;
}
```
results in formatting that looks like this
![Alt text](./images/formatted-exception.png)
Note that this uses both color formatting and NewLineIndent formatting.

## Benchmarking

I have tried to take a "profiling driven" approach to this version of Lightning logging, aimed to keep it competitive or
faster than other commonly used logging frameworks such as spdlog and boost logging. At the present, I have mostly
focused on single threaded logging performance. The code containing these experiments and generating this table can be
found in *applications/profile-table-generation.cpp*, and there is more extensive profiling (that doesn't generate markdown tables)
in *applications/profile-greased-lightning.cpp*.

******************************************************************************************

Single threaded: 250,000 messages

| Experiment Name                            |Elapsed time (secs)     |Rate               |
|--------------------------------------------|:----------------------:|-------------------|
|MsgFormatter                                |0.083953                |2,977,872/sec      |
|std::ofstream, with message, static header  |0.039416                |6,342,555/sec      |
|EmptySink                                   |0.031420                |7,956,662/sec      |
|TrivialDispatchSink                         |0.062762                |3,983,278/sec      |

******************************************************************************************

Single threaded, Types: 250,000 messages

| Experiment Name                            |Elapsed time (secs)     |Rate               |
|--------------------------------------------|:----------------------:|-------------------|
|C-string                                    |0.082985                |3,012,592/sec      |
|Long C-string                               |0.127386                |1,962,546/sec      |
|Many C-strings                              |0.138126                |1,809,942/sec      |
|String                                      |0.102179                |2,446,683/sec      |
|Integer                                     |0.112620                |2,219,860/sec      |
|Many integers                               |0.209955                |1,190,728/sec      |
|Colored Integer                             |0.115283                |2,168,570/sec      |
|Bool                                        |0.146225                |1,709,696/sec      |
|Float                                       |0.131685                |1,898,472/sec      |
|Thread ID                                   |0.143803                |1,738,486/sec      |
|Combo                                       |0.220906                |1,131,704/sec      |
|Use-defined exception formatting            |0.387635                |644,937/sec        |

******************************************************************************************

Single threaded: 250,000 messages, non-acceptance

| Experiment Name                            |Elapsed time (secs)     |Rate               |
|--------------------------------------------|:----------------------:|-------------------|
|Non-accepting sink                          |0.000289                |866,049,808/sec    |
|Non-accepting core                          |0.000165                |1,516,686,585/sec  |

******************************************************************************************

Multi threaded (4 threads): 250,000 messages

| Experiment Name                            |Elapsed time (secs)     |Rate               |
|--------------------------------------------|:----------------------:|-------------------|
|One logger, multiple threads                |0.165496                |1,510,612/sec      |
|Multiple loggers, same sink                 |0.195100                |1,281,393/sec      |
