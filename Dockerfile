FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
      wget \
      curl \
      gnupg \
      ca-certificates \
      lsb-release \
      software-properties-common \
      sudo \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh 21 \
    && rm llvm.sh

RUN apt-get update && apt-get install -y \
      ninja-build \
      libc++-21-dev \
      libc++abi-21-dev \
      clang-format-21 \
      clang-tidy-21 \
    && rm -rf /var/lib/apt/lists/*


ARG CMAKE_VERSION=4.3.1
RUN curl -L https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz -o /tmp/cmake.tar.gz \
    && tar -xzf /tmp/cmake.tar.gz -C /opt \
    && rm /tmp/cmake.tar.gz
ENV PATH="/opt/cmake-${CMAKE_VERSION}-linux-x86_64/bin:${PATH}"

RUN clang++-21 --version && cmake --version && ninja --version

WORKDIR /workspace
CMD ["bash"]
