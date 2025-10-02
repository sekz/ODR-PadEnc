/*
    StreamDAB Content Manager Integration
    Copyright (C) 2024 StreamDAB Project
    
    Centralized content orchestration
    Smart content scheduling  
    Emergency content override
    Multi-channel coordination
*/

#ifndef CONTENT_MANAGER_H_
#define CONTENT_MANAGER_H_

#include "common.h"
#include "enhanced_mot.h"
#include "thai_rendering.h"
#include "smart_dls.h"
#include "api_interface.h"

#include <string>
#include <memory>
#include <chrono>
#include <map>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

namespace StreamDAB {

// Content scheduling priority
enum class SchedulePriority {
    EMERGENCY = 0,      // Immediate override
    URGENT = 1,         // High priority, schedule ASAP
    NORMAL = 2,         // Regular scheduling
    LOW = 3,            // Fill gaps
    BACKGROUND = 4      // Only when nothing else
};

// Content item type
enum class ContentType {
    MOT_SLIDESHOW,      // Image slideshow
    DLS_MESSAGE,        // Text message
    COMBINED,           // Both image and text
    EMERGENCY_ALERT,    // Emergency broadcast
    MAINTENANCE_MSG,    // System maintenance
    PROMOTIONAL,        // Advertisement/promotion
    WEATHER_UPDATE,     // Weather information
    TRAFFIC_UPDATE,     // Traffic information
    NEWS_FLASH         // Breaking news
};

// Content scheduling window
struct ScheduleWindow {
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    bool repeat_daily = false;
    bool repeat_weekly = false;
    std::vector<int> days_of_week; // 0=Sunday, 1=Monday, etc.
    std::chrono::minutes duration{0};
    int max_repeats = 0;
    int current_repeats = 0;
};

// Content item for scheduling
struct ContentItem {
    std::string item_id;
    ContentType type;
    SchedulePriority priority;
    ScheduleWindow schedule;
    
    // Content data
    std::string text_content;
    std::string image_path;
    std::vector<uint8_t> binary_data;
    std::map<std::string, std::string> metadata;
    
    // Thai language support
    bool is_thai_content = false;
    std::string thai_romanization;
    CulturalValidation cultural_validation;
    
    // Scheduling state
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point last_scheduled;
    std::chrono::system_clock::time_point next_scheduled;
    int schedule_count = 0;
    bool is_active = true;
    bool is_emergency = false;
    
    // Performance tracking
    struct Metrics {
        std::chrono::milliseconds total_display_time{0};
        int display_count = 0;
        double user_engagement = 0.0;
        bool delivery_confirmed = false;
        std::chrono::system_clock::time_point last_displayed;
    } metrics;
    
    // Source information
    std::string source_channel;
    std::string source_application;
    std::string creator_id;
    std::string content_hash;
};

// Content scheduling engine
class ContentScheduler {
private:
    std::vector<std::shared_ptr<ContentItem>> scheduled_content_;
    mutable std::mutex schedule_mutex_;
    std::atomic<bool> scheduler_running_{false};
    std::thread scheduler_thread_;
    
    // Current active content
    std::shared_ptr<ContentItem> current_mot_content_;
    std::shared_ptr<ContentItem> current_dls_content_;
    
    // Emergency override
    std::atomic<bool> emergency_override_{false};
    std::shared_ptr<ContentItem> emergency_content_;
    std::chrono::system_clock::time_point emergency_start_;
    std::chrono::seconds emergency_duration_{0};
    
    // Scheduling algorithms
    std::shared_ptr<ContentItem> SelectNextMOTContent();
    std::shared_ptr<ContentItem> SelectNextDLSContent();
    bool ShouldScheduleContent(const ContentItem& item) const;
    double CalculateSchedulingScore(const ContentItem& item) const;
    
    void SchedulingLoop();
    void ProcessScheduledContent();
    void UpdateContentMetrics();
    
public:
    ContentScheduler();
    ~ContentScheduler();
    
    // Scheduler control
    void Start();
    void Stop();
    bool IsRunning() const { return scheduler_running_; }
    
    // Content management
    bool AddContent(std::shared_ptr<ContentItem> item);
    bool RemoveContent(const std::string& item_id);
    bool UpdateContent(std::shared_ptr<ContentItem> item);
    std::shared_ptr<ContentItem> GetContent(const std::string& item_id) const;
    std::vector<std::shared_ptr<ContentItem>> GetActiveContent() const;
    
    // Emergency override
    void TriggerEmergency(std::shared_ptr<ContentItem> emergency_item, 
                         std::chrono::seconds duration = std::chrono::seconds{300});
    void ClearEmergency();
    bool IsEmergencyActive() const { return emergency_override_; }
    
    // Current content access
    std::shared_ptr<ContentItem> GetCurrentMOTContent() const;
    std::shared_ptr<ContentItem> GetCurrentDLSContent() const;
    
    // Statistics and monitoring
    struct SchedulerStatistics {
        size_t total_content_items = 0;
        size_t active_content_items = 0;
        size_t scheduled_today = 0;
        size_t emergency_overrides = 0;
        std::chrono::system_clock::time_point last_schedule_update;
        std::map<ContentType, size_t> content_type_counts;
        std::map<SchedulePriority, size_t> priority_distribution;
        double average_display_duration = 0.0;
    };
    SchedulerStatistics GetStatistics() const;
};

// Content coordination with other StreamDAB components
class ContentCoordinator {
private:
    // Component interfaces
    std::unique_ptr<EnhancedMOTProcessor> mot_processor_;
    std::unique_ptr<SmartDLSProcessor> dls_processor_;
    std::unique_ptr<ThaiLanguageProcessor> thai_processor_;
    std::unique_ptr<ContentScheduler> scheduler_;
    std::unique_ptr<StreamDABAPIService> api_service_;
    
    // Configuration
    struct CoordinatorConfig {
        std::chrono::seconds content_sync_interval{30};
        std::chrono::seconds status_broadcast_interval{10};
        bool enable_thai_processing = true;
        bool enable_emergency_override = true;
        bool enable_cultural_validation = true;
        std::string content_manager_url;
        std::string compliance_monitor_url;
        std::vector<std::string> trusted_sources;
    } config_;
    
    // State management
    std::atomic<bool> coordinator_running_{false};
    std::thread coordination_thread_;
    std::mutex state_mutex_;
    
    // Content synchronization
    std::chrono::system_clock::time_point last_sync_;
    std::map<std::string, std::chrono::system_clock::time_point> component_status_;
    
    // Inter-component communication
    void CoordinationLoop();
    void SynchronizeContent();
    void BroadcastStatus();
    void HandleEmergencyAlert(const std::string& alert_data);
    void ValidateContentCompliance();
    
    // Component integration
    void ProcessMOTContent(std::shared_ptr<ContentItem> item);
    void ProcessDLSContent(std::shared_ptr<ContentItem> item);
    void ProcessThaiContent(std::shared_ptr<ContentItem> item);
    
public:
    explicit ContentCoordinator(const CoordinatorConfig& config);
    ContentCoordinator(); // Default constructor
    ~ContentCoordinator();
    
    // Coordinator control
    bool Initialize();
    void Start();
    void Stop();
    bool IsRunning() const { return coordinator_running_; }
    
    // Component access
    ContentScheduler* GetScheduler() { return scheduler_.get(); }
    StreamDABAPIService* GetAPIService() { return api_service_.get(); }
    
    // Content management interface
    bool AddContent(const std::string& content_data, ContentType type, SchedulePriority priority = SchedulePriority::NORMAL);
    bool RemoveContent(const std::string& item_id);
    bool UpdateContent(const std::string& item_id, const std::string& new_data);
    
    // Emergency management
    void TriggerEmergencyBroadcast(const std::string& message, std::chrono::seconds duration = std::chrono::seconds{300});
    void ClearEmergencyBroadcast();
    
    // System integration
    void RegisterWithContentManager(const std::string& manager_url);
    void RegisterWithComplianceMonitor(const std::string& monitor_url);
    
    // Status and health
    struct SystemHealth {
        bool overall_healthy = true;
        std::map<std::string, bool> component_status;
        std::vector<std::string> warnings;
        std::vector<std::string> errors;
        std::chrono::system_clock::time_point last_check;
        double system_load = 0.0;
        size_t memory_usage = 0;
        size_t active_connections = 0;
    };
    SystemHealth GetSystemHealth() const;
    
    // Performance monitoring
    struct PerformanceReport {
        std::chrono::microseconds avg_mot_processing_time{0};
        std::chrono::microseconds avg_dls_processing_time{0};
        std::chrono::microseconds avg_thai_processing_time{0};
        size_t content_items_processed_per_hour = 0;
        double thai_content_percentage = 0.0;
        size_t emergency_activations = 0;
        std::chrono::system_clock::time_point report_generated;
    };
    PerformanceReport GeneratePerformanceReport() const;
    
    // Configuration management
    void UpdateConfiguration(const CoordinatorConfig& new_config);
    CoordinatorConfig GetConfiguration() const { return config_; }
};

// Content validation and compliance checking
class ContentValidator {
private:
    std::unique_ptr<SecurePathValidator> path_validator_;
    std::unique_ptr<ContentSecurityScanner> security_scanner_;
    std::unique_ptr<ThaiLanguageProcessor> thai_processor_;
    
    // Validation rules
    struct ValidationRules {
        size_t max_image_size = 5 * 1024 * 1024; // 5MB
        size_t max_text_length = 256;
        bool require_cultural_validation = true;
        bool block_inappropriate_content = true;
        bool require_thai_compliance = true;
        std::vector<std::string> allowed_image_formats = {"JPEG", "PNG", "WebP"};
        std::vector<std::string> blocked_keywords;
    } rules_;
    
public:
    explicit ContentValidator(const ValidationRules& rules);
    ContentValidator(); // Default constructor
    
    struct ValidationResult {
        bool is_valid = false;
        bool is_safe = true;
        bool requires_review = false;
        std::vector<std::string> violations;
        std::vector<std::string> warnings;
        std::vector<std::string> suggestions;
        CulturalValidation cultural_result;
        SecurityValidation security_result;
        double compliance_score = 0.0;
    };
    
    // Content validation
    ValidationResult ValidateContentItem(const ContentItem& item);
    ValidationResult ValidateImage(const std::vector<uint8_t>& image_data, const std::string& format);
    ValidationResult ValidateText(const std::string& text, bool is_thai = false);
    ValidationResult ValidateScheduling(const ScheduleWindow& schedule);
    
    // ETSI compliance validation
    bool ValidateETSICompliance(const ContentItem& item);
    std::vector<std::string> CheckETSIViolations(const ContentItem& item);
    
    // Cultural and regulatory compliance
    bool ValidateThaiCompliance(const ContentItem& item);
    bool ValidateNBTCCompliance(const ContentItem& item);
    
    // Configuration
    void UpdateValidationRules(const ValidationRules& new_rules);
    ValidationRules GetValidationRules() const { return rules_; }
};

// Integration utilities
namespace ContentUtils {
    // Content conversion utilities
    std::shared_ptr<ContentItem> CreateContentFromImage(const std::string& image_path, SchedulePriority priority = SchedulePriority::NORMAL);
    std::shared_ptr<ContentItem> CreateContentFromText(const std::string& text, SchedulePriority priority = SchedulePriority::NORMAL);
    std::shared_ptr<ContentItem> CreateEmergencyContent(const std::string& message);
    
    // Scheduling utilities
    ScheduleWindow CreateDailySchedule(int hour, int minute, int duration_minutes);
    ScheduleWindow CreateWeeklySchedule(const std::vector<int>& days, int hour, int minute, int duration_minutes);
    ScheduleWindow CreateImmediateSchedule(std::chrono::seconds duration);
    
    // Content analysis
    ContentType DetectContentType(const std::vector<uint8_t>& data);
    bool IsThaiContent(const std::string& text);
    std::string ExtractContentHash(const ContentItem& item);
    
    // Format conversion
    std::string ContentItemToJSON(const ContentItem& item);
    std::shared_ptr<ContentItem> ContentItemFromJSON(const std::string& json);
    std::vector<uint8_t> SerializeContentItem(const ContentItem& item);
    std::shared_ptr<ContentItem> DeserializeContentItem(const std::vector<uint8_t>& data);
    
    // Performance optimization
    void OptimizeImageForBroadcast(std::vector<uint8_t>& image_data);
    void OptimizeTextForDLS(std::string& text, size_t max_length = 128);
    
    // Integration helpers
    bool NotifyContentManager(const std::string& manager_url, const std::string& event, const std::map<std::string, std::string>& data);
    bool SendComplianceReport(const std::string& monitor_url, const std::vector<std::string>& violations);
}

} // namespace StreamDAB

#endif // CONTENT_MANAGER_H_