#ifndef QBDL_LOGGING_H_
#define QBDL_LOGGING_H_

#include "spdlog/spdlog.h"
#include <QBDL/log.hpp>

#ifndef NDEBUG
static constexpr bool QBDL_DEBUG_ENABLED = false;
#else
static constexpr bool QBDL_DEBUG_ENABLED = true;
#endif

#define QBDL_DEBUG(...) QBDL::Logger::debug(__VA_ARGS__)
#define QBDL_INFO(...) QBDL::Logger::info(__VA_ARGS__)
#define QBDL_ERROR(...) QBDL::Logger::err(__VA_ARGS__)
#define QBDL_WARN(...) QBDL::Logger::warn(__VA_ARGS__)

namespace QBDL {
class Logger {
public:
  static Logger &instance();

  void setLogLevel(LogLevel level);

  template <typename... Args>
  static void debug(const char *fmt, const Args &...args) {
    if constexpr (QBDL_DEBUG_ENABLED) {
      Logger::instance().sink_->debug(fmt, args...);
    }
  }

  template <typename... Args>
  static void info(const char *fmt, const Args &...args) {
    Logger::instance().sink_->info(fmt, args...);
  }

  template <typename... Args>
  static void err(const char *fmt, const Args &...args) {
    Logger::instance().sink_->error(fmt, args...);
  }

  template <typename... Args>
  static void warn(const char *fmt, const Args &...args) {
    Logger::instance().sink_->warn(fmt, args...);
  }

private:
  Logger(void);
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  std::shared_ptr<spdlog::logger> sink_;
};

} // namespace QBDL

#endif
