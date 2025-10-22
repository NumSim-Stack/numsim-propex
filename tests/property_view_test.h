#include <gtest/gtest.h>
#include <memory>
#include <atomic>
#include <stdexcept>

#include "propex/ownership_policies.h"
#include "propex/property_view.h"
#include "propex/propex_node.h"

using namespace numsim::propex;


// Helper wrapper so we can pass template templates to GoogleTest
template <template<class> class Policy>
struct OwnershipTag {
    template<class T>
    using type = Policy<T>;
};

template<class OwnershipTag>
class PropertyViewTest : public ::testing::Test {
protected:
    template<class T>
    using Ownership = typename OwnershipTag::template type<T>;

    using node_type = node<int, Ownership>;
    using view_type = property_view<int, node, Ownership>;
};

using OwnershipTypes = ::testing::Types<
    OwnershipTag<ownership::by_value>,
    OwnershipTag<ownership::by_reference>,
    OwnershipTag<ownership::by_shared>,
    OwnershipTag<ownership::by_atomic>
    >;

TYPED_TEST_SUITE(PropertyViewTest, OwnershipTypes);

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------
TYPED_TEST(PropertyViewTest, DefaultConstructedViewIsInvalid) {
    using View = typename TestFixture::view_type;
    View v;
    EXPECT_FALSE(v.valid());
}

TYPED_TEST(PropertyViewTest, BoundViewIsValid) {
    using Node = typename TestFixture::node_type;
    using View = typename TestFixture::view_type;
    int val{17};
    Node n(val);
    View v(&n);
    EXPECT_TRUE(v.valid());
}

TYPED_TEST(PropertyViewTest, CheckedAndUncheckedGetMatch) {
    using Node = typename TestFixture::node_type;
    using View = typename TestFixture::view_type;
    int val{17};
    Node n(val);
    View v(&n);
    EXPECT_EQ(v.get_checked(), v.get());
}

TYPED_TEST(PropertyViewTest, CheckedGetThrowsWhenUnbound) {
    using View = typename TestFixture::view_type;
    View v;
    EXPECT_THROW(v.get_checked(), std::runtime_error);
}

TYPED_TEST(PropertyViewTest, CheckedSetThrowsWhenUnbound) {
    using View = typename TestFixture::view_type;
    View v;
    EXPECT_THROW(v.set_checked(5), std::runtime_error);
}

TYPED_TEST(PropertyViewTest, AssignmentOperatorSetsValue) {
    using Node = typename TestFixture::node_type;
    using View = typename TestFixture::view_type;
    int val{5};
    Node n(val);
    View v(&n);
    v = 10;
    EXPECT_EQ(v.get_checked(), 10);
}

TYPED_TEST(PropertyViewTest, MoveTransfersBinding) {
    using Node = typename TestFixture::node_type;
    using View = typename TestFixture::view_type;
    int val{88};
    Node n(val);
    View a(&n);
    View b(std::move(a));
    EXPECT_TRUE(b.valid());
    EXPECT_FALSE(a.valid());
    EXPECT_EQ(b.get_checked(), 88);
}

// -----------------------------------------------------------------------------
// Ownership-specific Tests
// -----------------------------------------------------------------------------
TEST(PropertyViewOwnership, ByReferenceReflectsExternalChange) {
    int external = 99;
    node<int, ownership::by_reference> n(external);
    property_view<int, node, ownership::by_reference> v(&n);
    EXPECT_EQ(v.get_checked(), 99);
    external = 123;
    EXPECT_EQ(v.get_checked(), 123);
}

TEST(PropertyViewOwnership, BySharedReflectsSharedChange) {
    auto sp = std::make_shared<int>(7);
    node<int, ownership::by_shared> n(sp);
    property_view<int, node, ownership::by_shared> v(&n);
    *sp = 44;
    EXPECT_EQ(v.get_checked(), 44);
}

TEST(PropertyViewOwnership, ByAtomicStoresAtomically) {
    node<int, ownership::by_atomic> n(5);
    property_view<int, node, ownership::by_atomic> v(&n);
    v.set(100);
    EXPECT_EQ(v.get_checked(), 100);
}
