/*
    Thai Language Rendering and Cultural Features Implementation
    Copyright (C) 2024 StreamDAB Project
*/

#include "thai_rendering.h"
#include <algorithm>
#include <regex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <iostream>

namespace StreamDAB {

ThaiLanguageProcessor::ThaiLanguageProcessor() {
    InitializeUTF8ToDABMapping();
    InitializeHolidayCalendar();
    InitializeCulturalData();
    InitializeFontMetrics();
}

void ThaiLanguageProcessor::InitializeUTF8ToDABMapping() {
    // ETSI TS 101 756 Thai character set (0x0E) mapping
    // Thai consonants (0x0E01 - 0x0E2E)
    for (uint16_t i = 0x0E01; i <= 0x0E2E; ++i) {
        utf8_to_dab_map_[i] = static_cast<uint8_t>(i - 0x0E01 + 0x01);
    }
    
    // Thai vowels (0x0E30 - 0x0E4F)
    for (uint16_t i = 0x0E30; i <= 0x0E4F; ++i) {
        utf8_to_dab_map_[i] = static_cast<uint8_t>(i - 0x0E30 + 0x30);
    }
    
    // Thai digits (0x0E50 - 0x0E59)
    for (uint16_t i = 0x0E50; i <= 0x0E59; ++i) {
        utf8_to_dab_map_[i] = static_cast<uint8_t>(i - 0x0E50 + 0x50);
    }
    
    // Thai symbols
    utf8_to_dab_map_[0x0E5A] = 0x5A; // Thai character angkhankhu
    utf8_to_dab_map_[0x0E5B] = 0x5B; // Thai character khomut
    
    // Special Thai characters
    utf8_to_dab_map_[0x0E4F] = 0x4F; // Thai character fongman
    utf8_to_dab_map_[0x0E46] = 0x46; // Thai character maiyamok
    utf8_to_dab_map_[0x0E47] = 0x47; // Thai character maitaikhu
    
    // Thai tone marks (0x0E48 - 0x0E4B)
    utf8_to_dab_map_[0x0E48] = 0x48; // Thai character mai ek
    utf8_to_dab_map_[0x0E49] = 0x49; // Thai character mai tho
    utf8_to_dab_map_[0x0E4A] = 0x4A; // Thai character mai tri
    utf8_to_dab_map_[0x0E4B] = 0x4B; // Thai character mai chattawa
    
    // Above vowels (0x0E34 - 0x0E3A)
    for (uint16_t i = 0x0E34; i <= 0x0E3A; ++i) {
        utf8_to_dab_map_[i] = static_cast<uint8_t>(i - 0x0E34 + 0x34);
    }
    
    // Below vowels (0x0E38 - 0x0E3A)
    utf8_to_dab_map_[0x0E38] = 0x38; // Thai character sara u
    utf8_to_dab_map_[0x0E39] = 0x39; // Thai character sara uu
    utf8_to_dab_map_[0x0E3A] = 0x3A; // Thai character phinthu
}

void ThaiLanguageProcessor::InitializeHolidayCalendar() {
    // Buddhist holy days and Thai national holidays
    // Note: This is a simplified calendar - full implementation would use astronomical calculations
    
    // Fixed national holidays
    holiday_calendar_["วันปีใหม่"] = {2567, 2024, 1, 1, "มกราคม", "วันจันทร์", false, true, "วันปีใหม่", "New Year's Day"};
    holiday_calendar_["วันมาฆบูชา"] = {2567, 2024, 2, 24, "กุมภาพันธ์", "วันเสาร์", true, true, "วันมาฆบูชา", "Magha Puja Day"};
    holiday_calendar_["วันจักรี"] = {2567, 2024, 4, 6, "เมษายน", "วันเสาร์", false, true, "วันจักรี", "Chakri Day"};
    holiday_calendar_["วันสงกรานต์"] = {2567, 2024, 4, 13, "เมษายน", "วันเสาร์", false, true, "วันสงกรานต์", "Songkran Festival"};
    holiday_calendar_["วันแรงงาน"] = {2567, 2024, 5, 1, "พฤษภาคม", "วันพุธ", false, true, "วันแรงงานแห่งชาติ", "Labor Day"};
    holiday_calendar_["วันฉัตรมงคล"] = {2567, 2024, 5, 4, "พฤษภาคม", "วันเสาร์", false, true, "วันฉัตรมงคล", "Coronation Day"};
    holiday_calendar_["วันวิสาขบูชา"] = {2567, 2024, 5, 22, "พฤษภาคม", "วันพุธ", true, true, "วันวิสาขบูชา", "Vesak Day"};
    holiday_calendar_["วันเฉลิมพระชนมพรรษา"] = {2567, 2024, 7, 28, "กรกฎาคม", "วันอาทิตย์", false, true, "วันเฉลิมพระชนมพรรษาพระบาทสมเด็จพระเจ้าอยู่หัว", "HM the King's Birthday"};
    holiday_calendar_["วันแม่"] = {2567, 2024, 8, 12, "สิงหาคม", "วันจันทร์", false, true, "วันแม่แห่งชาติ", "Mother's Day"};
    holiday_calendar_["วันปิยมหาราช"] = {2567, 2024, 10, 23, "ตุลาคม", "วันพุธ", false, true, "วันปิยมหาราช", "Chulalongkorn Day"};
    holiday_calendar_["วันพ่อ"] = {2567, 2024, 12, 5, "ธันวาคม", "วันพฤหัสบดี", false, true, "วันพ่อแห่งชาติ", "Father's Day"};
    holiday_calendar_["วันรัฐธรรมนูญ"] = {2567, 2024, 12, 10, "ธันวาคม", "วันอังคาร", false, true, "วันรัฐธรรมนูญ", "Constitution Day"};
}

void ThaiLanguageProcessor::InitializeCulturalData() {
    // Royal vocabulary requiring special respect
    royal_terms_ = {
        "พระบาทสมเด็จพระเจ้าอยู่หัว", "สมเด็จพระนางเจ้า", "พระองค์", "พระราชา", 
        "พระราชินี", "เจ้าฟ้า", "พระเจ้าหลานเธอ", "หม่อมเจ้า", "หม่อมราชวงศ์",
        "พระบาทสมเด็จพระปรมินทรมหาภูมิพลอดุลยเดช", "สมเด็จพระนางเจ้าสิริกิติ์",
        "พระบาทสมเด็จพระวชิราคลาวเรศ ราชกิจ", "สมเด็จพระนางเจ้าสุทิดา"
    };
    
    // Religious terms requiring respectful treatment
    religious_terms_ = {
        "พระพุทธเจ้า", "พระธรรม", "พระสงฆ์", "วัด", "พระ", "หลวงพ่อ", "หลวงปู่",
        "พระอริยสงฆ์", "พุทธศาสนา", "ธรรม", "วินัย", "สมาธิ", "วิปัสสนา", "นิพพาน",
        "บุญ", "กุศล", "อกุศล", "กรรม", "วิบาก", "บาป", "ปุณณะ", "ทาน", "ศีล"
    };
    
    // Inappropriate words for broadcasting
    inappropriate_words_ = {
        // This would contain actual inappropriate words in a real implementation
        // For demo purposes, keeping it minimal
        "เฮ้ย", "ชิบหาย", "บ้า", "โง่", "งี่เง่า"
    };
}

void ThaiLanguageProcessor::InitializeFontMetrics() {
    // Thai font metrics optimized for DAB displays
    font_metrics_.line_height = 16;
    font_metrics_.baseline = 12;
    font_metrics_.ascent = 4;
    font_metrics_.descent = 4;
    
    // Character width mapping (simplified)
    // Thai consonants
    for (uint16_t i = 0x0E01; i <= 0x0E2E; ++i) {
        font_metrics_.character_widths[i] = 8; // Most Thai consonants
    }
    
    // Wider characters
    font_metrics_.character_widths[0x0E27] = 10; // ว
    font_metrics_.character_widths[0x0E21] = 10; // ม
    font_metrics_.character_widths[0x0E2D] = 10; // อ
    
    // Narrower characters
    font_metrics_.character_widths[0x0E34] = 0;  // ิ (combining)
    font_metrics_.character_widths[0x0E35] = 0;  // ี (combining)
    font_metrics_.character_widths[0x0E36] = 0;  // ึ (combining)
    font_metrics_.character_widths[0x0E37] = 0;  // ื (combining)
    font_metrics_.character_widths[0x0E38] = 0;  // ุ (combining)
    font_metrics_.character_widths[0x0E39] = 0;  // ู (combining)
    
    // Tone marks (combining)
    for (uint16_t i = 0x0E48; i <= 0x0E4B; ++i) {
        font_metrics_.character_widths[i] = 0;
    }
    
    // Thai digits
    for (uint16_t i = 0x0E50; i <= 0x0E59; ++i) {
        font_metrics_.character_widths[i] = 8;
    }
}

bool ThaiLanguageProcessor::IsThaiCharacter(uint16_t codepoint) const {
    return (codepoint >= 0x0E00 && codepoint <= 0x0E7F);
}

bool ThaiLanguageProcessor::IsThaiVowel(uint16_t codepoint) const {
    return (codepoint >= 0x0E30 && codepoint <= 0x0E4F);
}

bool ThaiLanguageProcessor::IsThaiTone(uint16_t codepoint) const {
    return (codepoint >= 0x0E48 && codepoint <= 0x0E4B);
}

bool ThaiLanguageProcessor::IsThaiConsonant(uint16_t codepoint) const {
    return (codepoint >= 0x0E01 && codepoint <= 0x0E2E);
}

bool ThaiLanguageProcessor::RequiresComplexLayout(const std::string& text) const {
    // Check for combining characters that require complex layout
    std::u16string utf16_text;
    // Convert UTF-8 to UTF-16 for easier processing
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    try {
        utf16_text = converter.from_bytes(text);
    } catch (...) {
        return false;
    }
    
    for (char16_t c : utf16_text) {
        if (IsThaiVowel(c) || IsThaiTone(c)) {
            return true;
        }
    }
    return false;
}

bool ThaiLanguageProcessor::ConvertUTF8ToDAB(const std::string& utf8_text, std::vector<uint8_t>& dab_data) {
    try {
        dab_data.clear();
        dab_data.push_back(0x0E); // Thai character set identifier
        
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string utf16_text = converter.from_bytes(utf8_text);
        
        for (char16_t c : utf16_text) {
            auto it = utf8_to_dab_map_.find(c);
            if (it != utf8_to_dab_map_.end()) {
                dab_data.push_back(it->second);
            } else if (c < 0x80) {
                // ASCII characters
                dab_data.push_back(static_cast<uint8_t>(c));
            } else {
                // Unsupported character, use replacement
                dab_data.push_back(0x3F); // '?'
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error converting UTF-8 to DAB: " << e.what() << std::endl;
        return false;
    }
}

std::string ThaiLanguageProcessor::ConvertDABToUTF8(const std::vector<uint8_t>& dab_data) {
    if (dab_data.empty() || dab_data[0] != 0x0E) {
        return ""; // Not Thai character set
    }
    
    try {
        std::u16string utf16_result;
        
        for (size_t i = 1; i < dab_data.size(); ++i) {
            uint8_t dab_char = dab_data[i];
            bool found = false;
            
            // Reverse lookup in mapping
            for (const auto& pair : utf8_to_dab_map_) {
                if (pair.second == dab_char) {
                    utf16_result.push_back(pair.first);
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                if (dab_char < 0x80) {
                    // ASCII character
                    utf16_result.push_back(static_cast<char16_t>(dab_char));
                } else {
                    // Unknown character
                    utf16_result.push_back(0x003F); // '?'
                }
            }
        }
        
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        return converter.to_bytes(utf16_result);
        
    } catch (const std::exception& e) {
        std::cerr << "Error converting DAB to UTF-8: " << e.what() << std::endl;
        return "";
    }
}

ThaiTextLayout ThaiLanguageProcessor::AnalyzeTextLayout(const std::string& utf8_text, 
                                                        uint16_t max_width_pixels,
                                                        uint16_t max_lines) {
    ThaiTextLayout layout;
    layout.original_text = utf8_text;
    layout.requires_complex_layout = RequiresComplexLayout(utf8_text);
    
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string utf16_text = converter.from_bytes(utf8_text);
        
        uint16_t current_line_width = 0;
        uint16_t current_line = 0;
        std::string current_line_text;
        
        for (size_t i = 0; i < utf16_text.length(); ++i) {
            char16_t c = utf16_text[i];
            uint8_t char_width = 8; // default width
            
            auto width_it = font_metrics_.character_widths.find(c);
            if (width_it != font_metrics_.character_widths.end()) {
                char_width = width_it->second;
            }
            
            layout.character_positions.push_back(current_line_width);
            layout.character_widths.push_back(char_width);
            
            current_line_width += char_width;
            
            // Check for line break
            if (current_line_width > max_width_pixels || c == '\n') {
                layout.line_breaks.push_back(current_line_text);
                current_line_text.clear();
                current_line_width = char_width;
                current_line++;
                
                if (current_line >= max_lines) {
                    break;
                }
            } else {
                current_line_text += converter.to_bytes(std::u16string(1, c));
            }
        }
        
        if (!current_line_text.empty()) {
            layout.line_breaks.push_back(current_line_text);
        }
        
        layout.total_width_pixels = max_width_pixels;
        layout.total_height_pixels = layout.line_breaks.size() * font_metrics_.line_height;
        
        // Convert to DAB format
        ConvertUTF8ToDAB(utf8_text, layout.dab_encoded_data);
        
    } catch (const std::exception& e) {
        std::cerr << "Error analyzing text layout: " << e.what() << std::endl;
    }
    
    return layout;
}

bool ThaiLanguageProcessor::FormatTextForDLS(const std::string& input_text, 
                                            std::string& output_text,
                                            size_t max_length) {
    output_text = input_text;
    
    // Remove excessive whitespace
    std::regex whitespace_regex(R"(\s+)");
    output_text = std::regex_replace(output_text, whitespace_regex, " ");
    
    // Trim leading/trailing spaces
    output_text.erase(0, output_text.find_first_not_of(" \t\n\r"));
    output_text.erase(output_text.find_last_not_of(" \t\n\r") + 1);
    
    // Truncate if too long
    if (output_text.length() > max_length) {
        // Try to break at word boundary
        size_t break_pos = max_length;
        while (break_pos > max_length * 0.8) {
            char c = output_text[break_pos];
            if (c == ' ' || (c & 0x80) == 0) { // ASCII or space
                break;
            }
            break_pos--;
        }
        
        output_text = output_text.substr(0, break_pos);
        if (break_pos < input_text.length()) {
            output_text += "...";
        }
    }
    
    return true;
}

std::string ThaiLanguageProcessor::FormatNumber(int number, ThaiNumberFormat format) {
    switch (format) {
        case ThaiNumberFormat::WESTERN_DIGITS:
            return std::to_string(number);
            
        case ThaiNumberFormat::THAI_DIGITS: {
            std::string result;
            std::string western = std::to_string(number);
            for (char c : western) {
                if (c >= '0' && c <= '9') {
                    // Convert western digit to Thai digit
                    uint16_t thai_digit = 0x0E50 + (c - '0');
                    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
                    result += converter.to_bytes(std::u16string(1, thai_digit));
                } else {
                    result += c;
                }
            }
            return result;
        }
        
        case ThaiNumberFormat::THAI_WORDS: {
            // Thai number words implementation
            std::vector<std::string> thai_digits = {
                "ศูนย์", "หนึ่ง", "สอง", "สาม", "สี่", "ห้า", "หก", "เจ็ด", "แปด", "เก้า"
            };
            
            if (number == 0) return thai_digits[0];
            
            std::string result;
            std::string num_str = std::to_string(std::abs(number));
            
            // Simple implementation for numbers 0-99
            if (number < 10) {
                result = thai_digits[number];
            } else if (number < 20) {
                result = "สิบ";
                if (number > 10) {
                    result += thai_digits[number - 10];
                }
            } else if (number < 100) {
                int tens = number / 10;
                int ones = number % 10;
                result = thai_digits[tens] + "สิบ";
                if (ones > 0) {
                    result += thai_digits[ones];
                }
            } else {
                // For larger numbers, fall back to Thai digits
                return FormatNumber(number, ThaiNumberFormat::THAI_DIGITS);
            }
            
            if (number < 0) {
                result = "ลบ" + result;
            }
            
            return result;
        }
        
        default:
            return FormatNumber(number, ThaiNumberFormat::THAI_DIGITS);
    }
}

BuddhistDate ThaiLanguageProcessor::GetBuddhistDate(const std::chrono::system_clock::time_point& date) {
    auto time_t = std::chrono::system_clock::to_time_t(date);
    auto tm = *std::localtime(&time_t);
    
    BuddhistDate bd;
    bd.year_ce = tm.tm_year + 1900;
    bd.year_be = bd.year_ce + 543;
    bd.month = tm.tm_mon + 1;
    bd.day = tm.tm_mday;
    bd.thai_month_name = GetThaiMonthName(bd.month);
    bd.thai_day_name = GetThaiDayName(date);
    bd.is_holy_day = IsHolyDay(date);
    
    return bd;
}

std::string ThaiLanguageProcessor::GetThaiMonthName(int month) {
    std::vector<std::string> thai_months = {
        "", "มกราคม", "กุมภาพันธ์", "มีนาคม", "เมษายน", "พฤษภาคม", "มิถุนายน",
        "กรกฎาคม", "สิงหาคม", "กันยายน", "ตุลาคม", "พฤศจิกายน", "ธันวาคม"
    };
    
    if (month >= 1 && month <= 12) {
        return thai_months[month];
    }
    return "";
}

std::string ThaiLanguageProcessor::GetThaiDayName(const std::chrono::system_clock::time_point& date) {
    auto time_t = std::chrono::system_clock::to_time_t(date);
    auto tm = *std::localtime(&time_t);
    
    std::vector<std::string> thai_days = {
        "วันอาทิตย์", "วันจันทร์", "วันอังคาร", "วันพุธ", 
        "วันพฤหัสบดี", "วันศุกร์", "วันเสาร์"
    };
    
    return thai_days[tm.tm_wday];
}

bool ThaiLanguageProcessor::IsHolyDay(const std::chrono::system_clock::time_point& date) {
    // Simplified check - real implementation would use astronomical calculations
    auto time_t = std::chrono::system_clock::to_time_t(date);
    auto tm = *std::localtime(&time_t);
    
    // Check fixed holy days
    for (const auto& holiday : holiday_calendar_) {
        if (holiday.second.is_holy_day && 
            holiday.second.month == (tm.tm_mon + 1) &&
            holiday.second.day == tm.tm_mday) {
            return true;
        }
    }
    
    // Check for Buddhist observance days (simplified)
    // In real implementation, this would calculate lunar calendar dates
    return false;
}

CulturalValidation ThaiLanguageProcessor::ValidateContent(const std::string& text) {
    CulturalValidation validation;
    validation.is_appropriate = true;
    validation.cultural_sensitivity_score = 1.0;
    
    // Check for inappropriate words
    for (const auto& word : inappropriate_words_) {
        if (text.find(word) != std::string::npos) {
            validation.is_appropriate = false;
            validation.warnings.push_back("Contains inappropriate language: " + word);
            validation.cultural_sensitivity_score -= 0.2;
        }
    }
    
    // Check for royal references
    for (const auto& term : royal_terms_) {
        if (text.find(term) != std::string::npos) {
            validation.contains_royal_references = true;
            validation.requires_special_formatting = true;
            validation.suggestions.push_back("Royal reference detected - ensure respectful formatting");
        }
    }
    
    // Check for religious content
    for (const auto& term : religious_terms_) {
        if (text.find(term) != std::string::npos) {
            validation.contains_religious_content = true;
            validation.suggestions.push_back("Religious content detected - ensure respectful treatment");
        }
    }
    
    // Ensure score doesn't go below 0
    validation.cultural_sensitivity_score = std::max(0.0, validation.cultural_sensitivity_score);
    
    return validation;
}

uint16_t ThaiLanguageProcessor::CalculateTextWidth(const std::string& text) const {
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string utf16_text = converter.from_bytes(text);
        
        uint16_t total_width = 0;
        for (char16_t c : utf16_text) {
            auto it = font_metrics_.character_widths.find(c);
            if (it != font_metrics_.character_widths.end()) {
                total_width += it->second;
            } else {
                total_width += 8; // Default width
            }
        }
        
        return total_width;
    } catch (const std::exception& e) {
        std::cerr << "Error calculating text width: " << e.what() << std::endl;
        return text.length() * 8; // Fallback estimate
    }
}

std::vector<std::string> ThaiLanguageProcessor::WrapText(const std::string& text, uint16_t max_width) {
    std::vector<std::string> lines;
    
    try {
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
        std::u16string utf16_text = converter.from_bytes(text);
        
        std::u16string current_line;
        uint16_t current_width = 0;
        
        for (char16_t c : utf16_text) {
            uint8_t char_width = 8; // default
            auto it = font_metrics_.character_widths.find(c);
            if (it != font_metrics_.character_widths.end()) {
                char_width = it->second;
            }
            
            if (c == '\n') {
                lines.push_back(converter.to_bytes(current_line));
                current_line.clear();
                current_width = 0;
            } else if (current_width + char_width > max_width) {
                if (!current_line.empty()) {
                    lines.push_back(converter.to_bytes(current_line));
                    current_line.clear();
                    current_width = 0;
                }
                current_line += c;
                current_width += char_width;
            } else {
                current_line += c;
                current_width += char_width;
            }
        }
        
        if (!current_line.empty()) {
            lines.push_back(converter.to_bytes(current_line));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error wrapping text: " << e.what() << std::endl;
        lines.push_back(text); // Fallback
    }
    
    return lines;
}

} // namespace StreamDAB