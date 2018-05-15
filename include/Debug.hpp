/* 
	Header only for debug/loging calls
*/
#pragma once

#ifndef SRC_DEBUG_H_
#define SRC_DEBUG_H_

#include <iostream>
#include <utility>

#define LOG(...) std::cout , __VA_ARGS__ , std::endl
template <typename T>
std::ostream& operator,(std::ostream& out, const T& t) {
  out << t;
  return out;
}

//overloaded version to handle all those special std::endl and others...
std::ostream& operator,(std::ostream& out, std::ostream&(*f)(std::ostream&)) {
  out << f;
  return out;
}

#endif // SRC_DEBUG_H_