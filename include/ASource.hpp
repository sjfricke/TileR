/*
  Header for audio source
*/
#include <algorithm>
#include <vector>
#include "Debug.hpp"
#include "SampleBlock.hpp"

#ifndef TILER_ASOURCE_H_
#define TILER_ASOURCE_H_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

template <typename T>
class ASource {
 public:
  ASource(int size, const std::string &fileName);
  ~ASource();

  int Size() { return mSampleBlocks.size(); }

  SampleBlock<T> *operator[](int index) { return mSampleBlocks.at(index); }

  void PrintInfo();

 private:
  std::vector<SampleBlock<T> *> mSampleBlocks;
  int mSamplingRate = 0;
  int mNumberSamples = 0;
  int mNumberBlocks = 0;
  int mBlockSize = 0;
  int mLast = 0;
};

template <typename T>
ASource<T>::ASource(int size, const std::string &fileName) {
  AVFrame *pFrame = nullptr;
  AVFormatContext *pFormatContext = NULL;
  AVStream *pAudioStream = nullptr;
  AVCodecContext *pCodecContext = nullptr;

  mBlockSize = size;

  av_register_all();  // Initialize FFmpeg

  pFrame = av_frame_alloc();
  if (!pFrame) {
    FATAL("Error allocating the frame", -1);
  }

  if (avformat_open_input(&pFormatContext, fileName.c_str(), NULL, NULL) != 0) {
    // std::cout << "DYING" << std::flushed;
    av_free(pFrame);
    LOG("MORE_DYING");
    FATAL("Error opening the file", -1);
  }

  if (avformat_find_stream_info(pFormatContext, NULL) < 0) {
    av_free(pFrame);
    avformat_close_input(&pFormatContext);
    FATAL("Error finding the stream info", -1);
  }

  // Find the audio stream
  AVCodec *pCdc = nullptr;
  int streamIndex = av_find_best_stream(pFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &pCdc, 0);
  if (streamIndex < 0) {
    av_free(pFrame);
    avformat_close_input(&pFormatContext);
    FATAL("Could not find any audio stream in the file", -1);
  }

  if (!pCdc) {
    FATAL("Codec couldn't be opened", -1);
  }

  LOG("CODEC_TYPE ", pFormatContext->streams[streamIndex]->codec->codec_type);
  LOG("STRING VERSION: ",
      av_get_media_type_string(pFormatContext->streams[streamIndex]->codec->codec_type));
  pAudioStream = pFormatContext->streams[streamIndex];
  pCodecContext = pAudioStream->codec;
  pCodecContext->codec = pCdc;

  if (avcodec_open2(pCodecContext, pCdc, NULL) < 0) {
    av_free(pFrame);
    avformat_close_input(&pFormatContext);
    FATAL("Couldn't open the context with the decoder", -1);
  }

  mSamplingRate = pCodecContext->sample_rate;

  AVPacket readingPacket;
  AVPacket decodingPacket;
  av_init_packet(&readingPacket);
  av_init_packet(&decodingPacket);

  std::vector<T> allSamples;
  FILE *outFile = fopen("./data/processed_samples.txt", "wb");

  // Read the packets in a loop
  mNumberSamples = 0;  // PARANOIA
  while (av_read_frame(pFormatContext, &readingPacket) == 0) {
    if (readingPacket.stream_index == pAudioStream->index) {
      // readingPacket;
      av_packet_ref(&decodingPacket, &readingPacket);

      // Get the data size per element in the stream
      int dataSize = av_get_bytes_per_sample(pCodecContext->sample_fmt);

      // Audio packets can have multiple audio frames in a single packet
      while (decodingPacket.size > 0) {
        // Try to decode the packet into a frame
        // Some frames rely on multiple packets, so we have to make sure the
        // frame is finished before
        // we can use it
        int gotFrame = 0;
        int result = avcodec_decode_audio4(pCodecContext, pFrame, &gotFrame, &decodingPacket);
        if (result >= 0 && gotFrame) {
          decodingPacket.size -= result;
          decodingPacket.data += result;
          mNumberSamples += pFrame->nb_samples;
          // Write float32 data for the channel
          for (int jj = 0; jj < pFrame->nb_samples; jj++) {
            fwrite(pFrame->data[0] + jj * dataSize, dataSize, 1, outFile);
            float sample = *((float *)(pFrame->data[0] + jj * dataSize));
            allSamples.push_back(sample);
          }

        } else {
          decodingPacket.size = 0;
          decodingPacket.data = nullptr;
        }
      }
    }

    // You *must* call av_free_packet() after each call to av_read_frame() or
    // else you'll leak memory
    // av_free_packet(&readingPacket); DEPRECATED
    av_packet_unref(&readingPacket);
    av_packet_unref(&decodingPacket);
  }

  // Some codecs will cause frames to be buffered up in the decoding process. If
  // the CODEC_CAP_DELAY flag
  // is set, there can be buffered up frames that need to be flushed, so we'll
  // do that
  if (pCodecContext->codec->capabilities /*& CODEC_CAP_DELAY*/) {
    av_init_packet(&readingPacket);
    // Decode all the remaining frames in the buffer, until the end is reached
    int gotFrame = 0;
    while (avcodec_decode_audio4(pCodecContext, pFrame, &gotFrame, &readingPacket) >= 0 &&
           gotFrame) {
      // We now have a fully decoded audio frame
    }
  }

  int sampleIndex = 0;
  mNumberBlocks =
      (mNumberSamples % mBlockSize) ? (mNumberSamples / mBlockSize + 1) : mNumberSamples / size;
  mSampleBlocks = std::vector<SampleBlock<T> *>(mNumberBlocks);

  for (auto &blockptr : mSampleBlocks) {
    std::vector<T> block;
    for (int kk = 0; kk < size; kk++) {
      if ((sampleIndex * mBlockSize + kk) >= mNumberSamples) {
        mLast = (kk % size);
        break;
      }
      block.push_back(allSamples[sampleIndex * mBlockSize + kk]);
    }
    blockptr = new SampleBlock<T>(block);
    sampleIndex++;
  }

  // Todo: Complete Cleanup
  av_free(pFrame);
  avcodec_close(pCodecContext);
  avformat_close_input(&pFormatContext);
}

template <typename T>
ASource<T>::~ASource() {
  for (auto &sampleBlock : mSampleBlocks) {
    if (sampleBlock != NULL) {
      delete sampleBlock;
    }
  }
}

template <typename T>
void ASource<T>::PrintInfo() {
  //  int index = 0;
  //  for(auto & blockptr : mSampleBlocks){
  //      LOG("INDEX: ", index);
  //      blockptr->PrintInfo();
  //      index++;
  //  }
  LOG("SAMPLING RATE: ", mSamplingRate);
  LOG("NUMBER SAMPLES: ", mNumberSamples);
  LOG("NUMBER BLOCKS: ", mNumberBlocks);
  LOG("BLOCK SIZE: ", mBlockSize);
  LOG("LAST ", mLast);
}

#endif  // TILER_ASOURCE_H_
