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
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

av_always_inline char* aav_err2str(int errnum)
{
  static char str[AV_ERROR_MAX_STRING_SIZE];
  memset(str, 0, sizeof(str));
  return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

// Forward Declaration
// TODO

// TODO - Not have in global header scope
typedef struct OutputStream {
  AVStream *stream;

  /* pts of the next frame that will be generated */
  int64_t next_pts = 0;
  int samples_count = 0;

  AVFrame *frame;

} OutputStream;


class AV {
 public:
  AV(std::string fileName);
  ~AV();

  void PrintAVInfo(void);
  void AddInputSource(std::string fileName);
  void Stich(std::string outFile, int start, int duration);

 private:
  std::vector<std::string> mInputFiles;
  AVFrame* dummyFrame(OutputStream *);
};

#endif  // TILER_AV_H_
