/*
    Smart DLS Processing Implementation
    Copyright (C) 2024 StreamDAB Project
*/

#include "smart_dls.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>
#include <openssl/md5.h>
#include <iostream>
#include <random>

namespace StreamDAB {

SmartDLSQueue::SmartDLSQueue() {
    // Initialize with cleanup timer
}

std::string SmartDLSQueue::GenerateContentHash(const std::string& text) const {
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(text.c_str()), text.length(), hash);
    
    std::ostringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool SmartDLSQueue::IsDuplicate(const std::string& content_hash, 
                               std::chrono::seconds dedup_window) const {
    auto it = content_hashes_.find(content_hash);
    if (it == content_hashes_.end()) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    return (now - it->second) < dedup_window;
}

bool SmartDLSQueue::AddMessage(std::shared_ptr<DLSMessage> message) {
    if (!message || message->text.empty()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    // Generate content hash for deduplication
    message->content_hash = GenerateContentHash(message->text);
    
    // Check for duplicates
    if (IsDuplicate(message->content_hash)) {
        return false; // Skip duplicate message
    }
    
    // Set creation time if not set
    if (message->created_at.time_since_epoch().count() == 0) {
        message->created_at = std::chrono::system_clock::now();
    }
    
    // Set default expiration if not set (24 hours from creation)
    if (message->expires_at.time_since_epoch().count() == 0) {
        message->expires_at = message->created_at + std::chrono::hours{24};
    }
    
    // Add to priority queue
    priority_queue_.push(message);
    
    // Add to index
    message_index_[message->source_id] = message;
    
    // Update content hash tracking
    content_hashes_[message->content_hash] = message->created_at;
    
    total_messages_++;
    
    return true;
}

std::shared_ptr<DLSMessage> SmartDLSQueue::GetNextMessage(const SelectionCriteria& criteria) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    // Clean expired messages first
    CleanupExpiredMessages();
    
    if (priority_queue_.empty()) {
        return nullptr;
    }
    
    // Find best message matching criteria
    std::vector<std::shared_ptr<DLSMessage>> candidates;
    auto temp_queue = priority_queue_;
    
    while (!temp_queue.empty()) {
        auto message = temp_queue.top();
        temp_queue.pop();
        
        // Check if message meets criteria
        bool meets_criteria = true;
        
        // Check priority range
        if (message->priority < criteria.min_priority || 
            message->priority > criteria.max_priority) {
            meets_criteria = false;
        }
        
        // Check age
        auto age = std::chrono::system_clock::now() - message->created_at;
        if (age > criteria.max_age) {
            meets_criteria = false;
        }
        
        // Check source allowlist/blocklist
        if (!criteria.allowed_sources.empty()) {
            bool source_allowed = std::find(criteria.allowed_sources.begin(),
                                           criteria.allowed_sources.end(),
                                           message->source) != criteria.allowed_sources.end();
            if (!source_allowed) {
                meets_criteria = false;
            }
        }
        
        if (!criteria.blocked_sources.empty()) {
            bool source_blocked = std::find(criteria.blocked_sources.begin(),
                                           criteria.blocked_sources.end(),
                                           message->source) != criteria.blocked_sources.end();
            if (source_blocked) {
                meets_criteria = false;
            }
        }
        
        // Check repeat constraints
        if (!criteria.allow_repeats && message->send_count > 0) {
            meets_criteria = false;
        }
        
        if (message->send_count >= criteria.max_repeat_count) {
            meets_criteria = false;
        }
        
        if (message->send_count > 0) {
            auto time_since_last = std::chrono::system_clock::now() - message->last_sent;
            if (time_since_last < criteria.min_repeat_interval) {
                meets_criteria = false;
            }
        }
        
        // Check text length
        if (message->text.length() > criteria.max_text_length) {
            meets_criteria = false;
        }
        
        // Check Thai content preference
        if (criteria.prefer_thai_content && !message->is_thai_content) {
            // Lower priority for non-Thai content
            message->importance_score *= 0.8;
        }
        
        // Check max sends
        if (message->max_sends > 0 && message->send_count >= message->max_sends) {
            meets_criteria = false;
        }
        
        if (meets_criteria) {
            candidates.push_back(message);
        }
    }
    
    if (candidates.empty()) {
        return nullptr;
    }
    
    // Apply custom scoring function if provided
    if (criteria.scoring_function) {
        std::sort(candidates.begin(), candidates.end(),
                 [&criteria](const std::shared_ptr<DLSMessage>& a, 
                           const std::shared_ptr<DLSMessage>& b) {
                     return criteria.scoring_function(*a) > criteria.scoring_function(*b);
                 });
    }
    
    // Return best candidate
    auto selected = candidates[0];
    selected->last_sent = std::chrono::system_clock::now();
    selected->send_count++;
    
    return selected;
}

void SmartDLSQueue::CleanupExpiredMessages() {
    auto now = std::chrono::system_clock::now();
    
    // This is a simplified cleanup - in practice, we'd need to rebuild the queue
    // to properly remove expired messages from the priority queue
    size_t expired_count = 0;
    
    for (auto it = message_index_.begin(); it != message_index_.end();) {
        if (it->second->expires_at < now) {
            content_hashes_.erase(it->second->content_hash);
            it = message_index_.erase(it);
            expired_count++;
        } else {
            ++it;
        }
    }
    
    expired_messages_ += expired_count;
}

size_t SmartDLSQueue::GetQueueSize() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return priority_queue_.size();
}

SmartDLSQueue::QueueStatistics SmartDLSQueue::GetStatistics() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    
    QueueStatistics stats;
    stats.total_messages = total_messages_;
    stats.expired_messages = expired_messages_;
    
    // Analyze current queue
    auto temp_queue = priority_queue_;
    double total_importance = 0.0;
    bool first = true;
    
    while (!temp_queue.empty()) {
        auto message = temp_queue.top();
        temp_queue.pop();
        
        stats.priority_counts[message->priority]++;
        stats.source_counts[message->source]++;
        total_importance += message->importance_score;
        
        if (first) {
            stats.newest_message = stats.oldest_message = message->created_at;
            first = false;
        } else {
            if (message->created_at < stats.oldest_message) {
                stats.oldest_message = message->created_at;
            }
            if (message->created_at > stats.newest_message) {
                stats.newest_message = message->created_at;
            }
        }
    }
    
    if (priority_queue_.size() > 0) {
        stats.average_importance = total_importance / priority_queue_.size();
    }
    
    return stats;
}

MessageLengthOptimizer::MessageLengthOptimizer() {
    InitializeRules();
}

void MessageLengthOptimizer::InitializeRules() {
    // English abbreviations
    abbreviation_rules_.push_back({"information", "info", 1, false});
    abbreviation_rules_.push_back({"and", "&", 2, false});
    abbreviation_rules_.push_back({"with", "w/", 2, false});
    abbreviation_rules_.push_back({"without", "w/o", 2, false});
    abbreviation_rules_.push_back({"tonight", "tonite", 1, false});
    abbreviation_rules_.push_back({"tomorrow", "tmrw", 1, false});
    abbreviation_rules_.push_back({"today", "2day", 1, false});
    
    // Thai abbreviations
    abbreviation_rules_.push_back({"ข้อมูล", "ข้อม.", 1, true});
    abbreviation_rules_.push_back({"รายการ", "ราย.", 1, true});
    abbreviation_rules_.push_back({"โครงการ", "โครง.", 1, true});
    abbreviation_rules_.push_back({"กิจกรรม", "กิจ.", 1, true});
    abbreviation_rules_.push_back({"มหาวิทยาลัย", "ม.", 1, true});
    abbreviation_rules_.push_back({"จังหวัด", "จ.", 1, true});
    abbreviation_rules_.push_back({"ประเทศไทย", "ไทย", 1, true});
    
    // Common phrases
    common_phrases_["Now Playing"] = "♪";
    common_phrases_["Coming Up"] = "Next:";
    common_phrases_["Breaking News"] = "BREAKING:";
    common_phrases_["Weather Update"] = "Weather:";
    common_phrases_["Traffic Alert"] = "Traffic:";
    
    // Thai common phrases
    common_phrases_["กำลังเล่น"] = "♪";
    common_phrases_["ต่อไป"] = "→";
    common_phrases_["ข่าวด่วน"] = "ด่วน:";
    common_phrases_["สภาพอากาศ"] = "อากาศ:";
    common_phrases_["การจราจร"] = "จราจร:";
    
    // Compression rules
    compression_rules_.push_back({R"(\s+)", " ", 1, false}); // Multiple spaces to single
    compression_rules_.push_back({R"(\s*,\s*)", ",", 1, false}); // Remove spaces around commas
    compression_rules_.push_back({R"(\s*\.\s*)", ".", 1, false}); // Remove spaces around periods
    compression_rules_.push_back({R"(\s*:\s*)", ":", 1, false}); // Remove spaces around colons
}

MessageLengthOptimizer::OptimizationResult 
MessageLengthOptimizer::OptimizeMessage(const std::string& text, size_t target_length) {
    OptimizationResult result;
    result.original_text = text;
    result.optimized_text = text;
    result.original_length = text.length();
    
    if (text.length() <= target_length) {
        result.optimized_length = text.length();
        result.compression_ratio = 1.0;
        result.is_lossless = true;
        return result;
    }
    
    // Apply optimizations in order of preference
    
    // 1. Compress whitespace
    result.optimized_text = CompressWhitespace(result.optimized_text);
    if (result.optimized_text.length() != result.original_length) {
        result.applied_rules.push_back("Whitespace compression");
    }
    
    // 2. Apply common phrase replacements
    for (const auto& phrase : common_phrases_) {
        if (result.optimized_text.find(phrase.first) != std::string::npos) {
            result.optimized_text = std::regex_replace(result.optimized_text, 
                                                      std::regex(phrase.first), 
                                                      phrase.second);
            result.applied_rules.push_back("Phrase replacement: " + phrase.first);
        }
    }
    
    // 3. Apply abbreviations
    bool is_thai = result.optimized_text.find_first_of("กขคฆงจฉชซฌญฎฏฐฑฒณดตถทธนบปผฝพฟภมยรลวศษสหฬอฮ") != std::string::npos;
    for (const auto& rule : abbreviation_rules_) {
        if (rule.thai_specific == is_thai) {
            if (result.optimized_text.find(rule.pattern) != std::string::npos) {
                result.optimized_text = std::regex_replace(result.optimized_text, 
                                                          std::regex(rule.pattern), 
                                                          rule.replacement);
                result.applied_rules.push_back("Abbreviation: " + rule.pattern);
            }
        }
    }
    
    // 4. Remove redundancy
    std::string deduplicated = RemoveRedundancy(result.optimized_text);
    if (deduplicated != result.optimized_text) {
        result.optimized_text = deduplicated;
        result.applied_rules.push_back("Redundancy removal");
    }
    
    // 5. Smart truncation if still too long
    if (result.optimized_text.length() > target_length) {
        result.optimized_text = SmartTruncate(result.optimized_text, target_length);
        result.applied_rules.push_back("Smart truncation");
        result.is_lossless = false;
    }
    
    result.optimized_length = result.optimized_text.length();
    result.compression_ratio = static_cast<double>(result.optimized_length) / result.original_length;
    
    return result;
}

std::string MessageLengthOptimizer::CompressWhitespace(const std::string& text) {
    std::string result = text;
    
    // Replace multiple spaces with single space
    result = std::regex_replace(result, std::regex(R"(\s+)"), " ");
    
    // Remove leading and trailing whitespace
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    
    return result;
}

std::string MessageLengthOptimizer::RemoveRedundancy(const std::string& text) {
    std::string result = text;
    
    // Simple redundancy removal - remove repeated words
    std::istringstream iss(result);
    std::vector<std::string> words;
    std::string word;
    
    while (iss >> word) {
        words.push_back(word);
    }
    
    // Remove consecutive duplicate words
    auto new_end = std::unique(words.begin(), words.end());
    words.erase(new_end, words.end());
    
    // Reconstruct string
    std::ostringstream oss;
    for (size_t i = 0; i < words.size(); ++i) {
        if (i > 0) oss << " ";
        oss << words[i];
    }
    
    return oss.str();
}

std::string MessageLengthOptimizer::SmartTruncate(const std::string& text, size_t max_length) {
    if (text.length() <= max_length) {
        return text;
    }
    
    // Try to find a good break point
    size_t break_pos = max_length - 3; // Leave room for "..."
    
    // Look for word boundary
    while (break_pos > max_length * 0.7 && break_pos > 0) {
        char c = text[break_pos];
        if (c == ' ' || c == ',' || c == '.' || c == '!' || c == '?') {
            break;
        }
        break_pos--;
    }
    
    std::string result = text.substr(0, break_pos);
    if (break_pos < text.length()) {
        result += "...";
    }
    
    return result;
}

ContextAwareSelector::ContextAwareSelector() {
    // Initialize default context preferences
    context_source_preferences_[MessageContext::LIVE_SHOW] = {
        ContentSource::MANUAL, ContentSource::METADATA_EXTRACTOR, ContentSource::SOCIAL_MEDIA
    };
    
    context_source_preferences_[MessageContext::NEWS] = {
        ContentSource::NEWS_API, ContentSource::RSS_FEED, ContentSource::MANUAL
    };
    
    context_source_preferences_[MessageContext::MUSIC] = {
        ContentSource::METADATA_EXTRACTOR, ContentSource::MANUAL, ContentSource::SOCIAL_MEDIA
    };
    
    context_source_preferences_[MessageContext::EMERGENCY] = {
        ContentSource::EMERGENCY_SYSTEM, ContentSource::MANUAL
    };
    
    // Set default criteria for each context
    SelectionCriteria live_criteria;
    live_criteria.preferred_context = MessageContext::LIVE_SHOW;
    live_criteria.min_priority = MessagePriority::NORMAL;
    live_criteria.max_age = std::chrono::hours{1};
    context_criteria_[MessageContext::LIVE_SHOW] = live_criteria;
    
    SelectionCriteria news_criteria;
    news_criteria.preferred_context = MessageContext::NEWS;
    news_criteria.min_priority = MessagePriority::HIGH;
    news_criteria.max_age = std::chrono::minutes{30};
    context_criteria_[MessageContext::NEWS] = news_criteria;
    
    SelectionCriteria emergency_criteria;
    emergency_criteria.preferred_context = MessageContext::EMERGENCY;
    emergency_criteria.min_priority = MessagePriority::EMERGENCY;
    emergency_criteria.allow_repeats = true;
    emergency_criteria.max_repeat_count = 10;
    emergency_criteria.min_repeat_interval = std::chrono::seconds{30};
    context_criteria_[MessageContext::EMERGENCY] = emergency_criteria;
}

void ContextAwareSelector::SetCurrentContext(MessageContext context) {
    current_context_ = context;
}

double ContextAwareSelector::CalculateContextScore(const DLSMessage& message) const {
    double score = 0.5; // Base score
    
    // Boost score if message context matches current context
    if (message.context == current_context_) {
        score += 0.3;
    }
    
    // Check source preferences for current context
    auto pref_it = context_source_preferences_.find(current_context_);
    if (pref_it != context_source_preferences_.end()) {
        auto& preferred_sources = pref_it->second;
        auto source_it = std::find(preferred_sources.begin(), preferred_sources.end(), message.source);
        if (source_it != preferred_sources.end()) {
            // Higher score for more preferred sources
            size_t preference_index = std::distance(preferred_sources.begin(), source_it);
            score += 0.2 * (1.0 - static_cast<double>(preference_index) / preferred_sources.size());
        }
    }
    
    return score;
}

double ContextAwareSelector::CalculateRecencyScore(const DLSMessage& message) const {
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - message.created_at).count();
    
    // Exponential decay with 12-hour half-life
    return std::exp(-age / 12.0);
}

double ContextAwareSelector::DefaultScoringFunction(const DLSMessage& message) {
    double score = 0.0;
    
    // Priority component (40% weight)
    score += (4 - static_cast<int>(message.priority)) * 0.1; // Higher priority = higher score
    
    // Importance component (30% weight)
    score += message.importance_score * 0.3;
    
    // Recency component (20% weight)
    auto now = std::chrono::system_clock::now();
    auto age_hours = std::chrono::duration_cast<std::chrono::hours>(now - message.created_at).count();
    double recency_score = std::exp(-age_hours / 24.0); // 24-hour half-life
    score += recency_score * 0.2;
    
    // Send count penalty (10% weight)
    double send_penalty = 1.0 / (1.0 + message.send_count * 0.5);
    score += send_penalty * 0.1;
    
    return score;
}

SmartDLSProcessor::SmartDLSProcessor() {
    stats_.start_time = std::chrono::system_clock::now();
}

SmartDLSProcessor::~SmartDLSProcessor() {
    Stop();
}

bool SmartDLSProcessor::AddMessage(const std::string& text, 
                                  MessagePriority priority,
                                  ContentSource source,
                                  const std::map<std::string, std::string>& metadata) {
    auto message = std::make_shared<DLSMessage>();
    message->text = text;
    message->priority = priority;
    message->source = source;
    message->metadata = metadata;
    message->source_id = std::to_string(std::hash<std::string>{}(text + std::to_string(std::time(nullptr))));
    
    // Detect Thai content
    message->is_thai_content = text.find_first_of("กขคฆงจฉชซฌญฎฏฐฑฒณดตถทธนบปผฝพฟภมยรลวศษสหฬอฮ") != std::string::npos;
    
    // Optimize message if too long
    if (text.length() > max_message_length_) {
        auto optimization_result = optimizer_.OptimizeMessage(text, max_message_length_);
        message->text = optimization_result.optimized_text;
        message->metadata["optimization_applied"] = "true";
        message->metadata["original_length"] = std::to_string(optimization_result.original_length);
        message->metadata["compression_ratio"] = std::to_string(optimization_result.compression_ratio);
        stats_.messages_optimized++;
    }
    
    if (message_queue_.AddMessage(message)) {
        stats_.messages_processed++;
        return true;
    } else {
        stats_.messages_rejected++;
        return false;
    }
}

std::string SmartDLSProcessor::GetNextDLSText() {
    auto criteria = selector_.GetCriteriaForContext(selector_.GetCurrentContext());
    criteria.scoring_function = ContextAwareSelector::DefaultScoringFunction;
    
    auto message = message_queue_.GetNextMessage(criteria);
    if (message) {
        stats_.messages_sent++;
        last_message_time_ = std::chrono::steady_clock::now();
        return message->text;
    }
    
    return "";
}

void SmartDLSProcessor::SetContext(MessageContext context) {
    selector_.SetCurrentContext(context);
}

void SmartDLSProcessor::Start() {
    if (!processing_active_.exchange(true)) {
        background_processor_ = std::thread(&SmartDLSProcessor::BackgroundProcessingLoop, this);
    }
}

void SmartDLSProcessor::Stop() {
    if (processing_active_.exchange(false)) {
        if (background_processor_.joinable()) {
            background_processor_.join();
        }
    }
}

void SmartDLSProcessor::BackgroundProcessingLoop() {
    while (processing_active_) {
        try {
            // Periodic maintenance tasks
            std::this_thread::sleep_for(std::chrono::seconds(30));
            
            // Clean up expired messages
            message_queue_.CleanupMessages();
            
            // Update statistics
            // Additional background processing can be added here
            
        } catch (const std::exception& e) {
            std::cerr << "Error in DLS background processing: " << e.what() << std::endl;
        }
    }
}

SmartDLSProcessor::SystemStatistics SmartDLSProcessor::GetStatistics() const {
    SystemStatistics stats;
    stats.queue_size = message_queue_.GetQueueSize();
    stats.messages_processed = stats_.messages_processed;
    stats.messages_sent = stats_.messages_sent;
    stats.messages_optimized = stats_.messages_optimized;
    stats.messages_rejected = stats_.messages_rejected;
    stats.current_context = selector_.GetCurrentContext();
    
    if (last_message_time_.time_since_epoch().count() > 0) {
        auto duration = last_message_time_.time_since_epoch();
        auto sys_time = std::chrono::system_clock::time_point{
            std::chrono::duration_cast<std::chrono::system_clock::duration>(duration)
        };
        stats.last_message_time = sys_time;
    }
    
    // Get priority distribution
    auto queue_stats = message_queue_.GetStatistics();
    stats.priority_distribution = queue_stats.priority_counts;
    
    return stats;
}

} // namespace StreamDAB