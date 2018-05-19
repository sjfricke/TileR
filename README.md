# TileR

[Style Guide](STYLE.md)

Clang format to style code
`clang-format -i -style=Google ./src/*.cpp ./include/*.hpp`

## Dependices

### Linux
```
sudo apt update
sudo apt install libavcodec-dev libavformat-dev libavutil-dev
```

### Mac
- Get FFMPEG installed and map libraries in CMake

### Windows
- [Instructions here](http://lmgtfy.com/?q=how+to+replace+windows+with+linux)

## Build

```
cmake .
make
```

## Run

```
./bin/TileR
```
