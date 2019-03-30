#include <sys/stat.h>   // mkdir check
#include <sys/types.h>  // mkdir
#include <vector>
#include <string>
#include "utils.hpp"
#include "source.hpp"

extern "C" {
#include "libavcodec/version.h"
#include "libavformat/version.h"
#include "libavutil/version.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


int main(int argc, char *argv[]) {
  LOG("libavcodec V.", LIBAVCODEC_VERSION_MAJOR, ".", LIBAVCODEC_VERSION_MINOR, ".",
      LIBAVCODEC_VERSION_MICRO);
  LOG("libavformat V.", LIBAVFORMAT_VERSION_MAJOR, ".", LIBAVFORMAT_VERSION_MINOR, ".",
      LIBAVFORMAT_VERSION_MICRO);
  LOG("libavutil V.", LIBAVUTIL_VERSION_MAJOR, ".", LIBAVUTIL_VERSION_MINOR, ".",
      LIBAVUTIL_VERSION_MICRO);

  // By default ffmpeg will not know to look for mp4
  // avformat_open_input will return Invalid data found from input
  av_register_all();
  
  std::string video_0("./data/video/source_0.mp4");
  std::string video_1("./data/video/source_1.mp4");
  uint32_t input_source_size = 2;

  Source** input_source = (Source**)malloc(sizeof(Source*) * input_source_size);
  
  input_source[0] = new Source(video_0);
  input_source[1] = new Source(video_1);
  
  //string inputVideoPath{"./data/video/"};
  // std::string inputSongPath{"./data/song/smash_mouth.wav"};
  std::string out_file{"./output/out.mp4"};
 
	    
  // need to make sure output folder is there
  struct stat st = {0};
  if (stat("./output", &st) == -1) {
    mkdir("./output", 0777);
  }

  for (uint32_t i = 0; i < input_source_size; i++) {
    delete input_source[i];
  }
  free(input_source);
  
  return 0;
}
