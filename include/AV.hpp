/*
  AV (audio/video) represents the class to hold instance of a source
  Each AV has only 1 source
*/
#pragma once

#ifndef TILER_AV_H_
#define TILER_AV_H_

#include <iostream>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
};

av_always_inline char* aav_err2str(int errnum) {
  static char str[AV_ERROR_MAX_STRING_SIZE];
  memset(str, 0, sizeof(str));
  return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

// Forward Declaration
// TODO

// TODO - Not have in global header scope
struct VFrames {
  int64_t startFrame;
  int64_t duration;
  std::string source;
};

class AV {
 public:
  AV(std::string fileName);
  ~AV();

  void PrintAVInfo(void);
  void AddInputSource(std::string fileName);

  // Video
  void VStich(std::string outFile, std::vector<VFrames>& vFrames);
  int VCut(double from_seconds, double end_seconds, const char* in_filename,
           const char* out_filename);

 private:
  std::vector<std::string> mInputFiles;
};

#endif  // TILER_AV_H_
