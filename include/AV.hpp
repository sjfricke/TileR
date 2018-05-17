/*
  AV (audio/video) represents the class to hold instance of a source
  Each AV has only 1 source
*/
#pragma once

#ifndef TILER_AV_H_
#define TILER_AV_H_

#include <iostream>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

// Forward Declaration
// TODO

class AV {
public:
  AV(std::string fileName);
  ~AV();

  void PrintAVInfo(void);
  void ReadPackets(bool verbose = false);
private:
  void printAudioFrameInfo(const AVCodecContext* codecContext, const AVFrame* frame);

  AVFrame* mpFrame;
  AVFormatContext* mpFormatContext;
  AVStream* mpAudioStream;
  AVCodecContext* mpCodecContext;
};

#endif // TILER_AV_H_
