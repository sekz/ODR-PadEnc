/*
    Security and Performance Utilities
    Copyright (C) 2024 StreamDAB Project
    
    File path traversal protection
    Content validation and sanitization
    Memory safety improvements
    Performance optimizations
*/

#ifndef SECURITY_UTILS_H_
#define SECURITY_UTILS_H_

#include "common.h"
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

namespace StreamDAB {

// Security validation results
struct SecurityValidation {
    bool is_safe = true;
    std::vector<std::string> threats_detected;
    std::vector<std::string> warnings;
    std::string sanitized_content;
    double risk_score = 0.0; // 0.0 = safe, 1.0 = maximum risk
};

// File validation results
struct FileValidation {
    bool is_valid = false;
    bool is_safe = false;
    std::string file_type;
    size_t file_size = 0;
    std::string mime_type;
    std::vector<std::string> security_issues;
    std::string sanitized_path;
};

// Memory usage statistics
struct MemoryStats {
    size_t current_usage_bytes = 0;
    size_t peak_usage_bytes = 0;
    size_t allocated_blocks = 0;
    size_t freed_blocks = 0;
    double fragmentation_ratio = 0.0;
    std::chrono::system_clock::time_point last_updated;
};

// Performance metrics
struct PerformanceMetrics {
    std::chrono::microseconds average_processing_time{0};
    std::chrono::microseconds peak_processing_time{0};
    size_t operations_per_second = 0;
    double cpu_usage_percent = 0.0;
    size_t thread_count = 0;
    size_t queue_depth = 0;
    std::chrono::system_clock::time_point measurement_time;
};

// Secure path validator
class SecurePathValidator {
private:
    std::vector<std::string> allowed_directories_;
    std::vector<std::string> blocked_patterns_;
    bool strict_mode_ = true;
    
    bool ContainsTraversal(const std::string& path) const;
    bool IsInAllowedDirectory(const std::string& path) const;
    bool MatchesBlockedPattern(const std::string& path) const;
    std::string ResolvePath(const std::string& path) const;
    
public:
    SecurePathValidator();
    explicit SecurePathValidator(const std::vector<std::string>& allowed_dirs, bool strict = true);
    
    // Path validation
    FileValidation ValidatePath(const std::string& path) const;
    std::string SanitizePath(const std::string& path) const;
    bool IsPathSafe(const std::string& path) const;
    
    // Configuration
    void AddAllowedDirectory(const std::string& directory);
    void AddBlockedPattern(const std::string& pattern);
    void SetStrictMode(bool strict);
    
    // Utility methods
    static std::string NormalizePath(const std::string& path);
    static bool IsAbsolutePath(const std::string& path);
    static std::string GetParentDirectory(const std::string& path);
    static std::string GetFileName(const std::string& path);
};

// Content security scanner
class ContentSecurityScanner {
private:
    std::vector<std::string> malicious_patterns_;
    std::vector<std::string> suspicious_extensions_;
    std::map<std::string, std::function<bool(const std::vector<uint8_t>&)>> format_validators_;
    
    bool ScanForMaliciousContent(const std::vector<uint8_t>& data) const;
    bool ValidateImageFormat(const std::vector<uint8_t>& data, const std::string& expected_format) const;
    bool ValidateTextContent(const std::string& text) const;
    
public:
    ContentSecurityScanner();
    
    // Content validation
    SecurityValidation ScanContent(const std::vector<uint8_t>& data, const std::string& content_type = "");
    SecurityValidation ScanTextContent(const std::string& text);
    SecurityValidation ScanImageFile(const std::string& filepath);
    
    // File format validation
    bool ValidateJPEG(const std::vector<uint8_t>& data) const;
    bool ValidatePNG(const std::vector<uint8_t>& data) const;
    bool ValidateWebP(const std::vector<uint8_t>& data) const;
    bool ValidateHEIF(const std::vector<uint8_t>& data) const;
    
    // Configuration
    void AddMaliciousPattern(const std::string& pattern);
    void LoadMaliciousPatterns(const std::string& config_file);
};

// Input sanitizer
class InputSanitizer {
private:
    std::map<std::string, std::string> html_entities_;
    std::vector<std::string> dangerous_tags_;
    std::vector<std::string> dangerous_attributes_;
    
    void InitializeEntities();
    
public:
    InputSanitizer();
    
    // Text sanitization
    std::string SanitizeText(const std::string& input, bool allow_basic_formatting = false);
    std::string EscapeHTML(const std::string& input);
    std::string UnescapeHTML(const std::string& input);
    std::string RemoveControlCharacters(const std::string& input);
    std::string NormalizeWhitespace(const std::string& input);
    
    // Path sanitization
    std::string SanitizeFilename(const std::string& filename);
    std::string SanitizeDirectoryName(const std::string& dirname);
    
    // URL sanitization
    std::string SanitizeURL(const std::string& url);
    bool IsValidURL(const std::string& url) const;
    
    // Configuration
    void SetAllowedTags(const std::vector<std::string>& tags);
    void SetAllowedAttributes(const std::vector<std::string>& attributes);
};

// Memory manager with leak detection
class SecureMemoryManager {
private:
    struct AllocationInfo {
        void* ptr;
        size_t size;
        std::chrono::system_clock::time_point allocated_at;
        std::string source_location;
    };
    
    std::map<void*, AllocationInfo> allocations_;
    std::mutex allocations_mutex_;
    std::atomic<size_t> total_allocated_{0};
    std::atomic<size_t> peak_allocated_{0};
    std::atomic<size_t> allocation_count_{0};
    std::atomic<size_t> deallocation_count_{0};
    
    void RecordAllocation(void* ptr, size_t size, const std::string& location = "");
    void RecordDeallocation(void* ptr);
    
public:
    SecureMemoryManager();
    ~SecureMemoryManager();
    
    // Memory allocation/deallocation
    void* SecureAlloc(size_t size, const std::string& location = "");
    void SecureFree(void* ptr);
    void* SecureRealloc(void* ptr, size_t new_size, const std::string& location = "");
    
    // Secure memory operations
    void SecureZero(void* ptr, size_t size);
    bool SecureCompare(const void* ptr1, const void* ptr2, size_t size);
    
    // Memory statistics and leak detection
    MemoryStats GetMemoryStats() const;
    std::vector<AllocationInfo> DetectLeaks() const;
    bool HasLeaks() const;
    void PrintMemoryReport() const;
    
    // Memory pool management
    void* AllocateFromPool(size_t size);
    void ReturnToPool(void* ptr, size_t size);
    void OptimizePools();
    
    // Singleton access
    static SecureMemoryManager& GetInstance();
};

// Performance monitor
class PerformanceMonitor {
private:
    struct TimingData {
        std::chrono::high_resolution_clock::time_point start_time;
        std::chrono::microseconds total_time{0};
        size_t call_count = 0;
        std::chrono::microseconds min_time{std::chrono::microseconds::max()};
        std::chrono::microseconds max_time{0};
    };
    
    std::map<std::string, TimingData> timing_data_;
    std::mutex timing_mutex_;
    std::atomic<bool> monitoring_active_{true};
    std::thread monitoring_thread_;
    
    void MonitoringLoop();
    
public:
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    // Performance measurement
    class ScopedTimer {
    private:
        PerformanceMonitor& monitor_;
        std::string operation_name_;
        std::chrono::high_resolution_clock::time_point start_time_;
        
    public:
        ScopedTimer(PerformanceMonitor& monitor, const std::string& operation);
        ~ScopedTimer();
    };
    
    // Timing operations
    void StartTiming(const std::string& operation);
    void EndTiming(const std::string& operation);
    ScopedTimer CreateScopedTimer(const std::string& operation);
    
    // Performance reporting
    PerformanceMetrics GetMetrics(const std::string& operation) const;
    std::map<std::string, PerformanceMetrics> GetAllMetrics() const;
    void PrintPerformanceReport() const;
    
    // System monitoring
    double GetCPUUsage() const;
    size_t GetMemoryUsage() const;
    size_t GetThreadCount() const;
    
    // Control
    void Enable();
    void Disable();
    void Reset();
    void ResetOperation(const std::string& operation);
};

// Thread pool for concurrent processing
class ThreadPool {
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_tasks_{0};
    
public:
    explicit ThreadPool(size_t thread_count = std::thread::hardware_concurrency());
    ~ThreadPool();
    
    // Task submission
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    
    void SubmitTask(std::function<void()> task);
    
    // Pool management
    size_t GetThreadCount() const;
    size_t GetActiveTaskCount() const;
    size_t GetQueuedTaskCount() const;
    
    void WaitForAllTasks();
    void Stop();
    void Resize(size_t new_thread_count);
};

// Buffer overflow protection
class SafeBuffer {
private:
    std::unique_ptr<uint8_t[]> data_;
    size_t size_;
    size_t capacity_;
    bool read_only_ = false;
    
    void CheckBounds(size_t offset, size_t length) const;
    
public:
    explicit SafeBuffer(size_t capacity);
    SafeBuffer(const void* data, size_t size);
    SafeBuffer(const SafeBuffer& other);
    SafeBuffer(SafeBuffer&& other) noexcept;
    
    SafeBuffer& operator=(const SafeBuffer& other);
    SafeBuffer& operator=(SafeBuffer&& other) noexcept;
    
    // Safe data access
    bool Write(size_t offset, const void* data, size_t length);
    bool Read(size_t offset, void* data, size_t length) const;
    bool Append(const void* data, size_t length);
    
    // Safe string operations
    bool WriteString(size_t offset, const std::string& str);
    std::string ReadString(size_t offset, size_t max_length) const;
    
    // Buffer properties
    size_t Size() const { return size_; }
    size_t Capacity() const { return capacity_; }
    bool IsEmpty() const { return size_ == 0; }
    bool IsFull() const { return size_ == capacity_; }
    
    // Buffer operations
    void Clear();
    bool Resize(size_t new_size);
    bool Reserve(size_t new_capacity);
    
    // Safe access
    const uint8_t* Data() const { return data_.get(); }
    uint8_t* Data() { return read_only_ ? nullptr : data_.get(); }
    
    void SetReadOnly(bool read_only = true) { read_only_ = read_only; }
    bool IsReadOnly() const { return read_only_; }
};

// Security utility functions
namespace SecurityUtils {
    // Cryptographic functions
    std::string CalculateSHA256(const std::vector<uint8_t>& data);
    std::string CalculateMD5(const std::vector<uint8_t>& data);
    bool VerifyChecksum(const std::vector<uint8_t>& data, const std::string& expected_hash, const std::string& algorithm = "SHA256");
    
    // Random number generation
    std::vector<uint8_t> GenerateRandomBytes(size_t count);
    std::string GenerateRandomString(size_t length, const std::string& charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");
    
    // Time-based operations
    bool IsTimestampValid(std::chrono::system_clock::time_point timestamp, std::chrono::seconds max_age);
    std::string CreateTimestampedToken(const std::string& data);
    bool VerifyTimestampedToken(const std::string& token, std::chrono::seconds max_age);
    
    // System security
    void SecureShutdown();
    bool RunSecuritySelfTest();
    std::vector<std::string> GetSecurityWarnings();
}

} // namespace StreamDAB

#endif // SECURITY_UTILS_H_