cmake_minimum_required(VERSION 3.14)
project(ODR-PadEnc)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable static linking for better compatibility (user suggestion)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

# Enable testing
enable_testing()

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find ImageMagick with WebP and HEIF support (optional)
find_package(ImageMagick COMPONENTS Magick++)

# Find additional libraries (optional)
pkg_check_modules(WEBP libwebp)
pkg_check_modules(HEIF libheif)

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# Include directories
include_directories(src)
include_directories(${ImageMagick_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/odr-padenc.cpp
    src/common.cpp
    src/charset.cpp
    src/sls.cpp
    src/pad_interface.cpp
    src/pad_common.cpp
    src/dls.cpp
    src/crc.cpp
)

# Enhanced source files
set(ENHANCED_SOURCES
    src/enhanced_mot.cpp
    src/thai_rendering.cpp
    src/smart_dls.cpp
    src/api_interface.cpp
    src/security_utils.cpp
    src/content_manager.cpp
)

# Create main executable
add_executable(odr-padenc ${SOURCES} ${ENHANCED_SOURCES})

# Link libraries
target_link_libraries(odr-padenc
    Threads::Threads
)

if(ImageMagick_FOUND)
    target_link_libraries(odr-padenc ${ImageMagick_LIBRARIES})
    target_include_directories(odr-padenc PRIVATE ${ImageMagick_INCLUDE_DIRS})
    target_compile_definitions(odr-padenc PRIVATE HAVE_IMAGEMAGICK ${ImageMagick_DEFINITIONS})
endif()

if(WEBP_FOUND)
    target_link_libraries(odr-padenc ${WEBP_LIBRARIES})
    target_compile_definitions(odr-padenc PRIVATE HAVE_WEBP ${WEBP_CFLAGS})
endif()

if(HEIF_FOUND)
    target_link_libraries(odr-padenc ${HEIF_LIBRARIES})
    target_compile_definitions(odr-padenc PRIVATE HAVE_HEIF)
endif()

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50f33f9142a332e68bb5b4c75ad960.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Test executable
add_executable(
  padenc_tests
  tests/test_mot_slideshow.cpp
  tests/test_thai_rendering.cpp
  tests/test_dls_processing.cpp
  tests/test_api_interface.cpp
  tests/test_security.cpp
  tests/test_performance.cpp
  ${ENHANCED_SOURCES}
  src/common.cpp
  src/charset.cpp
  src/sls.cpp
  src/pad_interface.cpp
  src/pad_common.cpp
  src/dls.cpp
  src/crc.cpp
)

target_link_libraries(
  padenc_tests
  GTest::gtest_main
  Threads::Threads
)

if(ImageMagick_FOUND)
    target_link_libraries(padenc_tests ${ImageMagick_LIBRARIES})
    target_include_directories(padenc_tests PRIVATE ${ImageMagick_INCLUDE_DIRS})
    target_compile_definitions(padenc_tests PRIVATE HAVE_IMAGEMAGICK ${ImageMagick_DEFINITIONS})
endif()

if(WEBP_FOUND)
    target_link_libraries(padenc_tests ${WEBP_LIBRARIES})
    target_compile_definitions(padenc_tests PRIVATE HAVE_WEBP ${WEBP_CFLAGS})
endif()

if(HEIF_FOUND)
    target_link_libraries(padenc_tests ${HEIF_LIBRARIES})
    target_compile_definitions(padenc_tests PRIVATE HAVE_HEIF)
endif()

# Coverage target
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    find_program(GCOV_PATH gcov)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
        add_custom_target(coverage
            # Cleanup lcov
            COMMAND ${LCOV_PATH} --directory . --zerocounters

            # Run tests
            COMMAND $<TARGET_FILE:padenc_tests>

            # Capturing lcov counters and generating report
            COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '*/tests/*' '*/test-data/*' --output-file coverage.info.cleaned
            COMMAND ${GENHTML_PATH} -o coverage coverage.info.cleaned

            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report"
        )
    endif()
endif()

include(GoogleTest)
gtest_discover_tests(padenc_tests)

# Install targets
install(TARGETS odr-padenc DESTINATION bin)
