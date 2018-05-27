/*
  Header only for debug/loging calls
*/
#pragma once

#ifndef TILER_DEBUG_H_
#define TILER_DEBUG_H_

#include <iostream>
#include <utility>

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

#endif  // TILER_DEBUG_H_
