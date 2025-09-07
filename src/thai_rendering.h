/*
    Thai Language Rendering and Cultural Features
    Copyright (C) 2024 StreamDAB Project
    
    UTF-8 to Thai DAB profile conversion
    Buddhist calendar integration
    Cultural content validation
    ETSI TS 101 756 Thai character set compliance
*/

#ifndef THAI_RENDERING_H_
#define THAI_RENDERING_H_

#include "common.h"
#include "charset.h"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <functional>

namespace StreamDAB {

// Thai character ranges and classifications
namespace ThaiChars {
    constexpr uint16_t THAI_CONSONANT_START = 0x0E01;
    constexpr uint16_t THAI_CONSONANT_END = 0x0E2E;
    constexpr uint16_t THAI_VOWEL_START = 0x0E30;
    constexpr uint16_t THAI_VOWEL_END = 0x0E4F;
    constexpr uint16_t THAI_DIGIT_START = 0x0E50;
    constexpr uint16_t THAI_DIGIT_END = 0x0E59;
    constexpr uint16_t THAI_SYMBOL_START = 0x0E5A;
    constexpr uint16_t THAI_SYMBOL_END = 0x0E5B;
}

// Buddhist calendar dates and events
struct BuddhistDate {
    int year_be = 0;      // Buddhist Era year
    int year_ce = 0;      // Common Era year
    int month = 0;
    int day = 0;
    std::string thai_month_name;
    std::string thai_day_name;
    bool is_holy_day = false;
    bool is_national_holiday = false;
    std::string event_description_thai;
    std::string event_description_english;
};

// Thai text layout and rendering information
struct ThaiTextLayout {
    std::string original_text;
    std::vector<uint8_t> dab_encoded_data;
    std::vector<uint16_t> character_positions;
    std::vector<uint8_t> character_widths;
    size_t total_width_pixels = 0;
    size_t total_height_pixels = 0;
    bool requires_complex_layout = false;
    std::vector<std::string> line_breaks;
};

// Cultural content validation results
struct CulturalValidation {
    bool is_appropriate = true;
    bool contains_religious_content = false;
    bool contains_royal_references = false;
    bool requires_special_formatting = false;
    std::vector<std::string> warnings;
    std::vector<std::string> suggestions;
    double cultural_sensitivity_score = 1.0;
};

// Thai number formatting options
enum class ThaiNumberFormat {
    WESTERN_DIGITS,     // 0-9
    THAI_DIGITS,        // ๐-๙
    THAI_WORDS,         // หนึ่ง สอง สาม...
    MIXED               // Context dependent
};

class ThaiLanguageProcessor {
private:
    std::map<uint16_t, uint8_t> utf8_to_dab_map_;
    std::map<std::string, BuddhistDate> holiday_calendar_;
    std::vector<std::string> inappropriate_words_;
    std::vector<std::string> royal_terms_;
    std::vector<std::string> religious_terms_;
    
    // Font and rendering data
    struct ThaiFontMetrics {
        std::map<uint16_t, uint8_t> character_widths;
        uint8_t line_height = 16;
        uint8_t baseline = 12;
        uint8_t ascent = 4;
        uint8_t descent = 4;
    } font_metrics_;
    
    void InitializeUTF8ToDABMapping();
    void InitializeHolidayCalendar();
    void InitializeCulturalData();
    void InitializeFontMetrics();
    
    bool IsThaiCharacter(uint16_t codepoint) const;
    bool IsThaiVowel(uint16_t codepoint) const;
    bool IsThaiTone(uint16_t codepoint) const;
    bool IsThaiConsonant(uint16_t codepoint) const;
    bool RequiresComplexLayout(const std::string& text) const;
    
public:
    ThaiLanguageProcessor();
    ~ThaiLanguageProcessor() = default;
    
    // Core conversion functions
    bool ConvertUTF8ToDAB(const std::string& utf8_text, std::vector<uint8_t>& dab_data);
    std::string ConvertDABToUTF8(const std::vector<uint8_t>& dab_data);
    
    // Text layout and rendering
    ThaiTextLayout AnalyzeTextLayout(const std::string& utf8_text, 
                                     uint16_t max_width_pixels = 128,
                                     uint16_t max_lines = 4);
    bool FormatTextForDLS(const std::string& input_text, 
                         std::string& output_text,
                         size_t max_length = 128);
    
    // Number formatting
    std::string FormatNumber(int number, ThaiNumberFormat format = ThaiNumberFormat::THAI_DIGITS);
    std::string FormatCurrency(double amount, bool use_thai_digits = true);
    std::string FormatDate(const std::chrono::system_clock::time_point& date, bool buddhist_era = true);
    std::string FormatTime(const std::chrono::system_clock::time_point& time, bool use_thai_digits = true);
    
    // Buddhist calendar functions
    BuddhistDate GetBuddhistDate(const std::chrono::system_clock::time_point& date);
    std::vector<BuddhistDate> GetHolidaysInMonth(int year_be, int month);
    bool IsHolyDay(const std::chrono::system_clock::time_point& date);
    std::string GetThaiMonthName(int month);
    std::string GetThaiDayName(const std::chrono::system_clock::time_point& date);
    
    // Cultural content validation
    CulturalValidation ValidateContent(const std::string& text);
    bool IsAppropriateForBroadcast(const std::string& text);
    std::string SanitizeText(const std::string& text);
    
    // Font and display optimization
    uint16_t CalculateTextWidth(const std::string& text) const;
    uint8_t CalculateTextHeight(const std::string& text) const;
    std::vector<std::string> WrapText(const std::string& text, uint16_t max_width);
    
    // ETSI compliance
    bool ValidateETSICompliance(const std::vector<uint8_t>& dab_data);
    std::vector<uint8_t> EnsureETSICompliance(const std::vector<uint8_t>& input_data);
    
    // Configuration and customization
    void SetFontMetrics(const ThaiFontMetrics& metrics);
    ThaiFontMetrics GetFontMetrics() const { return font_metrics_; }
    void LoadCustomHolidays(const std::string& config_file);
    void LoadCulturalRules(const std::string& config_file);
};

// Thai text utilities
class ThaiTextUtils {
public:
    // Word segmentation for Thai text (no spaces)
    static std::vector<std::string> SegmentWords(const std::string& text);
    
    // Syllable analysis
    static std::vector<std::string> AnalyzeSyllables(const std::string& text);
    
    // Romanization
    static std::string ToRoman(const std::string& thai_text, bool use_royal_system = true);
    
    // Text statistics
    static size_t CountCharacters(const std::string& text);
    static size_t CountWords(const std::string& text);
    static size_t CountSyllables(const std::string& text);
    
    // Text validation
    static bool HasValidThaiStructure(const std::string& text);
    static std::vector<std::string> FindInvalidSequences(const std::string& text);
    
    // Text normalization
    static std::string NormalizeText(const std::string& text);
    static std::string RemoveInvisibleCharacters(const std::string& text);
};

// Buddhist calendar utilities
class BuddhistCalendar {
public:
    static int CEtoBE(int ce_year) { return ce_year + 543; }
    static int BEtoCE(int be_year) { return be_year - 543; }
    
    static bool IsHolyDay(int year_be, int month, int day);
    static std::vector<BuddhistDate> GetHolyDays(int year_be);
    static std::vector<BuddhistDate> GetNationalHolidays(int year_be);
    
    // Moon phase calculations for Buddhist dates
    static int GetMoonPhase(int year_ce, int month, int day);
    static bool IsFullMoon(int year_ce, int month, int day);
    static bool IsNewMoon(int year_ce, int month, int day);
    
    // Traditional Thai calendar
    static std::string GetThaiEra(int year_be);
    static std::string GetAnimalYear(int year_be);
    static std::string GetZodiacSign(int month, int day);
};

// Thai cultural content analyzer
class CulturalContentAnalyzer {
private:
    std::map<std::string, double> sensitivity_scores_;
    std::vector<std::string> royal_vocabulary_;
    std::vector<std::string> religious_vocabulary_;
    std::vector<std::string> formal_vocabulary_;
    
public:
    CulturalContentAnalyzer();
    
    double AnalyzeFormalityLevel(const std::string& text);
    double AnalyzeReligiousContent(const std::string& text);
    double AnalyzeRoyalContent(const std::string& text);
    
    std::vector<std::string> SuggestAlternatives(const std::string& inappropriate_text);
    std::string AdaptForContext(const std::string& text, const std::string& context);
    
    bool IsAppropriateForTime(const std::string& text, 
                             const std::chrono::system_clock::time_point& broadcast_time);
    bool IsAppropriateForAudience(const std::string& text, const std::string& audience_type);
};

// DLS message optimization for Thai content
class ThaiDLSOptimizer {
public:
    struct OptimizationResult {
        std::string optimized_text;
        size_t original_length = 0;
        size_t optimized_length = 0;
        double compression_ratio = 0.0;
        std::vector<std::string> applied_optimizations;
    };
    
    static OptimizationResult OptimizeForDLS(const std::string& input_text, 
                                            size_t max_length = 128);
    
    static std::string CompressText(const std::string& text);
    static std::string ExpandAbbreviations(const std::string& text);
    static std::string UseCommonAbbreviations(const std::string& text);
    
private:
    static std::map<std::string, std::string> common_abbreviations_;
    static std::map<std::string, std::string> expansion_rules_;
};

} // namespace StreamDAB

#endif // THAI_RENDERING_H_