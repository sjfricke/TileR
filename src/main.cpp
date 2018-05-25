#include <sys/types.h> // mkdir
#include <sys/stat.h> // mkdir check
#include <unistd.h>  // access(2)
#include <vector>
#include "ASource.hpp"
#include "AV.hpp"
#include "Debug.hpp"
#include "SampleBlock.hpp"

int menuPrint() {
  LOG("\n******** Main Menu ********");
  LOG("\t1) Add new AV Source");
  LOG("\t2) Add new Song");
  LOG("\t3) Output merged sources");
  LOG("\t4) Print AV Source status and info");
  LOG("\t5) Print Song status and info");
  LOG("\t6) Suprise me mothur fuker!");
  LOG("\t69) EXIT");
  std::cout << "\nEnter Menu: ";

  int input;
  std::cin >> input;
  std::cout << std::endl;  // new line
  return input;
}

int main(int argc, char *argv[]) {
  LOG("libavcodec V.", LIBAVCODEC_VERSION_MAJOR, ".", LIBAVCODEC_VERSION_MINOR, ".",
      LIBAVCODEC_VERSION_MICRO);
  LOG("libavformat V.", LIBAVFORMAT_VERSION_MAJOR, ".", LIBAVFORMAT_VERSION_MINOR, ".",
      LIBAVFORMAT_VERSION_MICRO);

  bool exitProgram = false;
  int indie = 0;
  std::vector<ASource<float> *> sources;
  std::string fileName{"./data/video0.mp4"};
  std::string outFile{"./test.mp4"};

  while (!exitProgram) {
    int input = menuPrint();

    switch (input) {
      case 1:  // add AV Source
        while (1) {
          std::cout << "Enter AV Source path: ";
          // std::cin >> fileName;
          if (access(fileName.c_str(), F_OK) == -1) {
            LOG("... file is fake news, try again");
          } else {
            break;
          }
        }
        sources.push_back(new ASource<float>(44100, fileName));
        break;
      case 2:  // add Song
        break;
      case 3:  // Merge source to output

	std::cout << "Enter output path: ";
	// std::cin >> outFile;

	{
	  // need to make sure output folder is there
	  struct stat st = {0};
	  if (stat("./output", &st) == -1) {
	    mkdir("./output", 0777);
	  }

	  AV testAV("./data/video0.mp4");
	  testAV.Stich("output/test.mp4", 0, 0);
	}

        break;
      case 4:  // AV info
        indie = 0;
        for (auto &source : sources) {
          LOG("SOURCE ", indie);
          indie++;
          source->PrintInfo();
        }

        break;
      case 5:  // Song info
        break;
      case 6:  // Suprise
        LOG("P = 1");
        LOG("N = NP");
        LOG("(ﾉﾟ0ﾟ)ﾉ~\n");
        break;
      case 69:
        LOG("Ha, gayyyyyyyyyyyy");
        exitProgram = true;
        break;
      default:
        LOG("\n... bruh, ", input, " wasn't a fuken option now was it, timing you out a$$hole");
        sleep(2);
        LOG("... lets try this again");
    }
  }

  // clean up sources (for if/when we migrate code in future)
  for (auto &source : sources) {
    delete source;
  }
  return 0;
}
