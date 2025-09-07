/*
    Google Test Suite - MOT SlideShow Testing
    Copyright (C) 2024 StreamDAB Project
    
    Tests for advanced MOT slideshow features:
    - WebP and HEIF format support validation
    - Image optimization and processing
    - Smart carousel functionality
    - ETSI TS 101 499 compliance validation
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/enhanced_mot.h"
#include "../src/security_utils.h"
#include <filesystem>
#include <fstream>

using namespace StreamDAB;
using namespace testing;
namespace fs = std::filesystem;

class MOTSlideshowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directories
        test_image_dir_ = "/tmp/test_images";
        fs::create_directories(test_image_dir_);
        
        // Create test images
        CreateTestImages();
        
        // Initialize MOT processor with test configuration
        CarouselConfig config;
        config.max_images = 10;
        config.quality_threshold = 0.5;
        config.enable_duplicate_detection = true;
        
        mot_processor_ = std::make_unique<EnhancedMOTProcessor>(config);
    }
    
    void TearDown() override {
        // Clean up test files
        if (fs::exists(test_image_dir_)) {
            fs::remove_all(test_image_dir_);
        }
    }
    
    void CreateTestImages() {
        // Create minimal valid JPEG image
        std::vector<uint8_t> jpeg_data = {
            0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46,
            0x00, 0x01, 0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00,
            0xFF, 0xDB, 0x00, 0x43, 0x00, 0x08, 0x06, 0x06, 0x07, 0x06,
            0x05, 0x08, 0x07, 0x07, 0x07, 0x09, 0x09, 0x08, 0x0A, 0x0C,
            0x14, 0x0D, 0x0C, 0x0B, 0x0B, 0x0C, 0x19, 0x12, 0x13, 0x0F,
            0x14, 0x1D, 0x1A, 0x1F, 0x1E, 0x1D, 0x1A, 0x1C, 0x1C, 0x20,
            0x24, 0x2E, 0x27, 0x20, 0x22, 0x2C, 0x23, 0x1C, 0x1C, 0x28,
            0x37, 0x29, 0x2C, 0x30, 0x31, 0x34, 0x34, 0x34, 0x1F, 0x27,
            0x39, 0x3D, 0x38, 0x32, 0x3C, 0x2E, 0x33, 0x34, 0x32, 0xFF,
            0xC0, 0x00, 0x11, 0x08, 0x00, 0x01, 0x00, 0x01, 0x01, 0x01,
            0x11, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xFF, 0xC4,
            0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
            0xFF, 0xC4, 0x00, 0x14, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xFF, 0xDA, 0x00, 0x0C, 0x03, 0x01, 0x00, 0x02,
            0x11, 0x03, 0x11, 0x00, 0x3F, 0x00, 0xB2, 0xC0, 0x07, 0xFF, 0xD9
        };
        
        // Create PNG image data (minimal valid PNG)
        std::vector<uint8_t> png_data = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, // PNG signature
            0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, // IHDR chunk
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, // 1x1 image
            0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53,
            0xDE, 0x00, 0x00, 0x00, 0x0C, 0x49, 0x44, 0x41, // IDAT chunk
            0x54, 0x08, 0xD7, 0x63, 0xF8, 0x0F, 0x00, 0x00,
            0x01, 0x00, 0x01, 0x5C, 0xC2, 0x8A, 0x8E,
            0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, // IEND chunk
            0xAE, 0x42, 0x60, 0x82
        };
        
        // Create WebP image data (minimal valid WebP)
        std::vector<uint8_t> webp_data = {
            0x52, 0x49, 0x46, 0x46, // "RIFF"
            0x20, 0x00, 0x00, 0x00, // File size
            0x57, 0x45, 0x42, 0x50, // "WEBP"
            0x56, 0x50, 0x38, 0x20, // "VP8 "
            0x14, 0x00, 0x00, 0x00, // Chunk size
            0x30, 0x01, 0x00, 0x9D, // VP8 bitstream
            0x01, 0x2A, 0x01, 0x00,
            0x01, 0x00, 0x62, 0x00,
            0x04, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00
        };
        
        // Write test images
        WriteTestFile(test_image_dir_ + "/test1.jpg", jpeg_data);
        WriteTestFile(test_image_dir_ + "/test2.png", png_data);
        WriteTestFile(test_image_dir_ + "/test3.webp", webp_data);
        
        // Create a large image for size testing
        std::vector<uint8_t> large_jpeg = jpeg_data;
        large_jpeg.resize(2 * 1024 * 1024); // 2MB
        WriteTestFile(test_image_dir_ + "/large_test.jpg", large_jpeg);
        
        // Create an invalid image file
        std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02, 0x03};
        WriteTestFile(test_image_dir_ + "/invalid.jpg", invalid_data);
    }
    
    void WriteTestFile(const std::string& path, const std::vector<uint8_t>& data) {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    std::string test_image_dir_;
    std::unique_ptr<EnhancedMOTProcessor> mot_processor_;
};

// Test basic MOT processor initialization
TEST_F(MOTSlideshowTest, ProcessorInitialization) {
    EXPECT_NE(mot_processor_, nullptr);
    EXPECT_EQ(mot_processor_->GetImageCount(), 0);
}

// Test image format detection
TEST_F(MOTSlideshowTest, ImageFormatDetection) {
    // Test JPEG detection
    EXPECT_TRUE(mot_processor_->AddImage(test_image_dir_ + "/test1.jpg"));
    
    // Test PNG detection  
    EXPECT_TRUE(mot_processor_->AddImage(test_image_dir_ + "/test2.png"));
    
    // Test WebP detection
    EXPECT_TRUE(mot_processor_->AddImage(test_image_dir_ + "/test3.webp"));
    
    // Verify images were added
    EXPECT_EQ(mot_processor_->GetImageCount(), 3);
}

// Test invalid image rejection
TEST_F(MOTSlideshowTest, InvalidImageRejection) {
    // Should reject invalid image
    EXPECT_FALSE(mot_processor_->AddImage(test_image_dir_ + "/invalid.jpg"));
    
    // Should reject non-existent file
    EXPECT_FALSE(mot_processor_->AddImage(test_image_dir_ + "/nonexistent.jpg"));
    
    // Image count should remain 0
    EXPECT_EQ(mot_processor_->GetImageCount(), 0);
}

// Test directory processing
TEST_F(MOTSlideshowTest, DirectoryProcessing) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Should have processed valid images (3 valid ones)
    EXPECT_EQ(mot_processor_->GetImageCount(), 3);
    
    // Test invalid directory
    EXPECT_FALSE(mot_processor_->ProcessImageDirectory("/nonexistent/path"));
}

// Test image carousel functionality
TEST_F(MOTSlideshowTest, ImageCarousel) {
    // Add test images
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Get next image
    auto image = mot_processor_->GetNextImage();
    ASSERT_NE(image, nullptr);
    EXPECT_FALSE(image->filename.empty());
    EXPECT_GT(image->processed_data.size(), 0);
    
    // Get another image (should be different due to carousel)
    auto image2 = mot_processor_->GetNextImage();
    ASSERT_NE(image2, nullptr);
    
    // Should cycle through images
    auto image3 = mot_processor_->GetNextImage();
    ASSERT_NE(image3, nullptr);
}

// Test duplicate detection
TEST_F(MOTSlideshowTest, DuplicateDetection) {
    // Add same image twice
    EXPECT_TRUE(mot_processor_->AddImage(test_image_dir_ + "/test1.jpg"));
    EXPECT_FALSE(mot_processor_->AddImage(test_image_dir_ + "/test1.jpg")); // Should be rejected as duplicate
    
    // Should only have one image
    EXPECT_EQ(mot_processor_->GetImageCount(), 1);
}

// Test image optimization
TEST_F(MOTSlideshowTest, ImageOptimization) {
    // Test optimization for DAB size constraints
    std::vector<uint8_t> output;
    EXPECT_TRUE(ImageOptimizer::OptimizeForDAB(test_image_dir_ + "/test1.jpg", output, 32768)); // 32KB limit
    
    EXPECT_GT(output.size(), 0);
    EXPECT_LE(output.size(), 32768);
}

// Test image resizing
TEST_F(MOTSlideshowTest, ImageResizing) {
    // This would require actual ImageMagick integration
    // For now, test the interface
    try {
        Magick::Image image;
        image.read(test_image_dir_ + "/test1.jpg");
        
        EXPECT_TRUE(ImageOptimizer::ResizeImage(image, 320, 240));
        EXPECT_LE(image.columns(), 320);
        EXPECT_LE(image.rows(), 240);
    } catch (const std::exception& e) {
        // If ImageMagick is not available, mark as skipped
        GTEST_SKIP() << "ImageMagick not available: " << e.what();
    }
}

// Test DAB profile application
TEST_F(MOTSlideshowTest, DABProfileApplication) {
    try {
        Magick::Image image;
        image.read(test_image_dir_ + "/test1.jpg");
        
        EXPECT_TRUE(ImageOptimizer::ApplyDABProfile(image));
        EXPECT_EQ(image.depth(), 8);
    } catch (const std::exception& e) {
        GTEST_SKIP() << "ImageMagick not available: " << e.what();
    }
}

// Test smart content selection
TEST_F(MOTSlideshowTest, SmartContentSelection) {
    // Add images with different qualities
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Test smart selection vs random selection
    SmartContentSelector selector;
    
    // Create mock image data
    std::vector<std::unique_ptr<EnhancedImageData>> images;
    auto image1 = std::make_unique<EnhancedImageData>();
    image1->filename = "test1.jpg";
    image1->quality.sharpness = 0.8;
    image1->quality.contrast = 0.7;
    image1->quality.freshness_score = 1.0;
    
    auto image2 = std::make_unique<EnhancedImageData>();
    image2->filename = "test2.jpg";
    image2->quality.sharpness = 0.6;
    image2->quality.contrast = 0.5;
    image2->quality.freshness_score = 0.5;
    
    images.push_back(std::move(image1));
    images.push_back(std::move(image2));
    
    auto selected = selector.SelectContent(images, 1);
    EXPECT_EQ(selected.size(), 1);
    EXPECT_EQ(selected[0], 0); // Should select first image (higher quality)
}

// Test ETSI compliance validation
TEST_F(MOTSlideshowTest, ETSIComplianceValidation) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    auto image = mot_processor_->GetNextImage();
    ASSERT_NE(image, nullptr);
    
    // Test ETSI compliance
    EXPECT_TRUE(mot_processor_->ValidateETSICompliance(*image));
    
    // Test MOT object generation
    auto mot_object = mot_processor_->GenerateMOTObject(*image, 1234);
    EXPECT_GT(mot_object.size(), 0);
}

// Test performance metrics
TEST_F(MOTSlideshowTest, PerformanceMetrics) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    auto stats = mot_processor_->GetStatistics();
    EXPECT_GT(stats.total_images, 0);
    EXPECT_GE(stats.average_quality, 0.0);
    EXPECT_LE(stats.average_quality, 1.0);
}

// Test background processing
TEST_F(MOTSlideshowTest, BackgroundProcessing) {
    mot_processor_->StartBackgroundProcessing();
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Wait a bit for background processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    mot_processor_->StopBackgroundProcessing();
    
    // Background processing should have optimized freshness scores
    auto image = mot_processor_->GetNextImage();
    if (image) {
        EXPECT_GE(image->quality.freshness_score, 0.0);
        EXPECT_LE(image->quality.freshness_score, 1.0);
    }
}

// Test image quality analysis
TEST_F(MOTSlideshowTest, ImageQualityAnalysis) {
    EXPECT_TRUE(mot_processor_->AddImage(test_image_dir_ + "/test1.jpg"));
    
    auto image = mot_processor_->GetNextImage();
    ASSERT_NE(image, nullptr);
    
    // Quality metrics should be calculated
    EXPECT_GE(image->quality.sharpness, 0.0);
    EXPECT_GE(image->quality.contrast, 0.0);
    EXPECT_GE(image->quality.brightness, 0.0);
    EXPECT_GE(image->quality.freshness_score, 0.0);
    EXPECT_LE(image->quality.freshness_score, 1.0);
}

// Test WebP format support
TEST_F(MOTSlideshowTest, WebPFormatSupport) {
    EXPECT_TRUE(mot_processor_->AddImage(test_image_dir_ + "/test3.webp"));
    
    auto image = mot_processor_->GetNextImage();
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(image->format, ImageFormat::WEBP);
    EXPECT_GT(image->processed_data.size(), 0);
}

// Test configuration updates
TEST_F(MOTSlideshowTest, ConfigurationUpdates) {
    CarouselConfig new_config;
    new_config.max_images = 20;
    new_config.quality_threshold = 0.8;
    
    mot_processor_->UpdateConfig(new_config);
    
    auto config = mot_processor_->GetConfig();
    EXPECT_EQ(config.max_images, 20);
    EXPECT_DOUBLE_EQ(config.quality_threshold, 0.8);
}

// Test error handling
TEST_F(MOTSlideshowTest, ErrorHandling) {
    // Test with corrupted image
    std::vector<uint8_t> corrupted_data = {0xFF, 0xD8, 0x00, 0x00}; // Incomplete JPEG
    WriteTestFile(test_image_dir_ + "/corrupted.jpg", corrupted_data);
    
    // Should handle gracefully
    EXPECT_FALSE(mot_processor_->AddImage(test_image_dir_ + "/corrupted.jpg"));
    
    // Test with empty file
    WriteTestFile(test_image_dir_ + "/empty.jpg", {});
    EXPECT_FALSE(mot_processor_->AddImage(test_image_dir_ + "/empty.jpg"));
}

// Test memory management
TEST_F(MOTSlideshowTest, MemoryManagement) {
    // Add many images to test memory handling
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Get statistics before
    auto stats_before = mot_processor_->GetStatistics();
    
    // Process images multiple times
    for (int i = 0; i < 10; ++i) {
        auto image = mot_processor_->GetNextImage();
        EXPECT_NE(image, nullptr);
    }
    
    // Memory should be managed properly
    auto stats_after = mot_processor_->GetStatistics();
    EXPECT_GE(stats_after.total_images, stats_before.total_images);
}

// Performance benchmark test
TEST_F(MOTSlideshowTest, PerformanceBenchmark) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Measure processing time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        auto image = mot_processor_->GetNextImage();
        EXPECT_NE(image, nullptr);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should process images quickly (less than 1000ms for 100 iterations)
    EXPECT_LT(duration.count(), 1000);
}

// Test thread safety
TEST_F(MOTSlideshowTest, ThreadSafety) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    std::atomic<int> successful_gets{0};
    std::atomic<bool> test_running{true};
    
    // Start multiple threads accessing images
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            while (test_running) {
                auto image = mot_processor_->GetNextImage();
                if (image) {
                    successful_gets++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    test_running = false;
    
    // Wait for threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have successfully retrieved images
    EXPECT_GT(successful_gets.load(), 0);
}