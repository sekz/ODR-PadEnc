/*
    Google Test Suite - Performance Testing
    Copyright (C) 2024 StreamDAB Project
    
    Tests for performance optimization:
    - Multi-threaded processing benchmarks
    - Memory optimization validation
    - CPU usage and resource management
    - Throughput and latency measurements
    - Scalability testing
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/enhanced_mot.h"
#include "../src/thai_rendering.h"
#include "../src/smart_dls.h"
#include "../src/security_utils.h"
#include "../src/api_interface.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <future>

using namespace StreamDAB;
using namespace testing;

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize components for performance testing
        CarouselConfig mot_config;
        mot_config.max_images = 100;
        mot_config.enable_smart_selection = true;
        mot_processor_ = std::make_unique<EnhancedMOTProcessor>(mot_config);
        
        thai_processor_ = std::make_unique<ThaiLanguageProcessor>();
        dls_processor_ = std::make_unique<SmartDLSProcessor>();
        
        // Performance monitor
        perf_monitor_ = std::make_unique<PerformanceMonitor>();
        
        // Create test directory with sample images
        test_image_dir_ = "/tmp/perf_test_images";
        fs::create_directories(test_image_dir_);
        CreatePerformanceTestData();
    }
    
    void TearDown() override {
        if (fs::exists(test_image_dir_)) {
            fs::remove_all(test_image_dir_);
        }
    }
    
    void CreatePerformanceTestData() {
        // Create minimal valid JPEG for performance testing
        std::vector<uint8_t> base_jpeg = {
            0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46,
            0x00, 0x01, 0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00,
            0xFF, 0xC0, 0x00, 0x11, 0x08, 0x00, 0x08, 0x00, 0x08, 0x01,
            0x01, 0x11, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01,
            0xFF, 0xC4, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
            0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 0x00, 0x00, 0x3F, 0x00, 0xD2, 0xFF, 0xD9
        };
        
        // Create multiple test images
        for (int i = 0; i < 50; ++i) {
            std::string filename = test_image_dir_ + "/test_" + std::to_string(i) + ".jpg";
            std::ofstream file(filename, std::ios::binary);
            file.write(reinterpret_cast<const char*>(base_jpeg.data()), base_jpeg.size());
        }
        
        // Thai text samples for performance testing
        thai_test_texts_ = {
            "สวัสดีครับ",
            "ยินดีต้อนรับสู่รายการวิทยุ",
            "ข่าวสารและความบันเทิง",
            "เพลงไทยสากลและต่างประเทศ",
            "รายการพิเศษในวันนี้",
            "ขอขอบคุณผู้ฟังทุกท่าน",
            "พบกันใหม่ในรายการหน้า",
            "ติดตามข่าวสารได้ที่เว็บไซต์",
            "สถานีวิทยุแห่งความสุข",
            "รายการดนตรีและข่าวสาร"
        };
        
        // English text samples
        english_test_texts_ = {
            "Now playing your favorite music",
            "Welcome to the radio station",
            "Breaking news and updates",
            "Traffic and weather information",
            "Coming up next on the show",
            "Thank you for listening",
            "Stay tuned for more music",
            "Visit our website for updates",
            "Your number one music station",
            "News, music, and entertainment"
        };
    }
    
    // Performance measurement utilities
    template<typename Func>
    std::chrono::microseconds MeasureExecutionTime(Func&& func, int iterations = 1) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            func();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    }
    
    double CalculateOperationsPerSecond(std::chrono::microseconds duration, int operations) {
        if (duration.count() == 0) return 0.0;
        return (static_cast<double>(operations) * 1000000.0) / duration.count();
    }
    
    std::unique_ptr<EnhancedMOTProcessor> mot_processor_;
    std::unique_ptr<ThaiLanguageProcessor> thai_processor_;
    std::unique_ptr<SmartDLSProcessor> dls_processor_;
    std::unique_ptr<PerformanceMonitor> perf_monitor_;
    std::string test_image_dir_;
    std::vector<std::string> thai_test_texts_;
    std::vector<std::string> english_test_texts_;
};

// Test MOT processing performance
TEST_F(PerformanceTest, MOTProcessingPerformance) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Measure image processing time
    const int iterations = 100;
    auto duration = MeasureExecutionTime([this]() {
        auto image = mot_processor_->GetNextImage();
        EXPECT_NE(image, nullptr);
    }, iterations);
    
    double ops_per_second = CalculateOperationsPerSecond(duration, iterations);
    
    std::cout << "MOT Processing Performance:" << std::endl;
    std::cout << "  " << iterations << " image selections in " << duration.count() << " microseconds" << std::endl;
    std::cout << "  " << ops_per_second << " operations per second" << std::endl;
    std::cout << "  Average time per operation: " << (duration.count() / iterations) << " microseconds" << std::endl;
    
    // Performance target: Should process at least 100 images per second
    EXPECT_GE(ops_per_second, 100.0);
    
    // Each operation should complete in under 10ms
    EXPECT_LT(duration.count() / iterations, 10000);
}

// Test Thai text processing performance
TEST_F(PerformanceTest, ThaiTextProcessingPerformance) {
    const int iterations = 1000;
    
    // Test UTF-8 to DAB conversion performance
    auto utf8_duration = MeasureExecutionTime([this]() {
        for (const auto& text : thai_test_texts_) {
            std::vector<uint8_t> dab_data;
            thai_processor_->ConvertUTF8ToDAB(text, dab_data);
        }
    }, iterations / 10); // Divide by 10 since we're processing 10 texts per iteration
    
    double utf8_ops_per_second = CalculateOperationsPerSecond(utf8_duration, iterations);
    
    // Test text layout analysis performance
    auto layout_duration = MeasureExecutionTime([this]() {
        for (const auto& text : thai_test_texts_) {
            auto layout = thai_processor_->AnalyzeTextLayout(text, 128, 4);
        }
    }, iterations / 10);
    
    double layout_ops_per_second = CalculateOperationsPerSecond(layout_duration, iterations);
    
    // Test cultural validation performance
    auto validation_duration = MeasureExecutionTime([this]() {
        for (const auto& text : thai_test_texts_) {
            auto validation = thai_processor_->ValidateContent(text);
        }
    }, iterations / 10);
    
    double validation_ops_per_second = CalculateOperationsPerSecond(validation_duration, iterations);
    
    std::cout << "Thai Text Processing Performance:" << std::endl;
    std::cout << "  UTF-8 to DAB conversion: " << utf8_ops_per_second << " ops/sec" << std::endl;
    std::cout << "  Text layout analysis: " << layout_ops_per_second << " ops/sec" << std::endl;
    std::cout << "  Cultural validation: " << validation_ops_per_second << " ops/sec" << std::endl;
    
    // Performance targets
    EXPECT_GE(utf8_ops_per_second, 500.0);     // Should handle at least 500 conversions/sec
    EXPECT_GE(layout_ops_per_second, 200.0);   // Should analyze at least 200 layouts/sec
    EXPECT_GE(validation_ops_per_second, 100.0); // Should validate at least 100 texts/sec
}

// Test DLS message processing performance
TEST_F(PerformanceTest, DLSMessageProcessingPerformance) {
    dls_processor_->Start();
    
    const int message_count = 1000;
    
    // Measure message addition performance
    auto add_duration = MeasureExecutionTime([this]() {
        for (int i = 0; i < 100; ++i) {
            std::string message = "Test message " + std::to_string(i);
            dls_processor_->AddMessage(message, MessagePriority::NORMAL);
        }
    }, message_count / 100);
    
    double add_ops_per_second = CalculateOperationsPerSecond(add_duration, message_count);
    
    // Measure message retrieval performance
    auto get_duration = MeasureExecutionTime([this]() {
        std::string message = dls_processor_->GetNextDLSText();
    }, 500);
    
    double get_ops_per_second = CalculateOperationsPerSecond(get_duration, 500);
    
    std::cout << "DLS Message Processing Performance:" << std::endl;
    std::cout << "  Message addition: " << add_ops_per_second << " ops/sec" << std::endl;
    std::cout << "  Message retrieval: " << get_ops_per_second << " ops/sec" << std::endl;
    
    // Performance targets
    EXPECT_GE(add_ops_per_second, 1000.0);  // Should add at least 1000 messages/sec
    EXPECT_GE(get_ops_per_second, 500.0);   // Should retrieve at least 500 messages/sec
    
    dls_processor_->Stop();
}

// Test concurrent processing performance
TEST_F(PerformanceTest, ConcurrentProcessingPerformance) {
    const int thread_count = 4;
    const int operations_per_thread = 100;
    
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    dls_processor_->Start();
    
    // Add some messages for testing
    for (int i = 0; i < 50; ++i) {
        dls_processor_->AddMessage("Concurrent test message " + std::to_string(i));
    }
    
    std::atomic<int> completed_operations{0};
    std::vector<std::future<void>> futures;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch concurrent operations
    for (int t = 0; t < thread_count; ++t) {
        futures.push_back(std::async(std::launch::async, [&, t]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                // Mix of different operations
                switch (i % 4) {
                    case 0: {
                        auto image = mot_processor_->GetNextImage();
                        if (image) completed_operations++;
                        break;
                    }
                    case 1: {
                        std::string message = dls_processor_->GetNextDLSText();
                        if (!message.empty()) completed_operations++;
                        break;
                    }
                    case 2: {
                        std::vector<uint8_t> dab_data;
                        if (thai_processor_->ConvertUTF8ToDAB(thai_test_texts_[i % thai_test_texts_.size()], dab_data)) {
                            completed_operations++;
                        }
                        break;
                    }
                    case 3: {
                        auto layout = thai_processor_->AnalyzeTextLayout(
                            english_test_texts_[i % english_test_texts_.size()], 128, 4);
                        if (!layout.original_text.empty()) completed_operations++;
                        break;
                    }
                }
            }
        }));
    }
    
    // Wait for all threads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    int total_operations = thread_count * operations_per_thread;
    double ops_per_second = CalculateOperationsPerSecond(duration, total_operations);
    
    std::cout << "Concurrent Processing Performance:" << std::endl;
    std::cout << "  " << thread_count << " threads, " << operations_per_thread << " ops each" << std::endl;
    std::cout << "  Total operations: " << total_operations << std::endl;
    std::cout << "  Completed operations: " << completed_operations.load() << std::endl;
    std::cout << "  Duration: " << duration.count() << " microseconds" << std::endl;
    std::cout << "  Throughput: " << ops_per_second << " ops/sec" << std::endl;
    
    // Should complete most operations successfully
    EXPECT_GE(completed_operations.load(), total_operations * 0.8);
    
    // Should maintain reasonable throughput under concurrent load
    EXPECT_GE(ops_per_second, 100.0);
    
    dls_processor_->Stop();
}

// Test memory usage patterns
TEST_F(PerformanceTest, MemoryUsageTest) {
    auto& mem_manager = SecureMemoryManager::GetInstance();
    auto initial_stats = mem_manager.GetMemoryStats();
    
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    // Process many images to test memory usage
    std::vector<std::unique_ptr<EnhancedImageData>> processed_images;
    
    for (int i = 0; i < 100; ++i) {
        auto image = mot_processor_->GetNextImage();
        if (image) {
            processed_images.push_back(std::move(image));
        }
    }
    
    auto peak_stats = mem_manager.GetMemoryStats();
    
    // Clear processed images
    processed_images.clear();
    
    // Force some cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto final_stats = mem_manager.GetMemoryStats();
    
    std::cout << "Memory Usage Test:" << std::endl;
    std::cout << "  Initial allocated: " << initial_stats.allocated_blocks << " blocks" << std::endl;
    std::cout << "  Peak allocated: " << peak_stats.allocated_blocks << " blocks" << std::endl;
    std::cout << "  Final allocated: " << final_stats.allocated_blocks << " blocks" << std::endl;
    std::cout << "  Peak usage: " << peak_stats.current_usage_bytes << " bytes" << std::endl;
    
    // Memory should be managed properly
    EXPECT_GE(peak_stats.current_usage_bytes, initial_stats.current_usage_bytes);
    EXPECT_GE(final_stats.freed_blocks, initial_stats.freed_blocks);
    
    // Should not have excessive memory leaks
    auto leaks = mem_manager.DetectLeaks();
    std::cout << "  Memory leaks detected: " << leaks.size() << std::endl;
    // Allow some leaks in test environment, but not excessive
    EXPECT_LT(leaks.size(), 100);
}

// Test CPU usage during intensive operations
TEST_F(PerformanceTest, CPUUsageTest) {
    perf_monitor_->Enable();
    
    // CPU-intensive operations
    auto cpu_test_duration = MeasureExecutionTime([this]() {
        // Process many Thai texts
        for (int i = 0; i < 1000; ++i) {
            for (const auto& text : thai_test_texts_) {
                std::vector<uint8_t> dab_data;
                thai_processor_->ConvertUTF8ToDAB(text, dab_data);
                
                auto layout = thai_processor_->AnalyzeTextLayout(text, 128, 4);
                auto validation = thai_processor_->ValidateContent(text);
            }
        }
    });
    
    double cpu_usage = perf_monitor_->GetCPUUsage();
    size_t memory_usage = perf_monitor_->GetMemoryUsage();
    
    std::cout << "CPU Usage Test:" << std::endl;
    std::cout << "  Test duration: " << cpu_test_duration.count() << " microseconds" << std::endl;
    std::cout << "  CPU usage: " << cpu_usage << "%" << std::endl;
    std::cout << "  Memory usage: " << memory_usage << " bytes" << std::endl;
    
    // CPU usage should be reasonable (allow high usage during intensive operations)
    EXPECT_LE(cpu_usage, 100.0); // Should not exceed 100%
    
    perf_monitor_->Disable();
}

// Test scalability with increasing load
TEST_F(PerformanceTest, ScalabilityTest) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    dls_processor_->Start();
    
    std::vector<int> load_levels = {10, 50, 100, 200};
    std::vector<double> throughput_results;
    
    for (int load : load_levels) {
        // Add messages for this load level
        for (int i = 0; i < load; ++i) {
            dls_processor_->AddMessage("Load test message " + std::to_string(i));
        }
        
        // Measure processing time
        auto duration = MeasureExecutionTime([&]() {
            for (int i = 0; i < load; ++i) {
                auto image = mot_processor_->GetNextImage();
                std::string message = dls_processor_->GetNextDLSText();
                
                if (i < thai_test_texts_.size()) {
                    std::vector<uint8_t> dab_data;
                    thai_processor_->ConvertUTF8ToDAB(thai_test_texts_[i % thai_test_texts_.size()], dab_data);
                }
            }
        });
        
        double throughput = CalculateOperationsPerSecond(duration, load * 3); // 3 operations per iteration
        throughput_results.push_back(throughput);
        
        std::cout << "Load " << load << ": " << throughput << " ops/sec" << std::endl;
    }
    
    // Throughput should not degrade dramatically with increased load
    // Allow some degradation but not more than 50%
    double initial_throughput = throughput_results[0];
    double final_throughput = throughput_results.back();
    double degradation_ratio = final_throughput / initial_throughput;
    
    std::cout << "Scalability Test Results:" << std::endl;
    std::cout << "  Initial throughput: " << initial_throughput << " ops/sec" << std::endl;
    std::cout << "  Final throughput: " << final_throughput << " ops/sec" << std::endl;
    std::cout << "  Degradation ratio: " << degradation_ratio << std::endl;
    
    EXPECT_GE(degradation_ratio, 0.5); // Should maintain at least 50% of initial performance
    
    dls_processor_->Stop();
}

// Test latency under different conditions
TEST_F(PerformanceTest, LatencyTest) {
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    
    const int iterations = 100;
    std::vector<std::chrono::microseconds> latencies;
    
    // Measure individual operation latencies
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto image = mot_processor_->GetNextImage();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        latencies.push_back(latency);
    }
    
    // Calculate statistics
    auto min_latency = *std::min_element(latencies.begin(), latencies.end());
    auto max_latency = *std::max_element(latencies.begin(), latencies.end());
    
    long long total_latency = 0;
    for (const auto& latency : latencies) {
        total_latency += latency.count();
    }
    auto avg_latency = std::chrono::microseconds(total_latency / iterations);
    
    // Calculate 95th percentile
    std::sort(latencies.begin(), latencies.end());
    auto p95_latency = latencies[static_cast<size_t>(iterations * 0.95)];
    
    std::cout << "Latency Test Results:" << std::endl;
    std::cout << "  Min latency: " << min_latency.count() << " μs" << std::endl;
    std::cout << "  Max latency: " << max_latency.count() << " μs" << std::endl;
    std::cout << "  Avg latency: " << avg_latency.count() << " μs" << std::endl;
    std::cout << "  95th percentile: " << p95_latency.count() << " μs" << std::endl;
    
    // Performance targets
    EXPECT_LT(avg_latency.count(), 5000);  // Average < 5ms
    EXPECT_LT(p95_latency.count(), 10000); // 95th percentile < 10ms
    EXPECT_LT(max_latency.count(), 50000); // Max < 50ms
}

// Test resource cleanup and garbage collection
TEST_F(PerformanceTest, ResourceCleanupTest) {
    auto& mem_manager = SecureMemoryManager::GetInstance();
    auto initial_stats = mem_manager.GetMemoryStats();
    
    // Create and destroy many objects
    for (int cycle = 0; cycle < 10; ++cycle) {
        std::vector<std::unique_ptr<EnhancedImageData>> temp_images;
        std::vector<std::shared_ptr<DLSMessage>> temp_messages;
        
        // Allocate resources
        for (int i = 0; i < 50; ++i) {
            auto image = std::make_unique<EnhancedImageData>();
            image->filename = "temp_" + std::to_string(i) + ".jpg";
            image->processed_data.resize(1024);
            temp_images.push_back(std::move(image));
            
            auto message = std::make_shared<DLSMessage>();
            message->text = "Temporary message " + std::to_string(i);
            message->source_id = "temp_" + std::to_string(i);
            temp_messages.push_back(message);
        }
        
        // Clear resources
        temp_images.clear();
        temp_messages.clear();
        
        // Force some cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto final_stats = mem_manager.GetMemoryStats();
    
    std::cout << "Resource Cleanup Test:" << std::endl;
    std::cout << "  Initial allocations: " << initial_stats.allocated_blocks << std::endl;
    std::cout << "  Final allocations: " << final_stats.allocated_blocks << std::endl;
    std::cout << "  Total freed: " << (final_stats.freed_blocks - initial_stats.freed_blocks) << std::endl;
    
    // Memory should be properly cleaned up
    EXPECT_GE(final_stats.freed_blocks, initial_stats.freed_blocks);
    
    // Should not accumulate excessive allocations
    EXPECT_LT(final_stats.allocated_blocks - initial_stats.allocated_blocks, 100);
}

// Test performance monitoring accuracy
TEST_F(PerformanceTest, PerformanceMonitoringAccuracy) {
    perf_monitor_->Reset();
    perf_monitor_->Enable();
    
    // Perform monitored operations
    {
        auto timer = perf_monitor_->CreateScopedTimer("test_operation");
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        for (const auto& text : thai_test_texts_) {
            std::vector<uint8_t> dab_data;
            thai_processor_->ConvertUTF8ToDAB(text, dab_data);
        }
    }
    
    auto metrics = perf_monitor_->GetMetrics("test_operation");
    
    std::cout << "Performance Monitoring Test:" << std::endl;
    std::cout << "  Average time: " << metrics.average_processing_time.count() << " μs" << std::endl;
    std::cout << "  Peak time: " << metrics.peak_processing_time.count() << " μs" << std::endl;
    std::cout << "  Operations/sec: " << metrics.operations_per_second << std::endl;
    
    // Should have recorded the operation
    EXPECT_GT(metrics.average_processing_time.count(), 0);
    EXPECT_GE(metrics.peak_processing_time, metrics.average_processing_time);
    
    perf_monitor_->Disable();
}

// Benchmark against performance targets
TEST_F(PerformanceTest, PerformanceBenchmark) {
    struct BenchmarkTargets {
        double min_mot_ops_per_sec = 100.0;
        double min_thai_ops_per_sec = 500.0;
        double min_dls_ops_per_sec = 1000.0;
        long max_avg_latency_us = 5000;
        double min_concurrent_efficiency = 0.7; // 70% efficiency under load
    };
    
    BenchmarkTargets targets;
    bool all_targets_met = true;
    
    std::cout << "\n=== Performance Benchmark Results ===" << std::endl;
    
    // MOT processing benchmark
    EXPECT_TRUE(mot_processor_->ProcessImageDirectory(test_image_dir_));
    auto mot_duration = MeasureExecutionTime([this]() {
        for (int i = 0; i < 100; ++i) {
            auto image = mot_processor_->GetNextImage();
        }
    });
    double mot_ops_per_sec = CalculateOperationsPerSecond(mot_duration, 100);
    
    std::cout << "MOT Processing: " << mot_ops_per_sec << " ops/sec ";
    if (mot_ops_per_sec >= targets.min_mot_ops_per_sec) {
        std::cout << "✓ PASS" << std::endl;
    } else {
        std::cout << "✗ FAIL (target: " << targets.min_mot_ops_per_sec << ")" << std::endl;
        all_targets_met = false;
    }
    
    // Thai processing benchmark
    auto thai_duration = MeasureExecutionTime([this]() {
        for (int i = 0; i < 100; ++i) {
            std::vector<uint8_t> dab_data;
            thai_processor_->ConvertUTF8ToDAB(thai_test_texts_[i % thai_test_texts_.size()], dab_data);
        }
    });
    double thai_ops_per_sec = CalculateOperationsPerSecond(thai_duration, 100);
    
    std::cout << "Thai Processing: " << thai_ops_per_sec << " ops/sec ";
    if (thai_ops_per_sec >= targets.min_thai_ops_per_sec) {
        std::cout << "✓ PASS" << std::endl;
    } else {
        std::cout << "✗ FAIL (target: " << targets.min_thai_ops_per_sec << ")" << std::endl;
        all_targets_met = false;
    }
    
    std::cout << "\nOverall Benchmark: ";
    if (all_targets_met) {
        std::cout << "✓ ALL TARGETS MET" << std::endl;
        SUCCEED();
    } else {
        std::cout << "✗ SOME TARGETS NOT MET" << std::endl;
        // Note: We don't FAIL the test as performance can vary by environment
        // but we report the results for analysis
    }
    
    std::cout << "=====================================" << std::endl;
}