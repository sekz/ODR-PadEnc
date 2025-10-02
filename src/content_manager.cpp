/*
    StreamDAB Content Manager Integration Implementation
    Copyright (C) 2024 StreamDAB Project
*/

#include "content_manager.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace StreamDAB {

// ContentScheduler implementation
ContentScheduler::ContentScheduler() {
    std::cout << "Content Scheduler initialized" << std::endl;
}

ContentScheduler::~ContentScheduler() {
    Stop();
}

void ContentScheduler::Start() {
    if (scheduler_running_.exchange(true)) {
        return; // Already running
    }
    
    scheduler_thread_ = std::thread(&ContentScheduler::SchedulingLoop, this);
    std::cout << "Content Scheduler started" << std::endl;
}

void ContentScheduler::Stop() {
    if (!scheduler_running_.exchange(false)) {
        return; // Not running
    }
    
    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }
    
    std::cout << "Content Scheduler stopped" << std::endl;
}

void ContentScheduler::SchedulingLoop() {
    while (scheduler_running_) {
        try {
            ProcessScheduledContent();
            UpdateContentMetrics();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } catch (const std::exception& e) {
            std::cerr << "Error in scheduling loop: " << e.what() << std::endl;
        }
    }
}

void ContentScheduler::ProcessScheduledContent() {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    
    auto now = std::chrono::system_clock::now();
    
    // Process emergency overrides first
    if (emergency_override_ && emergency_content_) {
        auto emergency_duration = now - emergency_start_;
        if (emergency_duration < emergency_duration_) {
            current_mot_content_ = emergency_content_;
            current_dls_content_ = emergency_content_;
            return;
        } else {
            // Emergency period expired
            emergency_override_ = false;
            emergency_content_.reset();
        }
    }
    
    // Select next content based on schedule
    current_mot_content_ = SelectNextMOTContent();
    current_dls_content_ = SelectNextDLSContent();
}

std::shared_ptr<ContentItem> ContentScheduler::SelectNextMOTContent() {
    std::vector<std::shared_ptr<ContentItem>> candidates;
    
    // Find eligible MOT content
    for (auto& item : scheduled_content_) {
        if (!item->is_active) continue;
        if (item->type != ContentType::MOT_SLIDESHOW && item->type != ContentType::COMBINED) continue;
        if (!ShouldScheduleContent(*item)) continue;
        
        candidates.push_back(item);
    }
    
    if (candidates.empty()) return nullptr;
    
    // Sort by scheduling score
    std::sort(candidates.begin(), candidates.end(), 
             [this](const auto& a, const auto& b) {
                 return CalculateSchedulingScore(*a) > CalculateSchedulingScore(*b);
             });
    
    return candidates.front();
}

std::shared_ptr<ContentItem> ContentScheduler::SelectNextDLSContent() {
    std::vector<std::shared_ptr<ContentItem>> candidates;
    
    // Find eligible DLS content
    for (auto& item : scheduled_content_) {
        if (!item->is_active) continue;
        if (item->type != ContentType::DLS_MESSAGE && item->type != ContentType::COMBINED) continue;
        if (!ShouldScheduleContent(*item)) continue;
        
        candidates.push_back(item);
    }
    
    if (candidates.empty()) return nullptr;
    
    // Sort by scheduling score
    std::sort(candidates.begin(), candidates.end(), 
             [this](const auto& a, const auto& b) {
                 return CalculateSchedulingScore(*a) > CalculateSchedulingScore(*b);
             });
    
    return candidates.front();
}

bool ContentScheduler::ShouldScheduleContent(const ContentItem& item) const {
    auto now = std::chrono::system_clock::now();
    
    // Check if expired
    if (now > item.schedule.end_time) return false;
    
    // Check if started
    if (now < item.schedule.start_time) return false;
    
    // Check repeat constraints
    if (item.schedule.max_repeats > 0 && 
        item.schedule.current_repeats >= item.schedule.max_repeats) {
        return false;
    }
    
    return true;
}

double ContentScheduler::CalculateSchedulingScore(const ContentItem& item) const {
    double score = 0.0;
    
    // Priority component (40% weight)
    score += (4 - static_cast<int>(item.priority)) * 0.4;
    
    // Time-based component (30% weight)
    auto now = std::chrono::system_clock::now();
    auto time_in_window = std::chrono::duration_cast<std::chrono::minutes>(
        now - item.schedule.start_time).count();
    auto window_duration = std::chrono::duration_cast<std::chrono::minutes>(
        item.schedule.end_time - item.schedule.start_time).count();
    
    if (window_duration > 0) {
        double time_factor = 1.0 - (time_in_window / static_cast<double>(window_duration));
        score += std::max(0.0, time_factor) * 0.3;
    }
    
    // Usage frequency component (20% weight)
    double usage_factor = 1.0 / (1.0 + item.schedule_count * 0.1);
    score += usage_factor * 0.2;
    
    // Content quality component (10% weight)
    if (item.type == ContentType::MOT_SLIDESHOW) {
        // Assume quality is stored in metadata
        auto quality_it = item.metadata.find("quality");
        if (quality_it != item.metadata.end()) {
            double quality = std::stod(quality_it->second);
            score += quality * 0.1;
        }
    }
    
    return score;
}

void ContentScheduler::UpdateContentMetrics() {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    
    // Update current content metrics
    if (current_mot_content_) {
        current_mot_content_->metrics.display_count++;
        current_mot_content_->metrics.last_displayed = std::chrono::system_clock::now();
    }
    
    if (current_dls_content_) {
        current_dls_content_->metrics.display_count++;
        current_dls_content_->metrics.last_displayed = std::chrono::system_clock::now();
    }
}

bool ContentScheduler::AddContent(std::shared_ptr<ContentItem> item) {
    if (!item) return false;
    
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    scheduled_content_.push_back(item);
    
    std::cout << "Content added: " << item->item_id << " (type: " << static_cast<int>(item->type) << ")" << std::endl;
    return true;
}

bool ContentScheduler::RemoveContent(const std::string& item_id) {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    
    auto it = std::find_if(scheduled_content_.begin(), scheduled_content_.end(),
                          [&item_id](const auto& item) { return item->item_id == item_id; });
    
    if (it != scheduled_content_.end()) {
        scheduled_content_.erase(it);
        std::cout << "Content removed: " << item_id << std::endl;
        return true;
    }
    
    return false;
}

void ContentScheduler::TriggerEmergency(std::shared_ptr<ContentItem> emergency_item, 
                                       std::chrono::seconds duration) {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    
    emergency_override_ = true;
    emergency_content_ = emergency_item;
    emergency_start_ = std::chrono::system_clock::now();
    emergency_duration_ = duration;
    
    std::cout << "Emergency content activated for " << duration.count() << " seconds" << std::endl;
}

void ContentScheduler::ClearEmergency() {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    
    emergency_override_ = false;
    emergency_content_.reset();
    
    std::cout << "Emergency content cleared" << std::endl;
}

std::shared_ptr<ContentItem> ContentScheduler::GetCurrentMOTContent() const {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    return current_mot_content_;
}

std::shared_ptr<ContentItem> ContentScheduler::GetCurrentDLSContent() const {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    return current_dls_content_;
}

ContentScheduler::SchedulerStatistics ContentScheduler::GetStatistics() const {
    std::lock_guard<std::mutex> lock(schedule_mutex_);
    
    SchedulerStatistics stats;
    stats.total_content_items = scheduled_content_.size();
    stats.last_schedule_update = std::chrono::system_clock::now();
    
    for (const auto& item : scheduled_content_) {
        if (item->is_active) {
            stats.active_content_items++;
        }
        
        stats.content_type_counts[item->type]++;
        stats.priority_distribution[item->priority]++;
        
        // Check if scheduled today
        auto now = std::chrono::system_clock::now();
        if (item->metrics.last_displayed > (now - std::chrono::hours{24})) {
            stats.scheduled_today++;
        }
    }
    
    if (emergency_override_) {
        stats.emergency_overrides++;
    }
    
    return stats;
}

// ContentCoordinator implementation
ContentCoordinator::ContentCoordinator(const CoordinatorConfig& config) : config_(config) {
    scheduler_ = std::make_unique<ContentScheduler>();
    
    APIConfig api_config;
    api_config.port = 8008; // StreamDAB allocated port
    api_service_ = std::make_unique<StreamDABAPIService>(api_config);
}

ContentCoordinator::ContentCoordinator() : ContentCoordinator(CoordinatorConfig{}) {
    
    std::cout << "Content Coordinator initialized" << std::endl;
}

ContentCoordinator::~ContentCoordinator() {
    Stop();
}

bool ContentCoordinator::Initialize() {
    // Initialize all components
    if (!api_service_) return false;
    if (!scheduler_) return false;
    
    std::cout << "Content Coordinator initialized successfully" << std::endl;
    return true;
}

void ContentCoordinator::Start() {
    if (coordinator_running_.exchange(true)) {
        return; // Already running
    }
    
    // Start components
    scheduler_->Start();
    api_service_->Start();
    
    // Start coordination thread
    coordination_thread_ = std::thread(&ContentCoordinator::CoordinationLoop, this);
    
    std::cout << "Content Coordinator started" << std::endl;
}

void ContentCoordinator::Stop() {
    if (!coordinator_running_.exchange(false)) {
        return; // Not running
    }
    
    if (coordination_thread_.joinable()) {
        coordination_thread_.join();
    }
    
    // Stop components
    if (scheduler_) scheduler_->Stop();
    if (api_service_) api_service_->Stop();
    
    std::cout << "Content Coordinator stopped" << std::endl;
}

void ContentCoordinator::CoordinationLoop() {
    while (coordinator_running_) {
        try {
            SynchronizeContent();
            BroadcastStatus();
            ValidateContentCompliance();
            
            std::this_thread::sleep_for(config_.content_sync_interval);
        } catch (const std::exception& e) {
            std::cerr << "Error in coordination loop: " << e.what() << std::endl;
        }
    }
}

void ContentCoordinator::SynchronizeContent() {
    // Get current content from scheduler
    auto mot_content = scheduler_->GetCurrentMOTContent();
    auto dls_content = scheduler_->GetCurrentDLSContent();
    
    // Process content through appropriate processors
    if (mot_content) {
        ProcessMOTContent(mot_content);
    }
    
    if (dls_content) {
        ProcessDLSContent(dls_content);
        
        // Check for Thai content
        if (dls_content->is_thai_content) {
            ProcessThaiContent(dls_content);
        }
    }
    
    last_sync_ = std::chrono::system_clock::now();
}

void ContentCoordinator::ProcessMOTContent(std::shared_ptr<ContentItem> item) {
    auto mot_processor = api_service_->GetMOTProcessor();
    if (mot_processor && !item->image_path.empty()) {
        mot_processor->AddImage(item->image_path);
    }
}

void ContentCoordinator::ProcessDLSContent(std::shared_ptr<ContentItem> item) {
    auto dls_processor = api_service_->GetDLSProcessor();
    if (dls_processor && !item->text_content.empty()) {
        MessagePriority msg_priority = MessagePriority::NORMAL;
        switch (item->priority) {
            case SchedulePriority::EMERGENCY: msg_priority = MessagePriority::EMERGENCY; break;
            case SchedulePriority::URGENT: msg_priority = MessagePriority::HIGH; break;
            case SchedulePriority::NORMAL: msg_priority = MessagePriority::NORMAL; break;
            case SchedulePriority::LOW: msg_priority = MessagePriority::LOW; break;
            case SchedulePriority::BACKGROUND: msg_priority = MessagePriority::BACKGROUND; break;
        }
        
        dls_processor->AddMessage(item->text_content, msg_priority, ContentSource::MANUAL);
    }
}

void ContentCoordinator::ProcessThaiContent(std::shared_ptr<ContentItem> item) {
    auto thai_processor = api_service_->GetThaiProcessor();
    if (thai_processor && !item->text_content.empty()) {
        // Validate Thai content
        auto validation = thai_processor->ValidateContent(item->text_content);
        item->cultural_validation = validation;
        
        // Format for DLS if needed
        std::string formatted_text;
        thai_processor->FormatTextForDLS(item->text_content, formatted_text, 128);
        if (!formatted_text.empty() && formatted_text != item->text_content) {
            item->text_content = formatted_text;
        }
    }
}

void ContentCoordinator::BroadcastStatus() {
    if (api_service_) {
        api_service_->BroadcastStatusUpdate();
    }
}

void ContentCoordinator::ValidateContentCompliance() {
    // Placeholder for ETSI compliance validation
    // In a real implementation, this would check all active content
    // against ETSI standards and NBTC regulations
}

bool ContentCoordinator::AddContent(const std::string& content_data, 
                                   ContentType type, 
                                   SchedulePriority priority) {
    auto item = std::make_shared<ContentItem>();
    item->item_id = "content_" + std::to_string(std::hash<std::string>{}(content_data));
    item->type = type;
    item->priority = priority;
    item->created_at = std::chrono::system_clock::now();
    item->is_active = true;
    
    // Set content based on type
    if (type == ContentType::DLS_MESSAGE || type == ContentType::COMBINED) {
        item->text_content = content_data;
        
        // Check if Thai content
        if (content_data.find_first_of("กขคฆงจฉชซฌญฎฏฐฑฒณดตถทธนบปผฝพฟภมยรลวศษสหฬอฮ") != std::string::npos) {
            item->is_thai_content = true;
        }
    } else if (type == ContentType::MOT_SLIDESHOW) {
        item->image_path = content_data;
    }
    
    // Set default schedule (immediate, 1 hour duration)
    item->schedule.start_time = std::chrono::system_clock::now();
    item->schedule.end_time = item->schedule.start_time + std::chrono::hours{1};
    item->schedule.duration = std::chrono::hours{1};
    
    return scheduler_->AddContent(item);
}

void ContentCoordinator::TriggerEmergencyBroadcast(const std::string& message, 
                                                  std::chrono::seconds duration) {
    // Create emergency content item
    auto emergency_item = std::make_shared<ContentItem>();
    emergency_item->item_id = "emergency_" + std::to_string(std::time(nullptr));
    emergency_item->type = ContentType::EMERGENCY_ALERT;
    emergency_item->priority = SchedulePriority::EMERGENCY;
    emergency_item->text_content = message;
    emergency_item->is_emergency = true;
    emergency_item->created_at = std::chrono::system_clock::now();
    emergency_item->is_active = true;
    
    // Set emergency schedule
    emergency_item->schedule.start_time = std::chrono::system_clock::now();
    emergency_item->schedule.end_time = emergency_item->schedule.start_time + duration;
    emergency_item->schedule.duration = std::chrono::duration_cast<std::chrono::minutes>(duration);
    
    // Trigger emergency in scheduler
    scheduler_->TriggerEmergency(emergency_item, duration);
    
    // Trigger emergency in API service
    if (api_service_) {
        api_service_->TriggerEmergencyMode(message);
    }
    
    std::cout << "Emergency broadcast triggered: " << message << std::endl;
}

void ContentCoordinator::ClearEmergencyBroadcast() {
    scheduler_->ClearEmergency();
    
    if (api_service_) {
        api_service_->ClearEmergencyMode();
    }
    
    std::cout << "Emergency broadcast cleared" << std::endl;
}

ContentCoordinator::SystemHealth ContentCoordinator::GetSystemHealth() const {
    SystemHealth health;
    health.last_check = std::chrono::system_clock::now();
    health.overall_healthy = true;
    
    // Check scheduler
    health.component_status["scheduler"] = scheduler_ && scheduler_->IsRunning();
    if (!health.component_status["scheduler"]) {
        health.overall_healthy = false;
        health.errors.push_back("Scheduler not running");
    }
    
    // Check API service
    health.component_status["api_service"] = api_service_ && api_service_->IsRunning();
    if (!health.component_status["api_service"]) {
        health.overall_healthy = false;
        health.errors.push_back("API service not running");
    }
    
    // Check coordinator
    health.component_status["coordinator"] = coordinator_running_;
    if (!health.component_status["coordinator"]) {
        health.overall_healthy = false;
        health.errors.push_back("Coordinator not running");
    }
    
    return health;
}

// ContentValidator implementation
ContentValidator::ContentValidator(const ValidationRules& rules) : rules_(rules) {
    path_validator_ = std::make_unique<SecurePathValidator>();
    security_scanner_ = std::make_unique<ContentSecurityScanner>();
    thai_processor_ = std::make_unique<ThaiLanguageProcessor>();
    
    std::cout << "Content Validator initialized" << std::endl;
}

ContentValidator::ContentValidator() : ContentValidator(ValidationRules{}) {
}

ContentValidator::ValidationResult ContentValidator::ValidateContentItem(const ContentItem& item) {
    ValidationResult result;
    result.compliance_score = 1.0;
    
    // Validate text content
    if (!item.text_content.empty()) {
        auto text_validation = ValidateText(item.text_content, item.is_thai_content);
        if (!text_validation.is_valid) {
            result.is_valid = false;
            result.violations.insert(result.violations.end(), 
                                   text_validation.violations.begin(), 
                                   text_validation.violations.end());
        }
    }
    
    // Validate image content
    if (!item.binary_data.empty()) {
        auto image_validation = ValidateImage(item.binary_data, "image/jpeg");
        if (!image_validation.is_valid) {
            result.is_valid = false;
            result.violations.insert(result.violations.end(), 
                                   image_validation.violations.begin(), 
                                   image_validation.violations.end());
        }
    }
    
    // Validate scheduling
    auto schedule_validation = ValidateScheduling(item.schedule);
    if (!schedule_validation.is_valid) {
        result.warnings.insert(result.warnings.end(), 
                              schedule_validation.warnings.begin(), 
                              schedule_validation.warnings.end());
    }
    
    // ETSI compliance check
    if (!ValidateETSICompliance(item)) {
        result.compliance_score *= 0.5;
        result.warnings.push_back("ETSI compliance issues detected");
    }
    
    result.is_valid = result.violations.empty();
    result.is_safe = result.is_valid && result.security_result.is_safe;
    
    return result;
}

ContentValidator::ValidationResult ContentValidator::ValidateText(const std::string& text, bool is_thai) {
    ValidationResult result;
    result.is_valid = true;
    result.is_safe = true;
    
    // Length validation
    if (text.length() > rules_.max_text_length) {
        result.violations.push_back("Text exceeds maximum length");
        result.is_valid = false;
    }
    
    // Thai content validation
    if (is_thai && thai_processor_) {
        result.cultural_result = thai_processor_->ValidateContent(text);
        if (!result.cultural_result.is_appropriate) {
            result.warnings.push_back("Cultural content issues detected");
        }
    }
    
    // Security validation
    if (security_scanner_) {
        result.security_result = security_scanner_->ScanTextContent(text);
        if (!result.security_result.is_safe) {
            result.is_safe = false;
            result.violations.push_back("Security threats detected in text");
        }
    }
    
    return result;
}

bool ContentValidator::ValidateETSICompliance(const ContentItem& item) {
    // Basic ETSI compliance checks
    
    // Text content compliance
    if (!item.text_content.empty()) {
        if (item.text_content.length() > 128) {
            return false; // DLS text too long
        }
    }
    
    // Image content compliance
    if (!item.binary_data.empty()) {
        if (item.binary_data.size() > 50 * 1024) {
            return false; // Image too large for MOT
        }
    }
    
    return true;
}

// ContentUtils implementation
namespace ContentUtils {

std::shared_ptr<ContentItem> CreateContentFromText(const std::string& text, SchedulePriority priority) {
    auto item = std::make_shared<ContentItem>();
    item->item_id = "text_" + std::to_string(std::hash<std::string>{}(text));
    item->type = ContentType::DLS_MESSAGE;
    item->priority = priority;
    item->text_content = text;
    item->created_at = std::chrono::system_clock::now();
    item->is_active = true;
    
    // Check if Thai content
    if (text.find_first_of("กขคฆงจฉชซฌญฎฏฐฑฒณดตถทธนบปผฝพฟภมยรลวศษสหฬอฮ") != std::string::npos) {
        item->is_thai_content = true;
    }
    
    // Set default schedule
    item->schedule.start_time = std::chrono::system_clock::now();
    item->schedule.end_time = item->schedule.start_time + std::chrono::hours{1};
    
    return item;
}

std::shared_ptr<ContentItem> CreateEmergencyContent(const std::string& message) {
    auto item = CreateContentFromText(message, SchedulePriority::EMERGENCY);
    item->type = ContentType::EMERGENCY_ALERT;
    item->is_emergency = true;
    
    // Emergency content has immediate schedule
    item->schedule.start_time = std::chrono::system_clock::now();
    item->schedule.end_time = item->schedule.start_time + std::chrono::minutes{30};
    
    return item;
}

ScheduleWindow CreateImmediateSchedule(std::chrono::seconds duration) {
    ScheduleWindow schedule;
    schedule.start_time = std::chrono::system_clock::now();
    schedule.end_time = schedule.start_time + duration;
    schedule.duration = std::chrono::duration_cast<std::chrono::minutes>(duration);
    return schedule;
}

bool IsThaiContent(const std::string& text) {
    return text.find_first_of("กขคฆงจฉชซฌญฎฏฐฑฒณดตถทธนบปผฝพฟภมยรลวศษสหฬอฮ") != std::string::npos;
}

std::string ExtractContentHash(const ContentItem& item) {
    std::ostringstream content;
    content << item.text_content << item.image_path;
    
    std::string content_str = content.str();
    return std::to_string(std::hash<std::string>{}(content_str));
}

} // namespace ContentUtils

} // namespace StreamDAB