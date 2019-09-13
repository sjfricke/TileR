/*
  Source is a demuxed and decoded file
 */

#ifndef TILER_SOURCE_H_
#define TILER_SOURCE_H_

#include <string>

class Source {
 public:
  Source(const std::string& file);
  ~Source();

 private:
  int Decode();
  
  std::string m_inputFile;
};

#endif // TILER_SOURCE_H_
