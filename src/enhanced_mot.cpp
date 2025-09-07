/*
    Enhanced MOT SlideShow Implementation
    Copyright (C) 2024 StreamDAB Project
*/

#include "enhanced_mot.h"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#ifdef HAVE_IMAGEMAGICK
#include <Magick++.h>
#endif
#ifdef HAVE_OPENSSL
#include <openssl/md5.h>
#endif

namespace fs = std::filesystem;
using namespace std::chrono;

namespace StreamDAB {

EnhancedMOTProcessor::EnhancedMOTProcessor(const CarouselConfig& config) 
    : config_(config) {
#ifdef HAVE_IMAGEMAGICK
    // Initialize Magick++
    try {
        Magick::InitializeMagick("");
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize Magick++: " << e.what() << std::endl;
    }
#endif
}

EnhancedMOTProcessor::~EnhancedMOTProcessor() {
    StopBackgroundProcessing();
}

ImageFormat EnhancedMOTProcessor::DetectImageFormat(const std::string& filepath) {
#ifdef HAVE_IMAGEMAGICK
    try {
        Magick::Image image;
        image.read(filepath);
        std::string format = image.format();
        
        if (format == "JPEG" || format == "JPG") return ImageFormat::JPEG;
        if (format == "PNG") return ImageFormat::PNG;
        if (format == "WEBP") return ImageFormat::WEBP;
        if (format == "HEIF" || format == "HEIC") return ImageFormat::HEIF;
        
    } catch (const std::exception& e) {
        std::cerr << "Error detecting format for " << filepath << ": " << e.what() << std::endl;
    }
#else
    // Simple file extension based detection without ImageMagick
    std::string ext = fs::path(filepath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".jpg" || ext == ".jpeg") return ImageFormat::JPEG;
    if (ext == ".png") return ImageFormat::PNG;
    if (ext == ".webp") return ImageFormat::WEBP;
    if (ext == ".heic" || ext == ".heif") return ImageFormat::HEIF;
#endif
    
    return ImageFormat::UNKNOWN;
}

std::string EnhancedMOTProcessor::CalculateImageHash(const std::vector<uint8_t>& data) {
#ifdef HAVE_OPENSSL
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5(data.data(), data.size(), hash);
    
    std::ostringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
#else
    // Simple hash fallback without OpenSSL
    size_t hash = std::hash<std::string>{}(std::string(data.begin(), data.end()));
    std::ostringstream ss;
    ss << std::hex << hash;
    return ss.str();
#endif
}

ImageQuality EnhancedMOTProcessor::AnalyzeImageQuality(const std::vector<uint8_t>& image_data) {
    ImageQuality quality;
    
#ifdef HAVE_IMAGEMAGICK
    try {
        // Create image from data
        Magick::Blob blob(image_data.data(), image_data.size());
        Magick::Image image(blob);
        
        // Calculate sharpness using Laplacian variance
        Magick::Image temp_image = image;
        temp_image.edge(1.0);
        quality.sharpness = temp_image.statistics().mean.quantum;
        
        // Calculate contrast using standard deviation
        auto stats = image.statistics();
        quality.contrast = stats.standard_deviation.quantum;
        
        // Calculate brightness using mean luminance
        quality.brightness = stats.mean.quantum;
        
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing image quality: " << e.what() << std::endl;
    }
#else
    // Simple quality estimation without ImageMagick
    quality.sharpness = 0.7; // Default reasonable values
    quality.contrast = 0.6;
    quality.brightness = 0.5;
#endif
    
    // Initialize freshness score
    quality.freshness_score = 1.0;
    quality.last_used = system_clock::now();
    quality.usage_count = 0;
    
    return quality;
}

bool EnhancedMOTProcessor::ConvertToWebP(const Magick::Image& image, 
                                         std::vector<uint8_t>& output_data, 
                                         int quality) {
    try {
        Magick::Image temp_image = image;
        temp_image.format("WEBP");
        temp_image.quality(quality);
        
        Magick::Blob blob;
        temp_image.write(&blob);
        
        const void* data = blob.data();
        size_t length = blob.length();
        
        output_data.assign(static_cast<const uint8_t*>(data), 
                          static_cast<const uint8_t*>(data) + length);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error converting to WebP: " << e.what() << std::endl;
        return false;
    }
}

bool EnhancedMOTProcessor::ConvertToHEIF(const Magick::Image& image, 
                                         std::vector<uint8_t>& output_data, 
                                         int quality) {
#ifdef HAVE_HEIF
    try {
        // Convert Magick::Image to raw data
        Magick::Image temp_image = image;
        temp_image.format("RGB");
        temp_image.type(Magick::TrueColorType);
        
        size_t width = temp_image.columns();
        size_t height = temp_image.rows();
        
        std::vector<uint8_t> rgb_data(width * height * 3);
        temp_image.write(0, 0, width, height, "RGB", Magick::CharPixel, rgb_data.data());
        
        // Initialize HEIF encoder
        heif_context* ctx = heif_context_alloc();
        heif_encoder* encoder = nullptr;
        heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
        
        if (!encoder) {
            heif_context_free(ctx);
            return false;
        }
        
        // Set quality
        heif_encoder_set_lossy_quality(encoder, quality);
        
        // Create image
        heif_image* heif_img = nullptr;
        heif_image_create(width, height, heif_colorspace_RGB, heif_chroma_444, &heif_img);
        
        // Add plane
        heif_image_add_plane(heif_img, heif_channel_R, width, height, 8);
        heif_image_add_plane(heif_img, heif_channel_G, width, height, 8);
        heif_image_add_plane(heif_img, heif_channel_B, width, height, 8);
        
        // Copy data to HEIF image planes
        int stride_r, stride_g, stride_b;
        uint8_t* plane_r = heif_image_get_plane(heif_img, heif_channel_R, &stride_r);
        uint8_t* plane_g = heif_image_get_plane(heif_img, heif_channel_G, &stride_g);
        uint8_t* plane_b = heif_image_get_plane(heif_img, heif_channel_B, &stride_b);
        
        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                size_t idx = (y * width + x) * 3;
                plane_r[y * stride_r + x] = rgb_data[idx];
                plane_g[y * stride_g + x] = rgb_data[idx + 1];
                plane_b[y * stride_b + x] = rgb_data[idx + 2];
            }
        }
        
        // Encode
        heif_image_handle* handle = nullptr;
        heif_context_encode_image(ctx, heif_img, encoder, nullptr, &handle);
        
        // Get encoded data
        uint8_t* data = nullptr;
        size_t data_size = 0;
        heif_context_write_to_memory_without_box(ctx, (void**)&data, &data_size, nullptr);
        
        output_data.assign(data, data + data_size);
        
        // Cleanup
        free(data);
        heif_image_handle_release(handle);
        heif_image_release(heif_img);
        heif_encoder_release(encoder);
        heif_context_free(ctx);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error converting to HEIF: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "HEIF support not compiled in" << std::endl;
    return false;
#endif
}

bool EnhancedMOTProcessor::ConvertToProgressiveJPEG(const Magick::Image& image, 
                                                    std::vector<uint8_t>& output_data, 
                                                    int quality) {
    try {
        Magick::Image temp_image = image;
        temp_image.format("JPEG");
        temp_image.quality(quality);
        temp_image.interlaceType(Magick::PlaneInterlace); // Progressive JPEG
        
        Magick::Blob blob;
        temp_image.write(&blob);
        
        const void* data = blob.data();
        size_t length = blob.length();
        
        output_data.assign(static_cast<const uint8_t*>(data), 
                          static_cast<const uint8_t*>(data) + length);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error converting to progressive JPEG: " << e.what() << std::endl;
        return false;
    }
}

bool EnhancedMOTProcessor::OptimizeImage(const std::string& input_path, 
                                         std::vector<uint8_t>& output_data, 
                                         ImageFormat target_format) {
    try {
        Magick::Image image;
        image.read(input_path);
        
        // Apply DAB-specific optimizations
        ImageOptimizer::ApplyDABProfile(image);
        
        // Resize if necessary
        if (image.columns() > 320 || image.rows() > 240) {
            ImageOptimizer::ResizeImage(image, 320, 240);
        }
        
        // Convert to target format
        switch (target_format) {
            case ImageFormat::WEBP:
                return ConvertToWebP(image, output_data, 80);
            case ImageFormat::HEIF:
                return ConvertToHEIF(image, output_data, 80);
            case ImageFormat::JPEG:
                if (config_.enable_progressive_jpeg) {
                    return ConvertToProgressiveJPEG(image, output_data, 85);
                } else {
                    image.format("JPEG");
                    image.quality(85);
                }
                break;
            default:
                // Keep original format
                break;
        }
        
        // Default fallback to JPEG
        Magick::Blob blob;
        image.write(&blob);
        
        const void* data = blob.data();
        size_t length = blob.length();
        
        output_data.assign(static_cast<const uint8_t*>(data), 
                          static_cast<const uint8_t*>(data) + length);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error optimizing image " << input_path << ": " << e.what() << std::endl;
        return false;
    }
}

double EnhancedMOTProcessor::CalculateFreshnessScore(const EnhancedImageData& image_data) {
    auto now = system_clock::now();
    auto time_since_last_use = duration_cast<hours>(now - image_data.quality.last_used).count();
    
    // Freshness decreases over time and with usage
    double time_factor = std::exp(-time_since_last_use / 24.0); // Half-life of 24 hours
    double usage_factor = 1.0 / (1.0 + image_data.quality.usage_count * 0.1);
    
    return time_factor * usage_factor;
}

std::vector<size_t> EnhancedMOTProcessor::SelectBestImages(size_t count) {
    std::vector<std::pair<double, size_t>> scored_images;
    
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    for (size_t i = 0; i < image_cache_.size(); ++i) {
        const auto& image = image_cache_[i];
        double score = 0.0;
        
        // Quality-based scoring
        score += image->quality.sharpness * 0.3;
        score += image->quality.contrast * 0.2;
        score += (1.0 - image->quality.brightness) * 0.1; // Prefer medium brightness
        
        // Freshness scoring
        score += CalculateFreshnessScore(*image) * 0.4;
        
        scored_images.emplace_back(score, i);
    }
    
    // Sort by score (highest first)
    std::sort(scored_images.begin(), scored_images.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    std::vector<size_t> selected_indices;
    for (size_t i = 0; i < std::min(count, scored_images.size()); ++i) {
        selected_indices.push_back(scored_images[i].second);
    }
    
    return selected_indices;
}

bool EnhancedMOTProcessor::IsDuplicate(const std::string& hash) {
    return hash_index_.find(hash) != hash_index_.end();
}

bool EnhancedMOTProcessor::ProcessImageDirectory(const std::string& directory_path) {
    try {
        if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
            std::cerr << "Invalid directory path: " << directory_path << std::endl;
            return false;
        }
        
        size_t processed_count = 0;
        size_t skipped_count = 0;
        
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (entry.is_regular_file()) {
                std::string filepath = entry.path().string();
                std::string extension = entry.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                // Check if it's an image file
                if (extension == ".jpg" || extension == ".jpeg" || 
                    extension == ".png" || extension == ".webp" || 
                    extension == ".heic" || extension == ".heif") {
                    
                    if (AddImage(filepath)) {
                        processed_count++;
                    } else {
                        skipped_count++;
                    }
                }
            }
        }
        
        std::cout << "Processed " << processed_count << " images, skipped " << skipped_count << std::endl;
        return processed_count > 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing directory " << directory_path << ": " << e.what() << std::endl;
        return false;
    }
}

bool EnhancedMOTProcessor::AddImage(const std::string& filepath) {
    try {
        // Load and process image
        Magick::Image image;
        image.read(filepath);
        
        auto image_data = std::make_unique<EnhancedImageData>();
        image_data->filename = fs::path(filepath).filename().string();
        image_data->format = DetectImageFormat(filepath);
        image_data->width = image.columns();
        image_data->height = image.rows();
        image_data->quality = AnalyzeImageQuality(image);
        
        // Optimize image
        if (OptimizeImage(filepath, image_data->processed_data, ImageFormat::JPEG)) {
            image_data->is_optimized = true;
            image_data->quality.file_size = image_data->processed_data.size();
            image_data->hash = CalculateImageHash(image_data->processed_data);
            
            // Check for duplicates
            if (config_.enable_duplicate_detection && IsDuplicate(image_data->hash)) {
                return false; // Skip duplicate
            }
            
            std::lock_guard<std::mutex> lock(cache_mutex_);
            
            // Add to cache
            size_t index = image_cache_.size();
            hash_index_[image_data->hash] = index;
            image_cache_.push_back(std::move(image_data));
            
            // Remove old images if cache is full
            if (image_cache_.size() > config_.max_images) {
                RemoveOldImages();
            }
            
            return true;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error adding image " << filepath << ": " << e.what() << std::endl;
    }
    
    return false;
}

std::unique_ptr<EnhancedImageData> EnhancedMOTProcessor::GetNextImage() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (image_cache_.empty()) {
        return nullptr;
    }
    
    if (config_.enable_smart_selection) {
        auto selected_indices = SelectBestImages(1);
        if (!selected_indices.empty()) {
            size_t index = selected_indices[0];
            auto& image = image_cache_[index];
            
            // Update usage statistics
            image->quality.last_used = system_clock::now();
            image->quality.usage_count++;
            image->quality.freshness_score = CalculateFreshnessScore(*image);
            
            // Create a copy to return
            auto result = std::make_unique<EnhancedImageData>(*image);
            return result;
        }
    }
    
    // Fallback to round-robin selection
    static size_t current_index = 0;
    if (current_index >= image_cache_.size()) {
        current_index = 0;
    }
    
    auto result = std::make_unique<EnhancedImageData>(*image_cache_[current_index]);
    current_index++;
    
    return result;
}

void EnhancedMOTProcessor::RemoveOldImages() {
    // Remove oldest images based on usage and quality
    if (image_cache_.size() <= config_.max_images) {
        return;
    }
    
    size_t to_remove = image_cache_.size() - config_.max_images;
    
    // Create vector of indices with scores (lower score = more likely to remove)
    std::vector<std::pair<double, size_t>> scored_for_removal;
    
    for (size_t i = 0; i < image_cache_.size(); ++i) {
        const auto& image = image_cache_[i];
        double removal_score = image->quality.freshness_score * 0.6 +
                              (image->quality.sharpness + image->quality.contrast) * 0.4;
        scored_for_removal.emplace_back(removal_score, i);
    }
    
    // Sort by removal score (lowest first)
    std::sort(scored_for_removal.begin(), scored_for_removal.end());
    
    // Remove images with lowest scores
    std::vector<size_t> indices_to_remove;
    for (size_t i = 0; i < to_remove; ++i) {
        indices_to_remove.push_back(scored_for_removal[i].second);
    }
    
    // Sort indices in descending order for proper removal
    std::sort(indices_to_remove.rbegin(), indices_to_remove.rend());
    
    for (size_t index : indices_to_remove) {
        // Remove from hash index
        hash_index_.erase(image_cache_[index]->hash);
        // Remove from cache
        image_cache_.erase(image_cache_.begin() + index);
    }
    
    // Rebuild hash index
    hash_index_.clear();
    for (size_t i = 0; i < image_cache_.size(); ++i) {
        hash_index_[image_cache_[i]->hash] = i;
    }
}

size_t EnhancedMOTProcessor::GetImageCount() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return image_cache_.size();
}

double EnhancedMOTProcessor::GetAverageQuality() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    if (image_cache_.empty()) {
        return 0.0;
    }
    
    double total_quality = 0.0;
    for (const auto& image : image_cache_) {
        total_quality += (image->quality.sharpness + image->quality.contrast) / 2.0;
    }
    
    return total_quality / image_cache_.size();
}

EnhancedMOTProcessor::Statistics EnhancedMOTProcessor::GetStatistics() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    
    Statistics stats;
    stats.total_images = image_cache_.size();
    
    size_t total_original_size = 0;
    size_t total_compressed_size = 0;
    double total_quality = 0.0;
    
    for (const auto& image : image_cache_) {
        if (image->is_optimized) {
            stats.optimized_images++;
        }
        
        total_compressed_size += image->quality.file_size;
        total_quality += (image->quality.sharpness + image->quality.contrast) / 2.0;
        
        // Estimate original size (this would need to be tracked during processing)
        total_original_size += image->quality.file_size * 1.5; // Rough estimate
    }
    
    stats.total_size_bytes = total_original_size;
    stats.compressed_size_bytes = total_compressed_size;
    stats.average_quality = stats.total_images > 0 ? total_quality / stats.total_images : 0.0;
    stats.compression_ratio = total_original_size > 0 ? 
        static_cast<double>(total_compressed_size) / total_original_size : 0.0;
    
    return stats;
}

void EnhancedMOTProcessor::StartBackgroundProcessing() {
    if (!processing_active_.exchange(true)) {
        background_processor_ = std::thread(&EnhancedMOTProcessor::BackgroundProcessingLoop, this);
    }
}

void EnhancedMOTProcessor::StopBackgroundProcessing() {
    if (processing_active_.exchange(false)) {
        if (background_processor_.joinable()) {
            background_processor_.join();
        }
    }
}

void EnhancedMOTProcessor::BackgroundProcessingLoop() {
    while (processing_active_) {
        try {
            // Periodic cleanup and optimization
            std::this_thread::sleep_for(std::chrono::minutes(5));
            
            // Update freshness scores
            {
                std::lock_guard<std::mutex> lock(cache_mutex_);
                for (auto& image : image_cache_) {
                    image->quality.freshness_score = CalculateFreshnessScore(*image);
                }
            }
            
            // Check if cache needs cleanup
            if (GetImageCount() > config_.max_images * 0.9) {
                std::lock_guard<std::mutex> lock(cache_mutex_);
                RemoveOldImages();
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in background processing: " << e.what() << std::endl;
        }
    }
}

// ImageOptimizer implementation
bool ImageOptimizer::OptimizeForDAB(const std::string& input_path, 
                                   std::vector<uint8_t>& output_data,
                                   size_t max_size) {
    try {
        Magick::Image image;
        image.read(input_path);
        
        // Apply DAB profile
        ApplyDABProfile(image);
        
        // Resize if necessary
        ResizeImage(image, 320, 240);
        
        // Try different qualities to fit size constraint
        for (int quality = 95; quality >= 50; quality -= 10) {
            image.quality(quality);
            
            Magick::Blob blob;
            image.write(&blob);
            
            if (blob.length() <= max_size) {
                const void* data = blob.data();
                size_t length = blob.length();
                
                output_data.assign(static_cast<const uint8_t*>(data), 
                                  static_cast<const uint8_t*>(data) + length);
                return true;
            }
        }
        
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error optimizing image for DAB: " << e.what() << std::endl;
        return false;
    }
}

bool ImageOptimizer::ResizeImage(Magick::Image& image, uint32_t max_width, uint32_t max_height) {
    try {
        uint32_t width = image.columns();
        uint32_t height = image.rows();
        
        if (width <= max_width && height <= max_height) {
            return true; // No resize needed
        }
        
        // Calculate aspect-preserving dimensions
        double width_ratio = static_cast<double>(max_width) / width;
        double height_ratio = static_cast<double>(max_height) / height;
        double scale_ratio = std::min(width_ratio, height_ratio);
        
        uint32_t new_width = static_cast<uint32_t>(width * scale_ratio);
        uint32_t new_height = static_cast<uint32_t>(height * scale_ratio);
        
        image.resize(Magick::Geometry(new_width, new_height));
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error resizing image: " << e.what() << std::endl;
        return false;
    }
}

bool ImageOptimizer::ApplyDABProfile(Magick::Image& image) {
    try {
        // DAB-specific optimizations
        image.type(Magick::TrueColorType);
        image.colorSpace(Magick::sRGBColorspace);
        
        // Enhance for small screens
        image.sharpen(0, 1.0);
        image.normalize();
        
        // Set appropriate color depth
        image.depth(8);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error applying DAB profile: " << e.what() << std::endl;
        return false;
    }
}

double ImageOptimizer::CalculateCompressionRatio(size_t original_size, size_t compressed_size) {
    if (original_size == 0) return 0.0;
    return static_cast<double>(compressed_size) / original_size;
}

} // namespace StreamDAB