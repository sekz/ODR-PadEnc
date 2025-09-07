# Enhanced ODR-PadEnc for Thailand DAB+

Advanced MOT SlideShow and DLS processing with Thai language support, WebP/HEIF format support, and comprehensive StreamDAB integration.

## Features

### Advanced MOT SlideShow
- **Enhanced Image Processing**: WebP and HEIF/HEIC format support
- **Smart Image Carousel**: Content-aware selection with quality analysis
- **Progressive JPEG**: Optimized encoding for DAB+ transmission
- **Duplicate Detection**: Automatic duplicate image filtering
- **Quality Optimization**: Advanced algorithms for DAB display optimization

### Thai Language Support
- **UTF-8 to DAB Profile**: Complete ETSI TS 101 756 Thai character set conversion
- **Cultural Features**: Buddhist calendar integration and Thai holiday recognition
- **Text Rendering**: Optimized Thai font rendering for DAB displays
- **Content Validation**: Cultural appropriateness checking for Thai content
- **Traditional Numbering**: Support for Thai number formatting

### Enhanced DLS Processing
- **Priority Queuing**: Emergency, high, normal, low, and background priority levels
- **Smart Optimization**: Dynamic message length optimization with Thai language support
- **Context-Aware Selection**: Message selection based on broadcast context
- **Rich Metadata**: Integration with social media, RSS feeds, and external APIs
- **Anti-Spam Protection**: Duplicate detection and content filtering

### Security & Performance
- **Path Traversal Protection**: Secure file system access validation
- **Content Scanning**: Malicious content detection and sanitization
- **Memory Safety**: Secure memory management with leak detection
- **Multi-Threading**: Optimized concurrent processing
- **Performance Monitoring**: Real-time performance metrics and optimization

### StreamDAB Integration
- **RESTful API**: Complete HTTP API on port 8008
- **WebSocket Support**: Real-time updates with MessagePack protocol
- **Content Management**: Integration with StreamDAB-ContentManager
- **Emergency Override**: Instant emergency broadcast capability
- **ETSI Compliance**: Full ETSI standards validation and reporting

## Requirements

### System Requirements
- Ubuntu 20.04+ or compatible Linux distribution
- C++17 compatible compiler (GCC 9+, Clang 10+)
- CMake 3.14+
- 4GB+ RAM for compilation
- 1GB+ disk space

### Dependencies
- **ImageMagick++**: Image processing with WebP/HEIF support
- **libwebp-dev**: WebP format support
- **libheif-dev**: HEIF/HEIC format support (optional)
- **libssl-dev**: Cryptographic functions
- **libzmq3-dev**: ZeroMQ messaging
- **libcurl4-openssl-dev**: HTTP client support
- **Google Test**: Unit testing framework
- **lcov/gcov**: Code coverage analysis

## Building

### Using CMake (Recommended)

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config \
    libssl-dev libfftw3-dev libzmq3-dev libcurl4-openssl-dev \
    libmagick++-dev libwebp-dev libheif-dev libicu-dev \
    libgtest-dev lcov gcov

# Clone and build
git clone https://github.com/streamdab/ODR-PadEnc.git
cd ODR-PadEnc

# Create build directory
mkdir build && cd build

# Configure with coverage support
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Build
make -j$(nproc)

# Run tests
make test

# Generate coverage report
make coverage
```

### Using Docker

```bash
# Build Docker image
docker build -t streamdab/odr-padenc .

# Run with port mapping
docker run -p 8008:8008 -v /path/to/images:/app/images streamdab/odr-padenc

# Run tests in container
docker run streamdab/odr-padenc ./build/padenc_tests
```

## Testing

### Test Coverage Requirements
- **Minimum Coverage**: 80% code coverage required
- **Image Format Testing**: WebP, HEIF, JPEG, PNG validation
- **Thai Language Testing**: UTF-8 conversion and cultural content validation
- **Security Testing**: Path traversal, content injection, memory safety
- **Performance Testing**: Multi-threading, memory optimization, throughput

### Running Tests

```bash
# Run all tests
cd build
./padenc_tests

# Run specific test suites
./padenc_tests --gtest_filter="MOTSlideshowTest.*"
./padenc_tests --gtest_filter="ThaiRenderingTest.*"
./padenc_tests --gtest_filter="DLSProcessingTest.*"
./padenc_tests --gtest_filter="SecurityTest.*"
./padenc_tests --gtest_filter="PerformanceTest.*"

# Generate detailed test report
./padenc_tests --gtest_output=xml:test_results.xml

# Coverage report
make coverage
open coverage/index.html
```

## Configuration

### Basic Configuration
```ini
# streamdab-padenc.conf
[mot]
image_directory = /var/lib/streamdab/images
max_images = 50
quality_threshold = 0.7
enable_webp = true
enable_heif = true

[dls]
max_message_length = 128
enable_thai_processing = true
enable_cultural_validation = true
emergency_interval = 3
normal_interval = 12

[api]
port = 8008
enable_websocket = true
max_connections = 100
enable_ssl = false

[thai]
enable_buddhist_calendar = true
enable_cultural_validation = true
default_font = /usr/share/fonts/thai/Norasi.ttf
```

### Thai Language Configuration
```json
{
  "thai_processing": {
    "character_set": "ETSI_TS_101_756",
    "font_rendering": {
      "line_height": 16,
      "char_spacing": 1,
      "enable_complex_layout": true
    },
    "cultural_validation": {
      "enable_royal_content_check": true,
      "enable_religious_content_check": true,
      "sensitivity_threshold": 0.8
    },
    "buddhist_calendar": {
      "enable_holiday_detection": true,
      "enable_holy_day_detection": true,
      "default_era": "buddhist"
    }
  }
}
```

## API Documentation

### REST Endpoints

#### Status and Health
- `GET /api/status` - Get system status
- `GET /api/health` - Health check endpoint
- `GET /api/statistics` - Performance statistics

#### Image Management
- `GET /api/images` - List active images
- `POST /api/images` - Add new image
- `DELETE /api/images/{id}` - Remove image

#### Message Management
- `GET /api/messages` - List queued messages
- `POST /api/messages` - Add new message
- `PUT /api/messages/{id}` - Update message

#### Thai Language Support
- `POST /api/thai/validate` - Validate Thai content
- `POST /api/thai/convert` - Convert UTF-8 to DAB
- `GET /api/thai/calendar` - Get Buddhist calendar info

#### Emergency Management
- `POST /api/emergency` - Trigger emergency broadcast
- `DELETE /api/emergency` - Clear emergency mode

### WebSocket Events

#### Status Updates (MessagePack)
```json
{
  "type": "STATUS_UPDATE",
  "timestamp": "2024-01-15T10:30:00Z",
  "data": {
    "is_running": true,
    "active_images": 25,
    "queued_messages": 5,
    "current_message": "Now playing: Artist - Song Title"
  }
}
```

#### Emergency Alerts
```json
{
  "type": "EMERGENCY_ALERT",
  "timestamp": "2024-01-15T10:30:00Z",
  "requires_ack": true,
  "data": {
    "message": "Emergency Alert: Severe weather warning",
    "duration": 300,
    "priority": "EMERGENCY"
  }
}
```

## Performance Benchmarks

### Target Performance Metrics
- **Image Processing**: >100 images/second
- **Thai Text Conversion**: >500 conversions/second  
- **DLS Message Processing**: >1000 messages/second
- **API Response Time**: <200ms (95th percentile)
- **WebSocket Latency**: <50ms
- **Memory Usage**: <256MB base + 10MB per 100 images

### Optimization Features
- **Multi-threaded Processing**: Concurrent image and text processing
- **Smart Caching**: Intelligent content caching and preloading
- **Memory Pooling**: Efficient memory allocation and reuse
- **Background Processing**: Non-blocking content optimization
- **Load Balancing**: Automatic workload distribution

## ETSI Compliance

### Supported Standards
- **ETSI EN 300 401**: Core DAB Standard compliance
- **ETSI TS 101 499**: MOT SlideShow User Application
- **ETSI TS 101 756**: Character Sets (Thai Profile 0x0E)
- **ETSI TS 102 563**: DAB+ Audio Coding integration
- **ETSI TS 102 818**: Service Programme Information

### Validation Features
- **Real-time Compliance Monitoring**: Continuous ETSI standards validation
- **Automated Reporting**: Compliance violation detection and reporting
- **Thai Character Set Validation**: Complete ETSI TS 101 756 compliance
- **MOT Object Validation**: ETSI-compliant MOT object generation
- **DLS Message Validation**: Proper DLS structure and encoding

## Cultural Content Support

### Thai Language Features
- **Complete Character Support**: All Thai Unicode characters
- **Proper Text Layout**: Complex script rendering with tone marks
- **Cultural Context**: Buddhist calendar, holidays, cultural validation
- **Royal Language Protocol**: Proper handling of royal references
- **Religious Content**: Appropriate Buddhist terminology handling

### Buddhist Calendar Integration
- **Holiday Detection**: Automatic Thai national and religious holiday detection
- **Era Conversion**: Buddhist Era (BE) to Common Era (CE) conversion
- **Holy Day Calculations**: Accurate Buddhist observance day calculations
- **Cultural Events**: Integration with Thai cultural calendar

## Security Features

### Input Validation
- **Path Traversal Protection**: Secure file system access
- **Content Scanning**: Malicious content detection
- **Input Sanitization**: XSS and injection attack prevention
- **File Format Validation**: Secure image format verification
- **Buffer Overflow Protection**: Memory-safe operations

### Content Security
- **Digital Signatures**: Content integrity verification
- **Encryption Support**: TLS 1.3 for API communications
- **Access Control**: Role-based access control (RBAC)
- **Audit Logging**: Comprehensive security event logging
- **Rate Limiting**: API abuse prevention

## Troubleshooting

### Common Issues

#### Build Issues
```bash
# Missing dependencies
sudo apt-get install -y build-essential cmake pkg-config

# ImageMagick++ not found
sudo apt-get install -y libmagick++-dev

# WebP support missing
sudo apt-get install -y libwebp-dev

# HEIF support missing (optional)
sudo apt-get install -y libheif-dev
```

#### Runtime Issues
```bash
# Permission denied on image directory
sudo chown -R streamdab:streamdab /var/lib/streamdab/images

# Port 8008 already in use
sudo netstat -tlnp | grep :8008
sudo pkill -f odr-padenc

# Thai fonts not rendering
sudo apt-get install -y fonts-thai-tlwg
fc-cache -f -v
```

#### Performance Issues
```bash
# Enable performance monitoring
export STREAMDAB_PERF_MONITOR=1

# Increase worker threads
export STREAMDAB_WORKER_THREADS=8

# Optimize memory settings
export STREAMDAB_MAX_IMAGE_CACHE=100
export STREAMDAB_MAX_MESSAGE_QUEUE=1000
```

### Debug Mode
```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with verbose logging
./build/odr-padenc --verbose --log-level=debug

# Memory leak detection
valgrind --leak-check=full ./build/padenc_tests

# Performance profiling
perf record ./build/padenc_tests
perf report
```

## Contributing

### Code Style
- **C++17 Standard**: Modern C++ features and best practices
- **Google Style Guide**: Consistent code formatting
- **Documentation**: Comprehensive inline documentation
- **Testing**: Minimum 80% code coverage required

### Testing Requirements
- **Unit Tests**: Google Test framework
- **Integration Tests**: Full component integration testing  
- **Performance Tests**: Benchmark validation
- **Security Tests**: Vulnerability scanning
- **Coverage Tests**: lcov/gcov coverage analysis

### Submitting Changes
1. Fork the repository
2. Create feature branch (`git checkout -b feature/thai-calendar-enhancement`)
3. Write tests for new functionality
4. Ensure all tests pass with >80% coverage
5. Submit pull request with detailed description

## License

Copyright (C) 2024 StreamDAB Project

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

## Support

- **Documentation**: https://docs.streamdab.org/odr-padenc
- **Issues**: https://github.com/streamdab/ODR-PadEnc/issues
- **Discussions**: https://github.com/streamdab/ODR-PadEnc/discussions
- **Email**: support@streamdab.org

## Acknowledgments

- **OpenDigitalRadio**: Original ODR-PadEnc implementation
- **ETSI**: European Telecommunications Standards Institute
- **NBTC**: National Broadcasting and Telecommunications Commission of Thailand
- **ImageMagick**: Image processing library
- **Google Test**: Unit testing framework