#include "AV.hpp"
#include "Debug.hpp"

AV::AV(std::string fileName) : mpFormatContext(NULL) {
  av_register_all();  // Initialize FFmpeg

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

  if (!cdc) {
    FATAL("Codec couldn't be opened", -1);
  }

  LOG("CODEC_TYPE ", mpFormatContext->streams[streamIndex]->codec->codec_type);
  LOG("STRING VERSION: ",
      av_get_media_type_string(mpFormatContext->streams[streamIndex]->codec->codec_type));

  mpAudioStream = mpFormatContext->streams[streamIndex];
  mpCodecContext = mpAudioStream->codec;
  mpCodecContext->codec = cdc;

  if (avcodec_open2(mpCodecContext, cdc, NULL) < 0) {
    av_free(mpFrame);
    avformat_close_input(&mpFormatContext);
    FATAL("Couldn't open the context with the decoder", -1);
  }

  mSamplingRate = mpCodecContext->sample_rate;
  mNumSamples = 0;
}

void AV::PrintAVInfo() {
  LOG("This stream has ", mpCodecContext->channels, " channels and a sample rate of ",
      mpCodecContext->sample_rate, "Hz");
  LOG("The data is in the format ", av_get_sample_fmt_name(mpCodecContext->sample_fmt));
  LOG("type again: ", av_get_media_type_string(mpCodecContext->codec_type));
  LOG("There are this many samples ", mNumSamples);
}

std::vector<float> AV::ReadPackets() {
  AVPacket readingPacket;
  AVPacket decodingPacket;
  av_init_packet(&readingPacket);
  av_init_packet(&decodingPacket);
  int numSamples;
  std::vector<float> allSamples;
  FILE* outFile = fopen("./data/processed_samples.txt", "wb");

  // Read the packets in a loop
  while (av_read_frame(mpFormatContext, &readingPacket) == 0) {
    if (readingPacket.stream_index == mpAudioStream->index) {
      // readingPacket;
      av_packet_ref(&decodingPacket, &readingPacket);

      // Get the data size per element in the stream
      int dataSize = av_get_bytes_per_sample(mpCodecContext->sample_fmt);

      // Audio packets can have multiple audio frames in a single packet
      while (decodingPacket.size > 0) {
        // std::cout << "DECODING_SIZE " << decodingPacket.size << std::endl;
        // Try to decode the packet into a frame
        // Some frames rely on multiple packets, so we have to make sure the frame is finished
        // before
        // we can use it
        int gotFrame = 0;
        int result = avcodec_decode_audio4(mpCodecContext, mpFrame, &gotFrame, &decodingPacket);
        if (result >= 0 && gotFrame) {
          decodingPacket.size -= result;
          decodingPacket.data += result;
          numSamples += mpFrame->nb_samples;
          // Write float32 data for the channel
          for (int jj = 0; jj < mpFrame->nb_samples; jj++) {
            fwrite(mpFrame->data[0] + jj * dataSize, dataSize, 1, outFile);
            float sample = *((float*)(mpFrame->data[0] + jj * dataSize));
            allSamples.push_back(sample);
          }

          // We now have a fully decoded audio frame
        } else {
          decodingPacket.size = 0;
          decodingPacket.data = nullptr;
        }
      }
    }

    // You *must* call av_free_packet() after each call to av_read_frame() or else you'll leak
    // memory
    // av_free_packet(&readingPacket); DEPRECATED
    av_packet_unref(&readingPacket);
    av_packet_unref(&decodingPacket);
  }

  // Some codecs will cause frames to be buffered up in the decoding process. If the CODEC_CAP_DELAY
  // flag
  // is set, there can be buffered up frames that need to be flushed, so we'll do that
  if (mpCodecContext->codec->capabilities /*& CODEC_CAP_DELAY*/) {
    av_init_packet(&readingPacket);
    // Decode all the remaining frames in the buffer, until the end is reached
    int gotFrame = 0;
    while (avcodec_decode_audio4(mpCodecContext, mpFrame, &gotFrame, &readingPacket) >= 0 &&
           gotFrame) {
      // We now have a fully decoded audio frame
    }
  }
  mNumSamples = allSamples.size();
  return allSamples;
}

AV::~AV() {
  av_free(mpFrame);
  avcodec_close(mpCodecContext);
  avformat_close_input(&mpFormatContext);
}
