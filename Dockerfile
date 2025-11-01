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

# Copy multi-instance entrypoint scripts (Phase 4.5b - ODR Multi-Instance)
COPY entrypoint-multi.sh ./
COPY entrypoint-multi-with-health.sh ./

# Set up runtime environment with proper user creation
ARG BUILD_UID=1000
ARG BUILD_GID=1000
RUN groupadd -g ${BUILD_GID} streamdab && \
    useradd -m -u ${BUILD_UID} -g streamdab -s /bin/bash -d /home/streamdab streamdab && \
    mkdir -p /home/streamdab/logs /var/run /tmp && \
    chmod 777 /var/run /tmp && \
    chmod +x /app/entrypoint*.sh && \
    chown -R streamdab:streamdab /app /home/streamdab

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
