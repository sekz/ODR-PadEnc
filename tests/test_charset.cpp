/*
    Tests for the UTF-8 to Complete EBU Latin conversion (ETSI TS 101 756 Annex C)
*/

#include "check.h"
#include "charset.h"
#include <string>
#include <vector>

static void test_ascii_letters_are_unchanged()
{
    CharsetConverter cc;
    CHECK(cc.convert("Hello World 0123456789") == "Hello World 0123456789");
}

static void test_accented_characters()
{
    CharsetConverter cc;
    // U+00E9 (é) is 0x82 in the EBU Latin table
    CHECK(cc.convert("caf\xc3\xa9") == "caf\x82");
}

static void test_roundtrip()
{
    CharsetConverter cc;
    const std::string text = "caf\xc3\xa9 \xc3\xbc\xc3\xb1";
    CHECK(cc.convert_ebu_to_utf8(cc.convert(text)) == text);
}

static void test_unconvertible_characters_are_replaced_and_reported()
{
    CharsetConverter cc;
    // Thai: U+0E2A U+0E27 followed by "DAB"
    const std::string text = "\xe0\xb8\xaa\xe0\xb8\xa7 DAB";

    std::vector<uint32_t> unconvertible;
    CHECK(cc.convert(text, true, &unconvertible) == "   DAB");
    CHECK(unconvertible.size() == 2);
    CHECK(unconvertible[0] == 0x0E2A);
    CHECK(unconvertible[1] == 0x0E27);

    // The output does not depend on whether the characters are reported
    CHECK(cc.convert(text) == "   DAB");
}

static void test_convertible_text_reports_nothing()
{
    CharsetConverter cc;
    std::vector<uint32_t> unconvertible;
    cc.convert("Hello caf\xc3\xa9", true, &unconvertible);
    CHECK(unconvertible.empty());
}

static void test_empty_input()
{
    CharsetConverter cc;
    CHECK(cc.convert("").empty());
}

int main()
{
    test_ascii_letters_are_unchanged();
    test_accented_characters();
    test_roundtrip();
    test_unconvertible_characters_are_replaced_and_reported();
    test_convertible_text_reports_nothing();
    test_empty_input();
    return 0;
}
