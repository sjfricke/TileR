/*
  Header only helper for avpackets
*/
#ifndef TILER_PACKET_H_
#define TILER_PACKET_H_

#include "utils.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
};

static void LogPacket(const AVPacket *packet, const char *tag) {
  LOG(tag, " pts:", packet->pts, " dts:", packet->dts, " duration:", packet->duration, " stream_index:", packet->stream_index);
}

static void DecodeVideoPacket(AVCodecContext *pCodecContext, AVPacket *pPacket, AVFrame *pFrame) {
  //int dataSize = av_get_bytes_per_sample(pCodecContext->sample_fmt);
  int ret;

  // send packet to get decode from libavcodec decoder
  ret = avcodec_send_packet(pCodecContext, pPacket);
  CHECK_ERR(ret);

  ret = avcodec_receive_frame(pCodecContext, pFrame);
  if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    if (ret == AVERROR_EOF) {
      LOG("AVERROR_EOF");
    }
  } else if (ret < 0) {
      FATAL("ASource()::av_receive_frame()", -1);
    }

  LOG("VIDEO pts: ", pFrame->pts, "\tpkt_dts: ", pFrame->pkt_dts, "\tpkt_pos: ", pFrame->pkt_pos,
        "\tpkt_dur: ", pFrame->pkt_duration);

  av_frame_unref(pFrame);  
}

static void DecodeAudioPacket(AVCodecContext *pCodecContext, AVPacket *pPacket, AVFrame *pFrame) {
  //int dataSize = av_get_bytes_per_sample(pCodecContext->sample_fmt);
  int ret;

  // send packet to get decode from libavcodec decoder
  ret = avcodec_send_packet(pCodecContext, pPacket);
  CHECK_ERR(ret);

  // Audio packets can have multiple audio frames in a single packet
  while (ret >= 0) {
    ret = avcodec_receive_frame(pCodecContext, pFrame);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      if (ret == AVERROR_EOF) {
        LOG("AVERROR_EOF");
      }
      break;
    } else if (ret < 0) {
      FATAL("ASource()::av_receive_frame()", -1);
    }
    LOG("AUDIO pts: ", pFrame->pts, "\tpkt_dts: ", pFrame->pkt_dts, "\tpkt_pos: ", pFrame->pkt_pos,
        "\tpkt_dur: ", pFrame->pkt_duration);

    av_frame_unref(pFrame);
  }  
}

#endif  // TILER_PACKET_H_
