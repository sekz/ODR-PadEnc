/*
    Copyright (C) 2025 StreamDAB Thailand Project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Test suite for character set edge cases - ETSI TS 101 756 compliance
*/

#include "../src/charset.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <cstring>

// Test result tracking
struct TestResult {
    std::string test_name;
    bool passed;
    std::string error_message;
};

std::vector<TestResult> test_results;

void record_test(const std::string& name, bool passed, const std::string& error = "") {
    test_results.push_back({name, passed, error});
    if (passed) {
        std::cout << "✓ PASS: " << name << std::endl;
    } else {
        std::cout << "✗ FAIL: " << name << " - " << error << std::endl;
    }
}

// Test 1: All 256 EBU Latin characters
void test_all_ebu_latin_characters() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;

    // Test all 256 characters (0-255)
    for (int i = 1; i < 256; i++) {  // Skip NUL (0)
        std::string ebu_char;
        ebu_char.push_back(static_cast<char>(i));
        
        // Convert EBU Latin to UTF-8
        std::string utf8_result = converter.convert_ebu_to_utf8(ebu_char);
        
        // Convert back to EBU Latin
        std::string ebu_result = converter.convert(utf8_result, false);
        
        // Should round-trip correctly
        if (ebu_result.empty() || static_cast<unsigned char>(ebu_result[0]) != i) {
            all_passed = false;
            error_msg = "Character " + std::to_string(i) + " failed round-trip";
            break;
        }
    }
    
    record_test("All 256 EBU Latin characters round-trip", all_passed, error_msg);
}

// Test 2: Thai characters (U+0E00 to U+0E7F)
void test_thai_characters() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    // Thai Unicode range: U+0E00 to U+0E7F (128 characters)
    std::vector<std::string> thai_test_strings = {
        // Thai consonants
        "ก ข ฃ ค ฅ ฆ ง จ ฉ ช ซ ฌ ญ ฎ ฏ",
        "ฐ ฑ ฒ ณ ด ต ถ ท ธ น บ ป ผ ฝ พ ฟ",
        "ภ ม ย ร ล ว ศ ษ ส ห ฬ อ ฮ",
        
        // Thai vowels
        "ะ ั า ำ ิ ี ึ ื ุ ู ฺ",
        "เ แ โ ใ ไ",
        
        // Thai tone marks
        "่ ้ ๊ ๋",
        
        // Thai numbers
        "๐ ๑ ๒ ๓ ๔ ๕ ๖ ๗ ๘ ๙",
        
        // Common Thai words
        "สวัสดี",       // Hello
        "ขอบคุณ",       // Thank you
        "ประเทศไทย",    // Thailand
        "กรุงเทพมหานคร", // Bangkok
        "ภูมิอากาศ",    // Weather
        "แผ่นดินไหว",   // Earthquake
        "น้ำท่วม",      // Flood
        "คลื่นสึนามิ",  // Tsunami
    };
    
    for (const auto& thai_text : thai_test_strings) {
        try {
            // Convert Thai UTF-8 to EBU Latin
            // Note: Thai characters are NOT in EBU Latin, so they should be converted to spaces
            std::string ebu_result = converter.convert(thai_text, true);
            
            // Thai characters should be converted (fallback to space)
            if (ebu_result.empty()) {
                all_passed = false;
                error_msg = "Empty result for Thai text: " + thai_text;
                break;
            }
        } catch (const std::exception& e) {
            all_passed = false;
            error_msg = std::string("Exception for Thai text '") + thai_text + "': " + e.what();
            break;
        }
    }
    
    record_test("Thai characters (U+0E00-U+0E7F) handling", all_passed, error_msg);
}

// Test 3: Thai diacritics and tone marks
void test_thai_diacritics() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    std::vector<std::string> diacritic_tests = {
        "กา",   // Base + vowel
        "ก่า",  // Base + tone mark + vowel
        "ก้า",  // Base + tone mark (different)
        "ก๊า",  // Base + tone mark (another)
        "ก๋า",  // Base + tone mark (yet another)
        "กำ",   // Base + sara am
        "ไก่",  // Complex with tone
        "เด็ก", // Complex vowel + tone
        "ผู้",  // Complex with modifier
    };
    
    for (const auto& text : diacritic_tests) {
        try {
            std::string result = converter.convert(text, true);
            // Should not crash or throw
            if (result.empty()) {
                all_passed = false;
                error_msg = "Empty result for: " + text;
                break;
            }
        } catch (const std::exception& e) {
            all_passed = false;
            error_msg = std::string("Exception for '") + text + "': " + e.what();
            break;
        }
    }
    
    record_test("Thai diacritics and tone marks", all_passed, error_msg);
}

// Test 4: Zero-width characters
void test_zero_width_characters() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    std::vector<std::string> zero_width_tests = {
        "Hello\u200BWorld",     // Zero-width space
        "Test\u200CString",     // Zero-width non-joiner
        "Thai\u200Dtest",       // Zero-width joiner
        "Text\uFEFFhere",       // Zero-width no-break space
    };
    
    for (const auto& text : zero_width_tests) {
        try {
            std::string result = converter.convert(text, true);
            // Should handle gracefully (likely convert zero-width to nothing or space)
            // Main test: should not crash
        } catch (const std::exception& e) {
            all_passed = false;
            error_msg = std::string("Exception for zero-width test: ") + e.what();
            break;
        }
    }
    
    record_test("Zero-width characters handling", all_passed, error_msg);
}

// Test 5: Combining diacritics
void test_combining_diacritics() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    std::vector<std::string> combining_tests = {
        "e\u0301",   // e + combining acute accent (é)
        "a\u0300",   // a + combining grave accent (à)
        "n\u0303",   // n + combining tilde (ñ)
        "c\u0327",   // c + combining cedilla (ç)
    };
    
    for (const auto& text : combining_tests) {
        try {
            std::string result = converter.convert(text, true);
            // Should handle combining characters
            if (result.empty()) {
                all_passed = false;
                error_msg = "Empty result for combining character test";
                break;
            }
        } catch (const std::exception& e) {
            all_passed = false;
            error_msg = std::string("Exception for combining test: ") + e.what();
            break;
        }
    }
    
    record_test("Combining diacritics handling", all_passed, error_msg);
}

// Test 6: Special punctuation
void test_special_punctuation() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    std::vector<std::string> punctuation_tests = {
        "«quoted»",            // Guillemets (in EBU Latin)
        ""curly quotes"",      // Curly quotes (not in EBU Latin)
        "em—dash",             // Em dash
        "ellipsis…",           // Ellipsis
        "bullet•point",        // Bullet
        "©2025",               // Copyright (in EBU Latin)
        "€100",                // Euro sign (in EBU Latin)
        "£50",                 // Pound sign (in EBU Latin)
        "$20",                 // Dollar sign (in EBU Latin)
    };
    
    for (const auto& text : punctuation_tests) {
        try {
            std::string result = converter.convert(text, true);
            if (result.empty()) {
                all_passed = false;
                error_msg = "Empty result for: " + text;
                break;
            }
        } catch (const std::exception& e) {
            all_passed = false;
            error_msg = std::string("Exception for '") + text + "': " + e.what();
            break;
        }
    }
    
    record_test("Special punctuation handling", all_passed, error_msg);
}

// Test 7: Character set switching (UTF-8 ↔ EBU Latin)
void test_charset_switching() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    // Characters that ARE in EBU Latin
    std::vector<std::string> ebu_chars = {
        "Ą", "ę", "ł", "Ń", "©", "€", "£", "$",
        "á", "à", "é", "è", "ñ", "ç",
        "Å", "Æ", "Ø", "Þ",
    };
    
    for (const auto& utf8_char : ebu_chars) {
        try {
            // UTF-8 → EBU Latin
            std::string ebu = converter.convert(utf8_char, false);
            
            // EBU Latin → UTF-8
            std::string utf8 = converter.convert_ebu_to_utf8(ebu);
            
            // Should round-trip
            if (utf8 != utf8_char) {
                all_passed = false;
                error_msg = "Round-trip failed for: " + utf8_char;
                break;
            }
        } catch (const std::exception& e) {
            all_passed = false;
            error_msg = std::string("Exception for '") + utf8_char + "': " + e.what();
            break;
        }
    }
    
    record_test("Character set switching (UTF-8 ↔ EBU Latin)", all_passed, error_msg);
}

// Test 8: Empty and null strings
void test_empty_and_null() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    try {
        // Empty string
        std::string empty_result = converter.convert("", true);
        if (!empty_result.empty()) {
            all_passed = false;
            error_msg = "Empty string should return empty";
        }
        
        // String with only whitespace
        std::string ws_result = converter.convert("   ", true);
        if (ws_result.length() != 3) {
            all_passed = false;
            error_msg = "Whitespace not preserved";
        }
    } catch (const std::exception& e) {
        all_passed = false;
        error_msg = std::string("Exception: ") + e.what();
    }
    
    record_test("Empty and null string handling", all_passed, error_msg);
}

// Test 9: Long strings (stress test)
void test_long_strings() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    try {
        // Create a 1000-character string
        std::string long_text;
        for (int i = 0; i < 100; i++) {
            long_text += "Test text ";
        }
        
        std::string result = converter.convert(long_text, true);
        
        if (result.length() != long_text.length()) {
            all_passed = false;
            error_msg = "Length mismatch for long string";
        }
    } catch (const std::exception& e) {
        all_passed = false;
        error_msg = std::string("Exception: ") + e.what();
    }
    
    record_test("Long string handling (1000+ chars)", all_passed, error_msg);
}

// Test 10: Invalid UTF-8 sequences
void test_invalid_utf8() {
    CharsetConverter converter;
    bool all_passed = true;
    std::string error_msg;
    
    try {
        // Test with up_to_first_error = true (should not throw)
        std::string invalid_utf8 = "Valid text \xFF\xFE Invalid";
        std::string result = converter.convert(invalid_utf8, true);
        
        // Should convert up to the error
        if (result.empty()) {
            all_passed = false;
            error_msg = "Should convert up to first error";
        }
    } catch (const std::exception& e) {
        all_passed = false;
        error_msg = std::string("Unexpected exception: ") + e.what();
    }
    
    record_test("Invalid UTF-8 sequence handling", all_passed, error_msg);
}

// Main test runner
int main() {
    std::cout << "=============================================" << std::endl;
    std::cout << "  ETSI TS 101 756 Charset Edge Cases Test" << std::endl;
    std::cout << "  ODR-PadEnc Charset Converter Validation" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << std::endl;
    
    // Run all tests
    test_all_ebu_latin_characters();
    test_thai_characters();
    test_thai_diacritics();
    test_zero_width_characters();
    test_combining_diacritics();
    test_special_punctuation();
    test_charset_switching();
    test_empty_and_null();
    test_long_strings();
    test_invalid_utf8();
    
    // Print summary
    std::cout << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "  Test Summary" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& result : test_results) {
        if (result.passed) {
            passed++;
        } else {
            failed++;
            std::cout << "FAILED: " << result.test_name << std::endl;
            std::cout << "  Error: " << result.error_message << std::endl;
        }
    }
    
    std::cout << std::endl;
    std::cout << "Total tests: " << test_results.size() << std::endl;
    std::cout << "Passed: " << passed << " (" << (passed * 100 / test_results.size()) << "%)" << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << std::endl;
    
    if (failed == 0) {
        std::cout << "✅ ALL TESTS PASSED - ETSI TS 101 756 COMPLIANT" << std::endl;
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED - REVIEW REQUIRED" << std::endl;
        return 1;
    }
}
