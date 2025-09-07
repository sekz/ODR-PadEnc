/*
    Enhanced MOT SlideShow Implementation
    Copyright (C) 2024 StreamDAB Project
    
    Advanced image processing with WebP, HEIF support
    Smart image carousel and content optimization
    ETSI TS 101 499 compliance with enhancements
*/

#ifndef ENHANCED_MOT_H_
#define ENHANCED_MOT_H_

#include "common.h"
#include "sls.h"
#ifdef HAVE_IMAGEMAGICK
#include <Magick++.h>
#endif
#ifdef HAVE_WEBP
#include <webp/encode.h>
#endif
#ifdef HAVE_HEIF
#include <libheif/heif.h>
#endif
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace StreamDAB {

// Image format support enumeration
enum class ImageFormat {
    JPEG,
    PNG,
    WEBP,
    HEIF,
    UNKNOWN
};

// Image quality metrics
struct ImageQuality {
    double sharpness = 0.0;
    double contrast = 0.0;
    double brightness = 0.0;
    double freshness_score = 0.0;
    size_t file_size = 0;
    std::chrono::system_clock::time_point last_used;
    int usage_count = 0;
};

// Enhanced image metadata
struct EnhancedImageData {
    std::string filename;
    std::string content_type;
    ImageFormat format;
    std::vector<uint8_t> processed_data;
    ImageQuality quality;
    std::string hash;
    bool is_optimized = false;
    uint32_t width = 0;
    uint32_t height = 0;
};

// Smart carousel configuration
struct CarouselConfig {
    size_t max_images = 50;
    std::chrono::seconds min_display_time{10};
    std::chrono::seconds max_display_time{60};
    double quality_threshold = 0.7;
    bool enable_duplicate_detection = true;
    bool enable_smart_selection = true;
    bool enable_progressive_jpeg = true;
};

class EnhancedMOTProcessor {
private:
    CarouselConfig config_;
    std::vector<std::unique_ptr<EnhancedImageData>> image_cache_;
    std::unordered_map<std::string, size_t> hash_index_;
    std::mutex cache_mutex_;
    std::atomic<bool> processing_active_{false};
    std::thread background_processor_;
    
    // Image processing methods
    ImageFormat DetectImageFormat(const std::string& filepath);
    std::string CalculateImageHash(const std::vector<uint8_t>& data);
    ImageQuality AnalyzeImageQuality(const std::vector<uint8_t>& image_data);
    bool OptimizeImage(const std::string& input_path, std::vector<uint8_t>& output_data, ImageFormat target_format);
    bool ConvertToWebP(const Magick::Image& image, std::vector<uint8_t>& output_data, int quality = 80);
    bool ConvertToHEIF(const Magick::Image& image, std::vector<uint8_t>& output_data, int quality = 80);
    bool ConvertToProgressiveJPEG(const Magick::Image& image, std::vector<uint8_t>& output_data, int quality = 85);
    
    // Smart selection algorithms
    double CalculateFreshnessScore(const EnhancedImageData& image_data);
    std::vector<size_t> SelectBestImages(size_t count);
    bool IsDuplicate(const std::string& hash);
    void RemoveOldImages();
    void BackgroundProcessingLoop();
    
public:
    explicit EnhancedMOTProcessor(const CarouselConfig& config = CarouselConfig{});
    ~EnhancedMOTProcessor();
    
    // Main interface methods
    bool ProcessImageDirectory(const std::string& directory_path);
    bool AddImage(const std::string& filepath);
    bool RemoveImage(const std::string& filename);
    std::unique_ptr<EnhancedImageData> GetNextImage();
    std::vector<std::string> GetImageList() const;
    
    // Configuration and status
    void UpdateConfig(const CarouselConfig& config);
    CarouselConfig GetConfig() const { return config_; }
    size_t GetImageCount() const;
    double GetAverageQuality() const;
    
    // ETSI compliance methods
    bool ValidateETSICompliance(const EnhancedImageData& image_data);
    std::vector<uint8_t> GenerateMOTObject(const EnhancedImageData& image_data, uint16_t transport_id);
    
    // Statistics and monitoring
    struct Statistics {
        size_t total_images = 0;
        size_t optimized_images = 0;
        size_t duplicates_removed = 0;
        double average_quality = 0.0;
        size_t total_size_bytes = 0;
        size_t compressed_size_bytes = 0;
        double compression_ratio = 0.0;
    };
    Statistics GetStatistics() const;
    
    // Thread safety
    void StartBackgroundProcessing();
    void StopBackgroundProcessing();
};

// Image optimization utilities
class ImageOptimizer {
public:
    static bool OptimizeForDAB(const std::string& input_path, 
                              std::vector<uint8_t>& output_data,
                              size_t max_size = SLSEncoder::MAXSLIDESIZE_SIMPLE);
    
    static bool ResizeImage(Magick::Image& image, uint32_t max_width, uint32_t max_height);
    
    static bool ApplyDABProfile(Magick::Image& image);
    
    static double CalculateCompressionRatio(size_t original_size, size_t compressed_size);
};

// Content-aware selection engine
class SmartContentSelector {
private:
    std::function<double(const EnhancedImageData&)> scoring_function_;
    
public:
    explicit SmartContentSelector(std::function<double(const EnhancedImageData&)> scorer = nullptr);
    
    std::vector<size_t> SelectContent(const std::vector<std::unique_ptr<EnhancedImageData>>& images,
                                     size_t count,
                                     const std::vector<std::string>& excluded_hashes = {});
    
    void SetScoringFunction(std::function<double(const EnhancedImageData&)> scorer);
    
    // Pre-defined scoring strategies
    static double QualityBasedScoring(const EnhancedImageData& image);
    static double RecencyBasedScoring(const EnhancedImageData& image);
    static double BalancedScoring(const EnhancedImageData& image);
};

} // namespace StreamDAB

#endif // ENHANCED_MOT_H_