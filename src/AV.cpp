#include "AV.hpp"
#include "Debug.hpp"

#define INBUF_SIZE 4096

AV::AV(std::string fileName) { mInputFiles.push_back(fileName); }

AV::~AV() {}

void AV::PrintAVInfo() {}

void AV::AddInputSource(std::string fileName) { mInputFiles.push_back(fileName); }

void AV::Stich(std::string outFile, int start, int duration) {
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

  ///////// Open Input and Decoder
  ret = avformat_open_input(&pInFormatCtx, mInputFiles[0].c_str(), NULL, NULL);
  if (ret < 0) {
    FATAL("avformat_open_input()", -1);
  }

  ret = avformat_find_stream_info(pInFormatCtx, NULL);
  if (ret < 0) {
    FATAL("avformat_find_stream_info()", -1);
  }

  ret = av_find_best_stream(pInFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pDecoderCodec, 0);
  if (ret < 0) {
    FATAL("av_find_best_strem()", -1);
  }
  videoStream = ret;

  pDecoderCtx = avcodec_alloc_context3(pDecoderCodec);
  if (!pDecoderCtx) {
    FATAL("avcodec_alloc_context3()", -1);
  }

  pInStream = pInFormatCtx->streams[videoStream];
  ret = avcodec_parameters_to_context(pDecoderCtx, pInStream->codecpar);
  if (ret < 0) {
    FATAL("avcodec_parameters_to_context()", -1);
  }

  ret = avcodec_open2(pDecoderCtx, pDecoderCodec, NULL);
  if (ret < 0) {
    FATAL("avcodec_open2(decoder)", -1);
  }

  ////////////// Setup Encoder for Output
  pEncoderCodec = avcodec_find_encoder_by_name("libx264");
  if (!pEncoderCodec) {
    FATAL("avcodec_find_encoder_by_name()", -1);
  }

  ret = avformat_alloc_output_context2(&pOutFormatCtx, NULL, NULL, outFile.c_str());
  if (ret < 0) {
    FATAL("avformat_alloc_output_context2()", -1);
  }

  pEncoderCtx = avcodec_alloc_context3(pEncoderCodec);
  if (ret < 0) {
    FATAL("avcodec_alloc_context3()", -1);
  }

  ret = avio_open(&pOutFormatCtx->pb, outFile.c_str(), AVIO_FLAG_WRITE);
  if (ret < 0) {
    FATAL("avio_open()", -1);
  }

  ////////// Initialize encoder context
  // pEncoderCtx->time_base = av_inv_q(pDecoderCtx->framerate);
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

  ret = avformat_write_header(pOutFormatCtx, NULL);
  if (ret < 0) {
    FATAL("avformat_write_header()", -1);
  }

  int FPS = 24;  // need to verify
  int packetDuration = 1001;
  int startDts = packetDuration * FPS * start;
  int endDts = packetDuration * FPS * duration;
  int test = 0;
  LOG("startDts: ", startDts, "\tendDts: ", endDts);
  //  ret = av_seek_frame(pInFormatCtx, videoStream, startDts, 0);
  // if (ret < 0) { FATAL("av_seek_frame()", -1); }

  ////////// Send Thou Frames
  while (ret >= 0) {
    av_init_packet(&decodePacket);
    decodePacket.data = NULL;
    decodePacket.size = 0;

    ret = av_read_frame(pInFormatCtx, &decodePacket);
    if (ret < 0) {
      break;
    }

    if (videoStream == decodePacket.stream_index) {
      LOG("pts: ", decodePacket.pts, "\tdts: ", decodePacket.dts, "\tpos: ", decodePacket.pos,
          "\tdur: ", decodePacket.duration);
      /*      if (decodePacket.dts > startDts && test == 0) {
        test = 1;
        LOG("=======================================");
        ret = av_seek_frame(pInFormatCtx, videoStream, endDts, 0);
        if (ret < 0) { FATAL("av_seek_frame()", -1); }
        av_packet_unref(&decodePacket);
        continue;
      }

      int ret2 = avcodec_send_packet(pDecoderCtx, &decodePacket);
      if (ret2 < 0) { FATAL("avcodec_send_packet()", -1); }

      while (ret2 >= 0) {
        pFrame = av_frame_alloc();
        if (!pFrame) { FATAL("av_frame_alloc()", -1); }

        // Decode frame
        ret2 = avcodec_receive_frame(pDecoderCtx, pFrame);
        if (ret2 == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
          //	  if (ret2 == AVERROR(EAGAIN)) { LOG("EAGAIN"); }
          if (ret2 == AVERROR_EOF) { LOG("AVERROR_EOF"); }
          av_frame_free(&pFrame);
          ret = 0;
          break;
        } else if (ret2 < 0) {
          FATAL("avcodec_receive_frame()", -1);
        }

        // Encode frame

        av_init_packet(&encodePacket);
        encodePacket.data = NULL;
        encodePacket.size = 0;

        int ret3 = avcodec_send_frame(pEncoderCtx, pFrame);
        if (ret3 < 0) { FATAL("avcodec_send_frame()", -1); }

        while (1) {
          ret3 = avcodec_receive_packet(pEncoderCtx, &encodePacket);
          if (ret3 < 0) {
            break;
          }

          encodePacket.stream_index = 0;
          std::cout << "encode: " << encodePacket.pts << "|" << encodePacket.dts << "\t";
          av_packet_rescale_ts(&encodePacket, pInFormatCtx->streams[videoStream]->time_base,
                               pOutFormatCtx->streams[0]->time_base);
          std::cout << "encode: " << encodePacket.pts << "|" << encodePacket.dts << "\n";
          ret3 = av_interleaved_write_frame(pOutFormatCtx, &encodePacket);
          //ret3 = av_write_frame(pOutFormatCtx, &encodePacket);
          if (ret3 < 0) { FATAL("av_interleaved_write_frame", -1); }
        }

        // clean up these ret for errors
        if (ret3 == AVERROR_EOF) {
          ret = 0;
        }

        av_frame_free(&pFrame);
      } // while (ret2 >= 0)
      */
      decodePacket.flags |= AV_PKT_FLAG_KEY;
      av_packet_rescale_ts(&decodePacket, pInFormatCtx->streams[videoStream]->time_base,
                           pOutFormatCtx->streams[0]->time_base);
      // decodePacket.stream_index = 0;
      ret = av_interleaved_write_frame(pOutFormatCtx, &decodePacket);
      if (ret < 0) {
        FATAL("av_interleaved_write_frame", -1);
      }
    }  // if (videoStream)

    av_packet_unref(&decodePacket);
  }  // while (ret >= 0)

  // Flush decoder
  decodePacket.data = NULL;
  decodePacket.size = 0;
  // TODO
  av_packet_unref(&decodePacket);

  // flush encoder

  av_write_trailer(pOutFormatCtx);

  avformat_close_input(&pInFormatCtx);
  avformat_close_input(&pOutFormatCtx);
  avcodec_free_context(&pDecoderCtx);
  avcodec_free_context(&pEncoderCtx);
}

AVFrame *AV::dummyFrame(OutputStream *ost) {
  /*
  AVCodecContext *c = ost->stream->codec;

  // check if we want to generate more frames
  if (av_compare_ts(ost->next_pts, ost->stream->codec->time_base,
                    10.0f, (AVRational){ 1, 1 }) >= 0) {
    return NULL;
  }

  {
    // fill with fake data
    int x, y, i, ret;

    ret = av_frame_make_writable(ost->frame);
    if (ret < 0)
      exit(1);

    i = ost->next_pts;

    // Y
    for (y = 0; y < c->height; y++)
      for (x = 0; x < c->width; x++)
        ost->frame->data[0][y * ost->frame->linesize[0] + x] = x + y + i * 3;

    // Cb and Cr
    for (y = 0; y < c->height / 2; y++) {
      for (x = 0; x < c->width / 2; x++) {
        ost->frame->data[1][y * ost->frame->linesize[1] + x] = 128 + y + i * 2;
        ost->frame->data[2][y * ost->frame->linesize[2] + x] = 64 + x + i * 5;
      }
    }
  }


  ost->frame->pts = ost->next_pts++;
  */
  return ost->frame;
}
