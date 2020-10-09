#include "logging.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace QBDL {

Logger::Logger(void) {
  this->sink_ = spdlog::stdout_color_mt("qbdl");
  this->sink_->set_level(spdlog::level::trace);
  this->sink_->set_pattern("%v");
  this->sink_->flush_on(spdlog::level::trace);
}

Logger &Logger::instance() {
  if (instance_ == nullptr) {
    instance_ = new Logger{};
    std::atexit(destroy);
  }
  return *instance_;
}

void Logger::destroy(void) { delete instance_; }

} // namespace QBDL
