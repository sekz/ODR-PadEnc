# StreamDAB Enhanced ODR-PadEnc Docker Image
FROM ubuntu:22.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Asia/Bangkok \
    LANG=en_US.UTF-8 \
    LC_ALL=en_US.UTF-8 \
    LANGUAGE=en_US:en

# Install system dependencies
RUN apt-get update && apt-get install --no-install-recommends -y \
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
    lcov \
    gcc \
    valgrind \
    locales \
    procps \
    && rm -rf /var/lib/apt/lists/* \
    && locale-gen en_US.UTF-8 th_TH.UTF-8 \
    && update-locale LANG=en_US.UTF-8

# Note: Google Test will be downloaded by CMake FetchContent during build
# This eliminates the problematic manual Google Test/GMock build

# Create application directory
WORKDIR /app

# Copy source code
COPY src/ ./src/
COPY tests/ ./tests/
COPY test-data/ ./test-data/
COPY CMakeLists.txt ./

# Build the application
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Debug .. && \
    make -j$(nproc)

# Run tests and generate coverage report
RUN cd build && \
    ./padenc_tests --gtest_output=xml:test_results.xml || true && \
    make coverage || true

# Copy entrypoint script AFTER build (fixes chmod order issue)
COPY entrypoint.sh ./

# Set up runtime environment
RUN useradd -m -u 1000 streamdab && \
    chmod +x /app/entrypoint.sh && \
    chown -R streamdab:streamdab /app

USER streamdab

# Expose StreamDAB API port
EXPOSE 8008

# Health check - ODR-PadEnc runs as a service, check process is running
# Use pgrep -x to match exact process name (not partial matches)
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD pgrep -x odr-padenc > /dev/null || exit 1

# Use entrypoint script with default configuration
ENTRYPOINT ["/app/entrypoint.sh"]
CMD []
