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
    stream << std::setfill('0') << std::setw(3)<< x / p;
    x %= p;
  }

  return stream.str();
}

std::string Format(const time::DateTime& x) {
  return Format("[%-%-% %:%:%.%]", x.GetYear(), x.GetMonthInt(), x.GetDay(), x.GetHour(), x.GetMinute(), x.GetSecond(), x.GetMicrosecond());
}

void gl(int howmany);
void bench_nonacceptingseverity_sink(int howmany);
void bench_nonacceptingseverity_core(int howmany);

void bench_fastdatetimegen(int howmany);
void bench_datetimenow(int howmany);
void bench_systemclock(int howmany);


int main() {

  auto iters = 1'250'000;
  auto num_threads = 4;

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "Single threaded: " << Format(iters) << " messages\n";
  LOG_SEV(Info) << "**************************************************************\n";
  gl(iters);
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
  bench_fastdatetimegen(iters);
  LOG_SEV(Info) << "\n";

  LOG_SEV(Info) << "**************************************************************\n";
  LOG_SEV(Info) << "System clock\n";
  LOG_SEV(Info) << "**************************************************************\n";
  bench_systemclock(iters);
  LOG_SEV(Info) << "\n";

  return 0;
}


void gl(int howmany) {
  auto fs = std::make_shared<FileSink>("logs/greased_lightning_basic_st.log");

  time::FastDateGenerator generator;

  Logger logger(fs);
  logger.SetDoTimeStamp(true);


  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    // auto x = generator.CurrentTime();
    //auto str = Format("[%-%-% %:%:%.%]", x.GetYear(), x.GetMonthInt(), x.GetDay(), x.GetHour(), x.GetMinute(), x.GetSecond(), x.GetMicrosecond());

    LOG_SEV_TO(logger, Info)
      << "[{DateTime}]"
      << " [basic_mt/backtrace-off] [{Severity}] " << "Hello logger: msg number " << i;

    // auto time = std::chrono::system_clock::now();


    // Format("%-%-%", x.GetYear(), x.GetMonthInt(), x.GetDay());

//    // get number of milliseconds for the current second
//    // (remainder after division into seconds)
//    int ms = static_cast<int>((duration_cast<std::chrono::microseconds>(time.time_since_epoch()) % 1'000'000).count());
//    // convert to std::time_t in order to convert to std::tm (broken time)
//    std::time_t t = std::chrono::system_clock::to_time_t(time);
//
//    std::tm now; // = std::gmtime(&t);
//    gmtime_r(&t, &now);

    // Format("", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, ms);
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  // LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec";
  std::cout << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
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

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
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
  std::cout << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_fastdatetimegen(int howmany) {
  time::DateTime dt;
  time::FastDateGenerator generator;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    dt = generator.CurrentTime();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << Format(dt) << " Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_datetimenow(int howmany) {
  time::DateTime dt;

  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    dt = time::DateTime::Now();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << Format(dt) << " Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}

void bench_systemclock(int howmany) {
  auto start = high_resolution_clock::now();
  for (auto i = 0; i < howmany; ++i) {
    auto current_time = std::chrono::system_clock::now();
  }
  auto delta_d = duration_cast<duration<double>>(high_resolution_clock::now() - start).count();

  LOG_SEV(Info) << "Elapsed: " << delta_d << " secs " <<  Format(static_cast<int>(howmany / delta_d)) << "/sec\n";
}
