/*
  Header for audio source
*/
#include <vector>
#include "SampleBlock.hpp"
#ifndef TILER_ASOURCE_H_
#define TILER_ASOURCE_H_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

// class SampleBlock;

class ASource {
 public:
  ASource(int size, const std::string &fileName);
  ~ASource();

  int Size() { return mSampleBlocks.size(); }

  SampleBlock<float> *operator[](int index) { return mSampleBlocks.at(index); }

  void PrintInfo();

 private:
  void decode(AVCodecContext *pCodecCtx, AVPacket *pPacket, AVFrame *pFrame, FILE *outfile);
  std::vector<SampleBlock<float> *> mSampleBlocks;
  int mSamplingRate = 0;
  int mNumberSamples = 0;
  int mBlockSize = 0;
  int mLast = 0;
};

#endif  // TILER_ASOURCE_H_
