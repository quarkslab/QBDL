#ifndef QBDL_LOG_H_
#define QBDL_LOG_H_

namespace QBDL {

enum LogLevel : int {
  trace,
  debug,
  info,
  warn,
  err,
  critical,
};

void setLogLevel(LogLevel level);

} // namespace QBDL

#endif
