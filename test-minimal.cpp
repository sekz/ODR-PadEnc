// Minimal test to verify Google Test framework works
#include <gtest/gtest.h>

// Simple test to verify Google Test is working
TEST(GoogleTestFramework, BasicTest) {
    EXPECT_EQ(2 + 2, 4);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST(GoogleTestFramework, StringTest) {
    std::string hello = "Hello";
    std::string world = "World";
    EXPECT_NE(hello, world);
    EXPECT_EQ(hello + world, "HelloWorld");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}