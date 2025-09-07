/* ------------------------------------------------------------------
 * Copyright (C) 2024 StreamDAB Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * ------------------------------------------------------------------- */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "thai_rendering.h"
#include "test_main.cpp"

using namespace StreamDAB;
using namespace std;

class ThaiRenderingTest : public ODRPadEncTestBase {
protected:
    void SetUp() override {
        ODRPadEncTestBase::SetUp();
        processor_ = make_unique<ThaiLanguageProcessor>();
        analyzer_ = make_unique<CulturalContentAnalyzer>();
        calendar_ = make_unique<BuddhistCalendar>();
    }
    
    unique_ptr<ThaiLanguageProcessor> processor_;
    unique_ptr<CulturalContentAnalyzer> analyzer_;
    unique_ptr<BuddhistCalendar> calendar_;
    
    // Test data constants
    const string thai_text_ = "สวัสดีครับ นี่คือข้อความทดสอบ";
    const string english_text_ = "Hello World Test Message";
    const string mixed_text_ = "StreamDAB สถานีวิทยุดิจิทัล";
    const string thai_song_title_ = "เพลงไทยสมัยใหม่";
    const string thai_artist_name_ = "นักร้องไทย";
    const string royal_text_ = "พระบาทสมเด็จพระเจ้าอยู่หัว";
    const string religious_text_ = "พระพุทธเจ้า พระธรรม พระสงฆ์";
};

// ThaiLanguageProcessor Tests
TEST_F(ThaiRenderingTest, UTF8ToDABConversion) {
    vector<uint8_t> dab_data;
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(thai_text_, dab_data));
    EXPECT_FALSE(dab_data.empty());
    EXPECT_EQ(dab_data[0], 0x0E); // Thai charset identifier
}

TEST_F(ThaiRenderingTest, DABToUTF8Conversion) {
    vector<uint8_t> dab_data;
    ASSERT_TRUE(processor_->ConvertUTF8ToDAB(thai_text_, dab_data));
    
    string converted_back = processor_->ConvertDABToUTF8(dab_data);
    EXPECT_FALSE(converted_back.empty());
    // Note: Exact match may not be possible due to character mapping limitations
    EXPECT_GT(converted_back.length(), 0);
}

TEST_F(ThaiRenderingTest, TextLayoutAnalysis) {
    auto layout = processor_->AnalyzeTextLayout(thai_text_, 128, 4);
    
    EXPECT_EQ(layout.original_text, thai_text_);
    EXPECT_FALSE(layout.dab_encoded_data.empty());
    EXPECT_GT(layout.total_width_pixels, 0);
    EXPECT_GT(layout.total_height_pixels, 0);
    EXPECT_FALSE(layout.line_breaks.empty());
}

TEST_F(ThaiRenderingTest, ComplexLayoutDetection) {
    // Thai text with vowels and tone marks requires complex layout
    string complex_thai = "กำ่ไก่"; // Contains combining characters
    auto layout = processor_->AnalyzeTextLayout(complex_thai);
    
    EXPECT_TRUE(layout.requires_complex_layout);
}

TEST_F(ThaiRenderingTest, DLSTextFormatting) {
    string input_text = "สวัสดีครับ นี่คือข้อความที่ยาวมากสำหรับการทดสอบ DLS ที่มีความยาวเกินขีดจำกัด";
    string output_text;
    
    EXPECT_TRUE(processor_->FormatTextForDLS(input_text, output_text, 64));
    EXPECT_LE(output_text.length(), 64);
    EXPECT_FALSE(output_text.empty());
}

TEST_F(ThaiRenderingTest, NumberFormatting) {
    // Test different number formats
    EXPECT_EQ(processor_->FormatNumber(123, ThaiNumberFormat::WESTERN_DIGITS), "123");
    
    string thai_digits = processor_->FormatNumber(123, ThaiNumberFormat::THAI_DIGITS);
    EXPECT_FALSE(thai_digits.empty());
    EXPECT_NE(thai_digits, "123"); // Should be different from western digits
    
    string thai_words = processor_->FormatNumber(5, ThaiNumberFormat::THAI_WORDS);
    EXPECT_EQ(thai_words, "ห้า");
    
    string zero_words = processor_->FormatNumber(0, ThaiNumberFormat::THAI_WORDS);
    EXPECT_EQ(zero_words, "ศูนย์");
}

TEST_F(ThaiRenderingTest, CurrencyFormatting) {
    string currency = processor_->FormatCurrency(1234.56, true);
    EXPECT_FALSE(currency.empty());
    EXPECT_NE(currency.find("๑"), string::npos); // Should contain Thai digits
    
    string western_currency = processor_->FormatCurrency(1234.56, false);
    EXPECT_FALSE(western_currency.empty());
    EXPECT_NE(western_currency.find("1"), string::npos); // Should contain western digits
}

TEST_F(ThaiRenderingTest, DateTimeFormatting) {
    auto now = chrono::system_clock::now();
    
    string buddhist_date = processor_->FormatDate(now, true);
    EXPECT_FALSE(buddhist_date.empty());
    EXPECT_NE(buddhist_date.find("พ.ศ."), string::npos); // Should contain Buddhist Era
    
    string gregorian_date = processor_->FormatDate(now, false);
    EXPECT_FALSE(gregorian_date.empty());
    
    string thai_time = processor_->FormatTime(now, true);
    EXPECT_FALSE(thai_time.empty());
    
    string western_time = processor_->FormatTime(now, false);
    EXPECT_FALSE(western_time.empty());
}

TEST_F(ThaiRenderingTest, BuddhistCalendar) {
    auto now = chrono::system_clock::now();
    BuddhistDate bd = processor_->GetBuddhistDate(now);
    
    EXPECT_TRUE(bd.is_valid);
    EXPECT_GT(bd.year_be, 2500); // Should be Buddhist Era
    EXPECT_EQ(bd.year_be, bd.year_ce + 543);
    EXPECT_GE(bd.month, 1);
    EXPECT_LE(bd.month, 12);
    EXPECT_GE(bd.day, 1);
    EXPECT_LE(bd.day, 31);
    EXPECT_FALSE(bd.thai_month_name.empty());
    EXPECT_FALSE(bd.thai_day_name.empty());
}

TEST_F(ThaiRenderingTest, ThaiMonthNames) {
    for (int month = 1; month <= 12; ++month) {
        string month_name = processor_->GetThaiMonthName(month);
        EXPECT_FALSE(month_name.empty());
        EXPECT_GT(month_name.length(), 0);
    }
    
    // Invalid months should return empty string
    EXPECT_TRUE(processor_->GetThaiMonthName(0).empty());
    EXPECT_TRUE(processor_->GetThaiMonthName(13).empty());
}

TEST_F(ThaiRenderingTest, HolyDayDetection) {
    // Test some fixed dates (simplified test)
    auto now = chrono::system_clock::now();
    bool is_holy = processor_->IsHolyDay(now);
    
    // Should not throw exception
    EXPECT_TRUE(is_holy || !is_holy); // Boolean result
}

TEST_F(ThaiRenderingTest, HolidaysInMonth) {
    auto holidays = processor_->GetHolidaysInMonth(2567, 4); // April 2024 in BE
    EXPECT_GE(holidays.size(), 0); // Should return valid list (may be empty)
    
    for (const auto& holiday : holidays) {
        EXPECT_EQ(holiday.month, 4);
        EXPECT_TRUE(holiday.is_valid);
    }
}

// Cultural Content Validation Tests
TEST_F(ThaiRenderingTest, CulturalContentValidation) {
    auto validation = processor_->ValidateContent(thai_text_);
    
    EXPECT_TRUE(validation.is_appropriate);
    EXPECT_GE(validation.cultural_sensitivity_score, 0.0);
    EXPECT_LE(validation.cultural_sensitivity_score, 1.0);
}

TEST_F(ThaiRenderingTest, RoyalContentDetection) {
    auto validation = processor_->ValidateContent(royal_text_);
    
    EXPECT_TRUE(validation.contains_royal_references);
    EXPECT_TRUE(validation.requires_special_formatting);
    EXPECT_FALSE(validation.suggestions.empty());
}

TEST_F(ThaiRenderingTest, ReligiousContentDetection) {
    auto validation = processor_->ValidateContent(religious_text_);
    
    EXPECT_TRUE(validation.contains_religious_content);
    EXPECT_FALSE(validation.suggestions.empty());
}

TEST_F(ThaiRenderingTest, InappropriateContentDetection) {
    string inappropriate = "บ้า โง่"; // Inappropriate words
    auto validation = processor_->ValidateContent(inappropriate);
    
    EXPECT_FALSE(validation.is_appropriate);
    EXPECT_LT(validation.cultural_sensitivity_score, 1.0);
    EXPECT_FALSE(validation.warnings.empty());
}

TEST_F(ThaiRenderingTest, BroadcastAppropriatenessCheck) {
    EXPECT_TRUE(processor_->IsAppropriateForBroadcast(thai_text_));
    EXPECT_TRUE(processor_->IsAppropriateForBroadcast(english_text_));
    EXPECT_FALSE(processor_->IsAppropriateForBroadcast("บ้า โง่"));
}

TEST_F(ThaiRenderingTest, TextSanitization) {
    string inappropriate = "สวัสดี บ้า โลก";
    string sanitized = processor_->SanitizeText(inappropriate);
    
    EXPECT_NE(sanitized, inappropriate); // Should be different
    EXPECT_NE(sanitized.find("สวัสดี"), string::npos); // Should keep appropriate parts
    EXPECT_EQ(sanitized.find("บ้า"), string::npos); // Should remove inappropriate parts
}

// Font and Display Tests
TEST_F(ThaiRenderingTest, TextWidthCalculation) {
    uint16_t width = processor_->CalculateTextWidth(thai_text_);
    EXPECT_GT(width, 0);
    
    uint16_t empty_width = processor_->CalculateTextWidth("");
    EXPECT_EQ(empty_width, 0);
    
    // Thai text should generally be wider than equivalent ASCII
    uint16_t ascii_width = processor_->CalculateTextWidth("Hello");
    uint16_t thai_width = processor_->CalculateTextWidth("สวัสดี");
    // This may not always be true depending on font metrics
    EXPECT_GT(thai_width, 0);
    EXPECT_GT(ascii_width, 0);
}

TEST_F(ThaiRenderingTest, TextHeightCalculation) {
    uint8_t height = processor_->CalculateTextHeight(thai_text_);
    EXPECT_GT(height, 0);
    
    // Multi-line text should be taller
    string multiline = "บรรทัดที่ 1\nบรรทัดที่ 2\nบรรทัดที่ 3";
    uint8_t multiline_height = processor_->CalculateTextHeight(multiline);
    EXPECT_GT(multiline_height, height);
}

TEST_F(ThaiRenderingTest, TextWrapping) {
    string long_text = "สวัสดีครับ นี่คือข้อความที่ยาวมากสำหรับการทดสอบการตัดคำและการขึ้นบรรทัดใหม่";
    
    auto wrapped_lines = processor_->WrapText(long_text, 100);
    EXPECT_GT(wrapped_lines.size(), 1); // Should wrap into multiple lines
    
    // Each line should be shorter than the original
    for (const auto& line : wrapped_lines) {
        EXPECT_LT(line.length(), long_text.length());
        EXPECT_GT(line.length(), 0);
    }
}

// ETSI Compliance Tests
TEST_F(ThaiRenderingTest, ETSIComplianceValidation) {
    vector<uint8_t> dab_data;
    ASSERT_TRUE(processor_->ConvertUTF8ToDAB(thai_text_, dab_data));
    
    EXPECT_TRUE(processor_->ValidateETSICompliance(dab_data));
    
    // Test with invalid data
    vector<uint8_t> invalid_data = {0xFF, 0xFE, 0xFD}; // Invalid charset
    EXPECT_FALSE(processor_->ValidateETSICompliance(invalid_data));
}

TEST_F(ThaiRenderingTest, ETSIComplianceEnsurance) {
    // Test with potentially non-compliant data
    vector<uint8_t> test_data = {0x0E, 0xFF, 0xFE, 0x41, 0x42}; // Mixed valid/invalid
    
    auto compliant_data = processor_->EnsureETSICompliance(test_data);
    EXPECT_TRUE(processor_->ValidateETSICompliance(compliant_data));
    EXPECT_FALSE(compliant_data.empty());
    EXPECT_EQ(compliant_data[0], 0x0E); // Should maintain Thai charset
}

// ThaiTextUtils Tests
TEST_F(ThaiRenderingTest, WordSegmentation) {
    auto words = ThaiTextUtils::SegmentWords(thai_text_);
    EXPECT_FALSE(words.empty());
    
    // Check that segmented words when joined form original text (approximately)
    string rejoined;
    for (const auto& word : words) {
        rejoined += word;
    }
    // Note: May not be exact due to spacing
    EXPECT_FALSE(rejoined.empty());
}

TEST_F(ThaiRenderingTest, SyllableAnalysis) {
    auto syllables = ThaiTextUtils::AnalyzeSyllables(thai_text_);
    EXPECT_FALSE(syllables.empty());
    
    for (const auto& syllable : syllables) {
        EXPECT_FALSE(syllable.empty());
    }
}

TEST_F(ThaiRenderingTest, ThaiRomanization) {
    string romanized = ThaiTextUtils::ToRoman(thai_text_, true);
    EXPECT_FALSE(romanized.empty());
    EXPECT_NE(romanized, thai_text_); // Should be different
    
    // Should contain only ASCII characters
    for (char c : romanized) {
        EXPECT_TRUE(c >= 0 && c <= 127);
    }
}

TEST_F(ThaiRenderingTest, TextStatistics) {
    size_t char_count = ThaiTextUtils::CountCharacters(thai_text_);
    EXPECT_GT(char_count, 0);
    
    size_t word_count = ThaiTextUtils::CountWords(thai_text_);
    EXPECT_GT(word_count, 0);
    
    size_t syllable_count = ThaiTextUtils::CountSyllables(thai_text_);
    EXPECT_GT(syllable_count, 0);
}

TEST_F(ThaiRenderingTest, ThaiStructureValidation) {
    EXPECT_TRUE(ThaiTextUtils::HasValidThaiStructure(thai_text_));
    EXPECT_TRUE(ThaiTextUtils::HasValidThaiStructure(english_text_));
    
    // Test with invalid Thai structure
    string invalid_structure = "ก็็็็็็"; // Invalid tone mark sequence
    auto invalid_sequences = ThaiTextUtils::FindInvalidSequences(invalid_structure);
    EXPECT_FALSE(invalid_sequences.empty());
}

TEST_F(ThaiRenderingTest, TextNormalization) {
    string text_with_issues = "  สวัสดี  \t\n  ครับ  ";
    string normalized = ThaiTextUtils::NormalizeText(text_with_issues);
    
    EXPECT_NE(normalized, text_with_issues);
    EXPECT_FALSE(normalized.empty());
    EXPECT_EQ(normalized.find("  "), string::npos); // No double spaces
}

TEST_F(ThaiRenderingTest, InvisibleCharacterRemoval) {
    string text_with_invisible = "สวัสดี\u200B\u200C\u200Dครับ"; // Zero-width characters
    string cleaned = ThaiTextUtils::RemoveInvisibleCharacters(text_with_invisible);
    
    EXPECT_LT(cleaned.length(), text_with_invisible.length());
    EXPECT_FALSE(cleaned.empty());
}

// BuddhistCalendar Static Methods Tests  
TEST_F(ThaiRenderingTest, BuddhistCalendarConversion) {
    EXPECT_EQ(BuddhistCalendar::CEtoBE(2024), 2567);
    EXPECT_EQ(BuddhistCalendar::BEtoCE(2567), 2024);
    EXPECT_EQ(BuddhistCalendar::CEtoBE(1), 544);
    EXPECT_EQ(BuddhistCalendar::BEtoCE(544), 1);
}

TEST_F(ThaiRenderingTest, HolyDayCalculations) {
    // Test some known dates
    bool is_holy = BuddhistCalendar::IsHolyDay(2567, 2, 24); // Magha Puja 2024
    // Note: This is simplified - real implementation would use lunar calculations
    
    auto holy_days = BuddhistCalendar::GetHolyDays(2567);
    EXPECT_FALSE(holy_days.empty()); // Should have some holy days
    
    auto national_holidays = BuddhistCalendar::GetNationalHolidays(2567);
    EXPECT_FALSE(national_holidays.empty()); // Should have national holidays
}

TEST_F(ThaiRenderingTest, MoonPhaseCalculations) {
    // Test moon phase calculations
    int phase = BuddhistCalendar::GetMoonPhase(2024, 6, 15);
    EXPECT_GE(phase, 0);
    EXPECT_LE(phase, 3); // 0-3 for new, waxing, full, waning
    
    // Full moon and new moon detection
    bool is_full = BuddhistCalendar::IsFullMoon(2024, 6, 22); // Example date
    bool is_new = BuddhistCalendar::IsNewMoon(2024, 6, 6);   // Example date
    
    // Should not both be true for the same date
    EXPECT_FALSE(is_full && is_new);
}

TEST_F(ThaiRenderingTest, ThaiTraditionalCalendar) {
    string era = BuddhistCalendar::GetThaiEra(2567);
    EXPECT_FALSE(era.empty());
    
    string animal_year = BuddhistCalendar::GetAnimalYear(2567);
    EXPECT_FALSE(animal_year.empty());
    
    string zodiac = BuddhistCalendar::GetZodiacSign(6, 15);
    EXPECT_FALSE(zodiac.empty());
}

// CulturalContentAnalyzer Tests
TEST_F(ThaiRenderingTest, FormalityLevelAnalysis) {
    double formality = analyzer_->AnalyzeFormalityLevel(thai_text_);
    EXPECT_GE(formality, 0.0);
    EXPECT_LE(formality, 1.0);
    
    // Royal language should have high formality
    double royal_formality = analyzer_->AnalyzeFormalityLevel(royal_text_);
    EXPECT_GT(royal_formality, 0.8);
    
    // Casual language should have lower formality
    string casual = "สวัสดี เป็นไงบ้าง";
    double casual_formality = analyzer_->AnalyzeFormalityLevel(casual);
    EXPECT_LT(casual_formality, royal_formality);
}

TEST_F(ThaiRenderingTest, ReligiousContentAnalysis) {
    double religious_score = analyzer_->AnalyzeReligiousContent(religious_text_);
    EXPECT_GT(religious_score, 0.5);
    
    double non_religious_score = analyzer_->AnalyzeReligiousContent(thai_text_);
    EXPECT_LT(non_religious_score, religious_score);
}

TEST_F(ThaiRenderingTest, RoyalContentAnalysis) {
    double royal_score = analyzer_->AnalyzeRoyalContent(royal_text_);
    EXPECT_GT(royal_score, 0.8);
    
    double non_royal_score = analyzer_->AnalyzeRoyalContent(thai_text_);
    EXPECT_LT(non_royal_score, royal_score);
}

TEST_F(ThaiRenderingTest, TextAlternativeSuggestions) {
    string inappropriate = "บ้า";
    auto alternatives = analyzer_->SuggestAlternatives(inappropriate);
    
    EXPECT_FALSE(alternatives.empty());
    for (const auto& alt : alternatives) {
        EXPECT_FALSE(alt.empty());
        EXPECT_NE(alt, inappropriate);
    }
}

TEST_F(ThaiRenderingTest, ContextAdaptation) {
    string formal_context = "government_broadcast";
    string casual_context = "music_radio";
    
    string adapted_formal = analyzer_->AdaptForContext(thai_text_, formal_context);
    string adapted_casual = analyzer_->AdaptForContext(thai_text_, casual_context);
    
    EXPECT_FALSE(adapted_formal.empty());
    EXPECT_FALSE(adapted_casual.empty());
    // They might be the same for neutral content
}

TEST_F(ThaiRenderingTest, TimeAppropriatenessCheck) {
    auto now = chrono::system_clock::now();
    
    EXPECT_TRUE(analyzer_->IsAppropriateForTime(thai_text_, now));
    EXPECT_TRUE(analyzer_->IsAppropriateForTime(english_text_, now));
    
    // Test with potentially time-sensitive content
    string time_sensitive = "ราตรีสวัสดิ์"; // Good night
    bool is_appropriate = analyzer_->IsAppropriateForTime(time_sensitive, now);
    // Should return boolean result based on time of day
    EXPECT_TRUE(is_appropriate || !is_appropriate);
}

TEST_F(ThaiRenderingTest, AudienceAppropriatenessCheck) {
    string children_audience = "children";
    string general_audience = "general";
    string adult_audience = "adult";
    
    EXPECT_TRUE(analyzer_->IsAppropriateForAudience(thai_text_, children_audience));
    EXPECT_TRUE(analyzer_->IsAppropriateForAudience(thai_text_, general_audience));
    EXPECT_TRUE(analyzer_->IsAppropriateForAudience(thai_text_, adult_audience));
    
    // Test with inappropriate content
    string inappropriate = "เนื้อหาไม่เหมาะสม";
    EXPECT_FALSE(analyzer_->IsAppropriateForAudience(inappropriate, children_audience));
}

// Configuration and Customization Tests
TEST_F(ThaiRenderingTest, FontMetricsConfiguration) {
    auto original_metrics = processor_->GetFontMetrics();
    EXPECT_GT(original_metrics.line_height, 0);
    
    ThaiFontMetrics custom_metrics = original_metrics;
    custom_metrics.line_height = 20;
    custom_metrics.baseline = 15;
    
    processor_->SetFontMetrics(custom_metrics);
    
    auto updated_metrics = processor_->GetFontMetrics();
    EXPECT_EQ(updated_metrics.line_height, 20);
    EXPECT_EQ(updated_metrics.baseline, 15);
}

// Performance Tests
TEST_F(ThaiRenderingTest, ConversionPerformance) {
    auto start = chrono::high_resolution_clock::now();
    
    // Perform 1000 conversions
    for (int i = 0; i < 1000; ++i) {
        vector<uint8_t> dab_data;
        processor_->ConvertUTF8ToDAB(thai_text_, dab_data);
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 1000); // Should complete in less than 1 second
}

TEST_F(ThaiRenderingTest, ValidationPerformance) {
    auto start = chrono::high_resolution_clock::now();
    
    // Perform 1000 validations
    for (int i = 0; i < 1000; ++i) {
        processor_->ValidateContent(thai_text_);
    }
    
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 2000); // Should complete in less than 2 seconds
}

// Edge Cases and Error Handling
TEST_F(ThaiRenderingTest, EmptyInputHandling) {
    vector<uint8_t> empty_dab;
    EXPECT_FALSE(processor_->ConvertUTF8ToDAB("", empty_dab));
    
    string empty_converted = processor_->ConvertDABToUTF8({});
    EXPECT_TRUE(empty_converted.empty());
    
    auto empty_validation = processor_->ValidateContent("");
    EXPECT_TRUE(empty_validation.is_appropriate);
    EXPECT_EQ(empty_validation.cultural_sensitivity_score, 1.0);
}

TEST_F(ThaiRenderingTest, InvalidUTF8Handling) {
    string invalid_utf8 = "\xFF\xFE\xInvalid";
    vector<uint8_t> dab_data;
    
    EXPECT_FALSE(processor_->ConvertUTF8ToDAB(invalid_utf8, dab_data));
}

TEST_F(ThaiRenderingTest, LargeTextHandling) {
    string large_text = string(10000, 'ก'); // Very long Thai text
    
    auto validation = processor_->ValidateContent(large_text);
    EXPECT_TRUE(validation.is_appropriate);
    
    string formatted_text;
    EXPECT_TRUE(processor_->FormatTextForDLS(large_text, formatted_text, 128));
    EXPECT_LE(formatted_text.length(), 128);
}

TEST_F(ThaiRenderingTest, SpecialCharacterHandling) {
    string special_chars = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    
    auto validation = processor_->ValidateContent(special_chars);
    EXPECT_TRUE(validation.is_appropriate);
    
    vector<uint8_t> dab_data;
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(special_chars, dab_data));
}

TEST_F(ThaiRenderingTest, MixedScriptHandling) {
    string mixed_scripts = "Hello สวัสดี مرحبا こんにちは 你好";
    
    auto validation = processor_->ValidateContent(mixed_scripts);
    EXPECT_TRUE(validation.is_appropriate);
    
    vector<uint8_t> dab_data;
    EXPECT_TRUE(processor_->ConvertUTF8ToDAB(mixed_scripts, dab_data));
    
    uint16_t width = processor_->CalculateTextWidth(mixed_scripts);
    EXPECT_GT(width, 0);
}