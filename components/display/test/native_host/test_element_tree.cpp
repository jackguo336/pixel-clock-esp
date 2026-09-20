#include <array>
#include <optional>
#include <string_view>
#include <variant>

#include "display/element_tree.hpp"
#include "display/elements.hpp"
#include "gtest/gtest.h"

namespace {

constexpr display::ElementId kRootId{.value = 1};
constexpr display::ElementId kLeafId{.value = 2};
constexpr display::ElementId kNestedContainerId{.value = 3};
constexpr display::ElementId kNestedLeafId{.value = 4};
constexpr display::ElementId kBitmapLeafId{.value = 5};
constexpr display::Position kRootPosition{.x = 0, .y = 0};
constexpr display::Position kLeafPosition{.x = 2, .y = 4};
constexpr display::Position kNestedContainerPosition{.x = 8, .y = 1};
constexpr display::Position kNestedLeafPosition{.x = 3, .y = 5};
constexpr display::Position kBitmapLeafPosition{.x = 6, .y = 7};
constexpr display::SolidPaint kRootPaint{.color = {.red = 1, .green = 2, .blue = 3}};
constexpr display::SolidPaint kLeafPaint{.color = {.red = 4, .green = 5, .blue = 6}};
constexpr display::SolidPaint kNestedContainerPaint{.color = {.red = 7, .green = 8, .blue = 9}};
constexpr display::SolidPaint kNestedLeafPaint{.color = {.red = 10, .green = 11, .blue = 12}};
constexpr display::SolidPaint kBitmapLeafPaint{.color = {.red = 13, .green = 14, .blue = 15}};
constexpr display::Size kLeafSize{.width = 11, .height = 13};
constexpr display::BitmapId kBitmapId{.value = 9};
constexpr std::string_view kNestedLeafText = "nested";

display::Element make_container(
    display::ElementId id,
    display::Position position,
    display::SolidPaint paint,
    display::LayoutDirection layout_direction)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = display::ContainerElement{.layout_direction = layout_direction},
    };
}

display::Element make_rectangle(
    display::ElementId id,
    display::Position position,
    display::SolidPaint paint,
    display::Size size)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = display::FilledRectangleElement{.size = size},
    };
}

display::Element make_text(
    display::ElementId id,
    display::Position position,
    display::SolidPaint paint,
    std::string_view text)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = display::TextElement{.text = text},
    };
}

display::Element make_bitmap(
    display::ElementId id,
    display::Position position,
    display::SolidPaint paint,
    display::BitmapId bitmap_id)
{
    return display::Element{
        .id = id,
        .position = position,
        .paint = paint,
        .payload = display::BitmapElement{.bitmap_id = bitmap_id},
    };
}

void expect_optional_index(const std::optional<display::ElementNodeIndex>& actual, display::ElementNodeIndex expected)
{
    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, expected);
}

void expect_solid_paint(const display::Paint& paint, const display::SolidPaint& expected)
{
    const auto* solid_paint = std::get_if<display::SolidPaint>(&paint);
    ASSERT_NE(solid_paint, nullptr);
    EXPECT_EQ(solid_paint->color.red, expected.color.red);
    EXPECT_EQ(solid_paint->color.green, expected.color.green);
    EXPECT_EQ(solid_paint->color.blue, expected.color.blue);
}

void expect_element(
    const display::Element& actual,
    display::ElementId id,
    display::Position position,
    const display::SolidPaint& paint)
{
    EXPECT_EQ(actual.id.value, id.value);
    EXPECT_EQ(actual.position.x, position.x);
    EXPECT_EQ(actual.position.y, position.y);
    expect_solid_paint(actual.paint, paint);
}

}  // namespace

TEST(ElementTreeBuilder, BuildsNestedContainerInDeclarationOrder)
{
    std::array<display::ElementTreeNode, 8> storage{};
    display::ElementTreeBuilder builder{storage};

    const bool built = builder.add_container(
        make_container(kRootId, kRootPosition, kRootPaint, display::LayoutDirection::TopToBottom),
        [](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize)));
            EXPECT_TRUE(children.add_container(
                make_container(
                    kNestedContainerId,
                    kNestedContainerPosition,
                    kNestedContainerPaint,
                    display::LayoutDirection::LeftToRight),
                [](auto& nested_children) {
                    EXPECT_TRUE(nested_children.add_terminal(
                        make_text(kNestedLeafId, kNestedLeafPosition, kNestedLeafPaint, kNestedLeafText)));
                }));
            EXPECT_TRUE(children.add_terminal(
                make_bitmap(kBitmapLeafId, kBitmapLeafPosition, kBitmapLeafPaint, kBitmapId)));
        });
    ASSERT_TRUE(built);

    const display::ElementTree tree = builder.get_tree();
    ASSERT_EQ(tree.nodes.size(), 5u);
    EXPECT_LT(tree.nodes.size(), storage.size());
    EXPECT_EQ(tree.root, display::ElementNodeIndex{0});
    EXPECT_EQ(tree.nodes.data(), storage.data());

    expect_element(tree.nodes[0].element, kRootId, kRootPosition, kRootPaint);
    const auto* root_container = std::get_if<display::ContainerElement>(&tree.nodes[0].element.payload);
    ASSERT_NE(root_container, nullptr);
    EXPECT_EQ(root_container->layout_direction, display::LayoutDirection::TopToBottom);
    expect_optional_index(tree.nodes[0].first_child, 1);
    EXPECT_FALSE(tree.nodes[0].next_sibling.has_value());

    expect_element(tree.nodes[1].element, kLeafId, kLeafPosition, kLeafPaint);
    const auto* leaf = std::get_if<display::FilledRectangleElement>(&tree.nodes[1].element.payload);
    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(leaf->size.width, kLeafSize.width);
    EXPECT_EQ(leaf->size.height, kLeafSize.height);
    EXPECT_FALSE(tree.nodes[1].first_child.has_value());
    expect_optional_index(tree.nodes[1].next_sibling, 2);

    expect_element(tree.nodes[2].element, kNestedContainerId, kNestedContainerPosition, kNestedContainerPaint);
    const auto* nested_container = std::get_if<display::ContainerElement>(&tree.nodes[2].element.payload);
    ASSERT_NE(nested_container, nullptr);
    EXPECT_EQ(nested_container->layout_direction, display::LayoutDirection::LeftToRight);
    expect_optional_index(tree.nodes[2].first_child, display::ElementNodeIndex{3});
    expect_optional_index(tree.nodes[2].next_sibling, display::ElementNodeIndex{4});

    expect_element(tree.nodes[3].element, kNestedLeafId, kNestedLeafPosition, kNestedLeafPaint);
    const auto* nested_leaf = std::get_if<display::TextElement>(&tree.nodes[3].element.payload);
    ASSERT_NE(nested_leaf, nullptr);
    EXPECT_EQ(nested_leaf->text, kNestedLeafText);
    EXPECT_FALSE(tree.nodes[3].first_child.has_value());
    EXPECT_FALSE(tree.nodes[3].next_sibling.has_value());

    expect_element(tree.nodes[4].element, kBitmapLeafId, kBitmapLeafPosition, kBitmapLeafPaint);
    const auto* bitmap_leaf = std::get_if<display::BitmapElement>(&tree.nodes[4].element.payload);
    ASSERT_NE(bitmap_leaf, nullptr);
    EXPECT_EQ(bitmap_leaf->bitmap_id.value, kBitmapId.value);
    EXPECT_FALSE(tree.nodes[4].first_child.has_value());
    EXPECT_FALSE(tree.nodes[4].next_sibling.has_value());
}

TEST(ElementTreeBuilder, SingleLeafRootSpansOnlyUsedStorage)
{
    std::array<display::ElementTreeNode, 5> storage{};
    display::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(builder.add_terminal(make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize)));

    const display::ElementTree tree = builder.get_tree();
    EXPECT_EQ(tree.root, display::ElementNodeIndex{0});
    EXPECT_EQ(tree.nodes.size(), 1u);
    EXPECT_LT(tree.nodes.size(), storage.size());
    EXPECT_EQ(tree.nodes.data(), storage.data());
    expect_element(tree.nodes[0].element, kLeafId, kLeafPosition, kLeafPaint);
}

TEST(ElementTreeBuilder, RejectsSecondRoot)
{
    const auto root =
        make_container(kRootId, kRootPosition, kRootPaint, display::LayoutDirection::TopToBottom);
    const auto leaf = make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize);
    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(builder.add_terminal(leaf));
    EXPECT_FALSE(builder.add_terminal(make_rectangle(
        kNestedLeafId, kNestedLeafPosition, kNestedLeafPaint, kLeafSize)));
    EXPECT_FALSE(builder.add_terminal(leaf));
    bool callback_ran = false;
    EXPECT_FALSE(builder.add_container(root, [&](auto&) { callback_ran = true; }));
    EXPECT_FALSE(callback_ran);
}

TEST(ElementTreeBuilder, RejectsNonContainerPassedToAddContainer)
{
    const auto leaf = make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize);
    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};

    bool callback_ran = false;
    EXPECT_FALSE(builder.add_container(leaf, [&](auto&) { callback_ran = true; }));
    EXPECT_FALSE(callback_ran);
    EXPECT_FALSE(builder.add_terminal(leaf));
}

TEST(ElementTreeBuilder, RejectsWhenNodeStorageIsExhausted)
{
    const auto root =
        make_container(kRootId, kRootPosition, kRootPaint, display::LayoutDirection::TopToBottom);
    const auto leaf = make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize);
    std::array<display::ElementTreeNode, 1> storage{};
    display::ElementTreeBuilder builder{storage};

    EXPECT_FALSE(builder.add_container(root, [&](auto& children) {
        EXPECT_FALSE(children.add_terminal(leaf));
    }));
    EXPECT_FALSE(builder.add_terminal(leaf));
}

TEST(ElementTreeBuilder, RejectsContainerPassedToAddTerminal)
{
    const auto root =
        make_container(kRootId, kRootPosition, kRootPaint, display::LayoutDirection::TopToBottom);
    const auto leaf = make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize);
    std::array<display::ElementTreeNode, 4> storage{};
    display::ElementTreeBuilder builder{storage};

    EXPECT_FALSE(builder.add_terminal(root));
    EXPECT_FALSE(builder.add_terminal(leaf));
}

TEST(ElementTreeBuilder, GetTreeReturnsRootAndUsedNodesAfterSuccess)
{
    std::array<display::ElementTreeNode, 6> storage{};
    display::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(builder.add_container(
        make_container(kRootId, kRootPosition, kRootPaint, display::LayoutDirection::LeftToRight),
        [](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize)));
            EXPECT_TRUE(children.add_terminal(
                make_text(kNestedLeafId, kNestedLeafPosition, kNestedLeafPaint, kNestedLeafText)));
        }));

    const display::ElementTree tree = builder.get_tree();
    EXPECT_EQ(tree.root, display::ElementNodeIndex{0});
    EXPECT_EQ(tree.nodes.size(), 3u);
    EXPECT_LT(tree.nodes.size(), storage.size());
    EXPECT_EQ(tree.nodes.data(), storage.data());
    expect_optional_index(tree.nodes[0].first_child, display::ElementNodeIndex{1});
    expect_optional_index(tree.nodes[1].next_sibling, display::ElementNodeIndex{2});
    EXPECT_FALSE(tree.nodes[2].next_sibling.has_value());
}
