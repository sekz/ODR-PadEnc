/*
    Smart DLS Processing with Priority Queuing
    Copyright (C) 2024 StreamDAB Project
    
    Dynamic message length optimization
    Context-aware message selection
    Rich metadata extraction
    Social media integration
*/

#ifndef SMART_DLS_H_
#define SMART_DLS_H_

#include "common.h"
#include "dls.h"
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace StreamDAB {

// DLS message priority levels
enum class MessagePriority {
    EMERGENCY = 0,      // Emergency alerts, warnings
    HIGH = 1,           // Important announcements
    NORMAL = 2,         // Regular program info
    LOW = 3,            // Background info, promotions
    BACKGROUND = 4      // Filler content
};

// Message context types
enum class MessageContext {
    LIVE_SHOW,          // Live broadcast context
    AUTOMATED,          // Automated programming
    NEWS,               // News broadcast
    MUSIC,              // Music program
    TALK,               // Talk show
    COMMERCIAL,         // Advertisement
    EMERGENCY,          // Emergency broadcast
    MAINTENANCE,        // System maintenance
    OFF_AIR            // Off-air periods
};

// Content source types
enum class ContentSource {
    MANUAL,             // Manually entered
    RSS_FEED,           // RSS/Atom feed
    SOCIAL_MEDIA,       // Twitter, Facebook, etc.
    METADATA_EXTRACTOR, // Audio metadata
    WEATHER_API,        // Weather service
    TRAFFIC_API,        // Traffic information
    NEWS_API,           // News service
    AUTOMATION_SYSTEM,  // Radio automation
    EMERGENCY_SYSTEM    // Emergency alert system
};

// DLS message metadata
struct DLSMessage {
    std::string text;
    MessagePriority priority = MessagePriority::NORMAL;
    MessageContext context = MessageContext::AUTOMATED;
    ContentSource source = ContentSource::MANUAL;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::chrono::system_clock::time_point last_sent;
    int send_count = 0;
    int max_sends = 0; // 0 = unlimited
    double importance_score = 0.5;
    std::string source_id;
    std::string content_hash;
    std::map<std::string, std::string> metadata;
    bool is_thai_content = false;
    size_t estimated_display_time_ms = 0;
    
    // Message statistics
    struct Stats {
        std::chrono::milliseconds total_display_time{0};
        int display_count = 0;
        double user_engagement = 0.0;
        bool delivery_confirmed = false;
    } stats;
};

// Message selection criteria
struct SelectionCriteria {
    MessageContext preferred_context = MessageContext::AUTOMATED;
    std::vector<ContentSource> allowed_sources;
    std::vector<ContentSource> blocked_sources;
    MessagePriority min_priority = MessagePriority::BACKGROUND;
    MessagePriority max_priority = MessagePriority::EMERGENCY;
    std::chrono::seconds max_age{3600}; // 1 hour
    bool allow_repeats = true;
    int max_repeat_count = 3;
    std::chrono::seconds min_repeat_interval{300}; // 5 minutes
    size_t max_text_length = 128;
    bool prefer_thai_content = false;
    std::function<double(const DLSMessage&)> scoring_function;
};

// Advanced message queue with priority and context awareness
class SmartDLSQueue {
private:
    struct MessageComparator {
        bool operator()(const std::shared_ptr<DLSMessage>& a, 
                       const std::shared_ptr<DLSMessage>& b) const {
            // Lower priority value = higher priority
            if (a->priority != b->priority) {
                return static_cast<int>(a->priority) > static_cast<int>(b->priority);
            }
            // Higher importance score = higher priority
            if (std::abs(a->importance_score - b->importance_score) > 0.001) {
                return a->importance_score < b->importance_score;
            }
            // Newer messages have higher priority
            return a->created_at < b->created_at;
        }
    };
    
    std::priority_queue<std::shared_ptr<DLSMessage>, 
                       std::vector<std::shared_ptr<DLSMessage>>,
                       MessageComparator> priority_queue_;
    
    std::map<std::string, std::shared_ptr<DLSMessage>> message_index_;
    mutable std::mutex queue_mutex_;
    std::atomic<size_t> total_messages_{0};
    std::atomic<size_t> expired_messages_{0};
    
    // Message deduplication
    std::map<std::string, std::chrono::system_clock::time_point> content_hashes_;
    
    void CleanupExpiredMessages();
    std::string GenerateContentHash(const std::string& text) const;
    bool IsDuplicate(const std::string& content_hash, 
                    std::chrono::seconds dedup_window = std::chrono::seconds{3600}) const;
    
public:
    SmartDLSQueue();
    ~SmartDLSQueue() = default;
    
    // Core queue operations
    bool AddMessage(std::shared_ptr<DLSMessage> message);
    std::shared_ptr<DLSMessage> GetNextMessage(const SelectionCriteria& criteria);
    bool RemoveMessage(const std::string& message_id);
    void ClearQueue();
    
    // Queue management
    size_t GetQueueSize() const;
    size_t GetMessageCount(MessagePriority priority) const;
    std::vector<std::shared_ptr<DLSMessage>> GetMessages(const SelectionCriteria& criteria, size_t limit = 0) const;
    
    // Statistics and monitoring
    struct QueueStatistics {
        size_t total_messages = 0;
        size_t expired_messages = 0;
        size_t sent_messages = 0;
        std::map<MessagePriority, size_t> priority_counts;
        std::map<ContentSource, size_t> source_counts;
        std::chrono::system_clock::time_point oldest_message;
        std::chrono::system_clock::time_point newest_message;
        double average_importance = 0.0;
    };
    QueueStatistics GetStatistics() const;
    
    // Maintenance
    size_t CleanupMessages();
    void OptimizeQueue();
};

// Dynamic message length optimizer
class MessageLengthOptimizer {
private:
    struct OptimizationRule {
        std::string pattern;
        std::string replacement;
        size_t priority = 0;
        bool thai_specific = false;
    };
    
    std::vector<OptimizationRule> abbreviation_rules_;
    std::vector<OptimizationRule> compression_rules_;
    std::map<std::string, std::string> common_phrases_;
    
    void InitializeRules();
    
public:
    MessageLengthOptimizer();
    
    struct OptimizationResult {
        std::string original_text;
        std::string optimized_text;
        size_t original_length = 0;
        size_t optimized_length = 0;
        double compression_ratio = 0.0;
        std::vector<std::string> applied_rules;
        bool is_lossless = true;
    };
    
    OptimizationResult OptimizeMessage(const std::string& text, size_t target_length = 128);
    
    // Specific optimization strategies
    std::string ApplyAbbreviations(const std::string& text, bool thai_content = false);
    std::string CompressWhitespace(const std::string& text);
    std::string RemoveRedundancy(const std::string& text);
    std::string SmartTruncate(const std::string& text, size_t max_length);
    
    // Configuration
    void AddCustomRule(const OptimizationRule& rule);
    void LoadRulesFromFile(const std::string& filename);
};

// Context-aware message selector
class ContextAwareSelector {
private:
    MessageContext current_context_ = MessageContext::AUTOMATED;
    std::map<MessageContext, std::vector<ContentSource>> context_source_preferences_;
    std::map<MessageContext, SelectionCriteria> context_criteria_;
    
    double CalculateContextScore(const DLSMessage& message) const;
    double CalculateRecencyScore(const DLSMessage& message) const;
    double CalculateRelevanceScore(const DLSMessage& message) const;
    
public:
    ContextAwareSelector();
    
    void SetCurrentContext(MessageContext context);
    MessageContext GetCurrentContext() const { return current_context_; }
    
    std::shared_ptr<DLSMessage> SelectBestMessage(const std::vector<std::shared_ptr<DLSMessage>>& candidates);
    
    SelectionCriteria GetCriteriaForContext(MessageContext context) const;
    void SetContextCriteria(MessageContext context, const SelectionCriteria& criteria);
    
    // Scoring functions
    static double DefaultScoringFunction(const DLSMessage& message);
    static double RecencyBasedScoring(const DLSMessage& message);
    static double PriorityBasedScoring(const DLSMessage& message);
    static double EngagementBasedScoring(const DLSMessage& message);
};

// Rich metadata extractor from various sources
class MetadataExtractor {
public:
    struct ExtractedMetadata {
        std::string title;
        std::string artist;
        std::string album;
        std::string genre;
        std::string year;
        std::string duration;
        std::string language;
        std::map<std::string, std::string> custom_fields;
        bool is_live = false;
        double confidence = 0.0;
    };
    
    // Extract from different sources
    ExtractedMetadata ExtractFromNowPlaying(const std::string& now_playing_text);
    ExtractedMetadata ExtractFromFilename(const std::string& filename);
    ExtractedMetadata ExtractFromID3(const std::string& audio_file);
    ExtractedMetadata ExtractFromRDS(const std::string& rds_text);
    
    // Social media integration
    std::vector<std::shared_ptr<DLSMessage>> FetchFromTwitter(const std::string& username, int count = 5);
    std::vector<std::shared_ptr<DLSMessage>> FetchFromRSS(const std::string& feed_url, int count = 10);
    
    // Weather and traffic
    std::shared_ptr<DLSMessage> GenerateWeatherMessage(const std::string& location);
    std::shared_ptr<DLSMessage> GenerateTrafficMessage(const std::string& area);
    
    // News integration
    std::vector<std::shared_ptr<DLSMessage>> FetchNewsHeadlines(const std::string& category = "general", int count = 5);
};

// Main Smart DLS Processor
class SmartDLSProcessor {
private:
    SmartDLSQueue message_queue_;
    MessageLengthOptimizer optimizer_;
    ContextAwareSelector selector_;
    MetadataExtractor extractor_;
    
    std::atomic<bool> processing_active_{false};
    std::thread background_processor_;
    std::chrono::steady_clock::time_point last_message_time_;
    
    // Configuration
    size_t max_message_length_ = 128;
    std::chrono::seconds default_message_interval_{12}; // 12 seconds default
    std::chrono::seconds emergency_message_interval_{3}; // 3 seconds for emergency
    
    // Statistics
    struct ProcessorStats {
        std::atomic<size_t> messages_processed{0};
        std::atomic<size_t> messages_sent{0};
        std::atomic<size_t> messages_optimized{0};
        std::atomic<size_t> messages_rejected{0};
        std::chrono::system_clock::time_point start_time;
    } stats_;
    
    void BackgroundProcessingLoop();
    bool ShouldSendMessage() const;
    std::chrono::seconds GetMessageInterval(MessagePriority priority) const;
    
public:
    SmartDLSProcessor();
    ~SmartDLSProcessor();
    
    // Core processing interface
    bool AddMessage(const std::string& text, 
                   MessagePriority priority = MessagePriority::NORMAL,
                   ContentSource source = ContentSource::MANUAL,
                   const std::map<std::string, std::string>& metadata = {});
    
    std::string GetNextDLSText();
    void SetContext(MessageContext context);
    
    // Content feeding
    void FeedFromRSS(const std::string& feed_url);
    void FeedFromSocialMedia(const std::string& platform, const std::string& account);
    void FeedFromWeatherAPI(const std::string& location);
    void FeedFromNowPlaying(const std::string& now_playing_info);
    
    // Configuration
    void SetMaxMessageLength(size_t length);
    void SetMessageInterval(std::chrono::seconds interval);
    void SetEmergencyInterval(std::chrono::seconds interval);
    
    // Control
    void Start();
    void Stop();
    void Pause();
    void Resume();
    
    // Statistics and monitoring
    struct SystemStatistics {
        size_t queue_size = 0;
        size_t messages_processed = 0;
        size_t messages_sent = 0;
        size_t messages_optimized = 0;
        size_t messages_rejected = 0;
        MessageContext current_context = MessageContext::AUTOMATED;
        std::chrono::system_clock::time_point last_message_time;
        double average_compression_ratio = 0.0;
        std::map<MessagePriority, size_t> priority_distribution;
    };
    SystemStatistics GetStatistics() const;
    
    // Integration with legacy DLS encoder
    void IntegrateWithLegacyDLS(DLSEncoder& legacy_encoder);
};

} // namespace StreamDAB

#endif // SMART_DLS_H_