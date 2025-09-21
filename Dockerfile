# StreamDAB Enhanced ODR-PadEnc Docker Image
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Bangkok

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    wget \
    curl \
    autotools-dev \
    autoconf \
    automake \
    libtool \
    libssl-dev \
    libfftw3-dev \
    libzmq3-dev \
    libcurl4-openssl-dev \
    libmagick++-dev \
    libwebp-dev \
    libheif-dev \
    libicu-dev \
    libutfcpp-dev \
    google-mock \
    libgtest-dev \
    lcov \
    gcc \
    valgrind \
    && rm -rf /var/lib/apt/lists/*

# Build and install Google Test
RUN cd /usr/src/gtest && \
    cmake . && \
    make && \
    cp lib/*.a /usr/lib/ && \
    cd /usr/src/gmock && \
    cmake . && \
    make && \
    cp lib/*.a /usr/lib/

# Create application directory
WORKDIR /app

# Copy source code
COPY src/ ./src/
COPY tests/ ./tests/
COPY test-data/ ./test-data/
COPY CMakeLists.txt ./
COPY README.md ./

# Build the application
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. && \
    make -j$(nproc)

# Run tests and generate coverage report
RUN cd build && \
    ./padenc_tests --gtest_output=xml:test_results.xml || true && \
    make coverage || true

# Set up runtime environment
RUN useradd -m -u 1000 streamdab && \
    chown -R streamdab:streamdab /app

USER streamdab

# Expose StreamDAB API port
EXPOSE 8008

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD curl -f http://localhost:8008/api/health || exit 1

# Default command
CMD ["./build/odr-padenc", "--help"]
