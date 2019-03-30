#include "utils.hpp"
#include "source.hpp"
#include <unistd.h>     // access(2)

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

Source::Source(const std::string& file) : m_input_file(file) {
  if (access(file.c_str(), F_OK) == -1) {
    LOG("... file is fake news, try again");
   }

  Decode();
}

Source::~Source() {
}

int Source::Decode() {

  LOG("Decoding ", m_input_file);

  return 0;
}
