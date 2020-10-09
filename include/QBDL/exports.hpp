#ifndef QBDL_EXPORTS_HPP
#define QBDL_EXPORTS_HPP

#if defined _WIN32 || defined __CYGWIN__
#define QBDL_HELPER_IMPORT __declspec(dllimport)
#define QBDL_HELPER_EXPORT __declspec(dllexport)
#define QBDL_HELPER_LOCAL
#else
#define QBDL_HELPER_IMPORT __attribute__((visibility("default")))
#define QBDL_HELPER_EXPORT __attribute__((visibility("default")))
#define QBDL_HELPER_LOCAL __attribute__((visibility("hidden")))
#endif

#ifdef QBDL_EXPORTS
#define QBDL_API QBDL_HELPER_EXPORT
#define QBDL_LOCAL QBDL_HELPER_LOCAL
#elif defined(QBDL_STATIC)
#define QBDL_API
#define QBDL_LOCAL
#else
#define QBDL_API QBDL_HELPER_IMPORT
#define QBDL_LOCAL QBDL_HELPER_LOCAL
#endif

#endif
