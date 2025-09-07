/*
    Google Test Suite - Security Testing
    Copyright (C) 2024 StreamDAB Project
    
    Tests for security utilities:
    - File path traversal protection
    - Content validation and sanitization
    - Memory safety improvements
    - Input validation
    - Cryptographic functions
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/security_utils.h"
#include <fstream>
#include <filesystem>

using namespace StreamDAB;
using namespace testing;
namespace fs = std::filesystem;

class SecurityTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory structure
        test_dir_ = "/tmp/security_test";
        fs::create_directories(test_dir_);
        fs::create_directories(test_dir_ + "/allowed");
        fs::create_directories(test_dir_ + "/blocked");
        
        // Create test files
        CreateTestFiles();
        
        // Initialize security components
        std::vector<std::string> allowed_dirs = {test_dir_ + "/allowed"};
        path_validator_ = std::make_unique<SecurePathValidator>(allowed_dirs, true);
        
        security_scanner_ = std::make_unique<ContentSecurityScanner>();
        input_sanitizer_ = std::make_unique<InputSanitizer>();
        memory_manager_ = &SecureMemoryManager::GetInstance();
    }
    
    void TearDown() override {
        // Clean up test files
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    void CreateTestFiles() {
        // Safe image file
        std::vector<uint8_t> safe_jpeg = {
            0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46,
            0x00, 0x01, 0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00,
            0xFF, 0xD9
        };
        WriteTestFile(test_dir_ + "/allowed/safe.jpg", safe_jpeg);
        
        // Malicious file with script content
        std::vector<uint8_t> malicious_content = {
            '<', 's', 'c', 'r', 'i', 'p', 't', '>',
            'a', 'l', 'e', 'r', 't', '(', '1', ')', ';',
            '<', '/', 's', 'c', 'r', 'i', 'p', 't', '>'
        };
        WriteTestFile(test_dir_ + "/blocked/malicious.html", malicious_content);
        
        // Test text file
        std::string test_text = "This is a normal text file for testing.";
        std::vector<uint8_t> text_data(test_text.begin(), test_text.end());
        WriteTestFile(test_dir_ + "/allowed/test.txt", text_data);
        
        // File with null bytes
        std::vector<uint8_t> null_bytes = {'A', 0x00, 'B', 0x00, 'C'};
        WriteTestFile(test_dir_ + "/blocked/nullbytes.txt", null_bytes);
    }
    
    void WriteTestFile(const std::string& path, const std::vector<uint8_t>& data) {
        std::ofstream file(path, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    std::string test_dir_;
    std::unique_ptr<SecurePathValidator> path_validator_;
    std::unique_ptr<ContentSecurityScanner> security_scanner_;
    std::unique_ptr<InputSanitizer> input_sanitizer_;
    SecureMemoryManager* memory_manager_;
};

// Test path traversal protection
TEST_F(SecurityTest, PathTraversalProtection) {
    // Valid path should pass
    EXPECT_TRUE(path_validator_->IsPathSafe(test_dir_ + "/allowed/safe.jpg"));
    
    // Directory traversal attempts should fail
    EXPECT_FALSE(path_validator_->IsPathSafe("../../../etc/passwd"));
    EXPECT_FALSE(path_validator_->IsPathSafe(test_dir_ + "/../../../etc/passwd"));
    EXPECT_FALSE(path_validator_->IsPathSafe("..\\..\\..\\windows\\system32"));
    
    // Encoded traversal should fail
    EXPECT_FALSE(path_validator_->IsPathSafe("%2e%2e%2f%2e%2e%2f%2e%2e%2fetc%2fpasswd"));
    
    // Path outside allowed directory should fail
    EXPECT_FALSE(path_validator_->IsPathSafe(test_dir_ + "/blocked/malicious.html"));
}

// Test file validation
TEST_F(SecurityTest, FileValidation) {
    // Valid file in allowed directory
    auto validation = path_validator_->ValidatePath(test_dir_ + "/allowed/safe.jpg");
    EXPECT_TRUE(validation.is_valid);
    EXPECT_TRUE(validation.is_safe);
    EXPECT_TRUE(validation.security_issues.empty());
    EXPECT_EQ(validation.file_type, "JPEG");
    
    // File in blocked directory
    validation = path_validator_->ValidatePath(test_dir_ + "/blocked/malicious.html");
    EXPECT_FALSE(validation.is_safe);
    EXPECT_FALSE(validation.security_issues.empty());
    
    // Non-existent file
    validation = path_validator_->ValidatePath(test_dir_ + "/allowed/nonexistent.jpg");
    EXPECT_FALSE(validation.is_valid);
}

// Test path normalization
TEST_F(SecurityTest, PathNormalization) {
    std::string messy_path = "//test/./path/../with/./extra/slashes//";
    std::string normalized = SecurePathValidator::NormalizePath(messy_path);
    
    EXPECT_NE(normalized, messy_path);
    EXPECT_EQ(normalized.find("//"), std::string::npos); // No double slashes
    EXPECT_EQ(normalized.find("./"), std::string::npos); // No relative references
}

// Test path sanitization
TEST_F(SecurityTest, PathSanitization) {
    std::string dangerous_path = "../../../etc/passwd\0hidden";
    std::string sanitized = path_validator_->SanitizePath(dangerous_path);
    
    EXPECT_NE(sanitized, dangerous_path);
    EXPECT_EQ(sanitized.find('\0'), std::string::npos); // No null bytes
    EXPECT_NE(sanitized.find("etc/passwd"), std::string::npos); // Still contains safe parts
}

// Test content scanning for malicious patterns
TEST_F(SecurityTest, MaliciousContentDetection) {
    // Test script injection
    std::vector<uint8_t> script_content = {
        '<', 's', 'c', 'r', 'i', 'p', 't', '>',
        'a', 'l', 'e', 'r', 't', '(', '1', ')', ')',
        '<', '/', 's', 'c', 'r', 'i', 'p', 't', '>'
    };
    
    auto validation = security_scanner_->ScanContent(script_content);
    EXPECT_FALSE(validation.is_safe);
    EXPECT_GT(validation.threats_detected.size(), 0);
    EXPECT_GT(validation.risk_score, 0.5);
    
    // Test safe content
    std::vector<uint8_t> safe_content = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    validation = security_scanner_->ScanContent(safe_content);
    EXPECT_TRUE(validation.is_safe);
    EXPECT_EQ(validation.threats_detected.size(), 0);
    EXPECT_LT(validation.risk_score, 0.2);
}

// Test image format validation
TEST_F(SecurityTest, ImageFormatValidation) {
    // Valid JPEG
    std::vector<uint8_t> valid_jpeg = {
        0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46,
        0x00, 0x01, 0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00,
        0xFF, 0xD9
    };
    EXPECT_TRUE(security_scanner_->ValidateJPEG(valid_jpeg));
    
    // Invalid JPEG
    std::vector<uint8_t> invalid_jpeg = {0x00, 0x01, 0x02, 0x03};
    EXPECT_FALSE(security_scanner_->ValidateJPEG(invalid_jpeg));
    
    // Valid PNG
    std::vector<uint8_t> valid_png = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
        0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x02, 0x00, 0x00, 0x00, 0x90, 0x77, 0x53, 0xDE,
        0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44,
        0xAE, 0x42, 0x60, 0x82
    };
    EXPECT_TRUE(security_scanner_->ValidatePNG(valid_png));
    
    // Valid WebP
    std::vector<uint8_t> valid_webp = {
        0x52, 0x49, 0x46, 0x46, 0x20, 0x00, 0x00, 0x00,
        0x57, 0x45, 0x42, 0x50, 0x56, 0x50, 0x38, 0x20
    };
    EXPECT_TRUE(security_scanner_->ValidateWebP(valid_webp));
}

// Test text sanitization
TEST_F(SecurityTest, TextSanitization) {
    // Test HTML escaping
    std::string dangerous_html = "<script>alert('xss')</script>";
    std::string sanitized = input_sanitizer_->SanitizeText(dangerous_html);
    
    EXPECT_NE(sanitized, dangerous_html);
    EXPECT_EQ(sanitized.find("<script>"), std::string::npos);
    EXPECT_NE(sanitized.find("&lt;"), std::string::npos); // Should be escaped
    
    // Test control character removal
    std::string control_chars = "Hello\x00\x01\x02World\x7F";
    sanitized = input_sanitizer_->SanitizeText(control_chars);
    EXPECT_EQ(sanitized, "HelloWorld"); // Control chars should be removed
    
    // Test whitespace normalization
    std::string messy_whitespace = "Hello    \t\n\r   World   ";
    sanitized = input_sanitizer_->SanitizeText(messy_whitespace);
    EXPECT_EQ(sanitized, "Hello World");
}

// Test filename sanitization
TEST_F(SecurityTest, FilenameSanitization) {
    // Dangerous filename
    std::string dangerous_name = "../../evil<file>.exe";
    std::string sanitized = input_sanitizer_->SanitizeFilename(dangerous_name);
    
    EXPECT_NE(sanitized, dangerous_name);
    EXPECT_EQ(sanitized.find(".."), std::string::npos);
    EXPECT_EQ(sanitized.find("<"), std::string::npos);
    EXPECT_EQ(sanitized.find(">"), std::string::npos);
    
    // Empty/invalid names
    EXPECT_NE(input_sanitizer_->SanitizeFilename(""), "");
    EXPECT_NE(input_sanitizer_->SanitizeFilename("."), ".");
    EXPECT_NE(input_sanitizer_->SanitizeFilename(".."), "..");
    
    // Very long filename
    std::string long_name(300, 'A');
    sanitized = input_sanitizer_->SanitizeFilename(long_name);
    EXPECT_LE(sanitized.length(), 255);
}

// Test URL sanitization
TEST_F(SecurityTest, URLSanitization) {
    // Valid URLs
    EXPECT_TRUE(input_sanitizer_->IsValidURL("http://example.com"));
    EXPECT_TRUE(input_sanitizer_->IsValidURL("https://example.com/path"));
    
    // Invalid/dangerous URLs
    EXPECT_FALSE(input_sanitizer_->IsValidURL("javascript:alert(1)"));
    EXPECT_FALSE(input_sanitizer_->IsValidURL("data:text/html,<script>"));
    EXPECT_FALSE(input_sanitizer_->IsValidURL("file:///etc/passwd"));
    EXPECT_FALSE(input_sanitizer_->IsValidURL("ftp://malicious.site"));
    
    // Malformed URLs
    EXPECT_FALSE(input_sanitizer_->IsValidURL("not_a_url"));
    EXPECT_FALSE(input_sanitizer_->IsValidURL(""));
}

// Test secure memory allocation
TEST_F(SecurityTest, SecureMemoryAllocation) {
    size_t test_size = 1024;
    void* ptr = memory_manager_->SecureAlloc(test_size, "SecurityTest");
    
    EXPECT_NE(ptr, nullptr);
    
    // Memory should be zeroed
    uint8_t* bytes = static_cast<uint8_t*>(ptr);
    bool all_zero = true;
    for (size_t i = 0; i < test_size; ++i) {
        if (bytes[i] != 0) {
            all_zero = false;
            break;
        }
    }
    EXPECT_TRUE(all_zero);
    
    // Free memory
    memory_manager_->SecureFree(ptr);
}

// Test memory leak detection
TEST_F(SecurityTest, MemoryLeakDetection) {
    auto initial_stats = memory_manager_->GetMemoryStats();
    
    // Allocate some memory
    void* ptr1 = memory_manager_->SecureAlloc(512, "LeakTest1");
    void* ptr2 = memory_manager_->SecureAlloc(256, "LeakTest2");
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    
    // Check for leaks
    auto leaks = memory_manager_->DetectLeaks();
    EXPECT_GE(leaks.size(), 2); // Should detect our allocations
    
    // Free one allocation
    memory_manager_->SecureFree(ptr1);
    
    leaks = memory_manager_->DetectLeaks();
    EXPECT_GE(leaks.size(), 1); // Should still have one leak
    
    // Free remaining allocation
    memory_manager_->SecureFree(ptr2);
    
    // Memory stats should be updated
    auto final_stats = memory_manager_->GetMemoryStats();
    EXPECT_GE(final_stats.allocated_blocks, initial_stats.allocated_blocks);
    EXPECT_GE(final_stats.freed_blocks, initial_stats.freed_blocks);
}

// Test secure memory operations
TEST_F(SecurityTest, SecureMemoryOperations) {
    size_t size = 256;
    void* ptr1 = memory_manager_->SecureAlloc(size, "SecureOpsTest1");
    void* ptr2 = memory_manager_->SecureAlloc(size, "SecureOpsTest2");
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    
    // Fill memory with test pattern
    memset(ptr1, 0xAA, size);
    memset(ptr2, 0xBB, size);
    
    // Test secure comparison
    EXPECT_FALSE(memory_manager_->SecureCompare(ptr1, ptr2, size));
    
    // Copy data and test again
    memcpy(ptr2, ptr1, size);
    EXPECT_TRUE(memory_manager_->SecureCompare(ptr1, ptr2, size));
    
    // Test secure zero
    memory_manager_->SecureZero(ptr1, size);
    uint8_t* bytes = static_cast<uint8_t*>(ptr1);
    bool all_zero = true;
    for (size_t i = 0; i < size; ++i) {
        if (bytes[i] != 0) {
            all_zero = false;
            break;
        }
    }
    EXPECT_TRUE(all_zero);
    
    memory_manager_->SecureFree(ptr1);
    memory_manager_->SecureFree(ptr2);
}

// Test cryptographic functions
TEST_F(SecurityTest, CryptographicFunctions) {
    std::vector<uint8_t> test_data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    
    // Test SHA256
    std::string sha256_hash = SecurityUtils::CalculateSHA256(test_data);
    EXPECT_FALSE(sha256_hash.empty());
    EXPECT_EQ(sha256_hash.length(), 64); // SHA256 produces 32 bytes = 64 hex chars
    
    // Same data should produce same hash
    std::string sha256_hash2 = SecurityUtils::CalculateSHA256(test_data);
    EXPECT_EQ(sha256_hash, sha256_hash2);
    
    // Different data should produce different hash
    std::vector<uint8_t> different_data = {'G', 'o', 'o', 'd', 'b', 'y', 'e'};
    std::string different_hash = SecurityUtils::CalculateSHA256(different_data);
    EXPECT_NE(sha256_hash, different_hash);
    
    // Test MD5
    std::string md5_hash = SecurityUtils::CalculateMD5(test_data);
    EXPECT_FALSE(md5_hash.empty());
    EXPECT_EQ(md5_hash.length(), 32); // MD5 produces 16 bytes = 32 hex chars
    
    // Test checksum verification
    EXPECT_TRUE(SecurityUtils::VerifyChecksum(test_data, sha256_hash, "SHA256"));
    EXPECT_TRUE(SecurityUtils::VerifyChecksum(test_data, md5_hash, "MD5"));
    EXPECT_FALSE(SecurityUtils::VerifyChecksum(test_data, "invalid_hash", "SHA256"));
}

// Test random number generation
TEST_F(SecurityTest, RandomNumberGeneration) {
    // Generate random bytes
    auto random_bytes1 = SecurityUtils::GenerateRandomBytes(32);
    auto random_bytes2 = SecurityUtils::GenerateRandomBytes(32);
    
    EXPECT_EQ(random_bytes1.size(), 32);
    EXPECT_EQ(random_bytes2.size(), 32);
    EXPECT_NE(random_bytes1, random_bytes2); // Should be different
    
    // Generate random strings
    std::string random_str1 = SecurityUtils::GenerateRandomString(16);
    std::string random_str2 = SecurityUtils::GenerateRandomString(16);
    
    EXPECT_EQ(random_str1.length(), 16);
    EXPECT_EQ(random_str2.length(), 16);
    EXPECT_NE(random_str1, random_str2); // Should be different
    
    // Test custom character set
    std::string hex_str = SecurityUtils::GenerateRandomString(10, "0123456789ABCDEF");
    EXPECT_EQ(hex_str.length(), 10);
    
    // All characters should be from the specified set
    for (char c : hex_str) {
        EXPECT_NE(std::string("0123456789ABCDEF").find(c), std::string::npos);
    }
}

// Test safe buffer operations
TEST_F(SecurityTest, SafeBufferOperations) {
    SafeBuffer buffer(1024);
    
    EXPECT_EQ(buffer.Capacity(), 1024);
    EXPECT_EQ(buffer.Size(), 0);
    EXPECT_TRUE(buffer.IsEmpty());
    EXPECT_FALSE(buffer.IsFull());
    
    // Test writing data
    std::string test_data = "Hello, Safe Buffer!";
    EXPECT_TRUE(buffer.WriteString(0, test_data));
    EXPECT_EQ(buffer.Size(), test_data.length());
    EXPECT_FALSE(buffer.IsEmpty());
    
    // Test reading data
    std::string read_data = buffer.ReadString(0, test_data.length());
    EXPECT_EQ(read_data, test_data);
    
    // Test bounds checking
    EXPECT_FALSE(buffer.WriteString(2000, "overflow")); // Should fail - out of bounds
    
    // Test append operation
    std::string append_data = " More data!";
    EXPECT_TRUE(buffer.Append(append_data.c_str(), append_data.length()));
    
    std::string full_data = buffer.ReadString(0, buffer.Size());
    EXPECT_EQ(full_data, test_data + append_data);
}

// Test buffer overflow protection
TEST_F(SecurityTest, BufferOverflowProtection) {
    SafeBuffer small_buffer(10);
    
    // Try to write more data than capacity
    std::string large_data(50, 'X');
    EXPECT_FALSE(small_buffer.WriteString(0, large_data)); // Should fail
    EXPECT_EQ(small_buffer.Size(), 0); // Should remain empty
    
    // Write within bounds should work
    std::string small_data = "12345";
    EXPECT_TRUE(small_buffer.WriteString(0, small_data));
    EXPECT_EQ(small_buffer.Size(), 5);
    
    // Try to read beyond bounds
    std::string read_result = small_buffer.ReadString(0, 20); // Request more than available
    EXPECT_LE(read_result.length(), small_buffer.Size()); // Should be limited
}

// Test security self-test
TEST_F(SecurityTest, SecuritySelfTest) {
    EXPECT_TRUE(SecurityUtils::RunSecuritySelfTest());
    
    auto warnings = SecurityUtils::GetSecurityWarnings();
    // In test environment, might have some warnings but should not fail critically
    
    // Self-test should detect obvious security issues
    SecurePathValidator test_validator;
    EXPECT_FALSE(test_validator.IsPathSafe("../../../etc/passwd"));
}

// Test input validation edge cases
TEST_F(SecurityTest, InputValidationEdgeCases) {
    // Empty inputs
    EXPECT_TRUE(input_sanitizer_->SanitizeText("").empty());
    EXPECT_FALSE(input_sanitizer_->SanitizeFilename("").empty()); // Should provide default
    
    // Very long inputs
    std::string huge_input(10000, 'A');
    std::string sanitized = input_sanitizer_->SanitizeText(huge_input);
    EXPECT_LE(sanitized.length(), huge_input.length()); // Should not grow
    
    // Unicode handling
    std::string unicode_text = "Hello 世界 🌍";
    sanitized = input_sanitizer_->SanitizeText(unicode_text);
    EXPECT_FALSE(sanitized.empty()); // Should handle gracefully
    
    // Binary data in text
    std::string binary_text;
    for (int i = 0; i < 256; ++i) {
        binary_text += static_cast<char>(i);
    }
    sanitized = input_sanitizer_->SanitizeText(binary_text);
    // Should remove/replace control characters
    EXPECT_LT(sanitized.length(), binary_text.length());
}

// Test concurrent security operations
TEST_F(SecurityTest, ConcurrentSecurityOperations) {
    std::atomic<int> successful_validations{0};
    std::atomic<int> successful_allocations{0};
    std::vector<std::thread> test_threads;
    
    // Multiple threads doing security operations
    for (int i = 0; i < 4; ++i) {
        test_threads.emplace_back([&, i]() {
            for (int j = 0; j < 10; ++j) {
                // Path validation
                if (path_validator_->IsPathSafe(test_dir_ + "/allowed/safe.jpg")) {
                    successful_validations++;
                }
                
                // Memory allocation
                void* ptr = memory_manager_->SecureAlloc(128, "ConcurrentTest");
                if (ptr) {
                    successful_allocations++;
                    memory_manager_->SecureFree(ptr);
                }
                
                // Text sanitization
                std::string sanitized = input_sanitizer_->SanitizeText("Test <script> data");
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Wait for all threads
    for (auto& thread : test_threads) {
        thread.join();
    }
    
    EXPECT_GT(successful_validations.load(), 0);
    EXPECT_GT(successful_allocations.load(), 0);
}

// Test performance of security operations
TEST_F(SecurityTest, SecurityPerformance) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Perform many security operations
    for (int i = 0; i < 100; ++i) {
        // Path validation
        path_validator_->IsPathSafe(test_dir_ + "/allowed/test.txt");
        
        // Content scanning
        std::vector<uint8_t> test_content = {'T', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
        security_scanner_->ScanContent(test_content);
        
        // Text sanitization
        input_sanitizer_->SanitizeText("Test input string");
        
        // Hash calculation
        SecurityUtils::CalculateSHA256(test_content);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Security operations should be reasonably fast
    EXPECT_LT(duration.count(), 1000); // Less than 1 second for 100 iterations
}