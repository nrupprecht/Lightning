# Lightning logging

A small but powerful header-only logging library.

Easy to use, easy to customize (that's the goal at least!).

Still a work in progress.

## Useage

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
