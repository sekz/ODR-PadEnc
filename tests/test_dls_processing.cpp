/*
    Google Test Suite - DLS Processing Testing
    Copyright (C) 2024 StreamDAB Project
    
    Tests for enhanced DLS processing:
    - Priority-based message queuing
    - Dynamic message length optimization
    - Context-aware message selection
    - Rich metadata extraction
    - Smart DLS management
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/smart_dls.h"
#include <chrono>
#include <thread>

using namespace StreamDAB;
using namespace testing;

class DLSProcessingTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<SmartDLSQueue>();
        optimizer_ = std::make_unique<MessageLengthOptimizer>();
        selector_ = std::make_unique<ContextAwareSelector>();
        processor_ = std::make_unique<SmartDLSProcessor>();
        
        // Sample messages for testing
        short_message_ = "Hello World";
        long_message_ = "This is a very long message that exceeds the typical DLS length limit and should be optimized or truncated to fit within the required constraints for DAB+ broadcasting systems.";
        thai_message_ = "สวัสดีครับ ยินดีต้อนรับสู่รายการวิทยุ";
        emergency_message_ = "Emergency Alert: Severe weather warning in effect";
        mixed_message_ = "Now Playing: สวัสดี - Hello World";
        
        // Create test messages with different priorities
        CreateTestMessages();
    }
    
    void TearDown() override {
        if (processor_) {
            processor_->Stop();
        }
    }
    
    void CreateTestMessages() {
        // High priority message
        high_priority_msg_ = std::make_shared<DLSMessage>();
        high_priority_msg_->text = "Important announcement";
        high_priority_msg_->priority = MessagePriority::HIGH;
        high_priority_msg_->source = ContentSource::MANUAL;
        high_priority_msg_->source_id = "high_001";
        high_priority_msg_->created_at = std::chrono::system_clock::now();
        high_priority_msg_->expires_at = high_priority_msg_->created_at + std::chrono::hours{1};
        high_priority_msg_->importance_score = 0.9;
        
        // Normal priority message
        normal_priority_msg_ = std::make_shared<DLSMessage>();
        normal_priority_msg_->text = "Regular program information";
        normal_priority_msg_->priority = MessagePriority::NORMAL;
        normal_priority_msg_->source = ContentSource::METADATA_EXTRACTOR;
        normal_priority_msg_->source_id = "normal_001";
        normal_priority_msg_->created_at = std::chrono::system_clock::now();
        normal_priority_msg_->expires_at = normal_priority_msg_->created_at + std::chrono::hours{2};
        normal_priority_msg_->importance_score = 0.5;
        
        // Low priority message
        low_priority_msg_ = std::make_shared<DLSMessage>();
        low_priority_msg_->text = "Background information";
        low_priority_msg_->priority = MessagePriority::LOW;
        low_priority_msg_->source = ContentSource::RSS_FEED;
        low_priority_msg_->source_id = "low_001";
        low_priority_msg_->created_at = std::chrono::system_clock::now();
        low_priority_msg_->expires_at = low_priority_msg_->created_at + std::chrono::hours{24};
        low_priority_msg_->importance_score = 0.2;
        
        // Emergency message
        emergency_msg_ = std::make_shared<DLSMessage>();
        emergency_msg_->text = emergency_message_;
        emergency_msg_->priority = MessagePriority::EMERGENCY;
        emergency_msg_->source = ContentSource::EMERGENCY_SYSTEM;
        emergency_msg_->source_id = "emergency_001";
        emergency_msg_->created_at = std::chrono::system_clock::now();
        emergency_msg_->expires_at = emergency_msg_->created_at + std::chrono::minutes{30};
        emergency_msg_->importance_score = 1.0;
        emergency_msg_->max_sends = 10;
        
        // Thai message
        thai_msg_ = std::make_shared<DLSMessage>();
        thai_msg_->text = thai_message_;
        thai_msg_->priority = MessagePriority::NORMAL;
        thai_msg_->source = ContentSource::MANUAL;
        thai_msg_->source_id = "thai_001";
        thai_msg_->created_at = std::chrono::system_clock::now();
        thai_msg_->expires_at = thai_msg_->created_at + std::chrono::hours{1};
        thai_msg_->importance_score = 0.7;
        thai_msg_->is_thai_content = true;
    }
    
    std::unique_ptr<SmartDLSQueue> queue_;
    std::unique_ptr<MessageLengthOptimizer> optimizer_;
    std::unique_ptr<ContextAwareSelector> selector_;
    std::unique_ptr<SmartDLSProcessor> processor_;
    
    std::string short_message_;
    std::string long_message_;
    std::string thai_message_;
    std::string emergency_message_;
    std::string mixed_message_;
    
    std::shared_ptr<DLSMessage> high_priority_msg_;
    std::shared_ptr<DLSMessage> normal_priority_msg_;
    std::shared_ptr<DLSMessage> low_priority_msg_;
    std::shared_ptr<DLSMessage> emergency_msg_;
    std::shared_ptr<DLSMessage> thai_msg_;
};

// Test queue initialization
TEST_F(DLSProcessingTest, QueueInitialization) {
    EXPECT_NE(queue_, nullptr);
    EXPECT_EQ(queue_->GetQueueSize(), 0);
}

// Test adding messages to queue
TEST_F(DLSProcessingTest, AddingMessages) {
    EXPECT_TRUE(queue_->AddMessage(high_priority_msg_));
    EXPECT_EQ(queue_->GetQueueSize(), 1);
    
    EXPECT_TRUE(queue_->AddMessage(normal_priority_msg_));
    EXPECT_EQ(queue_->GetQueueSize(), 2);
    
    EXPECT_TRUE(queue_->AddMessage(low_priority_msg_));
    EXPECT_EQ(queue_->GetQueueSize(), 3);
}

// Test priority-based message selection
TEST_F(DLSProcessingTest, PriorityBasedSelection) {
    // Add messages in reverse priority order
    EXPECT_TRUE(queue_->AddMessage(low_priority_msg_));
    EXPECT_TRUE(queue_->AddMessage(normal_priority_msg_));
    EXPECT_TRUE(queue_->AddMessage(high_priority_msg_));
    EXPECT_TRUE(queue_->AddMessage(emergency_msg_));
    
    SelectionCriteria criteria;
    
    // Emergency message should come first
    auto message = queue_->GetNextMessage(criteria);
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->priority, MessagePriority::EMERGENCY);
    
    // High priority should come next
    message = queue_->GetNextMessage(criteria);
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->priority, MessagePriority::HIGH);
}

// Test message expiration
TEST_F(DLSProcessingTest, MessageExpiration) {
    // Create expired message
    auto expired_msg = std::make_shared<DLSMessage>();
    expired_msg->text = "Expired message";
    expired_msg->priority = MessagePriority::HIGH;
    expired_msg->source_id = "expired_001";
    expired_msg->created_at = std::chrono::system_clock::now() - std::chrono::hours{2};
    expired_msg->expires_at = expired_msg->created_at + std::chrono::hours{1}; // Expired 1 hour ago
    
    EXPECT_TRUE(queue_->AddMessage(expired_msg));
    EXPECT_TRUE(queue_->AddMessage(high_priority_msg_)); // Valid message
    
    SelectionCriteria criteria;
    auto message = queue_->GetNextMessage(criteria);
    
    // Should get the valid message, not the expired one
    ASSERT_NE(message, nullptr);
    EXPECT_NE(message->source_id, "expired_001");
}

// Test duplicate detection
TEST_F(DLSProcessingTest, DuplicateDetection) {
    EXPECT_TRUE(queue_->AddMessage(high_priority_msg_));
    
    // Try to add the same message again
    auto duplicate = std::make_shared<DLSMessage>(*high_priority_msg_);
    duplicate->source_id = "duplicate_001"; // Different ID but same content
    
    // Should be rejected as duplicate (based on content hash)
    bool added = queue_->AddMessage(duplicate);
    
    // Queue size should still be 1
    EXPECT_EQ(queue_->GetQueueSize(), 1);
}

// Test message length optimization
TEST_F(DLSProcessingTest, MessageLengthOptimization) {
    auto result = optimizer_->OptimizeMessage(long_message_, 128);
    
    EXPECT_LE(result.optimized_length, 128);
    EXPECT_LT(result.optimized_length, result.original_length);
    EXPECT_GT(result.compression_ratio, 0.0);
    EXPECT_LT(result.compression_ratio, 1.0);
    EXPECT_FALSE(result.optimized_text.empty());
    EXPECT_GT(result.applied_rules.size(), 0);
    
    // Short message should not be changed
    result = optimizer_->OptimizeMessage(short_message_, 128);
    EXPECT_EQ(result.optimized_text, short_message_);
    EXPECT_EQ(result.compression_ratio, 1.0);
}

// Test abbreviation rules
TEST_F(DLSProcessingTest, AbbreviationRules) {
    std::string test_text = "information and with tonight";
    std::string abbreviated = optimizer_->ApplyAbbreviations(test_text, false);
    
    EXPECT_NE(abbreviated, test_text);
    EXPECT_LT(abbreviated.length(), test_text.length());
    EXPECT_NE(abbreviated.find("info"), std::string::npos);
    EXPECT_NE(abbreviated.find("&"), std::string::npos);
    EXPECT_NE(abbreviated.find("w/"), std::string::npos);
    EXPECT_NE(abbreviated.find("tonite"), std::string::npos);
}

// Test whitespace compression
TEST_F(DLSProcessingTest, WhitespaceCompression) {
    std::string test_text = "Hello    world   \t\n  test  ";
    std::string compressed = optimizer_->CompressWhitespace(test_text);
    
    EXPECT_LT(compressed.length(), test_text.length());
    EXPECT_EQ(compressed, "Hello world test");
}

// Test smart truncation
TEST_F(DLSProcessingTest, SmartTruncation) {
    std::string test_text = "This is a test message with multiple words for truncation testing";
    std::string truncated = optimizer_->SmartTruncate(test_text, 30);
    
    EXPECT_LE(truncated.length(), 30);
    EXPECT_TRUE(truncated.find("...") != std::string::npos || truncated.length() == test_text.length());
    
    // Should try to break at word boundary
    if (truncated != test_text) {
        EXPECT_TRUE(truncated.back() == '.' || truncated.back() == ' ' || isalpha(truncated.back()));
    }
}

// Test context-aware selection
TEST_F(DLSProcessingTest, ContextAwareSelection) {
    selector_->SetCurrentContext(MessageContext::NEWS);
    
    // News context should prefer news-related sources
    SelectionCriteria criteria = selector_->GetCriteriaForContext(MessageContext::NEWS);
    EXPECT_EQ(criteria.preferred_context, MessageContext::NEWS);
    EXPECT_NE(std::find(criteria.allowed_sources.begin(), criteria.allowed_sources.end(), 
                       ContentSource::NEWS_API), criteria.allowed_sources.end());
}

// Test scoring functions
TEST_F(DLSProcessingTest, ScoringFunctions) {
    double score1 = ContextAwareSelector::DefaultScoringFunction(*high_priority_msg_);
    double score2 = ContextAwareSelector::DefaultScoringFunction(*low_priority_msg_);
    
    // High priority message should have higher score
    EXPECT_GT(score1, score2);
    
    // Test priority-based scoring
    score1 = ContextAwareSelector::PriorityBasedScoring(*emergency_msg_);
    score2 = ContextAwareSelector::PriorityBasedScoring(*low_priority_msg_);
    EXPECT_GT(score1, score2);
    
    // Test recency-based scoring
    auto old_msg = std::make_shared<DLSMessage>(*normal_priority_msg_);
    old_msg->created_at = std::chrono::system_clock::now() - std::chrono::hours{24};
    
    score1 = ContextAwareSelector::RecencyBasedScoring(*normal_priority_msg_);
    score2 = ContextAwareSelector::RecencyBasedScoring(*old_msg);
    EXPECT_GT(score1, score2);
}

// Test queue statistics
TEST_F(DLSProcessingTest, QueueStatistics) {
    // Add various messages
    EXPECT_TRUE(queue_->AddMessage(emergency_msg_));
    EXPECT_TRUE(queue_->AddMessage(high_priority_msg_));
    EXPECT_TRUE(queue_->AddMessage(normal_priority_msg_));
    EXPECT_TRUE(queue_->AddMessage(low_priority_msg_));
    
    auto stats = queue_->GetStatistics();
    EXPECT_EQ(stats.total_messages, 4);
    EXPECT_GT(stats.priority_counts.size(), 0);
    EXPECT_GT(stats.source_counts.size(), 0);
    EXPECT_GT(stats.average_importance, 0.0);
}

// Test DLS processor integration
TEST_F(DLSProcessingTest, DLSProcessorIntegration) {
    EXPECT_TRUE(processor_->AddMessage(short_message_));
    EXPECT_TRUE(processor_->AddMessage(long_message_));
    EXPECT_TRUE(processor_->AddMessage(thai_message_, MessagePriority::HIGH));
    
    // Get next DLS text
    std::string dls_text = processor_->GetNextDLSText();
    EXPECT_FALSE(dls_text.empty());
    
    // Should get highest priority message first
    EXPECT_EQ(dls_text, thai_message_);
}

// Test emergency message handling
TEST_F(DLSProcessingTest, EmergencyMessageHandling) {
    EXPECT_TRUE(processor_->AddMessage(normal_priority_msg_->text, MessagePriority::NORMAL));
    EXPECT_TRUE(processor_->AddMessage(emergency_msg_->text, MessagePriority::EMERGENCY));
    
    // Emergency message should be retrieved first
    std::string dls_text = processor_->GetNextDLSText();
    EXPECT_EQ(dls_text, emergency_message_);
}

// Test context switching
TEST_F(DLSProcessingTest, ContextSwitching) {
    processor_->SetContext(MessageContext::NEWS);
    
    // Add news-related message
    EXPECT_TRUE(processor_->AddMessage("Breaking news update", MessagePriority::HIGH, ContentSource::NEWS_API));
    EXPECT_TRUE(processor_->AddMessage("Music information", MessagePriority::NORMAL, ContentSource::METADATA_EXTRACTOR));
    
    // Should prefer news content in news context
    std::string dls_text = processor_->GetNextDLSText();
    EXPECT_EQ(dls_text, "Breaking news update");
}

// Test message repeat constraints
TEST_F(DLSProcessingTest, MessageRepeatConstraints) {
    EXPECT_TRUE(queue_->AddMessage(high_priority_msg_));
    
    SelectionCriteria criteria;
    criteria.allow_repeats = false;
    
    // Get message first time
    auto message = queue_->GetNextMessage(criteria);
    ASSERT_NE(message, nullptr);
    EXPECT_GT(message->send_count, 0);
    
    // Should not get same message again with no-repeat policy
    message = queue_->GetNextMessage(criteria);
    EXPECT_EQ(message, nullptr); // No other messages available
}

// Test max sends limit
TEST_F(DLSProcessingTest, MaxSendsLimit) {
    emergency_msg_->max_sends = 2;
    emergency_msg_->send_count = 2; // Already at limit
    
    EXPECT_TRUE(queue_->AddMessage(emergency_msg_));
    EXPECT_TRUE(queue_->AddMessage(high_priority_msg_));
    
    SelectionCriteria criteria;
    auto message = queue_->GetNextMessage(criteria);
    
    // Should get high priority message since emergency reached send limit
    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->priority, MessagePriority::HIGH);
}

// Test Thai content handling
TEST_F(DLSProcessingTest, ThaiContentHandling) {
    SelectionCriteria criteria;
    criteria.prefer_thai_content = true;
    
    EXPECT_TRUE(queue_->AddMessage(normal_priority_msg_)); // English
    EXPECT_TRUE(queue_->AddMessage(thai_msg_)); // Thai
    
    auto message = queue_->GetNextMessage(criteria);
    ASSERT_NE(message, nullptr);
    
    // Should prefer Thai content when requested
    EXPECT_TRUE(message->is_thai_content);
}

// Test performance metrics
TEST_F(DLSProcessingTest, PerformanceMetrics) {
    processor_->Start();
    
    // Add many messages
    for (int i = 0; i < 100; ++i) {
        processor_->AddMessage("Test message " + std::to_string(i), MessagePriority::NORMAL);
    }
    
    auto stats = processor_->GetStatistics();
    EXPECT_GT(stats.messages_processed, 0);
    EXPECT_EQ(stats.queue_size, 100);
    
    processor_->Stop();
}

// Test queue cleanup
TEST_F(DLSProcessingTest, QueueCleanup) {
    // Add expired messages
    for (int i = 0; i < 10; ++i) {
        auto expired_msg = std::make_shared<DLSMessage>();
        expired_msg->text = "Expired " + std::to_string(i);
        expired_msg->source_id = "expired_" + std::to_string(i);
        expired_msg->created_at = std::chrono::system_clock::now() - std::chrono::hours{2};
        expired_msg->expires_at = expired_msg->created_at + std::chrono::hours{1};
        queue_->AddMessage(expired_msg);
    }
    
    size_t initial_size = queue_->GetQueueSize();
    EXPECT_EQ(initial_size, 10);
    
    size_t cleaned = queue_->CleanupMessages();
    EXPECT_GT(cleaned, 0);
    EXPECT_LT(queue_->GetQueueSize(), initial_size);
}

// Test thread safety
TEST_F(DLSProcessingTest, ThreadSafety) {
    std::atomic<int> successful_adds{0};
    std::atomic<int> successful_gets{0};
    std::atomic<bool> test_running{true};
    
    // Producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < 2; ++i) {
        producers.emplace_back([&, i]() {
            while (test_running) {
                auto msg = std::make_shared<DLSMessage>();
                msg->text = "Message from producer " + std::to_string(i);
                msg->source_id = "producer_" + std::to_string(i) + "_" + std::to_string(successful_adds.load());
                msg->created_at = std::chrono::system_clock::now();
                msg->expires_at = msg->created_at + std::chrono::hours{1};
                
                if (queue_->AddMessage(msg)) {
                    successful_adds++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Consumer threads
    std::vector<std::thread> consumers;
    for (int i = 0; i < 2; ++i) {
        consumers.emplace_back([&]() {
            SelectionCriteria criteria;
            while (test_running) {
                auto message = queue_->GetNextMessage(criteria);
                if (message) {
                    successful_gets++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Let threads run
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    test_running = false;
    
    // Wait for completion
    for (auto& thread : producers) {
        thread.join();
    }
    for (auto& thread : consumers) {
        thread.join();
    }
    
    EXPECT_GT(successful_adds.load(), 0);
    EXPECT_GT(successful_gets.load(), 0);
}

// Test memory management
TEST_F(DLSProcessingTest, MemoryManagement) {
    // Add many messages and verify no memory leaks
    for (int i = 0; i < 1000; ++i) {
        auto msg = std::make_shared<DLSMessage>();
        msg->text = "Memory test message " + std::to_string(i);
        msg->source_id = "memory_" + std::to_string(i);
        msg->created_at = std::chrono::system_clock::now();
        msg->expires_at = msg->created_at + std::chrono::hours{1};
        queue_->AddMessage(msg);
    }
    
    // Process all messages
    SelectionCriteria criteria;
    int processed = 0;
    while (auto message = queue_->GetNextMessage(criteria)) {
        processed++;
        if (processed > 1000) break; // Prevent infinite loop
    }
    
    EXPECT_GT(processed, 0);
    // Test should complete without memory issues
}

// Test edge cases
TEST_F(DLSProcessingTest, EdgeCases) {
    // Test empty message
    EXPECT_FALSE(processor_->AddMessage(""));
    
    // Test null message
    EXPECT_FALSE(queue_->AddMessage(nullptr));
    
    // Test very long message
    std::string huge_message(10000, 'A');
    EXPECT_TRUE(processor_->AddMessage(huge_message));
    std::string result = processor_->GetNextDLSText();
    EXPECT_LE(result.length(), 256); // Should be optimized
    
    // Test message with only whitespace
    EXPECT_TRUE(processor_->AddMessage("   \t\n   "));
    result = processor_->GetNextDLSText();
    EXPECT_TRUE(result.empty() || result.find_first_not_of(" \t\n") != std::string::npos);
}

// Test configuration changes
TEST_F(DLSProcessingTest, ConfigurationChanges) {
    processor_->SetMaxMessageLength(64);
    
    EXPECT_TRUE(processor_->AddMessage(long_message_));
    std::string result = processor_->GetNextDLSText();
    EXPECT_LE(result.length(), 64);
    
    // Change message interval
    processor_->SetMessageInterval(std::chrono::seconds(5));
    processor_->SetEmergencyInterval(std::chrono::seconds(1));
    
    // Intervals should be updated
    // (Would need additional interface to verify this in actual implementation)
}

// Integration test with all components
TEST_F(DLSProcessingTest, FullIntegrationTest) {
    processor_->Start();
    
    // Add various types of messages
    processor_->AddMessage("Regular message", MessagePriority::NORMAL, ContentSource::MANUAL);
    processor_->AddMessage(emergency_message_, MessagePriority::EMERGENCY, ContentSource::EMERGENCY_SYSTEM);
    processor_->AddMessage(thai_message_, MessagePriority::HIGH, ContentSource::MANUAL);
    processor_->AddMessage(long_message_, MessagePriority::LOW, ContentSource::RSS_FEED);
    
    // Set different contexts and retrieve messages
    processor_->SetContext(MessageContext::EMERGENCY);
    std::string msg1 = processor_->GetNextDLSText();
    EXPECT_EQ(msg1, emergency_message_);
    
    processor_->SetContext(MessageContext::LIVE_SHOW);
    std::string msg2 = processor_->GetNextDLSText();
    EXPECT_FALSE(msg2.empty());
    
    auto stats = processor_->GetStatistics();
    EXPECT_GT(stats.messages_processed, 0);
    EXPECT_GT(stats.messages_sent, 0);
    
    processor_->Stop();
}