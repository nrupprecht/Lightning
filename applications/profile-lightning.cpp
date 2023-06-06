//
// Created by Nathaniel Rupprecht on 6/1/23.
//

#include "Lightning/Lightning.h"
#include <cmath>
#include <format>

using namespace lightning;

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
    stream << std::setfill('0') << std::setw(3)<< x / p;
    x %= p;
  }

  return stream.str();
}

void bench(int howmany, Logger& logger);
void bench_mt(int howmany, Logger& logger, size_t thread_count);

int main() {
  using namespace lightning;
  using namespace attribute;
  using namespace formatting;

  Global::GetCore()
      ->AddSink<UnsynchronizedFrontend, OstreamSink>()
      .SetAllFormats("[{Severity}] {Message}");

  Global::GetLogger()
      .AddAttribute(DateTimeAttribute{}, DateTimeFormatter{})
      .AddLoggerAttributeFormatter(SeverityFormatter{});

  auto iters = 250'000;
  auto num_threads = 4;
  {
    auto sink = MakeSink<UnsynchronizedFrontend, FileSink>("logs/lightning_basic_st.log");
    sink->SetFormatFrom("[{DateTime}] [basic_st/backtrace-off] [{Severity}] {Message}");
    sink->AddAttributeFormatter(DateTimeFormatter("%Y-%m-%d %H:%M:%S:%u"));
    SeverityLogger logger({sink});
    logger.AddAttribute(DateTimeAttribute{});

    LOG_SEV(Info) << "**************************************************************";
    LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages";
    LOG_SEV(Info) << "**************************************************************";
    bench(iters, logger);
  }

  {
    auto sink = MakeSink<UnsynchronizedFrontend, FileSink>("logs/lightning_basic_mt.log");
    sink->SetFormatFrom("[{DateTime}] [basic_mt/backtrace-off] [{Severity}] {Message}");
    sink->AddAttributeFormatter(DateTimeFormatter("%Y-%m-%d %H:%M:%S:%u"));
    SeverityLogger logger({sink});
    logger.AddAttribute(DateTimeAttribute{});

    LOG_SEV(Info) << "**************************************************************";
    LOG_SEV(Info) << "Multi threaded: " << Format(num_threads) << " threads, " << Format(iters) << " messages";
    LOG_SEV(Info) << "**************************************************************";
    bench_mt(iters, logger, 4);
  }

  LOG_SEV(Info) << StartAnsiColor8Bit(AnsiForegroundColor::BrightYellow) << "Done with tests!";
}

void bench(int howmany, Logger& logger) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << i;
  }

  auto delta = high_resolution_clock::now() - start;
  auto delta_d = duration_cast<duration<double>>(delta).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec";
  std::cout << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_mt(int howmany, Logger& logger, size_t thread_count) {
  using std::chrono::duration;
  using std::chrono::duration_cast;
  using std::chrono::high_resolution_clock;

  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  auto start = high_resolution_clock::now();
  for (auto t = 0u; t < thread_count; ++t) {
    threads.emplace_back([&]() {
      for (int j = 0; j < howmany / static_cast<int>(thread_count); j++) {
        LOG_SEV_TO(logger, Info) << "Hello logger: msg number " << j;
      }
    });
  }

  for (auto& t: threads) {
    t.join();
  };

  auto delta = high_resolution_clock::now() - start;
  auto delta_d = duration_cast<duration<double>>(delta).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec";
  std::cout << "Elapsed: " << delta_d << " secs " << Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}