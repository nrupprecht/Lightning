//
// Created by Nathaniel Rupprecht on 7/4/23.
//

#include <cmath>
#include <iomanip>
#include <iostream>

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
  const char *begin = ex.what(), *end = ex.what();
  while (*end) {
    for (; *end && *end != '\n'; ++end)
      ;  // Find next newline.
    handler << NewLineIndent << string_view(begin, static_cast<std::string::size_type>(end - begin));
    for (; *end && *end == '\n'; ++end)
      ;  // Pass any number of newlines.
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
void bench_mt(int howmany, std::size_t thread_count);

void AddHeader() {
  LOG_SEV(Info) << "| Experiment Name" << PadUntil(pad_width) << "|"
                << "Elapsed time (secs)" << PadUntil(pad_width + 25) << "|"
                << "Rate" << PadUntil(pad_width + 45) << "|";
  LOG_SEV(Info) << "|" << FillUntil(pad_width, '-') << "|:" << FillUntil(pad_width + 24, '-') << ":|"
                << FillUntil(pad_width + 45, '-') << "|";
}

void AddRow(const std::string& name, double delta_d, int howmany) {
  LOG_SEV(Info) << "|" << name << PadUntil(pad_width) << "|" << delta_d << PadUntil(pad_width + 25) << "|"
                << formatting::Format("{:L}/sec", static_cast<int>(howmany / delta_d))
                << PadUntil(pad_width + 45) << "|";
}

int main() {
  // Set up global logger.
  auto sink = NewSink<StdoutSink, UnlockedSink>();
  Global::GetCore()->AddSink(sink).SetAllFormatters(formatting::MakeMsgFormatter("{}", formatting::MSG));

  constexpr std::size_t iters = 250'000;
  constexpr std::size_t num_threads = 4;

  // ========================================================
  //  Profiling functions.
  // ========================================================

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info);
  LOG_SEV(Info) << "Single threaded: " << formatting::Format("{:L}", iters) << " messages";
  LOG_SEV(Info);
  AddHeader();
  bench_st(iters);
  LOG_SEV(Info);

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info);
  LOG_SEV(Info) << "Single threaded, Types: " << formatting::Format("{:L}", iters) << " messages";
  LOG_SEV(Info);
  AddHeader();
  bench_st_types(iters);
  LOG_SEV(Info);

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info);
  LOG_SEV(Info) << "Single threaded: " << formatting::Format("{:L}", iters) << " messages, non-acceptance";
  LOG_SEV(Info);
  AddHeader();
  bench_nonaccepting(iters);
  LOG_SEV(Info);

  LOG_SEV(Info) << RepeatChar(header_length, '*');
  LOG_SEV(Info);
  LOG_SEV(Info) << "Multi threaded (" << num_threads << " threads): " << formatting::Format("{:L}", iters)
                << " messages";
  LOG_SEV(Info);
  AddHeader();
  bench_mt(iters, num_threads);
  LOG_SEV(Info);

  return 0;
}

void bench_st(int howmany) {
  {  // Benchmark using MsgFormatter
    auto fs = UnlockedSink::From<FileSink>("logs/lightning_basic_st.log");
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter {},
                                      formatting::LoggerNameAttributeFormatter {},
                                      formatting::SeverityAttributeFormatter {},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

    AddRow("MsgFormatter", delta_d, howmany);
  }

  {  // Benchmark using just an ofstream
    std::ofstream fout("logs/lightning_basic_st-ofstream.log");

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      fout << "[2023-07-04 12:00:00.000000] [basic_st/backtrace-off] [Info   ] Hello logger: msg number " << i
           << "\n";
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("std::ofstream, with message, static header", delta_d, howmany);
  }

  {  // Benchmark using EmptySink
    auto fs = UnlockedSink::From<EmptySink>();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter {},
                                      formatting::LoggerNameAttributeFormatter {},
                                      formatting::SeverityAttributeFormatter {},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("EmptySink", delta_d, howmany);
  }

  {  // Benchmark using TrivialDispatchSink
    auto fs = UnlockedSink::From<TrivialDispatchSink>();
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter {},
                                      formatting::LoggerNameAttributeFormatter {},
                                      formatting::SeverityAttributeFormatter {},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("TrivialDispatchSink", delta_d, howmany);
  }
}

void bench_st_types(int howmany) {
  auto make_logger = []() {
    const auto fs = UnlockedSink::From<FileSink>("logs/lightning_basic_st-types.log");
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter {},
                                      formatting::LoggerNameAttributeFormatter {},
                                      formatting::SeverityAttributeFormatter {},
                                      formatting::MSG));
    return logger;
  };

  // CString
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    constexpr char message[] = "Message";
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << message;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("C-string", delta_d, howmany);
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
    AddRow("Long C-string", delta_d, howmany);
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
    AddRow("Many C-strings", delta_d, howmany);
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
    AddRow("String", delta_d, howmany);
  }

  // Integer
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("Integer", delta_d, howmany);
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
    AddRow("Many integers", delta_d, howmany);
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
    AddRow("Colored Integer", delta_d, howmany);
  }

  // Bool
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << (i % 2 == 0);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("Bool", delta_d, howmany);
  }

  // Double
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    constexpr double x = 1.24525;
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << x;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("Double", delta_d, howmany);
  }

  // Thread ID
  {
    auto logger = make_logger();
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << std::this_thread::get_id();
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("Thread ID", delta_d, howmany);
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
    AddRow("Combo", delta_d, howmany);
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
    AddRow("Use-defined exception formatting", delta_d, howmany);
  }
}

void bench_nonaccepting(int howmany) {
  {
    const auto fs = UnlockedSink::From<FileSink>("logs/lightning_basic_st_nonaccepting.log");
    fs->GetFilter().Accept({Severity::Error});
    Logger logger(fs);
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter {},
                                      formatting::LoggerNameAttributeFormatter {},
                                      formatting::SeverityAttributeFormatter {},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("Non-accepting sink", delta_d, howmany);
  }
  {
    const auto fs = UnlockedSink::From<FileSink>("logs/lightning_basic_st_nonaccepting.log");
    Logger logger(fs);
    logger.GetCore()->GetFilter().Accept({Severity::Error});
    logger.SetName("basic_st/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(MakeMsgFormatter("[{}] [{}] [{}] {}",
                                      formatting::DateTimeAttributeFormatter {},
                                      formatting::LoggerNameAttributeFormatter {},
                                      formatting::SeverityAttributeFormatter {},
                                      formatting::MSG));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("Non-accepting core", delta_d, howmany);
  }

  {
    const auto fs = NewSink<FileSink, UnlockedSink>("logs/lightning_basic_st_nocore.log");
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
    AddRow("No core", delta_d, howmany);
  }
}

void bench_mt(int howmany, std::size_t thread_count) {
  {
    const auto fs = UnlockedSink::From<FileSink>("logs/lightning_basic_mt.log");
    Logger logger(fs);
    logger.SetName("basic_mt/backtrace-off");
    logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
    fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [{}] [{}] {}",
                                                  formatting::DateTimeAttributeFormatter {},
                                                  formatting::LoggerNameAttributeFormatter {},
                                                  formatting::SeverityAttributeFormatter {},
                                                  formatting::MSG));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    const auto start = high_resolution_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
      threads.emplace_back([&howmany, &thread_count, &logger] {
        for (int j = 0; j < howmany / static_cast<int>(thread_count); ++j) {
          LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << j;
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    const auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("One logger, multiple threads", delta_d, howmany);
  }
  {
    auto fs = UnlockedSink::From<FileSink>("logs/lightning_basic_mt_multiple_logger.log");
    fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [{}] [{}] {}",
                                                  formatting::DateTimeAttributeFormatter {},
                                                  formatting::LoggerNameAttributeFormatter {},
                                                  formatting::SeverityAttributeFormatter {},
                                                  formatting::MSG));

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    const auto start = high_resolution_clock::now();
    for (size_t t = 0; t < thread_count; ++t) {
      threads.emplace_back([&fs, &howmany, &thread_count, t] {
        Logger logger(fs);
        logger.SetName("basic_mt/logger-" + std::to_string(t));
        logger.GetCore()->SetSynchronousMode(false); // We do not need synchronous mode.
        for (int j = 0; j < howmany / static_cast<int>(thread_count); ++j) {
          LOG_SEV_TO(logger, Info) << "Hello logger " << GetThreadID() << ": msg number " << j;
        }
      });
    }

    for (auto& t : threads) {
      t.join();
    }

    const auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    AddRow("Multiple loggers, same sink", delta_d, howmany);
  }
}
