FROM ubuntu:26.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y software-properties-common
RUN add-apt-repository ppa:dotnet/backports
RUN apt-get install -y \
    git dotnet-sdk-8.0 clang cmake portaudio19-dev git-lfs libjpeg62 libwebp7 libpng16-16 libtiff6 libopenexr-3-1-30 libdc1394-25 libgtk-3-0 libyaml-cpp-dev