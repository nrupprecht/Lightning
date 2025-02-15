//
// Created by Nathaniel Rupprecht on 6/6/23.
//

#include <cmath>
#include <iomanip>
#include <iostream>
#include <filesystem>

#include "Lightning/Lightning.h"

using namespace std::string_literals;
using namespace lightning;

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

namespace std {

//! \brief Exception logstream formatter.
void format_logstream(const exception& ex, lightning::RefBundle& handler) {
  using namespace lightning::formatting;

  handler << NewLineIndent << AnsiColor8Bit(R"(""")", AnsiForegroundColor::Red)
          << AnsiColorSegment(AnsiForegroundColor::Yellow);  // Exception in yellow.
  const char* begin = ex.what(), * end = ex.what();
  while (*end) {
    for (; *end && *end != '\n'; ++end);  // Find next newline.
    handler << NewLineIndent << string_view(begin, static_cast<unsigned long>(end - begin));
    for (; *end && *end == '\n'; ++end);  // Pass any number of newlines.
    begin = end;
  }
  handler << AnsiResetSegment << NewLineIndent  // Reset colors to default.
          << AnsiColor8Bit(R"(""")", AnsiForegroundColor::Red);
}

}  // namespace std

constexpr unsigned pad_width = 45;
constexpr unsigned header_length = 90;

void bench_st(int howmany);

void bench_st_types(int howmany);

void bench_nonaccepting(int howmany);

void bench_datetime(int howmany);

void bench_recordformatting(int howmany);

void bench_segments(int howmany);

void bench_fmtdatetime(int howmany);

void bench_mt(int howmany, std::size_t thread_count);

auto main() -> int {
  // Set up global logger.
  Global::GetCore()->AddSink(NewSink<StdoutSink>())
      .SetAllFormatters(formatting::MakeMsgFormatter(
      "[{}] [{}] {}",
      formatting::SeverityAttributeFormatter{}.SeverityName(Severity::Info, "Info"),
      formatting::DateTimeAttributeFormatter{},
      formatting::MSG));

  constexpr auto iters = 250'000;
  constexpr auto num_threads = 10;

  LOG_SEV(Major) << AnsiColorSegment(formatting::AnsiForegroundColor::Yellow) << "Starting" << AnsiResetSegment
                 << " now.";
  LOG_SEV(Trace) << AnsiColor8Bit("Starting", formatting::AnsiForegroundColor::Yellow) << " now, again, "
                 << AnsiColor8Bit(12, formatting::AnsiForegroundColor::Blue) << "!\n";

  LOG_SEV(Info) << "Current path is " << std::filesystem::current_path();
  std::filesystem::create_directories("logs");

  // ========================================================
  //  Profiling functions.
  // ========================================================

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << formatting::Format("Single threaded: {:L} messages", iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_st(iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << formatting::Format("Single threaded, Types: {:L} messages", iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_st_types(iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << formatting::Format("Single threaded: {:L} messages, non-acceptance", iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_nonaccepting(iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << "Date/time generation";
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_datetime(iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << "Record formatting";
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_recordformatting(iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << "Segments";
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_segments(iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << "DateTime formatting time comparison";
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_fmtdatetime(iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info) << formatting::Format("Multi threaded ({}) threads): {:L} messages", num_threads, iters);
  LOG_SEV(Info) << RepeatChar(header_length, '*');
  bench_mt(iters, num_threads);
  LOG_SEV(Info) << RepeatChar(header_length, '*') << "\n";

  return 0;
}

void bench_st(int howmany) {
  std::size_t count = 0;

  auto make_sink = [&count]() {
    std::string file_name = formatting::Format("logs/lightning_basic_st-{}.log", count);
    return std::pair{NewSink<FileSink, UnlockedSink>(file_name), file_name};
  };

  {  // Benchmark using RecordFormatter
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    auto formatter = std::make_unique<formatting::RecordFormatter>();
    formatter->ClearSegments()
        .AddLiteralSegment("[")
        .AddAttributeFormatter(std::make_shared<formatting::DateTimeAttributeFormatter>())
        .AddLiteralSegment("] [")
        .AddAttributeFormatter(std::make_shared<formatting::LoggerNameAttributeFormatter>())
        .AddLiteralSegment("] [")
        .AddAttributeFormatter(std::make_shared<formatting::SeverityAttributeFormatter>())
        .AddLiteralSegment("] ")
        .AddMsgSegment();
    fs->SetFormatter(std::move(formatter));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "RecordFormatter:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using FormatterBySeverity
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    auto formatter = std::make_unique<formatting::FormatterBySeverity>();
    formatter->SetDefaultFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                                    formatting::DateTimeAttributeFormatter{},
                                                    formatting::LoggerNameAttributeFormatter{},
                                                    formatting::SeverityAttributeFormatter{},
                                                    formatting::MSG));
    fs->SetFormatter(std::move(formatter));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "FormatterBySeverity:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter
    ++count;
    auto [fs, file_name] = make_sink();
    fs->GetBackend().CreateFlushHandler<flush::AutoFlush>();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, auto flush:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter
    ++count;
    auto [fs, file_name] = make_sink();
    fs->GetBackend().CreateFlushHandler<flush::FlushEveryN>(10u);
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, flush every 10:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter and log the file name and line number
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}:{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::FileNameAttributeFormatter{false},
                                      formatting::FileLineAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, file and line:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter and log the file name and line number, compressing the file name
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}:{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::FileNameAttributeFormatter{true},
                                      formatting::FileLineAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, short file and line:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter and log the file name, function name, and line number
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}:{}] [{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::FileNameAttributeFormatter{false},
                                      formatting::FileLineAttributeFormatter{},
                                      formatting::FunctionNameAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, file, function, and line:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    ++count;
    auto fs =
        SynchronousSink::From<FileSink>("logs/lightning_basic_st-" + std::to_string(count)
                                        + ".log");
    Logger logger(fs);
    logger.SetName("synchronous-sink-logger");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [{}] [{}] {}",
                                                  formatting::DateTimeAttributeFormatter{},
                                                  formatting::LoggerNameAttributeFormatter{},
                                                  formatting::SeverityAttributeFormatter{},
                                                  formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Synchronous sink:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter, but no message.
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, no message:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter, without the message and without a header.
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, no msg, no header:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter, with the message, but without the header.
    ++count;
    auto [fs, file_name] = make_sink();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("{}", formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, with msg, no header:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using just an ofstream
    std::ofstream fout("logs/lightning_basic_st-ofstream.log");

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      fout << "[2023-07-04 12:00:00.000000] [basic_st/backtrace-off] [Info   ] Hello logger: msg number 0\n";
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "std::ofstream, with message, static header:" << PadUntil(pad_width)
                  << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using EmptySink
    auto fs = NewSink<EmptySink, UnlockedSink>();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "EmptySink:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using TrivialDispatchSink
    auto fs = NewSink<TrivialDispatchSink, UnlockedSink>();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "TrivialDispatchSink:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter, not really formatting.
    std::string file_name = "logs/lightning_basic_st-nonformatting.log";
    auto fs = NewSink<FileSink, UnlockedSink>(file_name);
    Logger logger(fs);
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter(
        "[2023-06-26 20:33:50.539002] [basic_st/backtrace-off] [Info   ] Hello logger: msg number {}",
        formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, not formatting:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {  // Benchmark using MsgFormatter, not really formatting.
    std::string file_name = "logs/lightning_basic_st-format-date.log";
    auto fs = NewSink<FileSink, UnlockedSink>(file_name);
    Logger logger(fs);
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [basic_st/backtrace-off] [Info   ] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter, format only Date:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}

void bench_st_types(int howmany) {
  auto make_logger = []() {
    auto fs = NewSink<FileSink, UnlockedSink>("logs/lightning_basic_st-types.log");
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));
    return logger;
  };

  // CString
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    const char message[] = "Message";
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << message;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "C-string:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Long CString
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Richard of york may have fought battle in vain, but do you know how many "
                                  "other famous characters have fought battle in "
                                  "vain? The answer may surprise you. The answer is 20.";
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Long C-string:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Many CStrings
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "1"
                               << "2"
                               << "3"
                               << "4"
                               << "5"
                               << "6"
                               << "7"
                               << "8"
                               << "9"
                               << "10"
                               << "11"
                               << "12"
                               << "13"
                               << "14"
                               << "15";
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Many C-strings:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // String
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    std::string message = "Message";
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << message;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "String:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Integer
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Integer:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Many integers
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << i << i + 1 << i + 2 << i + 3 << i + 4
                               << i + 5 << i + 6 << i + 7 << i + 8 << i + 9 << i + 10;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Many integers:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Colored Integer
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data "
                               << AnsiColor8Bit(i, formatting::AnsiForegroundColor::Blue);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Colored Integer:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Bool
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << (i % 2 == 0);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Bool:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Float
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    constexpr double x = 1.24525;
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << x;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Double:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Thread ID
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << std::this_thread::get_id();
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "ThreadID: " << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Combo
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data to " << i << " different sinks, done with "
                               << 100 * i / static_cast<double>(howmany) << "% of messages.";
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Combo:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  // Exception
  {
    auto logger = make_logger();
    std::runtime_error my_error("This is my error.\nIt is a big one!");
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << my_error;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Special exception formatting:" << PadUntil(pad_width)
                  << "Elapsed: " << PadUntil(pad_width) << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}

void bench_nonaccepting(int howmany) {
  {
    auto fs = NewSink<FileSink, UnlockedSink>("logs/lightning_basic_st_nonaccepting.log");
    fs->GetFilter().Accept({Severity::Error});
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

    LOG_SEV(Info) << "Nonaccepting sink:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
  {
    auto fs = NewSink<FileSink, UnlockedSink>("logs/lightning_basic_st_nonaccepting.log");
    Logger logger(fs);
    logger.GetCore()->GetFilter().Accept({Severity::Error});
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

    LOG_SEV(Info) << "Non-accepting core:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
  {
    auto fs = NewSink<FileSink, UnlockedSink>("logs/lightning_basic_st_nocore.log");
    Logger logger(NoCore);
    logger.SetName("basic_st/backtrace-off");
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter{},
                                      formatting::LoggerNameAttributeFormatter{},
                                      formatting::SeverityAttributeFormatter{},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

    LOG_SEV(Info) << "No core:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}

void bench_datetime(int howmany) {
  {
    time::DateTime dt;

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      dt = time::DateTime::Now();
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

    LOG_SEV(Info) << "DateTime::Now" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
  {
    time::FastDateGenerator generator;

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      [[maybe_unused]] auto dt = generator.CurrentTime();
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

    LOG_SEV(Info) << "Fast datetime generator:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
  {
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      [[maybe_unused]] auto current_time = std::chrono::system_clock::now();
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

    LOG_SEV(Info) << "SystemClock:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}

void bench_recordformatting(int howmany) {
  time::FastDateGenerator generator;

  Record record;
  record.Attributes().basic_attributes.level = Severity::Info;
  record.Attributes().basic_attributes.logger_name = "basic_st/backtrace-off";
  record.Bundle() << "Hello, world!";

  FormattingSettings sink_settings;

  {
    formatting::RecordFormatter record_formatter;
    record_formatter.ClearSegments()
        .AddLiteralSegment("[")
        .AddAttributeFormatter(std::make_shared<formatting::DateTimeAttributeFormatter>())
        .AddLiteralSegment("] [")
        .AddAttributeFormatter(std::make_shared<formatting::LoggerNameAttributeFormatter>())
        .AddLiteralSegment("] [")
        .AddAttributeFormatter(std::make_shared<formatting::SeverityAttributeFormatter>())
        .AddLiteralSegment("] ")
        .AddMsgSegment();

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      memory::MemoryBuffer<char> buffer;
      record_formatter.Format(record, sink_settings, buffer);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "RecordFormatter:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    formatting::MsgFormatter record_formatter("[{}] [{}] [{}] {}",
                                              formatting::DateTimeAttributeFormatter{},
                                              formatting::LoggerNameAttributeFormatter{},
                                              formatting::SeverityAttributeFormatter{},
                                              formatting::MSG);
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      memory::MemoryBuffer<char> buffer;
      record_formatter.Format(record, sink_settings, buffer);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    auto record_formatter = std::unique_ptr<formatting::BaseMessageFormatter>(
        new formatting::MsgFormatter("[{}] [{}] [{}] {}",
                                     formatting::DateTimeAttributeFormatter{},
                                     formatting::LoggerNameAttributeFormatter{},
                                     formatting::SeverityAttributeFormatter{},
                                     formatting::MSG));
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      memory::MemoryBuffer<char> buffer;
      record_formatter->Format(record, sink_settings, buffer);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Ptr-to-MsgFormatter:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}

void bench_segments(int howmany) {
  AnsiColorSegment info_colors{formatting::AnsiForegroundColor::Green};
  formatting::MessageInfo msg_info{};

  {
    formatting::SeverityAttributeFormatter severity_formatter;
    RecordAttributes attributes;
    FormattingSettings settings;
    attributes.basic_attributes.level = Severity::Warning;

    memory::MemoryBuffer<char> buffer;
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      buffer.Clear();
      severity_formatter.AddToBuffer(attributes, settings, msg_info, buffer);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Severity formatter:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    memory::MemoryBuffer<char> buffer;
    FormattingSettings settings;
    Segment<int> segment(4869244);
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      buffer.Clear();
      segment.AddToBuffer(settings, msg_info, buffer);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Segment<int>:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    memory::MemoryBuffer<char> buffer;
    FormattingSettings settings;
    Segment<int> segment(4869244);
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      buffer.Clear();
      segment.AddToBuffer(settings, msg_info, buffer, ":L");
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Segment<int> with commas:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    FormattingSettings settings;
    Segment<double> segment(1.2345);
    memory::MemoryBuffer<char> buffer;

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      buffer.Clear();
      segment.AddToBuffer(settings, msg_info, buffer);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Segment<double>:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    std::string buffer(7, ' ');
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      std::to_chars(&buffer[0], &buffer[0] + 7, 4869244);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "ToChars:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }

  {
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      std::string buffer(static_cast<unsigned long>(i % 15 + 50), ' ');
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Allocate string:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}

void bench_fmtdatetime(int howmany) {
  time::DateTime x(2023, 1, 3, 12, 34, 12, 34'567);

  {
    FormattingSettings settings;
    memory::MemoryBuffer<char> buffer;
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      buffer.Clear();
      formatting::FormatTo(buffer, settings,
                           "{}-{}-{} {}:{}:{}.{}",
                           x.GetYear(),
                           x.GetMonthInt(),
                           x.GetDay(),
                           x.GetHour(),
                           x.GetMinute(),
                           x.GetSecond(),
                           x.GetMicrosecond());
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Format date (FormatTo):" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}", static_cast<int>(howmany / delta_d)) << "/sec";
  }

  {
    FormattingSettings settings;
    auto start = high_resolution_clock::now();
    std::string buffer = "YYYY-mm-dd hh:mm:ss.uuuuuu";
    for (auto i = 0; i < howmany; ++i) {
      char* c = &buffer[0];
      formatting::FormatDateTo(c, c + buffer.size(), x);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Lightning FormatDateTo:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}

void bench_mt(int howmany, std::size_t thread_count) {
  {
    auto fs = SynchronousSink::From<FileSink>("logs/lightning_basic_mt.log");
    Logger logger(fs);
    logger.GetCore()->SetSynchronousMode(false); // We still do not need synchronous mode.
    fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [basic_mt/backtrace-off] [{}] {}",
                                                  formatting::DateTimeAttributeFormatter{},
                                                  formatting::SeverityAttributeFormatter{},
                                                  formatting::MSG));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    auto start = high_resolution_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
      threads.emplace_back([&]() {
        for (int j = 0; j < howmany / static_cast<int>(thread_count); ++j) {
          LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << j;
        }
      });
    }

    for (auto& t: threads) {
      t.join();
    }

    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Same logger:" << PadUntil(pad_width) << "Elapsed: " << delta_d << " secs "
                  << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
  {
    auto fs = SynchronousSink::From<FileSink>("logs/lightning_basic_mt_multiple_logger.log");
    fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [{}] [{}] {}",
                                                  formatting::DateTimeAttributeFormatter{},
                                                  formatting::LoggerNameAttributeFormatter{},
                                                  formatting::SeverityAttributeFormatter{},
                                                  formatting::MSG));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    auto start = high_resolution_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
      threads.emplace_back([&, t]() {
        Logger logger(fs);
        logger.SetName("basic_mt/logger-" + std::to_string(t));

        for (int j = 0; j < howmany / static_cast<int>(thread_count); ++j) {
          LOG_SEV_TO(logger, Info) << "Hello logger : msg number " << j;
        }
      });
    }

    for (auto& t: threads) {
      t.join();
    }

    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Multiple loggers, same sink:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
  {
    auto formatter = formatting::MakeMsgFormatter("[{}] [{}] [{}] {}",
                                                  formatting::DateTimeAttributeFormatter{},
                                                  formatting::LoggerNameAttributeFormatter{},
                                                  formatting::SeverityAttributeFormatter{},
                                                  formatting::MSG);

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    auto start = high_resolution_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
      threads.emplace_back([&, t]() {
        // Since each thread gets its own logger, we can use unlocked sinks.
        auto fs =
            UnlockedSink::From<FileSink>("logs/lightning_basic_mt_mt_" + std::to_string(t) + ".log");
        fs->SetFormatter(formatter->Copy());
        Logger logger(fs);
        logger.SetName("basic_mt/logger-" + std::to_string(t));

        for (int j = 0; j < howmany / static_cast<int>(thread_count); ++j) {
          LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << j;
        }
      });
    }

    for (auto& t: threads) {
      t.join();
    }

    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Multiple threads, multiple loggers:" << PadUntil(pad_width) << "Elapsed: " << delta_d
                  << " secs " << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d));
  }
}
