#include "Debug.hpp"
#include "AV.hpp"

int main(int argc, char *argv[]) {
  LOG("Hello ", "From ", 342);
  AV* source = new AV("../data/video0.mp4");

  delete source;
  return 0;
}
