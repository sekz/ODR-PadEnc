/*
    Security and Performance Utilities Implementation
    Copyright (C) 2024 StreamDAB Project
*/

#include "security_utils.h"
#include <algorithm>
#include <regex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/rand.h>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace StreamDAB {

SecurePathValidator::SecurePathValidator() : strict_mode_(true) {
    // Default blocked patterns
    blocked_patterns_ = {
        "..", "~", "$", "`", "|", "&", ";", 
        "<", ">", "\"", "'", "\\x", "\\u",
        "\\r", "\\n", "\\t", "\\0"
    };
}

SecurePathValidator::SecurePathValidator(const std::vector<std::string>& allowed_dirs, bool strict) 
    : allowed_directories_(allowed_dirs), strict_mode_(strict) {
    blocked_patterns_ = {
        "..", "~", "$", "`", "|", "&", ";", 
        "<", ">", "\"", "'", "\\x", "\\u",
        "\\r", "\\n", "\\t", "\\0"
    };
}

bool SecurePathValidator::ContainsTraversal(const std::string& path) const {
    // Check for directory traversal patterns
    if (path.find("..") != std::string::npos) return true;
    if (path.find("~") != std::string::npos) return true;
    if (path.find("//") != std::string::npos) return true;
    
    // Check for encoded traversal
    if (path.find("%2e%2e") != std::string::npos) return true;  // ..
    if (path.find("%2f") != std::string::npos) return true;     // /
    if (path.find("%5c") != std::string::npos) return true;     // backslash
    
    return false;
}

bool SecurePathValidator::IsInAllowedDirectory(const std::string& path) const {
    if (allowed_directories_.empty()) {
        return !strict_mode_; // If no restrictions and not strict, allow all
    }
    
    std::string normalized_path = NormalizePath(path);
    
    for (const auto& allowed_dir : allowed_directories_) {
        std::string normalized_allowed = NormalizePath(allowed_dir);
        
        // Check if path starts with allowed directory
        if (normalized_path.substr(0, normalized_allowed.length()) == normalized_allowed) {
            // Ensure it's actually within the directory (not just starts with same prefix)
            if (normalized_path.length() == normalized_allowed.length() ||
                normalized_path[normalized_allowed.length()] == '/') {
                return true;
            }
        }
    }
    
    return false;
}

bool SecurePathValidator::MatchesBlockedPattern(const std::string& path) const {
    for (const auto& pattern : blocked_patterns_) {
        if (path.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

std::string SecurePathValidator::ResolvePath(const std::string& path) const {
    try {
        return fs::canonical(fs::path(path)).string();
    } catch (const fs::filesystem_error&) {
        // If path doesn't exist or can't be resolved, return normalized path
        return NormalizePath(path);
    }
}

FileValidation SecurePathValidator::ValidatePath(const std::string& path) const {
    FileValidation validation;
    validation.sanitized_path = SanitizePath(path);
    
    // Check for traversal attempts
    if (ContainsTraversal(path)) {
        validation.security_issues.push_back("Directory traversal attempt detected");
        return validation;
    }
    
    // Check against blocked patterns
    if (MatchesBlockedPattern(path)) {
        validation.security_issues.push_back("Contains blocked pattern");
        return validation;
    }
    
    // Check if in allowed directory
    if (!IsInAllowedDirectory(path)) {
        validation.security_issues.push_back("Path not in allowed directory");
        return validation;
    }
    
    // Check if file exists and get properties
    try {
        if (fs::exists(validation.sanitized_path)) {
            validation.is_valid = true;
            validation.file_size = fs::file_size(validation.sanitized_path);
            
            // Basic file type detection
            std::string ext = fs::path(validation.sanitized_path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            if (ext == ".jpg" || ext == ".jpeg") {
                validation.file_type = "JPEG";
                validation.mime_type = "image/jpeg";
            } else if (ext == ".png") {
                validation.file_type = "PNG";
                validation.mime_type = "image/png";
            } else if (ext == ".webp") {
                validation.file_type = "WebP";
                validation.mime_type = "image/webp";
            } else if (ext == ".heic" || ext == ".heif") {
                validation.file_type = "HEIF";
                validation.mime_type = "image/heif";
            }
            
            validation.is_safe = true;
        }
    } catch (const fs::filesystem_error& e) {
        validation.security_issues.push_back("Filesystem error: " + std::string(e.what()));
    }
    
    return validation;
}

std::string SecurePathValidator::SanitizePath(const std::string& path) const {
    std::string sanitized = path;
    
    // Remove null bytes
    sanitized.erase(std::remove(sanitized.begin(), sanitized.end(), '\0'), sanitized.end());
    
    // Replace backslashes with forward slashes
    std::replace(sanitized.begin(), sanitized.end(), '\\', '/');
    
    // Remove double slashes
    while (sanitized.find("//") != std::string::npos) {
        sanitized = std::regex_replace(sanitized, std::regex("//+"), "/");
    }
    
    // Remove trailing slash (except for root)
    if (sanitized.length() > 1 && sanitized.back() == '/') {
        sanitized.pop_back();
    }
    
    return sanitized;
}

std::string SecurePathValidator::NormalizePath(const std::string& path) {
    std::string normalized = path;
    
    // Convert to lowercase for case-insensitive comparisons
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Replace backslashes with forward slashes
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Remove double slashes
    while (normalized.find("//") != std::string::npos) {
        normalized = std::regex_replace(normalized, std::regex("//+"), "/");
    }
    
    return normalized;
}

ContentSecurityScanner::ContentSecurityScanner() {
    // Initialize malicious patterns
    malicious_patterns_ = {
        "\\x00", "\\x0a", "\\x0d",  // Null, newline, carriage return
        "<script", "</script>",      // Script tags
        "javascript:", "vbscript:",  // Script URLs
        "data:text/html",           // Data URLs
        "<?php", "<?=",             // PHP tags
        "<!--#",                    // Server-side includes
        "\\xff\\xd8\\xff",          // Potential JPEG exploit signatures
    };
    
    suspicious_extensions_ = {
        ".exe", ".bat", ".cmd", ".com", ".scr", ".pif",
        ".php", ".asp", ".jsp", ".py", ".pl", ".sh"
    };
    
    // Initialize format validators
    format_validators_["image/jpeg"] = [this](const std::vector<uint8_t>& data) { return ValidateJPEG(data); };
    format_validators_["image/png"] = [this](const std::vector<uint8_t>& data) { return ValidatePNG(data); };
    format_validators_["image/webp"] = [this](const std::vector<uint8_t>& data) { return ValidateWebP(data); };
    format_validators_["image/heif"] = [this](const std::vector<uint8_t>& data) { return ValidateHEIF(data); };
}

bool ContentSecurityScanner::ScanForMaliciousContent(const std::vector<uint8_t>& data) const {
    std::string data_str(data.begin(), data.end());
    
    for (const auto& pattern : malicious_patterns_) {
        if (data_str.find(pattern) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

SecurityValidation ContentSecurityScanner::ScanContent(const std::vector<uint8_t>& data, const std::string& content_type) {
    SecurityValidation validation;
    validation.is_safe = true;
    validation.risk_score = 0.0;
    
    // Check for malicious patterns
    if (ScanForMaliciousContent(data)) {
        validation.is_safe = false;
        validation.threats_detected.push_back("Malicious pattern detected");
        validation.risk_score += 0.8;
    }
    
    // Validate format if specified
    if (!content_type.empty()) {
        auto validator_it = format_validators_.find(content_type);
        if (validator_it != format_validators_.end()) {
            if (!validator_it->second(data)) {
                validation.is_safe = false;
                validation.threats_detected.push_back("Invalid " + content_type + " format");
                validation.risk_score += 0.6;
            }
        }
    }
    
    // Check file size limits
    if (data.size() > 50 * 1024 * 1024) { // 50MB limit
        validation.warnings.push_back("File size exceeds recommended limit");
        validation.risk_score += 0.2;
    }
    
    // Ensure risk score doesn't exceed 1.0
    validation.risk_score = std::min(validation.risk_score, 1.0);
    
    return validation;
}

bool ContentSecurityScanner::ValidateJPEG(const std::vector<uint8_t>& data) const {
    if (data.size() < 4) return false;
    
    // Check JPEG magic bytes
    if (data[0] != 0xFF || data[1] != 0xD8 || data[2] != 0xFF) {
        return false;
    }
    
    // Check for proper JPEG end marker
    if (data.size() >= 2) {
        size_t end = data.size() - 2;
        if (data[end] != 0xFF || data[end + 1] != 0xD9) {
            return false;
        }
    }
    
    return true;
}

bool ContentSecurityScanner::ValidatePNG(const std::vector<uint8_t>& data) const {
    if (data.size() < 8) return false;
    
    // Check PNG magic bytes
    const uint8_t png_signature[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    
    for (int i = 0; i < 8; i++) {
        if (data[i] != png_signature[i]) {
            return false;
        }
    }
    
    return true;
}

bool ContentSecurityScanner::ValidateWebP(const std::vector<uint8_t>& data) const {
    if (data.size() < 12) return false;
    
    // Check RIFF header
    if (memcmp(data.data(), "RIFF", 4) != 0) {
        return false;
    }
    
    // Check WebP signature
    if (memcmp(data.data() + 8, "WEBP", 4) != 0) {
        return false;
    }
    
    return true;
}

bool ContentSecurityScanner::ValidateHEIF(const std::vector<uint8_t>& data) const {
    if (data.size() < 12) return false;
    
    // Check ftyp box (HEIF container format)
    if (memcmp(data.data() + 4, "ftyp", 4) != 0) {
        return false;
    }
    
    // Check for HEIF brand
    const char* brands[] = {"heic", "heix", "hevc", "hevx", "mif1"};
    bool valid_brand = false;
    
    for (const char* brand : brands) {
        if (data.size() >= 16 && memcmp(data.data() + 8, brand, 4) == 0) {
            valid_brand = true;
            break;
        }
    }
    
    return valid_brand;
}

InputSanitizer::InputSanitizer() {
    InitializeEntities();
    
    dangerous_tags_ = {
        "script", "iframe", "object", "embed", "applet", 
        "link", "meta", "style", "base", "form"
    };
    
    dangerous_attributes_ = {
        "onclick", "onload", "onerror", "onmouseover", "onmouseout",
        "onfocus", "onblur", "onchange", "onsubmit", "href", "src"
    };
}

void InputSanitizer::InitializeEntities() {
    html_entities_["&"] = "&amp;";
    html_entities_["<"] = "&lt;";
    html_entities_[">"] = "&gt;";
    html_entities_["\""] = "&quot;";
    html_entities_["'"] = "&#x27;";
    html_entities_["/"] = "&#x2F;";
}

std::string InputSanitizer::SanitizeText(const std::string& input, bool allow_basic_formatting) {
    std::string result = input;
    
    // Remove control characters
    result = RemoveControlCharacters(result);
    
    // Normalize whitespace
    result = NormalizeWhitespace(result);
    
    if (!allow_basic_formatting) {
        // Escape HTML entities
        result = EscapeHTML(result);
    } else {
        // Remove only dangerous tags and attributes
        for (const auto& tag : dangerous_tags_) {
            std::regex tag_regex("<\\s*" + tag + "[^>]*>", std::regex_constants::icase);
            result = std::regex_replace(result, tag_regex, "");
            
            std::regex close_tag_regex("<\\s*/\\s*" + tag + "\\s*>", std::regex_constants::icase);
            result = std::regex_replace(result, close_tag_regex, "");
        }
    }
    
    return result;
}

std::string InputSanitizer::EscapeHTML(const std::string& input) {
    std::string result = input;
    
    for (const auto& entity : html_entities_) {
        size_t pos = 0;
        while ((pos = result.find(entity.first, pos)) != std::string::npos) {
            result.replace(pos, entity.first.length(), entity.second);
            pos += entity.second.length();
        }
    }
    
    return result;
}

std::string InputSanitizer::RemoveControlCharacters(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    
    for (unsigned char c : input) {
        // Allow printable characters, tabs, newlines, and carriage returns
        if (c >= 32 || c == '\t' || c == '\n' || c == '\r') {
            result.push_back(c);
        }
    }
    
    return result;
}

std::string InputSanitizer::NormalizeWhitespace(const std::string& input) {
    std::string result = input;
    
    // Replace multiple whitespace with single space
    result = std::regex_replace(result, std::regex(R"(\s+)"), " ");
    
    // Trim leading and trailing whitespace
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    
    return result;
}

std::string InputSanitizer::SanitizeFilename(const std::string& filename) {
    std::string result = filename;
    
    // Remove/replace dangerous characters
    const std::string dangerous_chars = "\\/:*?\"<>|";
    for (char c : dangerous_chars) {
        std::replace(result.begin(), result.end(), c, '_');
    }
    
    // Remove control characters
    result = RemoveControlCharacters(result);
    
    // Limit length
    if (result.length() > 255) {
        result = result.substr(0, 255);
    }
    
    // Ensure it's not empty or just dots
    if (result.empty() || result == "." || result == "..") {
        result = "sanitized_filename";
    }
    
    return result;
}

SecureMemoryManager& SecureMemoryManager::GetInstance() {
    static SecureMemoryManager instance;
    return instance;
}

SecureMemoryManager::SecureMemoryManager() {
    // Initialize memory management
}

SecureMemoryManager::~SecureMemoryManager() {
    // Check for leaks on destruction
    auto leaks = DetectLeaks();
    if (!leaks.empty()) {
        std::cerr << "Memory leaks detected: " << leaks.size() << " allocations" << std::endl;
        PrintMemoryReport();
    }
}

void* SecureMemoryManager::SecureAlloc(size_t size, const std::string& location) {
    void* ptr = malloc(size);
    if (ptr) {
        // Zero the memory
        memset(ptr, 0, size);
        
        RecordAllocation(ptr, size, location);
    }
    return ptr;
}

void SecureMemoryManager::SecureFree(void* ptr) {
    if (!ptr) return;
    
    {
        std::lock_guard<std::mutex> lock(allocations_mutex_);
        auto it = allocations_.find(ptr);
        if (it != allocations_.end()) {
            // Zero memory before freeing
            SecureZero(ptr, it->second.size);
            allocations_.erase(it);
            deallocation_count_++;
        }
    }
    
    free(ptr);
}

void SecureMemoryManager::SecureZero(void* ptr, size_t size) {
    if (ptr && size > 0) {
        // Use volatile to prevent optimization
        volatile uint8_t* p = static_cast<volatile uint8_t*>(ptr);
        for (size_t i = 0; i < size; i++) {
            p[i] = 0;
        }
    }
}

void SecureMemoryManager::RecordAllocation(void* ptr, size_t size, const std::string& location) {
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    
    AllocationInfo info;
    info.ptr = ptr;
    info.size = size;
    info.allocated_at = std::chrono::system_clock::now();
    info.source_location = location;
    
    allocations_[ptr] = info;
    
    size_t current_total = total_allocated_ += size;
    size_t current_peak = peak_allocated_.load();
    while (current_total > current_peak && 
           !peak_allocated_.compare_exchange_weak(current_peak, current_total)) {
        // Retry
    }
    
    allocation_count_++;
}

std::vector<SecureMemoryManager::AllocationInfo> SecureMemoryManager::DetectLeaks() const {
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    
    std::vector<AllocationInfo> leaks;
    for (const auto& allocation : allocations_) {
        leaks.push_back(allocation.second);
    }
    
    return leaks;
}

MemoryStats SecureMemoryManager::GetMemoryStats() const {
    MemoryStats stats;
    
    std::lock_guard<std::mutex> lock(allocations_mutex_);
    
    stats.current_usage_bytes = total_allocated_;
    stats.peak_usage_bytes = peak_allocated_;
    stats.allocated_blocks = allocation_count_;
    stats.freed_blocks = deallocation_count_;
    stats.last_updated = std::chrono::system_clock::now();
    
    // Calculate fragmentation (simplified)
    if (allocations_.size() > 0) {
        stats.fragmentation_ratio = static_cast<double>(allocations_.size()) / allocation_count_;
    }
    
    return stats;
}

namespace SecurityUtils {

std::string CalculateSHA256(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::string CalculateMD5(const std::vector<uint8_t>& data) {
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5(data.data(), data.size(), hash);
    
    std::ostringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::vector<uint8_t> GenerateRandomBytes(size_t count) {
    std::vector<uint8_t> buffer(count);
    
    if (RAND_bytes(buffer.data(), count) != 1) {
        // Fallback to system random
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (auto& byte : buffer) {
            byte = static_cast<uint8_t>(dis(gen));
        }
    }
    
    return buffer;
}

bool VerifyChecksum(const std::vector<uint8_t>& data, const std::string& expected_hash, const std::string& algorithm) {
    std::string calculated_hash;
    
    if (algorithm == "SHA256") {
        calculated_hash = CalculateSHA256(data);
    } else if (algorithm == "MD5") {
        calculated_hash = CalculateMD5(data);
    } else {
        return false;
    }
    
    return calculated_hash == expected_hash;
}

bool RunSecuritySelfTest() {
    try {
        // Test path validation
        SecurePathValidator validator;
        if (validator.IsPathSafe("../../../etc/passwd")) {
            return false; // Should be detected as unsafe
        }
        
        // Test content scanning
        ContentSecurityScanner scanner;
        std::vector<uint8_t> malicious_data = {'<', 's', 'c', 'r', 'i', 'p', 't', '>'};
        auto result = scanner.ScanContent(malicious_data);
        if (result.is_safe) {
            return false; // Should be detected as unsafe
        }
        
        // Test memory management
        auto& mem_mgr = SecureMemoryManager::GetInstance();
        void* test_ptr = mem_mgr.SecureAlloc(1024, "self_test");
        if (!test_ptr) {
            return false;
        }
        mem_mgr.SecureFree(test_ptr);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Security self-test failed: " << e.what() << std::endl;
        return false;
    }
}

} // namespace SecurityUtils

} // namespace StreamDAB