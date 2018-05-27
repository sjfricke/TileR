#include "ASource.hpp"
#include <algorithm>
#include "Debug.hpp"

ASource::ASource(int size, const std::string &fileName) : mBlockSize(size) {
  AVFrame *pFrame = nullptr;
  AVFormatContext *pFormatCtx = nullptr;
  AVStream *pAudioStream = nullptr;
  AVCodecContext *pCodecCtx = nullptr;
  AVCodec *pCodec = nullptr;
  AVPacket packet;
  int ret;
  int audioStream;

  // Set up decoder codec and context
  ret = avformat_open_input(&pFormatCtx, fileName.c_str(), NULL, NULL);
  if (ret < 0) {
    FATAL("ASource()::avformat_open_input()", -1);
  }

  ret = avformat_find_stream_info(pFormatCtx, NULL);
  if (ret < 0) {
    FATAL("ASource()::avformat_find_stream_info()", -1);
  }

  ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &pCodec, 0);
  if (ret < 0) {
    FATAL("ASource()::av_find_best_strem()", -1);
  }
  audioStream = ret;

  pCodecCtx = avcodec_alloc_context3(pCodec);
  if (!pCodecCtx) {
    FATAL("ASource()::avcodec_alloc_context3()", -1);
  }

  pAudioStream = pFormatCtx->streams[audioStream];
  ret = avcodec_parameters_to_context(pCodecCtx, pAudioStream->codecpar);
  if (ret < 0) {
    FATAL("ASource()::avcodec_parameters_to_context()", -1);
  }

  ret = avcodec_open2(pCodecCtx, pCodec, NULL);
  if (ret < 0) {
    FATAL("ASource()::avcodec_open2(decoder)", -1);
  }

  // Get data yall
  mSamplingRate = pCodecCtx->sample_rate;
  mNumberSamples = 0;  // PARANOIA

  // AV_SAMPLE_FMT_FLTP == float, planar
  if (pCodecCtx->sample_fmt != AV_SAMPLE_FMT_FLTP) {
    LOG("==========\n", "Warning, Audio not in Float planer format", "\n==========");
  }
  LOG("\nAudio AVSampleFormat type: ", av_get_sample_fmt_name(pCodecCtx->sample_fmt));

  FILE *outFile = fopen("./data/processed_samples.txt", "wb");
  if (!outFile) {
    FATAL("ASource()::fopen()", -1);
  }

  pFrame = av_frame_alloc();
  if (!pFrame) {
    FATAL("ASource()::av_frame_alloc()", -1);
  }

  // Read the packets in a loop
  while (1) {
    ret = av_read_frame(pFormatCtx, &packet);
    if (ret < 0) {
      LOG("Done reading frames");
      break;
    }

    if (packet.stream_index == audioStream) {
      decode(pCodecCtx, &packet, pFrame, outFile);
    }

    av_packet_unref(&packet);
  }

  // Flush decoder packets for Narnia
  LOG("Flushing... FOR NARNIA");
  packet.data = NULL;
  packet.size = 0;
  decode(pCodecCtx, &packet, pFrame, outFile);

  // Todo: Complete Cleanup
  fclose(outFile);
  avcodec_free_context(&pCodecCtx);
  avformat_close_input(&pFormatCtx);
  av_free(pFrame);
}

ASource::~ASource() {
  for (auto &sampleBlock : mSampleBlocks) {
    if (sampleBlock != NULL) {
      delete sampleBlock;
    }
  }
}

void ASource::decode(AVCodecContext *pCodecCtx, AVPacket *pPacket, AVFrame *pFrame, FILE *outFile) {
  int dataSize = av_get_bytes_per_sample(pCodecCtx->sample_fmt);
  int ret;

  // send packet to get decode from libavcodec decoder
  ret = avcodec_send_packet(pCodecCtx, pPacket);
  if (ret < 0) {
    FATAL("ASource()::av_send_packet()", -1);
  }

  // Audio packets can have multiple audio frames in a single packet
  while (ret >= 0) {
    std::vector<float> sampleBlock;

    ret = avcodec_receive_frame(pCodecCtx, pFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      if (ret == AVERROR_EOF) {
        LOG("AVERROR_EOF");
      }
      break;
    } else if (ret < 0) {
      FATAL("ASource()::av_receive_frame()", -1);
    }

    mNumberSamples += pFrame->nb_samples;

    // Write float32 data for the channel
    for (int i = 0; i < pFrame->nb_samples; i++) {
      for (int ch = 0; ch < pCodecCtx->channels; ch++) {
        fwrite(pFrame->data[ch] + (dataSize * i), dataSize, 1, outFile);
        float sample = *((float *)(pFrame->data[ch] + (dataSize * i)));
        sampleBlock.push_back(sample);
      }
    }
    av_frame_unref(pFrame);  // need?
    mSampleBlocks.push_back(new SampleBlock<float>(sampleBlock));
  }  // while (ret >= 0)
}

void ASource::PrintInfo() {
  //  int index = 0;
  //  for(auto & blockptr : mSampleBlocks){
  //      LOG("INDEX: ", index);
  //      blockptr->PrintInfo();
  //      index++;
  //  }
  LOG("SAMPLING RATE: ", mSamplingRate);
  LOG("NUMBER SAMPLES: ", mNumberSamples);
  LOG("NUMBER BLOCKS: ", mSampleBlocks.size());
  LOG("BLOCK SIZE: ", mBlockSize);
  LOG("LAST ", mLast);
}
