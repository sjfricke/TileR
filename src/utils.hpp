/*
  Header only for debug/loging calls
*/
#ifndef TILER_UTILS_H_
#define TILER_UTILS_H_

#include <iostream>
#include <utility>

extern "C" {
#include <libavutil/avutil.h>
};

// TODO replace std::endl with \n to save from logging flushing
#define LOG(...) std::cout, __VA_ARGS__, std::endl
template <typename T>
std::ostream &operator,(std::ostream &out, const T &t) {
  out << t;
  return out;
}

// overloaded version to handle all those special std::endl and others...
static std::ostream &operator,(std::ostream &out, std::ostream &(*f)(std::ostream &)) {
  out << f;
  return out;
}

#define FATAL(err, code)                                 \
  {                                                      \
    LOG("\n===== FATEL ERROR: ", code, " =====\n", err); \
    exit(code);                                          \
  }

#define CHECK_ERR(ret) \
  { \
  if (ret < 0) { \
  char libErr[256]; \
  av_strerror(ret, libErr, 256); \
  fprintf(stderr, "%s:%d\t%s\n", __FILE__, __LINE__, libErr);	\
  exit(ret);\
}						\
}

#endif  // TILER_UTILS_H_
