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
