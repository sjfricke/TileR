/*
  Header for audio source
*/
#include "Debug.hpp"
#include "SampleBlock.hpp"
#include <vector>
#include <algorithm>

#ifndef TILER_ASOURCE_H_
#define TILER_ASOURCE_H_

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
};

template<typename T> class ASource {
    public:
        ASource(int size, const std::string & fileName);
        //~ASource();

        int Size(){
            return mSampleBlocks.size();
        }

        SampleBlock<T> * operator[] (int index){
            return mSampleBlocks.at(index);
        }

        void PrintInfo();

    private:
        std::vector<SampleBlock<T> *> mSampleBlocks;
        int mSamplingRate = 0;
        int mNumberSamples = 0;
        int mNumberBlocks = 0;
        int mBlockSize = 0;
        int mLast = 0;

};


template<typename T> ASource<T>::ASource(int size, const std::string & fileName){

    AVFrame * mpFrame = nullptr;
    AVFormatContext* mpFormatContext = NULL;
    AVStream* mpAudioStream = nullptr;
    AVCodecContext* mpCodecContext = nullptr;

    mBlockSize = size;

    av_register_all(); // Initialize FFmpeg

    mpFrame = av_frame_alloc();
  if (!mpFrame) {
    FATAL("Error allocating the frame", -1);
  }

  if (avformat_open_input(&mpFormatContext, fileName.c_str(), NULL, NULL) != 0) {
  // std::cout << "DYING" << std::flushed;
    av_free(mpFrame);
    LOG("MORE_DYING");
    FATAL("Error opening the file", -1);
  }

  if (avformat_find_stream_info(mpFormatContext, NULL) < 0) {
    av_free(mpFrame);
    avformat_close_input(&mpFormatContext);
    FATAL("Error finding the stream info", -1);
  }

  // Find the audio stream
  AVCodec* cdc = nullptr;
  int streamIndex = av_find_best_stream(mpFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &cdc, 0);
  if (streamIndex < 0) {
    av_free(mpFrame);
    avformat_close_input(&mpFormatContext);
    FATAL("Could not find any audio stream in the file", -1);
  }

  if(!cdc){
    FATAL("Codec couldn't be opened",-1);
  }

  LOG("CODEC_TYPE ", mpFormatContext->streams[streamIndex]->codec->codec_type);
  std::cout << "STRING VERSION: " << av_get_media_type_string(mpFormatContext->streams[streamIndex]->codec->codec_type) << std::endl;
  mpAudioStream = mpFormatContext->streams[streamIndex];
  mpCodecContext = mpAudioStream->codec;
  mpCodecContext->codec = cdc;

  if (avcodec_open2(mpCodecContext, cdc, NULL) < 0) {
    av_free(mpFrame);
    avformat_close_input(&mpFormatContext);
    FATAL("Couldn't open the context with the decoder", -1);
  }

  mSamplingRate = mpCodecContext->sample_rate;

  AVPacket readingPacket;
  AVPacket decodingPacket;
  av_init_packet(&readingPacket);
  av_init_packet(&decodingPacket);

  std::vector<T> allSamples;
  FILE * outfile = fopen("./data/processed_samples.txt", "wb");

  // Read the packets in a loop
  mNumberSamples = 0; // PARANOIA
  while (av_read_frame(mpFormatContext, &readingPacket) == 0) {
    if (readingPacket.stream_index == mpAudioStream->index) {
      
      // readingPacket;
      av_packet_ref(&decodingPacket, &readingPacket);

      // Get the data size per element in the stream
      int dataSize = av_get_bytes_per_sample(mpCodecContext->sample_fmt);
     
      // Audio packets can have multiple audio frames in a single packet
      while (decodingPacket.size > 0) {
	      // Try to decode the packet into a frame
	      // Some frames rely on multiple packets, so we have to make sure the frame is finished before
	      // we can use it
	      int gotFrame = 0;
	      int result = avcodec_decode_audio4(mpCodecContext, mpFrame, &gotFrame, &decodingPacket);
	      if (result >= 0 && gotFrame) {
	        decodingPacket.size -= result;
	        decodingPacket.data += result;
          mNumberSamples += mpFrame->nb_samples;
          // Write float32 data for the channel
          for(int jj = 0; jj < mpFrame->nb_samples; jj++){
            fwrite(mpFrame->data[0]+jj*dataSize, dataSize, 1, outfile);
            float sample = *((float *)(mpFrame->data[0]+jj*dataSize));
            allSamples.push_back(sample);
          }

	    } else {
	          decodingPacket.size = 0;
	          decodingPacket.data = nullptr;
	        }
      }
    }

    // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
    //av_free_packet(&readingPacket); DEPRECATED
    av_packet_unref(&readingPacket);
    av_packet_unref(&decodingPacket);
  }

  // Some codecs will cause frames to be buffered up in the decoding process. If the CODEC_CAP_DELAY flag
  // is set, there can be buffered up frames that need to be flushed, so we'll do that
  if (mpCodecContext->codec->capabilities /*& CODEC_CAP_DELAY*/) {
    av_init_packet(&readingPacket);
    // Decode all the remaining frames in the buffer, until the end is reached
    int gotFrame = 0;
    while (avcodec_decode_audio4(mpCodecContext, mpFrame, &gotFrame, &readingPacket) >= 0 && gotFrame) {
      // We now have a fully decoded audio frame
      //LOG("IN HERE");
    }
  }

  int sampleIndex = 0;
  mNumberBlocks = (mNumberSamples % mBlockSize) ? (mNumberSamples/mBlockSize + 1) : mNumberSamples/size;
  mSampleBlocks = std::vector<SampleBlock<T> *>(mNumberBlocks);
  for(auto & blockptr : mSampleBlocks){
      std::vector<T> block;
      for(int kk = 0; kk < size; kk++){
          if((sampleIndex*mBlockSize+kk) >= mNumberSamples){
              mLast = (kk % size);
              break;
          }
          block.push_back(allSamples[sampleIndex*mBlockSize + kk]);
      }
      blockptr = new SampleBlock<T>(block);
      sampleIndex++;
  }

  // Todo: Complete Cleanup
  av_free(mpFrame);
  avcodec_close(mpCodecContext);
  avformat_close_input(&mpFormatContext);

}


template<typename T> void ASource<T>::PrintInfo(){
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

#endif // TILER_ASOURCE_H_