//
// Created by Nathaniel Rupprecht on 6/6/23.
//


#include "Lightning/GreasedLightning.h"
#include <cmath>
#include <sstream>
#include <iostream>

using namespace std::string_literals;
using namespace lightning;

using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;

std::string Format(long long x) {
  auto digits = static_cast<int>(std::log10(x));
  auto periods = digits / 3;

  std::ostringstream stream;

  auto p = static_cast<int>(std::pow(10, 3 * periods));
  stream << x / p;
  x %= p;

  for (auto i = 1; i <= periods; ++i) {
    stream << ",";
    p = static_cast<int>(std::pow(10, 3 * (periods - i)));
    stream << std::setfill('0') << std::setw(3) << x / p;
    x %= p;
  }

  return stream.str();
}

std::string Format(const time::DateTime& x) {
  return lightning::formatting::Format(
      "[%-%-% %:%:%.%]", x.GetYear(), x.GetMonthInt(), x.GetDay(), x.GetHour(), x.GetMinute(), x.GetSecond(), x.GetMicrosecond());
}

void bench_st(int howmany);
void bench_st_types(int howmany);
void bench_nonacceptingseverity_sink(int howmany);
void bench_nonacceptingseverity_core(int howmany);

void bench_fastdatetimegen(int howmany);
void bench_datetimenow(int howmany);
void bench_systemclock(int howmany);
void bench_recordformatting(int howmany);

void bench_fmtdatetime(int howmany);

void bench_mt(int howmany, std::size_t thread_count);

int main() {
  // Set up global logger.
  auto sink = std::make_shared<OstreamSink>();
  Global::GetCore()->AddSink(sink);

  sink->SetFormatter(std::unique_ptr<formatting::BaseMessageFormatter>(
      new formatting::MsgFormatter("[{}] [{}] {}", formatting::SeverityAttributeFormatter{},
                                   formatting::DateTimeAttributeFormatter{},
                                   formatting::MSG)));

  auto iters = 250'000;
  auto num_threads = 4;

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages";
  LOG_SEV(Info) << "**************************************************************";
  bench_st(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "Single threaded, Types: " << Format(iters) << " messages";
  LOG_SEV(Info) << "**************************************************************";
  bench_st_types(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages, sink non-accepting";
  LOG_SEV(Info) << "**************************************************************";
  bench_nonacceptingseverity_sink(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages, core non-accepting";
  LOG_SEV(Info) << "**************************************************************";
  bench_nonacceptingseverity_core(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "Fast date generator";
  LOG_SEV(Info) << "**************************************************************";
  bench_fastdatetimegen(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "DateTime::Now";
  LOG_SEV(Info) << "**************************************************************";
  bench_datetimenow(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "System clock";
  LOG_SEV(Info) << "**************************************************************";
  bench_systemclock(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "Record formatting";
  LOG_SEV(Info) << "**************************************************************";
  bench_recordformatting(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "DateTime formatting time comparison";
  LOG_SEV(Info) << "**************************************************************";
  bench_fmtdatetime(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************";
  LOG_SEV(Info) << "Multi threaded (" << num_threads << " threads): " << Format(iters) << " messages";
  LOG_SEV(Info) << "**************************************************************";
  bench_mt(iters, num_threads);
  LOG_SEV(Info) << "\n";

  return 0;
}

void bench_st(int howmany) {
  {
    auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st-1.log");
    Logger logger(fs);
    auto formatter = std::make_unique<formatting::RecordFormatter>();
    formatter->ClearSegments()
        .AddLiteralSegment("[")
        .AddAttributeFormatter(std::make_shared<formatting::DateTimeAttributeFormatter>())
        .AddLiteralSegment("] [basic_st/backtrace-off] [")
        .AddAttributeFormatter(std::make_shared<formatting::SeverityAttributeFormatter>())
        .AddLiteralSegment("] ")
        .AddMsgSegment();
    fs->SetFormatter(std::move(formatter));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "RecordFormatter: Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  {
    auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st-2.log");
    Logger logger(fs);
    fs->SetFormatter(std::unique_ptr<formatting::BaseMessageFormatter>(
        new formatting::MsgFormatter("[{}] [basic_st/backtrace-off] [{}] {}",
                                     formatting::DateTimeAttributeFormatter{},
                                     formatting::SeverityAttributeFormatter{},
                                     formatting::MSG)));

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }
}

void bench_st_types(int howmany) {
  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st-types.log");
  Logger logger(fs);
  fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [basic_st/backtrace-off] [{}] {}",
                                                formatting::DateTimeAttributeFormatter{},
                                                formatting::SeverityAttributeFormatter{},
                                                formatting::MSG));

  // CString
  {
    auto start = high_resolution_clock::now();
    const char message[] = "Message";
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << message;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "C-string:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  // Long CString
  {
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Richard of york may have fought battle in vain, but do you know how many other famous characters have fought battle in "
                                  "vain? The answer may suprise you. The answer is 20.";
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Long C-string:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  // String
  {
    auto start = high_resolution_clock::now();
    std::string message = "Message";
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << message;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "String:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  // Integer
  {
    auto start = high_resolution_clock::now();
    int x = 32567734;
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << x;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Integer:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  // Float
  {
    auto start = high_resolution_clock::now();
    double x = 1.24525;
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data " << x;
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Double:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  // Combo
  {
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      LOG_SEV_TO(logger, Info) << "Hello logger: writing data to "
                               << i << " different sinks, done with "
                               << 100 * i / static_cast<double>(howmany)
                               << "% of messages.";
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Combo:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }
}

void bench_nonacceptingseverity_sink(int howmany) {
  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st_nonaccepting.log");
  fs->GetFilter().Accept({Severity::Error});
  Logger logger(fs);
  fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [basic_st/backtrace-off] [{}] {}",
                                                formatting::DateTimeAttributeFormatter{},
                                                formatting::SeverityAttributeFormatter{},
                                                formatting::MSG));

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    LOG_SEV_TO(logger, Info) << "[{DateTime}] [basic_mt/backtrace-off] [{Severity}] " << "Hello logger: msg number " << i;
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
}

void bench_nonacceptingseverity_core(int howmany) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;

  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st_nonaccepting.log");
  Logger logger(fs);
  logger.GetCore()->GetFilter().Accept({Severity::Error});
  fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [basic_st/backtrace-off] [{}] {}",
                                                formatting::DateTimeAttributeFormatter{},
                                                formatting::SeverityAttributeFormatter{},
                                                formatting::MSG));

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    LOG_SEV_TO(logger, Info) << "[{DateTime}] [basic_mt/backtrace-off] [{Severity}] " << "Hello logger: msg number " << i;
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
}

void bench_fastdatetimegen(int howmany) {
  time::DateTime dt;
  time::FastDateGenerator generator;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    dt = generator.CurrentTime();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
}

void bench_datetimenow(int howmany) {
  time::DateTime dt;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    dt = time::DateTime::Now();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
}

void bench_systemclock(int howmany) {
  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    [[maybe_unused]] auto current_time = std::chrono::system_clock::now();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
}

void bench_recordformatting(int howmany) {
  time::FastDateGenerator generator;

  Record record;
  record.Attributes().basic_attributes.level = Severity::Info;
  record.Bundle() << "Hello, world!";

  FormattingSettings sink_settings;

  {
    formatting::RecordFormatter record_formatter;
    record_formatter.ClearSegments()
        .AddLiteralSegment("[")
        .AddAttributeFormatter(std::make_shared<formatting::DateTimeAttributeFormatter>())
        .AddLiteralSegment("] [basic_st/backtrace-off] [")
        .AddAttributeFormatter(std::make_shared<formatting::SeverityAttributeFormatter>())
        .AddLiteralSegment("] ")
        .AddMsgSegment();

    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      auto message = record_formatter.Format(record, sink_settings);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "RecordFormatter:     Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  {
    formatting::MsgFormatter record_formatter("[{}] [basic_st/backtrace-off] [{}] {}",
                                              formatting::DateTimeAttributeFormatter{},
                                              formatting::SeverityAttributeFormatter{},
                                              formatting::MSG);
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      auto message = record_formatter.Format(record, sink_settings);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "MsgFormatter:        Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

  {
    auto record_formatter = std::unique_ptr<formatting::BaseMessageFormatter>(
        new formatting::MsgFormatter("[{}] [basic_st/backtrace-off] [{}] {}",
                                     formatting::DateTimeAttributeFormatter{},
                                     formatting::SeverityAttributeFormatter{},
                                     formatting::MSG));
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      auto message = record_formatter->Format(record, sink_settings);
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Ptr-to-MsgFormatter:    Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }

}

void bench_fmtdatetime(int howmany) {
  auto x = time::DateTime(2023, 1, 3, 12, 34, 12, 34'567);
  {
    FormattingSettings settings;
    auto start = high_resolution_clock::now();
    for (auto i = 0; i < howmany; ++i) {
      [[maybe_unused]] auto result = formatting::Format(settings, "%-%-% %:%:%.%",
                                                        x.GetYear(), x.GetMonthInt(), x.GetDay(),
                                                        x.GetHour(), x.GetMinute(), x.GetSecond(), x.GetMicrosecond());
    }
    auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
    LOG_SEV(Info) << "Lightning no-pad: Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  }
}

void bench_mt(int howmany, std::size_t thread_count) {
  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_mt.log");
  Logger logger(fs);
  fs->SetFormatter(formatting::MakeMsgFormatter("[{}] [basic_st/backtrace-off] [{}] {}",
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
  };

  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();
  LOG_SEV(Info) << "Lightning: Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
}