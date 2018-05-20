#include "AV.hpp"
#include "Debug.hpp"

AV::AV(std::string fileName)  {
  mInputFiles.push_back(fileName);
}

AV::~AV() {
}

void AV::PrintAVInfo() {
}

void AV::AddInputSource(std::string fileName) {
    mInputFiles.push_back(fileName);
}

void AV::Stich(std::string outFile) {
  OutputStream videoStream;
  AVOutputFormat *outFormat;
  AVFormatContext * formatContext;
  AVCodec *videoCodec;
  int ret;
  AVDictionary *opt;

  avformat_alloc_output_context2(&formatContext, NULL, NULL, outFile.c_str());
  if (!formatContext) {
    FATAL("Could not deduce output format from file extension", -1);
  }

  outFormat = formatContext->oformat;

  if (outFormat->video_codec == AV_CODEC_ID_NONE) {
    FATAL("No video support in codec, why you so bad bruh", -2);
  }

  ///////
  // ADD Stream
  AVCodecContext* codecContext;
  // find the encoder

  videoCodec = avcodec_find_encoder(outFormat->video_codec);
  if (!videoCodec) {
    FATAL("Could not find encoder for ", outFormat->video_codec);
  }

  videoStream.stream = avformat_new_stream(formatContext, videoCodec);
  if (!videoStream.stream) {
    FATAL("Could not allocate stream ", -1);
  }

  videoStream.stream->id = formatContext->nb_streams-1;
  codecContext = videoStream.stream->codec;

  codecContext->codec_id = outFormat->video_codec;

  codecContext->bit_rate = 400000;
  // Resolution must be a multiple of two.
  codecContext->width = 720;
  codecContext->height = 480;
  videoStream.stream->time_base = (AVRational){ 1, 25 }; // 30 Frame Rate (FPS)
  codecContext->time_base = videoStream.stream->time_base;

  codecContext->gop_size = 12; // emit one intra frame every twelve frames at most
  codecContext->pix_fmt = AV_PIX_FMT_YUV420P;

  // Some formats want stream headers to be separate.
  if (outFormat->flags & AVFMT_GLOBALHEADER) {
    codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  ////////
  // Open Video and allocate
  LOG("WTF: \n", codecContext, "\n  ",  videoCodec, "\n  ", opt, "\n  ");
  ret = avcodec_open2(codecContext, videoCodec, &opt);

  av_dict_free(&opt);

  if (ret < 0) {
    FATAL("Could not open video codec: ", ret);
  }

  AVFrame* picture = av_frame_alloc();
  if (!picture) {
    FATAL("Could not allocate video frame", -1);
  }

  picture->format = codecContext->pix_fmt;
  picture->width = codecContext->width;
  picture->height = codecContext->height;

  if (av_frame_get_buffer(picture, 32) < 0) {
    FATAL("Could not allocate frame data", -1);
  }

  videoStream.frame = picture;

  // idk what this does
  av_dump_format(formatContext, 0, outFile.c_str(), 1);

  if (avio_open(&formatContext->pb, outFile.c_str(), AVIO_FLAG_WRITE) < 0) {
    FATAL("Could not open outFile: ", ret);
  }

  if (avformat_write_header(formatContext, &opt) < 0) {
    FATAL("Error occurred when opening output file: ", -1);
  }

  //////////
  // Fill with data

  AVFrame* frame;
  AVPacket packet = {0};
  int gotPacket;
  while (1) {
    frame = dummyFrame(&videoStream);
    av_init_packet(&packet);
    if (avcodec_encode_video2(codecContext, &packet, frame, &gotPacket) < 0) {
      FATAL("Error encoding video frame", -1);
    }
    av_packet_rescale_ts(&packet, codecContext->time_base, videoStream.stream->time_base);
    packet.stream_index = videoStream.stream->index;

    if (gotPacket) {
      ret = av_interleaved_write_frame(formatContext, &packet);
    } else {
      ret = 0;
    }

    if (ret < 0) {
      LOG("FAIL: ", aav_err2str(ret));
      FATAL("Error while writing to video frame" , -1);
    }

    if (frame || gotPacket) {
      continue;
    } else {
      break;
    }
  }

  av_write_trailer(formatContext);

  ////////
  // Close stream
  avcodec_close(videoStream.stream->codec);
  av_frame_free(&videoStream.frame);

  avio_closep(&formatContext->pb);
  avformat_free_context(formatContext);
}


AVFrame *AV::dummyFrame(OutputStream *ost)
{
  AVCodecContext *c = ost->stream->codec;

  /* check if we want to generate more frames */
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

    /* Y */
    for (y = 0; y < c->height; y++)
      for (x = 0; x < c->width; x++)
	ost->frame->data[0][y * ost->frame->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < c->height / 2; y++) {
      for (x = 0; x < c->width / 2; x++) {
	ost->frame->data[1][y * ost->frame->linesize[1] + x] = 128 + y + i * 2;
	ost->frame->data[2][y * ost->frame->linesize[2] + x] = 64 + x + i * 5;
      }
    }
  }


  ost->frame->pts = ost->next_pts++;

  return ost->frame;
}
