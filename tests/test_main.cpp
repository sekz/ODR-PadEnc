/* ------------------------------------------------------------------
 * Copyright (C) 2024 StreamDAB Project
 * Copyright (C) 2011 Martin Storsjo
 * Copyright (C) 2022 Matthias P. Braendli
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * ------------------------------------------------------------------- */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <filesystem>

// Test environment setup
class ODRPadEncTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "Setting up ODR-PadEnc test environment..." << std::endl;
        
        // Create test directories
        std::filesystem::create_directories("test_data");
        std::filesystem::create_directories("test_output");
        std::filesystem::create_directories("test_slides");
        std::filesystem::create_directories("test_logs");
        
        // Initialize test data
        createTestData();
        
        std::cout << "Test environment initialized successfully" << std::endl;
    }
    
    void TearDown() override {
        std::cout << "Cleaning up ODR-PadEnc test environment..." << std::endl;
        
        // Clean up test files
        try {
            std::filesystem::remove_all("test_output");
            std::filesystem::remove_all("test_logs");
            // Keep test_data and test_slides for inspection
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to clean up test files: " << e.what() << std::endl;
        }
        
        std::cout << "Test environment cleaned up" << std::endl;
    }

private:
    void createTestData() {
        // Create sample Thai text file
        std::ofstream thai_file("test_data/sample_thai.txt");
        thai_file << "สวัสดีครับ นี่คือข้อความทดสอบภาษาไทย\n";
        thai_file << "วิทยุ DAB+ สำหรับประเทศไทย\n";
        thai_file << "Buddhist Era: พ.ศ. 2567\n";
        thai_file << "Thai numerals: ๐๑๒๓๔๕๖๗๘๙\n";
        thai_file.close();
        
        // Create sample English text file
        std::ofstream eng_file("test_data/sample_english.txt");
        eng_file << "Hello World - DAB+ Radio Station\n";
        eng_file << "Now Playing: Test Song by Test Artist\n";
        eng_file << "Streaming since 2024\n";
        eng_file.close();
        
        // Create sample mixed language text
        std::ofstream mixed_file("test_data/sample_mixed.txt");
        mixed_file << "StreamDAB - สถานีวิทยุดิจิทัล\n";
        mixed_file << "Now Playing: เพลงไทยสมัยใหม่ by นักร้องไทย\n";
        mixed_file << "Visit: https://streamdab.example.com\n";
        mixed_file.close();
    }
};

// Test fixture base class for common functionality
class ODRPadEncTestBase : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
        std::cout << "Setting up individual ODR-PadEnc test..." << std::endl;
        
        // Initialize working directory
        test_work_dir_ = std::filesystem::current_path() / "test_output";
        std::filesystem::create_directories(test_work_dir_);
    }
    
    void TearDown() override {
        // Common cleanup for all tests
        std::cout << "Cleaning up individual ODR-PadEnc test..." << std::endl;
    }
    
    // Helper methods for common test operations
    std::string getTestDataPath(const std::string& filename) const {
        return (std::filesystem::current_path() / "test_data" / filename).string();
    }
    
    std::string getTestOutputPath(const std::string& filename) const {
        return (test_work_dir_ / filename).string();
    }
    
    std::string getTestSlidesPath() const {
        return (std::filesystem::current_path() / "test_slides").string();
    }
    
    std::filesystem::path test_work_dir_;
};

int main(int argc, char **argv) {
    std::cout << "Starting ODR-PadEnc Enhanced Test Suite" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Add global test environment
    ::testing::AddGlobalTestEnvironment(new ODRPadEncTestEnvironment);
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    std::cout << "Test suite completed with result: " << result << std::endl;
    return result;
}