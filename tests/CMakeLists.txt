# CMakeLists.txt for ODR-PadEnc comprehensive testing framework
cmake_minimum_required(VERSION 3.10)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(GTest REQUIRED)
find_package(GMock REQUIRED)
find_package(ImageMagick++ REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
    ${ImageMagick++_INCLUDE_DIRS}
)

# Test sources
set(TEST_SOURCES
    test_main.cpp
    test_thai_rendering.cpp
    test_enhanced_mot.cpp
    test_smart_dls.cpp
    test_api_interface.cpp
    test_content_manager.cpp
    test_image_processing.cpp
    test_charset_conversion.cpp
    test_performance.cpp
    test_integration.cpp
)

# Production source files needed for testing
set(PRODUCTION_SOURCES
    ${CMAKE_SOURCE_DIR}/src/thai_rendering.cpp
    ${CMAKE_SOURCE_DIR}/src/enhanced_mot.cpp
    ${CMAKE_SOURCE_DIR}/src/smart_dls.cpp
    ${CMAKE_SOURCE_DIR}/src/api_interface.cpp
    ${CMAKE_SOURCE_DIR}/src/content_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/security_utils.cpp
    ${CMAKE_SOURCE_DIR}/src/charset.cpp
    ${CMAKE_SOURCE_DIR}/src/dls.cpp
    ${CMAKE_SOURCE_DIR}/src/sls.cpp
    ${CMAKE_SOURCE_DIR}/src/pad_common.cpp
    ${CMAKE_SOURCE_DIR}/src/crc.cpp
    ${CMAKE_SOURCE_DIR}/src/common.cpp
)

# Create test executable
add_executable(odr_padenc_tests
    ${TEST_SOURCES}
    ${PRODUCTION_SOURCES}
)

# Link libraries
target_link_libraries(odr_padenc_tests
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
    ${ImageMagick++_LIBRARIES}
    pthread
    ssl
    crypto
    zmq
    magic
    png
    jpeg
)

# Test discovery
enable_testing()
add_test(NAME ODR_PadEnc_Tests COMMAND odr_padenc_tests)

# Coverage target
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    target_compile_options(odr_padenc_tests PRIVATE --coverage)
    target_link_options(odr_padenc_tests PRIVATE --coverage)
endif()

# Test data files
configure_file(${CMAKE_SOURCE_DIR}/tests/test_data/sample_thai.txt ${CMAKE_BINARY_DIR}/test_data/sample_thai.txt COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tests/test_data/sample_image.jpg ${CMAKE_BINARY_DIR}/test_data/sample_image.jpg COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tests/test_data/sample_slide.png ${CMAKE_BINARY_DIR}/test_data/sample_slide.png COPYONLY)
