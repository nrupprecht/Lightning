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

#define LOG_SEV(arg) std::cout

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
void bench_nonacceptingseverity_sink(int howmany);
void bench_nonacceptingseverity_core(int howmany);

void bench_fastdatetimegen(int howmany);
void bench_datetimenow(int howmany);
void bench_systemclock(int howmany);
void bench_recordformatting(int howmany);

void bench_fmtdatetime(int howmany);

int main() {

  auto iters = 1'250'000;
  auto num_threads = 4;

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_st(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages, sink non-accepting\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_nonacceptingseverity_sink(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages, core non-accepting\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_nonacceptingseverity_core(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "Fast date generator\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_fastdatetimegen(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "DateTime::Now\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_datetimenow(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "System clock\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_systemclock(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "Record formatting\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_recordformatting(iters);
  LOG_SEV(Info) << "\n";


  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "DateTime formatting time comparison\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_fmtdatetime(iters);
  LOG_SEV(Info) << "\n";


  return 0;
}

void bench_st(int howmany) {
  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st.log");
  Logger logger(fs);

  // TODO:  Better way to format, including possibly fmt style composition of formatting.
  //        e.g. SetFormatting("[{}] [basic_st/backtrace-off] [{}] {}", DateTimeFormatter{}, SeverityAttributeFormatter{}, MSG{});
  //
  fs->GetFormatter()
      .ClearSegments()
      .AddLiteralSegment("[")
      .AddAttributeFormatter(std::make_shared<formatting::DateTimeAttributeFormatter>())
      .AddLiteralSegment("] [basic_st/backtrace-off] [")
      .AddAttributeFormatter(std::make_shared<formatting::SeverityAttributeFormatter>())
      .AddLiteralSegment("] ")
      .AddMsgSegment();

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  // LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec";
  std::cout << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_nonacceptingseverity_sink(int howmany) {
  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st_nonaccepting.log");
  fs->GetFilter().Accept({Severity::Error});
  Logger logger(fs);

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    LOG_SEV_TO(logger, Info) << "[{DateTime}] [basic_mt/backtrace-off] [{Severity}] " << "Hello logger: msg number " << i;
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_nonacceptingseverity_core(int howmany) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;

  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st_nonaccepting.log");
  Logger logger(fs);
  logger.GetCore()->GetFilter().Accept({Severity::Error});

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    LOG_SEV_TO(logger, Info) << "[{DateTime}] [basic_mt/backtrace-off] [{Severity}] " << "Hello logger: msg number " << i;
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  // LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec";
  std::cout << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_fastdatetimegen(int howmany) {
  time::DateTime dt;
  time::FastDateGenerator generator;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    dt = generator.CurrentTime();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << Format(dt) << " Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_datetimenow(int howmany) {
  time::DateTime dt;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    dt = time::DateTime::Now();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << Format(dt) << " Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_systemclock(int howmany) {
  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    auto current_time = std::chrono::system_clock::now();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_recordformatting(int howmany) {
  auto dt = time::DateTime::Now();
  time::FastDateGenerator generator;

  formatting::RecordFormatter record_formatter;
  record_formatter.ClearSegments();
  record_formatter.AddLiteralSegment("[");
  record_formatter.AddAttributeFormatter(std::make_shared<formatting::SeverityAttributeFormatter>());
  record_formatter.AddLiteralSegment("] ");
  record_formatter.AddMsgSegment();

  Record record;
  record.Attributes().basic_attributes.level = Severity::Info;
  record.Bundle() << "Hello, world!";

  FormattingSettings sink_settings;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    auto message = record_formatter.Format(record, sink_settings);
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << Format(dt) << " Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}
