# Use Ubuntu 22.04 as base image
FROM ubuntu:22.04

# Set environment variables to non-interactive to avoid prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    wget \
    curl \
    git \
    unzip \
    tar \
    libssl-dev \
    autoconf \
    automake \
    libtool \
    make \
    g++ \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /tmp

# Install CMake 3.20.4
RUN wget https://github.com/Kitware/CMake/releases/download/v3.20.4/cmake-3.20.4.tar.gz \
    && tar -xzf cmake-3.20.4.tar.gz \
    && cd cmake-3.20.4 \
    && ./configure \
    && make -j$(nproc) \
    && make install \
    && cd .. \
    && rm -rf cmake-3.20.4*

# Install Protobuf 3.6.1
RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v3.6.1/protobuf-all-3.6.1.tar.gz \
    && tar -xzf protobuf-all-3.6.1.tar.gz \
    && cd protobuf-3.6.1 \
    && ./configure \
    && make -j$(nproc) \
    && make install \
    && ldconfig \
    && cd .. \
    && rm -rf protobuf-3.6.1*

# Verify installations
RUN cmake --version && protoc --version

# Set working directory for the project
WORKDIR /workspaces/ad_data_closed_loop

# Default command
CMD ["/bin/bash"]