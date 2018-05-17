#include "AV.hpp"
#include "Debug.hpp"

void AV::printAudioFrameInfo(const AVCodecContext* codecContext, const AVFrame* frame)
{
  // See the following to know what data type (unsigned char, short, float, etc) to use to access the audio data:
  // http://ffmpeg.org/doxygen/trunk/samplefmt_8h.html#af9a51ca15301871723577c730b5865c5
  std::cout << "Audio frame info:\n"
	    << "  Sample count: " << frame->nb_samples << '\t'
	    << "  Channel count: " << codecContext->channels << '\t'
	    << "  Format: " << av_get_sample_fmt_name(codecContext->sample_fmt) << '\t'
	    << "  Bytes per sample: " << av_get_bytes_per_sample(codecContext->sample_fmt) << '\t'
	    << "  Is planar? " << av_sample_fmt_is_planar(codecContext->sample_fmt) << '\n';

  /*
  std::cout << "frame->linesize[0] tells you the size (in bytes) of each plane\n";

  if (codecContext->channels > AV_NUM_DATA_POINTERS && av_sample_fmt_is_planar(codecContext->sample_fmt)) {
    std::cout << "The audio stream (and its frames) have too many channels to fit in\n"
	      << "frame->data. Therefore, to access the audio data, you need to use\n"
	      << "frame->extended_data to access the audio data. It's planar, so\n"
	      << "each channel is in a different element. That is:\n"
	      << "  frame->extended_data[0] has the data for channel 1\n"
	      << "  frame->extended_data[1] has the data for channel 2\n"
	      << "  etc.\n";
  } else  {
    std::cout << "Either the audio data is not planar, or there is enough room in\n"
	      << "frame->data to store all the channels, so you can either use\n"
	      << "frame->data or frame->extended_data to access the audio data (they\n"
	      << "should just point to the same data).\n";
  }

  std::cout << "If the frame is planar, each channel is in a different element.\n"
	    << "That is:\n"
	    << "  frame->data[0]/frame->extended_data[0] has the data for channel 1\n"
	    << "  frame->data[1]/frame->extended_data[1] has the data for channel 2\n"
	    << "  etc.\n";

  std::cout << "If the frame is packed (not planar), then all the data is in\n"
	    << "frame->data[0]/frame->extended_data[0] (kind of like how some\n"
	    << "image formats have RGB pixels packed together, rather than storing\n"
	    << " the red, green, and blue channels separately in different arrays.\n";
  */
}


AV::AV(std::string fileName)
  : mpFormatContext(NULL)
{
  av_register_all(); // Initialize FFmpeg

  mpFrame = av_frame_alloc();
  if (!mpFrame) {
    FATAL("Error allocating the frame", -1);
  }


  if (avformat_open_input(&mpFormatContext, fileName.c_str(), NULL, NULL) != 0) {
    av_free(mpFrame);
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

  mpAudioStream = mpFormatContext->streams[streamIndex];
  mpCodecContext = mpAudioStream->codec;
  mpCodecContext->codec = cdc;

  if (avcodec_open2(mpCodecContext, mpCodecContext->codec, NULL) != 0) {
    av_free(mpFrame);
    avformat_close_input(&mpFormatContext);
    FATAL("Couldn't open the context with the decoder", -1);
  }
}

void AV::PrintAVInfo()
{
  LOG("This stream has ", mpCodecContext->channels, " channels and a sample rate of ", mpCodecContext->sample_rate, "Hz");
  LOG("The data is in the format ", av_get_sample_fmt_name(mpCodecContext->sample_fmt));
}

void AV::ReadPackets(bool verbose)
{

  AVPacket readingPacket;
  av_init_packet(&readingPacket);

  // Read the packets in a loop
  while (av_read_frame(mpFormatContext, &readingPacket) == 0) {
    if (readingPacket.stream_index == mpAudioStream->index) {
      AVPacket decodingPacket = readingPacket;

      if (verbose) {
	LOG(decodingPacket.size, " decode packets\n");
      }

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

	  // We now have a fully decoded audio frame
	  if (verbose) {
	    printAudioFrameInfo(mpCodecContext, mpFrame);
	  }
	} else {
	  decodingPacket.size = 0;
	  decodingPacket.data = nullptr;
	}
      }
    }

    // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak memory
    av_free_packet(&readingPacket);
  }

  // Some codecs will cause frames to be buffered up in the decoding process. If the CODEC_CAP_DELAY flag
  // is set, there can be buffered up frames that need to be flushed, so we'll do that
  if (mpCodecContext->codec->capabilities & CODEC_CAP_DELAY) {
    av_init_packet(&readingPacket);
    // Decode all the remaining frames in the buffer, until the end is reached
    int gotFrame = 0;
    while (avcodec_decode_audio4(mpCodecContext, mpFrame, &gotFrame, &readingPacket) >= 0 && gotFrame) {
      // We now have a fully decoded audio frame
      if (verbose) {
	printAudioFrameInfo(mpCodecContext, mpFrame);
      }
    }
  }
}

AV::~AV() {
  av_free(mpFrame);
  avcodec_close(mpCodecContext);
  avformat_close_input(&mpFormatContext);
}
