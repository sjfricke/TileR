#include "AV.hpp"
#include "Debug.hpp"

#define INBUF_SIZE 4096

AV::AV(std::string fileName) { mInputFiles.push_back(fileName); }

AV::~AV() {}

void AV::PrintAVInfo() {}

void AV::AddInputSource(std::string fileName) { mInputFiles.push_back(fileName); }

void AV::VStich(std::string outFile, std::vector<VFrames> &vFrames) {
  AVFormatContext *pInFormatCtx = NULL;
  AVFormatContext *pOutFormatCtx = NULL;
  AVCodecContext *pDecoderCtx = NULL;
  AVCodecContext *pEncoderCtx = NULL;
  AVStream *pInStream;
  AVStream *pOutStream;
  AVCodec *pDecoderCodec;
  AVCodec *pEncoderCodec;
  AVPacket decodePacket;
  AVPacket encodePacket;
  AVFrame *pFrame;  // one frame between all, maybe could multithread in far future
  int ret;
  int videoStream;
  char libErr[256];

  // By default ffmpeg will not know to look for mp4
  // avformat_open_input will return Invalid data found from input
  av_register_all();

  ///////// Open Input and Decoder
  ret = avformat_open_input(&pInFormatCtx, vFrames.at(0).source.c_str(), NULL, NULL);
  if (ret < 0) {
    av_strerror(ret, libErr, 256);
    fprintf(stderr, "%s\n", libErr);
    FATAL("avformat_open_input()", ret);
  }

  ret = avformat_find_stream_info(pInFormatCtx, NULL);
  if (ret < 0) {
    FATAL("avformat_find_stream_info()", ret);
  }

  ret = av_find_best_stream(pInFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pDecoderCodec, 0);
  if (ret < 0) {
    FATAL("av_find_best_strem()", ret);
  }
  videoStream = ret;

  pDecoderCtx = avcodec_alloc_context3(pDecoderCodec);
  if (!pDecoderCtx) {
    FATAL("avcodec_alloc_context3()", -1);
  }

  pInStream = pInFormatCtx->streams[videoStream];
  ret = avcodec_parameters_to_context(pDecoderCtx, pInStream->codecpar);
  if (ret < 0) {
    FATAL("avcodec_parameters_to_context()", ret);
  }

  ret = avcodec_open2(pDecoderCtx, pDecoderCodec, NULL);
  if (ret < 0) {
    FATAL("avcodec_open2(decoder)", ret);
  }

  ////////////// Setup Encoder for Output
  pEncoderCodec = avcodec_find_encoder_by_name("libx264");
  if (pEncoderCodec == nullptr) {
    FATAL("avcodec_find_encoder_by_name()", -1);
  }

  ret = avformat_alloc_output_context2(&pOutFormatCtx, NULL, NULL, outFile.c_str());
  if (ret < 0) {
    FATAL("avformat_alloc_output_context2()", ret);
  }

  pEncoderCtx = avcodec_alloc_context3(pEncoderCodec);
  if (pEncoderCtx == nullptr) {
    FATAL("avcodec_alloc_context3()", -1);
  }

  // Make sure running as sudo or might not get access
  ret = avio_open(&pOutFormatCtx->pb, outFile.c_str(), AVIO_FLAG_WRITE);
  if (ret < 0) {
    av_strerror(ret, libErr, 256);
    fprintf(stderr, "%s\n", libErr);
    FATAL("avio_open()", ret);
  }

  //////// Initialize encoder context
  pEncoderCtx->time_base = av_inv_q(pDecoderCtx->framerate);
  pEncoderCtx->time_base = (AVRational){1, 25};
  LOG("encode time_base :", pEncoderCtx->time_base.num, " / ", pEncoderCtx->time_base.den);
  LOG("decod etime_base :", pDecoderCtx->time_base.num, " / ", pDecoderCtx->time_base.den);
  pEncoderCtx->pix_fmt = pDecoderCtx->pix_fmt;
  pEncoderCtx->bit_rate = pDecoderCtx->bit_rate;
  pEncoderCtx->codec_id = pDecoderCtx->codec_id;
  pEncoderCtx->width = pDecoderCtx->width;
  pEncoderCtx->height = pDecoderCtx->height;

  ret = avcodec_open2(pEncoderCtx, pEncoderCodec, NULL);
  if (ret < 0) {
    FATAL("avcodec_open2(encoder)", -1);
  }

  pOutStream = avformat_new_stream(pOutFormatCtx, pDecoderCodec);
  if (!pOutStream) {
    FATAL("avformat_new_stream()", -1);
  }
  pOutStream->time_base = pDecoderCtx->time_base;

  ret = avcodec_parameters_from_context(pOutStream->codecpar, pDecoderCtx);
  if (ret < 0) {
    FATAL("avcodec_parameters_from_context()", -1);
  }
  // avcodec_flush_buffers( pInStream->codec );
  ret = avformat_write_header(pOutFormatCtx, NULL);
  if (ret < 0) {
    FATAL("avformat_write_header()", -1);
  }

  int FPS = 24;  // need to verify
  int64_t packetDuration = 1001;
  int64_t startDts = packetDuration * vFrames.at(0).startFrame;
  int64_t endDts =
      packetDuration * static_cast<int64_t>(vFrames.at(0).startFrame + vFrames.at(0).duration);
  int test = 0;
  LOG("startDts: ", startDts, "\tendDts: ", endDts);

  ret = av_seek_frame(pInFormatCtx, videoStream, startDts, 0);
  if (ret < 0) {
    FATAL("av_seek_frame()", -1);
  }

  // ret = avformat_seek_file(pInFormatCtx, videoStream, INT64_MIN, vFrames.at(0).startFrame,
  // INT64_MAX, 0); if (ret < 0) {
  //   av_strerror(ret, libErr, 256);
  //   fprintf(stderr, "%s\n", libErr);
  //   FATAL("avio_open()", ret);
  // }

  // pFrame = av_frame_alloc();
  // av_init_packet(&decodePacket);
  // int frameFinished;
  // int outputed;

  // // you used avformat_seek_file() to seek CLOSE to the point you want... in order to give
  // precision to your seek,
  //   // just go on reading the packets and checking the packets PTS (presentation timestamp)
  //   while(av_read_frame(pInFormatCtx, &decodePacket) >= 0) {
  //       if(decodePacket.stream_index == videoStream) {
  //           avcodec_decode_video2(pDecoderCtx, pFrame, &frameFinished, &decodePacket);
  //           // this line guarantees you are getting what you really want.
  //           LOG("**** ", pFrame->pkt_pts, " *** ", frameFinished);
  //           if(frameFinished && pFrame->pkt_pts >= startDts && pFrame->pkt_pts <= endDts) {
  //               av_init_packet(&encodePacket);
  //               avcodec_encode_video2(pEncoderCtx, &encodePacket, pFrame, &outputed);
  //               if(outputed) {
  //                   if (av_write_frame(pOutFormatCtx, &encodePacket) != 0) {
  //                       fprintf(stderr, "convert(): error while writing video frame\n");
  //                       FATAL("av_write_frame()", -1);
  //                   }
  //                   LOG("TEST ", pFrame->pkt_pts);
  //               }
  //               av_free_packet(&encodePacket);
  //           }

  //           // exit the loop if you got the frames you want.
  //           if(pFrame->pkt_pts > endDts) {
  //               break;
  //           }
  //       }
  //   }

  // ////////// Send Thou Frames
  // av_init_packet(&decodePacket);
  // decodePacket.data = NULL;
  // decodePacket.size = 0;

  // av_init_packet(&encodePacket);
  // encodePacket.data = NULL;
  // encodePacket.size = 0;

  // pFrame = av_frame_alloc();
  // if (!pFrame) { FATAL("av_frame_alloc()", -1); }

  // while (av_read_frame(pInFormatCtx, &decodePacket) >= 0) {

  //   if (videoStream == decodePacket.stream_index) {
  //     // LOG("pts: ", decodePacket.pts, "\tdts: ", decodePacket.dts, "\tpos: ", decodePacket.pos,
  //     "\tdur: ", decodePacket.duration);
  //     // if (decodePacket.dts > startDts && test == 0) {
  //     //   test = 1;
  //     //   LOG("=======================================");
  //     //   ret = av_seek_frame(pInFormatCtx, videoStream, endDts, 0);
  //     //   if (ret < 0) { FATAL("av_seek_frame()", -1); }
  //     //   av_packet_unref(&decodePacket);
  //     //   continue;
  //     // }

  //     int ret = avcodec_send_packet(pDecoderCtx, &decodePacket);
  //     if (ret < 0) { FATAL("avcodec_send_packet()", -1); }
  //     if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
  //       std::cout << "avcodec_send_packet: " << ret << std::endl;
  //       break;
  //     }

  //     while (ret >= 0) {

  //       // Decode frame
  //       ret = avcodec_receive_frame(pDecoderCtx, pFrame);
  //       if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
  //         // if (ret == AVERROR(EAGAIN)) { LOG("EAGAIN"); }
  //         if (ret == AVERROR_EOF) { LOG("AVERROR_EOF"); }
  //         break;
  //       } else if (ret < 0) {
  //         FATAL("avcodec_receive_frame()", -1);
  //       }

  //       // Encode frame
  //       int ret3 = avcodec_send_frame(pEncoderCtx, pFrame);
  //       if (ret3 < 0) { FATAL("avcodec_send_frame()", -1); }

  //       while (1) {
  //         ret3 = avcodec_receive_packet(pEncoderCtx, &encodePacket);
  //         if (ret3 == AVERROR(EAGAIN) || ret3 == AVERROR_EOF) {
  //           break;
  //         } else if (ret3 < 0) {
  //           FATAL("avcodec_receive_packet()", -1);
  //         }

  //         encodePacket.stream_index = 0;
  //          // decodePacket.flags |= AV_PKT_FLAG_KEY;
  //         // std::cout << "encode: " << encodePacket.pts << " | " << encodePacket.dts << "\t";
  //         // av_packet_rescale_ts(&encodePacket, pInFormatCtx->streams[videoStream]->time_base,
  //                             //  pOutFormatCtx->streams[0]->time_base);
  //         // fprintf(stdout, "encode: %ld | %ld\n", encodePacket.pts, encodePacket.dts);
  //         // ret3 = av_interleaved_write_frame(pOutFormatCtx, &encodePacket);
  //         ret3 = av_write_frame(pOutFormatCtx, &encodePacket);
  //         if (ret3 < 0) {
  //            av_strerror(ret3, libErr, 256);
  //            fprintf(stderr, "ERROR: %s\n", libErr);
  //            FATAL("av_interleaved_write_frame", -1); }
  //       }

  //       // clean up these ret for errors
  //       if (ret3 == AVERROR_EOF) {
  //         ret = 0;
  //       }

  //     } // while (ret2 >= 0)

  //   }  // if (videoStream)

  // }  // while (ret >= 0)

  // flush encoder
  av_write_trailer(pOutFormatCtx);

  av_frame_free(&pFrame);

  // Flush decoder
  decodePacket.data = NULL;
  decodePacket.size = 0;
  // TODO
  av_packet_unref(&decodePacket);

  avformat_close_input(&pInFormatCtx);
  avformat_close_input(&pOutFormatCtx);
  avcodec_free_context(&pDecoderCtx);
  avcodec_free_context(&pEncoderCtx);
}

#include <libavutil/timestamp.h>
#include <string.h>

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag) {
  // AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

  printf("%s: pts:%ld dts:%ld  duration:%ld stream_index:%d\n", tag, (pkt->pts), (pkt->dts),
         (pkt->duration), pkt->stream_index);
}

int AV::VCut(double from_seconds, double end_seconds, const char *in_filename,
             const char *out_filename) {
  AVOutputFormat *ofmt = NULL;
  AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
  AVCodecContext *pDecoderCtx = NULL;
  AVCodecContext *pEncoderCtx = NULL;
  AVPacket pkt;
  AVCodec *pDecoderCodec;
  int ret, i;
  int videoStream;

  av_register_all();

  if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
    fprintf(stderr, "Could not open input file '%s'", in_filename);
  }

  if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
    fprintf(stderr, "Failed to retrieve input stream information");
  }

  ret = av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &pDecoderCodec, 0);
  if (ret < 0) {
    FATAL("av_find_best_strem()", ret);
  }
  videoStream = ret;
  LOG("Stream: ", videoStream);

  pDecoderCtx = avcodec_alloc_context3(pDecoderCodec);
  if (!pDecoderCtx) {
    FATAL("avcodec_alloc_context3()", -1);
  }

  // av_dump_format(ifmt_ctx, 0, in_filename, 0);

  avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
  if (!ofmt_ctx) {
    fprintf(stderr, "Could not create output context\n");
    ret = AVERROR_UNKNOWN;
  }

  ofmt = ofmt_ctx->oformat;

  for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    AVStream *in_stream = ifmt_ctx->streams[i];
    ret = avcodec_parameters_to_context(pDecoderCtx, in_stream->codecpar);
    if (ret < 0) {
      FATAL("avcodec_parameters_to_context()", ret);
    }

    AVStream *out_stream = avformat_new_stream(ofmt_ctx, pDecoderCtx->codec);
    if (!out_stream) {
      fprintf(stderr, "Failed allocating output stream\n");
      ret = AVERROR_UNKNOWN;
    }

    ret = avcodec_parameters_to_context(pEncoderCtx, in_stream->codecpar);
    if (ret < 0) {
      FATAL("avcodec_parameters_to_context()", ret);
    }

    // ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
    // if (ret < 0) {
    //     fprintf(stderr, "Failed to copy context from input to output stream codec context\n");
    // }
    pEncoderCtx->codec_tag = 0;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
      pEncoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }
  // av_dump_format(ofmt_ctx, 0, out_filename, 1);

  if (!(ofmt->flags & AVFMT_NOFILE)) {
    ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
      fprintf(stderr, "Could not open output file '%s'", out_filename);
    }
  }

  ret = avformat_write_header(ofmt_ctx, NULL);
  if (ret < 0) {
    fprintf(stderr, "Error occurred when opening output file\n");
  }

  //    int indexs[8] = {0};

  //    int64_t start_from = 8*AV_TIME_BASE;
  ret = av_seek_frame(ifmt_ctx, -1, from_seconds * AV_TIME_BASE, AVSEEK_FLAG_ANY);
  if (ret < 0) {
    fprintf(stderr, "Error seek\n");
    // goto end;
  }

  int64_t *dts_start_from = (int64_t *)malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
  memset(dts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);
  int64_t *pts_start_from = (int64_t *)malloc(sizeof(int64_t) * ifmt_ctx->nb_streams);
  memset(pts_start_from, 0, sizeof(int64_t) * ifmt_ctx->nb_streams);

  while (1) {
    AVStream *in_stream, *out_stream;

    ret = av_read_frame(ifmt_ctx, &pkt);
    if (ret < 0) break;

    if (videoStream == pkt.stream_index) {
      in_stream = ifmt_ctx->streams[pkt.stream_index];
      out_stream = ofmt_ctx->streams[pkt.stream_index];

      log_packet(ifmt_ctx, &pkt, "in");

      if (av_q2d(in_stream->time_base) * pkt.pts > end_seconds) {
        av_free_packet(&pkt);
        break;
      }

      if (dts_start_from[pkt.stream_index] == 0) {
        dts_start_from[pkt.stream_index] = pkt.dts;
        printf("dts_start_from: %d\n", dts_start_from[pkt.stream_index]);
      }
      if (pts_start_from[pkt.stream_index] == 0) {
        pts_start_from[pkt.stream_index] = pkt.pts;
        printf("pts_start_from: %d\n", pts_start_from[pkt.stream_index]);
      }

      /* copy packet */
      pkt.pts = av_rescale_q_rnd(pkt.pts - pts_start_from[pkt.stream_index], in_stream->time_base,
                                 out_stream->time_base, AV_ROUND_NEAR_INF);
      pkt.dts = av_rescale_q_rnd(pkt.dts - dts_start_from[pkt.stream_index], in_stream->time_base,
                                 out_stream->time_base, AV_ROUND_NEAR_INF);
      if (pkt.pts < 0) {
        pkt.pts = 0;
      }
      if (pkt.dts < 0) {
        pkt.dts = 0;
      }
      pkt.duration =
          (int)av_rescale_q((int64_t)pkt.duration, in_stream->time_base, out_stream->time_base);
      pkt.pos = -1;
      log_packet(ofmt_ctx, &pkt, "out");
      printf("\n");

      ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
      if (ret < 0) {
        fprintf(stderr, "Error muxing packet\n");
        break;
      }
    }
    av_free_packet(&pkt);
  }
  free(dts_start_from);
  free(pts_start_from);

  av_write_trailer(ofmt_ctx);
end:

  avformat_close_input(&ifmt_ctx);

  /* close output */
  if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
  avformat_free_context(ofmt_ctx);

  if (ret < 0 && ret != AVERROR_EOF) {
    fprintf(stderr, "Error occurred: %d\n", (ret));
    return 1;
  }

  return 0;
}