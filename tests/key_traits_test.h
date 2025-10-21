#ifndef KEY_TRAITS_TEST_H
#define KEY_TRAITS_TEST_H

#include <gtest/gtest.h>
#include "propex/key_traits.h"

// ============================================================================
// Split tests — Default Delimiter ':'
// ============================================================================
TEST(KeyTraits_Split, BasicTwoParts) {
    auto parts = numsim::propex::key_traits<std::string>::split("carA:speed");
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "carA");
    EXPECT_EQ(parts[1], "speed");
}

TEST(KeyTraits_Split, MultipleParts) {
    auto parts = numsim::propex::key_traits<std::string>::split("a:b:c:d");
    ASSERT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[0], "a");
    EXPECT_EQ(parts[3], "d");
}

TEST(KeyTraits_Split, NoDelimiter) {
    auto parts = numsim::propex::key_traits<std::string>::split("single");
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "single");
}

TEST(KeyTraits_Split, EmptyString) {
    auto parts = numsim::propex::key_traits<std::string>::split("");
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "");
}

TEST(KeyTraits_Split, TrailingDelimiter) {
    auto parts = numsim::propex::key_traits<std::string>::split("root:child:");
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "root");
    EXPECT_EQ(parts[1], "child");
    EXPECT_EQ(parts[2], "");
}

TEST(KeyTraits_Split, LeadingDelimiter) {
    auto parts = numsim::propex::key_traits<std::string>::split(":child");
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "child");
}

// ============================================================================
// Merge tests — Default Delimiter ':'
// ============================================================================

TEST(KeyTraits_Merge, BasicTwoParts) {
    std::string merged = numsim::propex::key_traits<std::string>::merge("carA", "speed");
    EXPECT_EQ(merged, "carA:speed");
}

TEST(KeyTraits_Merge, MultipleParts) {
    std::string merged = numsim::propex::key_traits<std::string>::merge("a", "b", "c");
    EXPECT_EQ(merged, "a:b:c");
}

TEST(KeyTraits_Merge, SinglePart) {
    std::string merged = numsim::propex::key_traits<std::string>::merge("single");
    EXPECT_EQ(merged, "single");
}

TEST(KeyTraits_Merge, NoParts) {
    std::string merged = numsim::propex::key_traits<std::string>::merge();
    EXPECT_TRUE(merged.empty());
}

TEST(KeyTraits_Merge, TrailingEmptyPart) {
    std::string merged = numsim::propex::key_traits<std::string>::merge("root", "child", "");
    EXPECT_EQ(merged, "root:child:");
}

// ============================================================================
// Round-trip test
// ============================================================================

TEST(KeyTraits_RoundTrip, MergeSplitInverse) {
    const std::string original = "scene:camera:fov";
    auto parts = numsim::propex::key_traits<std::string>::split(original);

    ASSERT_EQ(parts.size(), 3u);
    std::string recomposed = numsim::propex::key_traits<std::string>::merge(
        std::string(parts[0]), std::string(parts[1]), std::string(parts[2])
        );
    EXPECT_EQ(recomposed, original);
}

// ============================================================================
// Custom delimiter specialization — ';'
// ============================================================================

TEST(KeyTraits_CustomDelimiter, BasicSemicolon) {
    using traits = numsim::propex::key_traits<std::string, ';'>;

    std::string merged = traits::merge("left", "right");
    EXPECT_EQ(merged, "left;right");

    auto parts = traits::split("left;right");
    ASSERT_EQ(parts.size(), 2);
    EXPECT_EQ(parts[0], "left");
    EXPECT_EQ(parts[1], "right");
}

TEST(KeyTraits_CustomDelimiter, MultiplePartsSemicolon) {
    using traits = numsim::propex::key_traits<std::string, ';'>;

    std::string merged = traits::merge("one", "two", "three");
    EXPECT_EQ(merged, "one;two;three");

    auto parts = traits::split("one;two;three");
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[2], "three");
}

TEST(KeyTraits_CustomDelimiter, LeadingTrailingSemicolon) {
    using traits = numsim::propex::key_traits<std::string, ';'>;
    auto parts = traits::split(";middle;");
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "middle");
    EXPECT_EQ(parts[2], "");
}

TEST(KeyTraits_CustomDelimiter, EmptyKeySemicolon) {
    using traits = numsim::propex::key_traits<std::string, ';'>;
    auto parts = traits::split("");
    ASSERT_EQ(parts.size(), 1);
    EXPECT_EQ(parts[0], "");
}

TEST(KeyTraits_CustomDelimiter, DelimiterGetter) {
    EXPECT_EQ(numsim::propex::key_traits<std::string>::delimiter(), ':');
    EXPECT_EQ((numsim::propex::key_traits<std::string, ';'>::delimiter()), ';');
}

// ============================================================================
// Custom delimiter specialization — '|'
// ============================================================================

TEST(KeyTraits_CustomDelimiter, PipeDelimiterBasic) {
    using traits = numsim::propex::key_traits<std::string, '|'>;
    std::string merged = traits::merge("user", "data", "settings");
    EXPECT_EQ(merged, "user|data|settings");

    auto parts = traits::split("user|data|settings");
    ASSERT_EQ(parts.size(), 3);
    EXPECT_EQ(parts[0], "user");
    EXPECT_EQ(parts[1], "data");
    EXPECT_EQ(parts[2], "settings");
}

TEST(KeyTraits_CustomDelimiter, PipeDelimiterEdgeCases) {
    using traits = numsim::propex::key_traits<std::string, '|'>;
    auto parts = traits::split("|start|end|");
    ASSERT_EQ(parts.size(), 4);
    EXPECT_EQ(parts[0], "");
    EXPECT_EQ(parts[1], "start");
    EXPECT_EQ(parts[2], "end");
    EXPECT_EQ(parts[3], "");
}

// ============================================================================
// Constexpr / noexcept checks (compile-time validation)
// ============================================================================

TEST(KeyTraits_StaticChecks, TraitsAreConstexpr) {
    constexpr char d1 = numsim::propex::key_traits<std::string>::delimiter();
    constexpr char d2 = numsim::propex::key_traits<std::string, ';'>::delimiter();
    constexpr char d3 = numsim::propex::key_traits<std::string, '|'>::delimiter();

    static_assert(d1 == ':', "Expected ':'");
    static_assert(d2 == ';', "Expected ';'");
    static_assert(d3 == '|', "Expected '|'");
}



#endif // KEY_TRAITS_TEST_H
