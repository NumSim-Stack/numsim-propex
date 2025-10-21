#ifndef REGISTRY_TEST_H
#define REGISTRY_TEST_H

#include <gtest/gtest.h>
#include "propex/propex_registry.h"
#include <string>
#include <map>
#include <unordered_map>
#include <memory>
#include <stdexcept>

using namespace numsim::propex;

// -----------------------------------------------------------------------------
// Mock node type
// -----------------------------------------------------------------------------
struct TestNode {
    explicit TestNode(int v = 0) : value(v) {}
    int value{};
};

// -----------------------------------------------------------------------------
// Custom key traits — use semicolon instead of colon
// -----------------------------------------------------------------------------
template <typename KeyType>
struct semicolon_traits final : public key_traits<KeyType, ';'>{
    using key_traits<KeyType, ';'>::merge;
    using key_traits<KeyType, ';'>::split;
};

// -----------------------------------------------------------------------------
// Type combinations for testing
// -----------------------------------------------------------------------------
template <
    template<class...> class Ptr,
    template<class...> class Map,
    template<class> class Traits
    >
struct RegistryCombo {
    using Reg = registry<std::string, TestNode, Ptr, Map, Traits>;
};

// Define aliases for combinations
using Combos = ::testing::Types<
    RegistryCombo<std::unique_ptr, std::unordered_map, key_traits>,
    RegistryCombo<std::shared_ptr, std::unordered_map, key_traits>,
    RegistryCombo<std::unique_ptr, std::map, key_traits>,
    RegistryCombo<std::shared_ptr, std::map, key_traits>,
    RegistryCombo<std::unique_ptr, std::unordered_map, semicolon_traits>,
    RegistryCombo<std::shared_ptr, std::unordered_map, semicolon_traits>
    >;

// -----------------------------------------------------------------------------
// Fixture (typed test)
// -----------------------------------------------------------------------------
template <typename Combo>
class RegistryTypedTest : public ::testing::Test {
protected:
    using Reg = typename Combo::Reg;
    Reg reg;
};

TYPED_TEST_SUITE(RegistryTypedTest, Combos);

// -----------------------------------------------------------------------------
// Basic insertion and retrieval
// -----------------------------------------------------------------------------
TYPED_TEST(RegistryTypedTest, AddAndFindSingleKey) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(42)), "key");
    auto* n = this->reg.find("key");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->value, 42);
}

TYPED_TEST(RegistryTypedTest, AddAndFindMergedKey) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(10)), "obj", "prop");
    // delimiter could be ':' or ';'
    auto colon = this->reg.find("obj:prop");
    auto semi  = this->reg.find("obj;prop");
    ASSERT_TRUE(colon || semi);
    EXPECT_EQ((colon ? colon->value : semi->value), 10);
}

TYPED_TEST(RegistryTypedTest, Contains) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(3)), "a", "b");
    EXPECT_TRUE(this->reg.contains("a:b") || this->reg.contains("a;b"));
    EXPECT_FALSE(this->reg.contains("unknown:key"));
}

// -----------------------------------------------------------------------------
// Checked access
// -----------------------------------------------------------------------------
TYPED_TEST(RegistryTypedTest, AtReturnsRefAndIsMutable) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(5)), "entry");
    auto& node = this->reg.at("entry");
    EXPECT_EQ(node.value, 5);
    node.value = 8;
    EXPECT_EQ(this->reg.at("entry").value, 8);
}

TYPED_TEST(RegistryTypedTest, AtThrowsIfMissing) {
    EXPECT_THROW(this->reg.at("nope"), std::out_of_range);
}

// -----------------------------------------------------------------------------
// Erase, clear
// -----------------------------------------------------------------------------
TYPED_TEST(RegistryTypedTest, EraseExisting) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(22)), "temp");
    EXPECT_TRUE(this->reg.erase("temp"));
    EXPECT_FALSE(this->reg.contains("temp"));
}

TYPED_TEST(RegistryTypedTest, EraseNonExisting) {
    EXPECT_FALSE(this->reg.erase("missing"));
}

TYPED_TEST(RegistryTypedTest, ClearRegistry) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(1)), "a");
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(2)), "b");
    this->reg.clear();
    EXPECT_TRUE(this->reg.data().empty());
}

// -----------------------------------------------------------------------------
// Move semantics (only meaningful for unique_ptr, but harmless for shared_ptr)
// -----------------------------------------------------------------------------
TYPED_TEST(RegistryTypedTest, MoveConstructorTransfersData) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(9)), "key");
    using Reg = typename TypeParam::Reg;
    Reg reg2(std::move(this->reg));
    auto* n = reg2.find("key");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->value, 9);
}

TYPED_TEST(RegistryTypedTest, MoveAssignmentTransfersData) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(11)), "key2");
    using Reg = typename TypeParam::Reg;
    Reg reg2;
    reg2 = std::move(this->reg);
    auto* n = reg2.find("key2");
    ASSERT_NE(n, nullptr);
    EXPECT_EQ(n->value, 11);
}

// -----------------------------------------------------------------------------
// Underlying container access
// -----------------------------------------------------------------------------
TYPED_TEST(RegistryTypedTest, DataAccessorsWork) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(1)), "x");
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(2)), "y");

    EXPECT_EQ(this->reg.data().size(), 2u);
    auto& mutable_map = this->reg.data();
    EXPECT_EQ(mutable_map.size(), 2u);
}

// -----------------------------------------------------------------------------
// Key composition
// -----------------------------------------------------------------------------
TYPED_TEST(RegistryTypedTest, AddWithMultipleArgsMergesCorrectly) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(5)), "a", "b", "c");
    bool found = (this->reg.find("a:b:c") != nullptr) || (this->reg.find("a;b;c") != nullptr);
    EXPECT_TRUE(found);
}

TYPED_TEST(RegistryTypedTest, AddOverwritesKey) {
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(1)), "dup");
    this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(2)), "dup");
    EXPECT_EQ(this->reg.find("dup")->value, 2);
}

// -----------------------------------------------------------------------------
// Edge cases
// -----------------------------------------------------------------------------
TYPED_TEST(RegistryTypedTest, AddEmptyKey) {
    EXPECT_NO_THROW(this->reg.add(typename TypeParam::Reg::node_pointer(new TestNode(99)), ""));
    EXPECT_NE(this->reg.find(""), nullptr);
}

TYPED_TEST(RegistryTypedTest, FindReturnsNullptrForMissing) {
    EXPECT_EQ(this->reg.find("notfound"), nullptr);
}

TYPED_TEST(RegistryTypedTest, ContainsEmptyRegistryIsFalse) {
    EXPECT_FALSE(this->reg.contains("whatever"));
}


#endif // REGISTRY_TEST_H
