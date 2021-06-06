#include "logging.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace QBDL {

Logger::Logger(void) {
  sink_ = spdlog::stdout_color_mt("qbdl");
  sink_->set_level(spdlog::level::trace);
  sink_->set_pattern("%v");
  sink_->flush_on(spdlog::level::trace);
}

void Logger::setLogLevel(LogLevel level) {
  spdlog::level::level_enum slevel;
  switch (level) {
#define LLMAP(ours, lspd)                                                      \
  case LogLevel::ours:                                                         \
    slevel = spdlog::level::lspd;                                              \
    break;

    LLMAP(trace, trace)
    LLMAP(debug, debug)
    LLMAP(info, info)
    LLMAP(warn, warn)
    LLMAP(err, err)
    LLMAP(critical, critical)
#undef LLMAP
  }
  sink_->set_level(slevel);
}

static std::unique_ptr<Logger> logger_instance_;

Logger &Logger::instance() {
  if (!logger_instance_) {
    logger_instance_.reset(new Logger{});
  }
  return *logger_instance_;
}

void setLogLevel(LogLevel level) { Logger::instance().setLogLevel(level); }

} // namespace QBDL
