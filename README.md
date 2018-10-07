# TileR

[Style Guide](STYLE.md)

Clang format to style code
`clang-format -i -style=file ./src/*.cpp ./include/*.hpp`

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

# Helpful stuff

- Debugging libav `av_log_set_level(AV_LOG_DEBUG);`
-
```
char libErr[256];
av_strerror(ret, libErr, 256);
fprintf(stderr, "%s\n", libErr);
```