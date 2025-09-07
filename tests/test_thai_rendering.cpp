/*
    Google Test Suite - Thai Language Rendering Testing
    Copyright (C) 2024 StreamDAB Project
    
    Tests for Thai language support:
    - UTF-8 to Thai DAB profile conversion
    - Thai text rendering and font optimization
    - Buddhist calendar integration
    - Cultural content validation
    - ETSI TS 101 756 Thai character set compliance
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/thai_rendering.h"
#include <chrono>
#include <locale>

using namespace StreamDAB;
using namespace testing;

class ThaiRenderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        processor_ = std::make_unique<ThaiLanguageProcessor>();
        
        // Test Thai text samples
        thai_text_simple_ = "สวัสดี";  // Hello
        thai_text_complex_ = "สวัสดีครับ ผมชื่อสมชาย"; // Hello, my name is Somchai
        thai_text_with_tones_ = "ไก่ ไข่ ใคร ใคร"; // Chicken, egg, who, who (with tone marks)
        thai_text_long_ = "ประเทศไทยมีประวัติศาสตร์อันยาวนานและมีวัฒนธรรมที่หลากหลาย"; // Long Thai sentence
        thai_text_religious_ = "พระพุทธเจ้า พระธรรม พระสงฆ์"; // Buddha, Dharma, Sangha
        thai_text_royal_ = "พระบาทสมเด็จพระเจ้าอยู่หัว"; // His Majesty the King
        
        english_text_ = "Hello World";
        mixed_text_ = "Hello สวัสดี World";
        
        // Invalid/test cases
        empty_text_ = "";
        control_chars_text_ = "Test\x00\x01\x02";
        very_long_text_ = std::string(500, 'ก'); // 500 Thai characters
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
    
    std::unique_ptr<ThaiLanguageProcessor> processor_;
    std::string thai_text_simple_;
    std::string thai_text_complex_;
    std::string thai_text_with_tones_;
    std::string thai_text_long_;
    std::string thai_text_religious_;
    std::string thai_text_royal_;
    std::string english_text_;
    std::string mixed_text_;
    std::string empty_text_;
    std::string control_chars_text_;
    std::string very_long_text_;
};

// Test basic processor initialization
TEST_F(ThaiRenderingTest, ProcessorInitialization) {
    EXPECT_NE(processor_, nullptr);
}

// Test UTF-8 to DAB conversion
TEST_F(ThaiRenderingTest, UTF8ToDABConversion) {
    std::vector<uint8_t> dab_data;
    
    // Test simple Thai text conversion
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(thai_text_simple_, dab_data));
    EXPECT_GT(dab_data.size(), 0);
    EXPECT_EQ(dab_data[0], 0x0E); // Thai character set identifier
    
    // Test empty string
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(empty_text_, dab_data));
    EXPECT_EQ(dab_data.size(), 1); // Should only contain character set identifier
    
    // Test English text (should still work but with ASCII characters)
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(english_text_, dab_data));
    EXPECT_GT(dab_data.size(), 1);
    EXPECT_EQ(dab_data[0], 0x0E);
}

// Test DAB to UTF-8 conversion (round trip)
TEST_F(ThaiRenderingTest, DABToUTF8Conversion) {
    std::vector<uint8_t> dab_data;
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(thai_text_simple_, dab_data));
    
    std::string converted_back = processor_->ConvertDABToUTF8(dab_data);
    EXPECT_EQ(converted_back, thai_text_simple_);
    
    // Test with complex text
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(thai_text_complex_, dab_data));
    converted_back = processor_->ConvertDABToUTF8(dab_data);
    EXPECT_EQ(converted_back, thai_text_complex_);
}

// Test text layout analysis
TEST_F(ThaiRenderingTest, TextLayoutAnalysis) {
    auto layout = processor_->AnalyzeTextLayout(thai_text_simple_, 128, 4);
    
    EXPECT_EQ(layout.original_text, thai_text_simple_);
    EXPECT_GT(layout.dab_encoded_data.size(), 0);
    EXPECT_GT(layout.character_positions.size(), 0);
    EXPECT_GT(layout.character_widths.size(), 0);
    EXPECT_GT(layout.total_width_pixels, 0);
    EXPECT_GT(layout.total_height_pixels, 0);
    
    // Test with text that requires complex layout (tone marks)
    auto complex_layout = processor_->AnalyzeTextLayout(thai_text_with_tones_, 128, 4);
    EXPECT_TRUE(complex_layout.requires_complex_layout);
}

// Test DLS message formatting
TEST_F(ThaiRenderingTest, DLSMessageFormatting) {
    std::string output_text;
    
    // Test normal text formatting
    EXPECT_TRUE(processor_->FormatTextForDLS(thai_text_simple_, output_text, 128));
    EXPECT_LE(output_text.length(), 128);
    EXPECT_FALSE(output_text.empty());
    
    // Test long text truncation
    EXPECT_TRUE(processor_->FormatTextForDLS(very_long_text_, output_text, 50));
    EXPECT_LE(output_text.length(), 50);
    
    // Test empty text
    EXPECT_TRUE(processor_->FormatTextForDLS(empty_text_, output_text, 128));
    EXPECT_TRUE(output_text.empty());
}

// Test Thai number formatting
TEST_F(ThaiRenderingTest, ThaiNumberFormatting) {
    // Test Western digits
    std::string result = processor_->FormatNumber(123, ThaiNumberFormat::WESTERN_DIGITS);
    EXPECT_EQ(result, "123");
    
    // Test Thai digits
    result = processor_->FormatNumber(123, ThaiNumberFormat::THAI_DIGITS);
    EXPECT_NE(result, "123"); // Should be different (Thai digits)
    
    // Test Thai words (basic numbers)
    result = processor_->FormatNumber(1, ThaiNumberFormat::THAI_WORDS);
    EXPECT_EQ(result, "หนึ่ง");
    
    result = processor_->FormatNumber(10, ThaiNumberFormat::THAI_WORDS);
    EXPECT_EQ(result, "สิบ");
    
    // Test zero
    result = processor_->FormatNumber(0, ThaiNumberFormat::THAI_WORDS);
    EXPECT_EQ(result, "ศูนย์");
    
    // Test negative numbers
    result = processor_->FormatNumber(-5, ThaiNumberFormat::THAI_WORDS);
    EXPECT_TRUE(result.find("ลบ") != std::string::npos); // Should contain "ลบ" (minus)
}

// Test Buddhist calendar functionality
TEST_F(ThaiRenderingTest, BuddhistCalendar) {
    auto now = std::chrono::system_clock::now();
    auto buddhist_date = processor_->GetBuddhistDate(now);
    
    EXPECT_GT(buddhist_date.year_be, 2500); // Should be in Buddhist Era
    EXPECT_EQ(buddhist_date.year_be, buddhist_date.year_ce + 543);
    EXPECT_GE(buddhist_date.month, 1);
    EXPECT_LE(buddhist_date.month, 12);
    EXPECT_GE(buddhist_date.day, 1);
    EXPECT_LE(buddhist_date.day, 31);
    EXPECT_FALSE(buddhist_date.thai_month_name.empty());
    EXPECT_FALSE(buddhist_date.thai_day_name.empty());
}

// Test Thai month names
TEST_F(ThaiRenderingTest, ThaiMonthNames) {
    EXPECT_EQ(processor_->GetThaiMonthName(1), "มกราคม");
    EXPECT_EQ(processor_->GetThaiMonthName(12), "ธันวาคม");
    EXPECT_TRUE(processor_->GetThaiMonthName(0).empty()); // Invalid month
    EXPECT_TRUE(processor_->GetThaiMonthName(13).empty()); // Invalid month
}

// Test day names
TEST_F(ThaiRenderingTest, ThaiDayNames) {
    auto now = std::chrono::system_clock::now();
    std::string day_name = processor_->GetThaiDayName(now);
    EXPECT_FALSE(day_name.empty());
    EXPECT_TRUE(day_name.find("วัน") != std::string::npos); // Should contain "วัน" (day)
}

// Test cultural content validation
TEST_F(ThaiRenderingTest, CulturalContentValidation) {
    // Test normal text
    auto validation = processor_->ValidateContent(thai_text_simple_);
    EXPECT_TRUE(validation.is_appropriate);
    EXPECT_FALSE(validation.contains_religious_content);
    EXPECT_FALSE(validation.contains_royal_references);
    
    // Test religious content
    validation = processor_->ValidateContent(thai_text_religious_);
    EXPECT_TRUE(validation.contains_religious_content);
    EXPECT_FALSE(validation.suggestions.empty());
    
    // Test royal content
    validation = processor_->ValidateContent(thai_text_royal_);
    EXPECT_TRUE(validation.contains_royal_references);
    EXPECT_TRUE(validation.requires_special_formatting);
    EXPECT_FALSE(validation.suggestions.empty());
    
    // Test empty text
    validation = processor_->ValidateContent(empty_text_);
    EXPECT_TRUE(validation.is_appropriate);
    EXPECT_EQ(validation.cultural_sensitivity_score, 1.0);
}

// Test text width calculation
TEST_F(ThaiRenderingTest, TextWidthCalculation) {
    uint16_t width = processor_->CalculateTextWidth(thai_text_simple_);
    EXPECT_GT(width, 0);
    
    // Longer text should have greater width
    uint16_t longer_width = processor_->CalculateTextWidth(thai_text_complex_);
    EXPECT_GT(longer_width, width);
    
    // Empty text should have zero width
    uint16_t empty_width = processor_->CalculateTextWidth(empty_text_);
    EXPECT_EQ(empty_width, 0);
}

// Test text wrapping
TEST_F(ThaiRenderingTest, TextWrapping) {
    auto lines = processor_->WrapText(thai_text_long_, 100); // 100 pixel width limit
    EXPECT_GT(lines.size(), 1); // Should wrap into multiple lines
    
    // Each line should fit within the width limit
    for (const auto& line : lines) {
        uint16_t line_width = processor_->CalculateTextWidth(line);
        EXPECT_LE(line_width, 100);
    }
    
    // Test with very wide limit (should not wrap)
    lines = processor_->WrapText(thai_text_simple_, 1000);
    EXPECT_EQ(lines.size(), 1);
}

// Test ETSI compliance validation
TEST_F(ThaiRenderingTest, ETSIComplianceValidation) {
    std::vector<uint8_t> dab_data;
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(thai_text_simple_, dab_data));
    
    // Test ETSI compliance
    EXPECT_TRUE(processor_->ValidateETSICompliance(dab_data));
    
    // Test with invalid data
    std::vector<uint8_t> invalid_data = {0x01, 0x02, 0x03}; // Not Thai character set
    EXPECT_FALSE(processor_->ValidateETSICompliance(invalid_data));
}

// Test Thai text utilities
TEST_F(ThaiRenderingTest, ThaiTextUtilities) {
    // Test character counting
    size_t char_count = ThaiTextUtils::CountCharacters(thai_text_simple_);
    EXPECT_GT(char_count, 0);
    
    // Test word counting (approximate for Thai)
    size_t word_count = ThaiTextUtils::CountWords(thai_text_complex_);
    EXPECT_GT(word_count, 0);
    
    // Test text normalization
    std::string normalized = ThaiTextUtils::NormalizeText(thai_text_simple_ + "  \n\t  ");
    EXPECT_FALSE(normalized.empty());
    EXPECT_NE(normalized.find("สวัสดี"), std::string::npos);
    
    // Test valid Thai structure
    EXPECT_TRUE(ThaiTextUtils::HasValidThaiStructure(thai_text_simple_));
    EXPECT_TRUE(ThaiTextUtils::HasValidThaiStructure(english_text_)); // ASCII should be valid
    EXPECT_FALSE(ThaiTextUtils::HasValidThaiStructure(control_chars_text_));
}

// Test Buddhist calendar utilities
TEST_F(ThaiRenderingTest, BuddhistCalendarUtilities) {
    // Test CE to BE conversion
    EXPECT_EQ(BuddhistCalendar::CEtoBE(2024), 2567);
    EXPECT_EQ(BuddhistCalendar::BEtoCE(2567), 2024);
    
    // Test holy days
    auto holy_days = BuddhistCalendar::GetHolyDays(2567);
    EXPECT_GT(holy_days.size(), 0);
    
    // Test national holidays
    auto national_holidays = BuddhistCalendar::GetNationalHolidays(2567);
    EXPECT_GT(national_holidays.size(), 0);
    
    // Verify some known holidays exist
    bool found_new_year = false;
    for (const auto& holiday : national_holidays) {
        if (holiday.month == 1 && holiday.day == 1) {
            found_new_year = true;
            break;
        }
    }
    EXPECT_TRUE(found_new_year);
}

// Test cultural content analyzer
TEST_F(ThaiRenderingTest, CulturalContentAnalyzer) {
    CulturalContentAnalyzer analyzer;
    
    // Test formality level analysis
    double formality = analyzer.AnalyzeFormalityLevel(thai_text_simple_);
    EXPECT_GE(formality, 0.0);
    EXPECT_LE(formality, 1.0);
    
    // Test religious content analysis
    double religious_score = analyzer.AnalyzeReligiousContent(thai_text_religious_);
    EXPECT_GT(religious_score, 0.0); // Should detect religious content
    
    // Test royal content analysis
    double royal_score = analyzer.AnalyzeRoyalContent(thai_text_royal_);
    EXPECT_GT(royal_score, 0.0); // Should detect royal content
    
    // Test time-appropriate content
    auto now = std::chrono::system_clock::now();
    EXPECT_TRUE(analyzer.IsAppropriateForTime(thai_text_simple_, now));
}

// Test Thai DLS optimizer
TEST_F(ThaiRenderingTest, ThaiDLSOptimizer) {
    auto result = ThaiDLSOptimizer::OptimizeForDLS(thai_text_long_, 64);
    
    EXPECT_LE(result.optimized_length, 64);
    EXPECT_GT(result.compression_ratio, 0.0);
    EXPECT_LE(result.compression_ratio, 1.0);
    EXPECT_FALSE(result.optimized_text.empty());
    EXPECT_GT(result.applied_optimizations.size(), 0);
    
    // Test with already short text
    result = ThaiDLSOptimizer::OptimizeForDLS(thai_text_simple_, 128);
    EXPECT_EQ(result.optimized_text, thai_text_simple_); // Should remain unchanged
    EXPECT_EQ(result.compression_ratio, 1.0);
}

// Test font metrics
TEST_F(ThaiRenderingTest, FontMetrics) {
    auto metrics = processor_->GetFontMetrics();
    
    EXPECT_GT(metrics.line_height, 0);
    EXPECT_GT(metrics.baseline, 0);
    EXPECT_GE(metrics.ascent, 0);
    EXPECT_GE(metrics.descent, 0);
    EXPECT_FALSE(metrics.character_widths.empty());
    
    // Test setting custom font metrics
    auto custom_metrics = metrics;
    custom_metrics.line_height = 20;
    processor_->SetFontMetrics(custom_metrics);
    
    auto updated_metrics = processor_->GetFontMetrics();
    EXPECT_EQ(updated_metrics.line_height, 20);
}

// Test error handling
TEST_F(ThaiRenderingTest, ErrorHandling) {
    std::vector<uint8_t> dab_data;
    
    // Test with control characters
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(control_chars_text_, dab_data));
    // Should handle gracefully (remove or replace control chars)
    
    // Test with very long text
    std::string output;
    EXPECT_TRUE(processor_->FormatTextForDLS(very_long_text_, output, 128));
    EXPECT_LE(output.length(), 128);
    
    // Test invalid date formatting
    auto past = std::chrono::system_clock::now() - std::chrono::hours(24 * 365 * 100); // 100 years ago
    std::string date_str = processor_->FormatDate(past, true);
    EXPECT_FALSE(date_str.empty()); // Should still work
}

// Test performance with large texts
TEST_F(ThaiRenderingTest, PerformanceTest) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Process many texts
    for (int i = 0; i < 100; ++i) {
        std::vector<uint8_t> dab_data;
        processor_->ConvertUTF8ToDAB(thai_text_complex_, dab_data);
        
        auto layout = processor_->AnalyzeTextLayout(thai_text_complex_, 128, 4);
        
        auto validation = processor_->ValidateContent(thai_text_complex_);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete within reasonable time (less than 1 second for 100 iterations)
    EXPECT_LT(duration.count(), 1000);
}

// Test thread safety
TEST_F(ThaiRenderingTest, ThreadSafety) {
    std::atomic<int> successful_conversions{0};
    std::atomic<bool> test_running{true};
    
    // Start multiple threads doing conversions
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&]() {
            while (test_running) {
                std::vector<uint8_t> dab_data;
                if (processor_->ConvertUTF8ToDAB(thai_text_simple_, dab_data)) {
                    successful_conversions++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Let threads run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    test_running = false;
    
    // Wait for threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Should have successful conversions
    EXPECT_GT(successful_conversions.load(), 0);
}

// Test memory usage with large texts
TEST_F(ThaiRenderingTest, MemoryUsage) {
    // Process very large text multiple times
    std::string huge_text = std::string(10000, 'ก'); // 10,000 Thai characters
    
    for (int i = 0; i < 10; ++i) {
        std::vector<uint8_t> dab_data;
        EXPECT_TRUE(processor_->ConvertUTF8ToDAB(huge_text, dab_data));
        
        auto layout = processor_->AnalyzeTextLayout(huge_text, 128, 10);
        EXPECT_GT(layout.line_breaks.size(), 1);
        
        // Memory should be released after each iteration
    }
    
    // Test should complete without memory issues
    SUCCEED();
}

// Test mixed language content
TEST_F(ThaiRenderingTest, MixedLanguageContent) {
    std::vector<uint8_t> dab_data;
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(mixed_text_, dab_data));
    
    // Should handle mixed content
    std::string converted_back = processor_->ConvertDABToUTF8(dab_data);
    EXPECT_FALSE(converted_back.empty());
    
    auto validation = processor_->ValidateContent(mixed_text_);
    EXPECT_TRUE(validation.is_appropriate);
}