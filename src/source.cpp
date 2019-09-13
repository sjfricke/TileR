#include "utils.hpp"
#include "packet.hpp"
#include "source.hpp"
#include <unistd.h>     // access(2)

extern "C" {
#include <libavutil/imgutils.h>
  //#include <libavutil/samplefmt.h>
  //#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

Source::Source(const std::string& file) : m_inputFile(file) {
  if (access(file.c_str(), F_OK) == -1) {
    LOG("... file is fake news, try again");
  }

  Decode();
}

Source::~Source() {
}

int Source::Decode() {
  LOG("Decoding ", m_inputFile);

  int ret;
 
  AVFormatContext *pInFormatContext = nullptr;
  AVCodecContext *pVideoDecodeContext = nullptr;
  AVCodecContext *pAudioDecodeContext = nullptr;
  AVStream *pVideoStream = nullptr;
  AVStream *pAudioStream = nullptr;
  AVCodec *pVideoDecoderCodec = nullptr;
  AVCodec *pAudioDecoderCodec = nullptr;
  int videoStreamIndex = -1;
  int audioStreamIndex = -1;
  uint8_t* pVideoImageData[4] = {nullptr};
  int videoImageDataSize;
  int videoLinesize[4];
  AVPacket decodePacket;
  AVFrame *pFrame;  // one frame between all, maybe could multithread in far future

  
  ret = avformat_open_input(&pInFormatContext, m_inputFile.c_str(), NULL, NULL);  
  CHECK_ERR(ret);

  av_dump_format(pInFormatContext, 0, m_inputFile.c_str(), 0);
  
  ret = avformat_find_stream_info(pInFormatContext, NULL);
  CHECK_ERR(ret);

  // Open codec context
  
  // Video
  ret = av_find_best_stream(pInFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, &pVideoDecoderCodec, 0);
  if (ret >= 0) {
    videoStreamIndex = ret;
    pVideoStream = pInFormatContext->streams[videoStreamIndex];
    
    pVideoDecodeContext = avcodec_alloc_context3(pVideoDecoderCodec);
    CHECK_ERR(ret);
   
    ret = avcodec_parameters_to_context(pVideoDecodeContext, pVideoStream->codecpar);
    CHECK_ERR(ret);
    
    ret = avcodec_open2(pVideoDecodeContext, pVideoDecoderCodec, NULL);
    CHECK_ERR(ret);

    if (pVideoDecodeContext->pix_fmt != AV_PIX_FMT_YUV420P) {
      FATAL("Unsupported video format: ", pVideoDecodeContext->pix_fmt);
    }
    
    ret = av_image_alloc(pVideoImageData, videoLinesize, pVideoDecodeContext->width, pVideoDecodeContext->height, pVideoDecodeContext->pix_fmt, 1);
    videoImageDataSize = ret;
    CHECK_ERR(ret);
  }

  // Audio
  ret = av_find_best_stream(pInFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, &pAudioDecoderCodec, 0);
  if (ret >= 0) {
    audioStreamIndex = ret;
    pAudioStream = pInFormatContext->streams[audioStreamIndex];
     
    pAudioDecodeContext = avcodec_alloc_context3(pAudioDecoderCodec);
    CHECK_ERR(ret);
    
    ret = avcodec_parameters_to_context(pAudioDecodeContext, pAudioStream->codecpar);
    CHECK_ERR(ret);
    
    ret = avcodec_open2(pAudioDecodeContext, pAudioDecoderCodec, NULL);
    CHECK_ERR(ret);

    LOG("Codec -> ", pAudioDecoderCodec->name);
  }

  pFrame = av_frame_alloc();
  if (pFrame == nullptr) {
    FATAL("av_frame_alloc failed", -1);
  }

  av_init_packet(&decodePacket);
  decodePacket.data = nullptr;
  decodePacket.size = 0;

  int audio = 0;
  int video = 0;
  while (av_read_frame(pInFormatContext, &decodePacket) >= 0) {
    AVPacket originalPacket = decodePacket;

    if (decodePacket.stream_index == audioStreamIndex) {
      DecodeAudioPacket(pAudioDecodeContext, &decodePacket, pFrame);
      audio++;
    }

    if (decodePacket.stream_index == videoStreamIndex) {
      DecodeVideoPacket(pVideoDecodeContext, &decodePacket, pFrame);
      video++;
    }

    av_packet_unref(&originalPacket);
  }

  LOG("Audio: ", audio, "  - Video: ", video);
  
  // Cleanup
  if (pVideoImageData[0] != nullptr) {
    av_free(pVideoImageData[0]);
    pVideoImageData[0] = nullptr;
  }  
  
  return 0;
}
