#include <array>
#include <optional>
#include <string_view>
#include <variant>

#include "bitmap_file.hpp"
#include "element_tree.hpp"
#include "elements.hpp"
#include "font.hpp"
#include "gtest/gtest.h"

namespace {

constexpr graphics::ElementId kRootId{.value = 1};
constexpr graphics::ElementId kLeafId{.value = 2};
constexpr graphics::ElementId kNestedContainerId{.value = 3};
constexpr graphics::ElementId kNestedLeafId{.value = 4};
constexpr graphics::ElementId kBitmapLeafId{.value = 5};
constexpr graphics::Position kRootPosition{.x = 0, .y = 0};
constexpr graphics::Position kLeafPosition{.x = 2, .y = 4};
constexpr graphics::Position kNestedContainerPosition{.x = 8, .y = 1};
constexpr graphics::Position kNestedLeafPosition{.x = 3, .y = 5};
constexpr graphics::Position kBitmapLeafPosition{.x = 6, .y = 7};
constexpr graphics::SolidPaint kRootPaint{.color = {.red = 1, .green = 2, .blue = 3}};
constexpr graphics::SolidPaint kLeafPaint{.color = {.red = 4, .green = 5, .blue = 6}};
constexpr graphics::SolidPaint kNestedContainerPaint{.color = {.red = 7, .green = 8, .blue = 9}};
constexpr graphics::SolidPaint kNestedLeafPaint{.color = {.red = 10, .green = 11, .blue = 12}};
constexpr graphics::SolidPaint kBitmapLeafPaint{.color = {.red = 13, .green = 14, .blue = 15}};
constexpr graphics::Size kLeafSize{.width = 11, .height = 13};
constexpr graphics::BitmapFile kBitmap{};
constexpr std::string_view kNestedLeafText = "nested";
const graphics::Font kNestedLeafFont{
    .config = {
        .character_size = {},
        .character_lookup = {},
        .bitmap_path = "/assets/fonts/nested.bmp",
    },
};

graphics::Element make_container(
    graphics::ElementId id,
    graphics::Position position,
    graphics::SolidPaint paint,
    graphics::StackDirection layout_direction,
    graphics::LayoutSystem layout_system = graphics::LayoutSystem::ChildDefinedPositions)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .paint = graphics::Paint{paint},
        .payload = graphics::ContainerElementPayload{
            .layout_direction = layout_direction,
            .layout_system = layout_system,
        },
    };
}

graphics::Element make_rectangle(
    graphics::ElementId id,
    graphics::Position position,
    graphics::SolidPaint paint,
    graphics::Size size)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .paint = graphics::Paint{paint},
        .payload = graphics::FilledRectangleElementPayload{.size = size},
    };
}

graphics::Element make_text(
    graphics::ElementId id,
    graphics::Position position,
    graphics::SolidPaint paint,
    std::string_view text,
    const graphics::Font* font = nullptr)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .paint = graphics::Paint{paint},
        .payload = graphics::TextElementPayload{.text = text, .font = font},
    };
}

graphics::Element make_bitmap(
    graphics::ElementId id,
    graphics::Position position,
    graphics::SolidPaint paint,
    const graphics::BitmapFile* bitmap)
{
    return graphics::Element{
        .id = id,
        .position = position,
        .paint = graphics::Paint{paint},
        .payload = graphics::BitmapElementPayload{.bitmap = bitmap},
    };
}

void expect_optional_index(const std::optional<graphics::ElementNodeIndex>& actual, graphics::ElementNodeIndex expected)
{
    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, expected);
}

void expect_solid_paint(const std::optional<graphics::Paint>& paint, const graphics::SolidPaint& expected)
{
    ASSERT_TRUE(paint.has_value());
    const auto* solid_paint = std::get_if<graphics::SolidPaint>(&*paint);
    ASSERT_NE(solid_paint, nullptr);
    EXPECT_EQ(solid_paint->color.red, expected.color.red);
    EXPECT_EQ(solid_paint->color.green, expected.color.green);
    EXPECT_EQ(solid_paint->color.blue, expected.color.blue);
}

void expect_element(
    const graphics::Element& actual,
    graphics::ElementId id,
    graphics::Position position,
    const graphics::SolidPaint& paint)
{
    EXPECT_EQ(actual.id.value, id.value);
    EXPECT_EQ(actual.position.x, position.x);
    EXPECT_EQ(actual.position.y, position.y);
    expect_solid_paint(actual.paint, paint);
}

}  // namespace

TEST(ElementTreeBuilder, StoresUnsetPaint)
{
    std::array<graphics::ElementTreeNode, 2> storage{};
    graphics::ElementTreeBuilder builder{storage};
    const graphics::Element rectangle{
        .id = kLeafId,
        .position = kLeafPosition,
        .payload = graphics::FilledRectangleElementPayload{.size = kLeafSize},
    };

    ASSERT_TRUE(builder.add_terminal(rectangle));

    const graphics::ElementTree tree = builder.build();
    ASSERT_EQ(tree.nodes.size(), 1u);
    EXPECT_FALSE(tree.nodes[0].element.paint.has_value());
}

TEST(ElementTreeBuilder, BuildsNestedContainerInDeclarationOrder)
{
    std::array<graphics::ElementTreeNode, 8> storage{};
    graphics::ElementTreeBuilder builder{storage};

    const bool built = builder.add_container(
        make_container(kRootId, kRootPosition, kRootPaint, graphics::StackDirection::TopToBottom),
        [](auto& children) {
            EXPECT_TRUE(children.add_terminal(
                make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize)));
            EXPECT_TRUE(children.add_container(
                make_container(
                    kNestedContainerId,
                    kNestedContainerPosition,
                    kNestedContainerPaint,
                    graphics::StackDirection::LeftToRight,
                    graphics::LayoutSystem::Stacked),
                [](auto& nested_children) {
                    EXPECT_TRUE(nested_children.add_terminal(make_text(
                        kNestedLeafId, kNestedLeafPosition, kNestedLeafPaint, kNestedLeafText, &kNestedLeafFont)));
                }));
            EXPECT_TRUE(children.add_terminal(
                make_bitmap(kBitmapLeafId, kBitmapLeafPosition, kBitmapLeafPaint, &kBitmap)));
        });
    ASSERT_TRUE(built);

    const graphics::ElementTree tree = builder.build();
    ASSERT_EQ(tree.nodes.size(), 5u);
    EXPECT_LT(tree.nodes.size(), storage.size());
    EXPECT_EQ(tree.root, graphics::ElementNodeIndex{0});
    EXPECT_EQ(tree.nodes.data(), storage.data());

    expect_element(tree.nodes[0].element, kRootId, kRootPosition, kRootPaint);
    const auto* root_container = std::get_if<graphics::ContainerElementPayload>(&tree.nodes[0].element.payload);
    ASSERT_NE(root_container, nullptr);
    EXPECT_EQ(root_container->layout_direction, graphics::StackDirection::TopToBottom);
    EXPECT_EQ(root_container->layout_system, graphics::LayoutSystem::ChildDefinedPositions);
    expect_optional_index(tree.nodes[0].first_child, 1);
    EXPECT_FALSE(tree.nodes[0].next_sibling.has_value());

    expect_element(tree.nodes[1].element, kLeafId, kLeafPosition, kLeafPaint);
    const auto* leaf = std::get_if<graphics::FilledRectangleElementPayload>(&tree.nodes[1].element.payload);
    ASSERT_NE(leaf, nullptr);
    EXPECT_EQ(leaf->size.width, kLeafSize.width);
    EXPECT_EQ(leaf->size.height, kLeafSize.height);
    EXPECT_FALSE(tree.nodes[1].first_child.has_value());
    expect_optional_index(tree.nodes[1].next_sibling, 2);

    expect_element(tree.nodes[2].element, kNestedContainerId, kNestedContainerPosition, kNestedContainerPaint);
    const auto* nested_container = std::get_if<graphics::ContainerElementPayload>(&tree.nodes[2].element.payload);
    ASSERT_NE(nested_container, nullptr);
    EXPECT_EQ(nested_container->layout_direction, graphics::StackDirection::LeftToRight);
    EXPECT_EQ(nested_container->layout_system, graphics::LayoutSystem::Stacked);
    expect_optional_index(tree.nodes[2].first_child, graphics::ElementNodeIndex{3});
    expect_optional_index(tree.nodes[2].next_sibling, graphics::ElementNodeIndex{4});

    expect_element(tree.nodes[3].element, kNestedLeafId, kNestedLeafPosition, kNestedLeafPaint);
    const auto* nested_leaf = std::get_if<graphics::TextElementPayload>(&tree.nodes[3].element.payload);
    ASSERT_NE(nested_leaf, nullptr);
    EXPECT_EQ(nested_leaf->text, kNestedLeafText);
    EXPECT_EQ(nested_leaf->font, &kNestedLeafFont);
    EXPECT_FALSE(tree.nodes[3].first_child.has_value());
    EXPECT_FALSE(tree.nodes[3].next_sibling.has_value());

    expect_element(tree.nodes[4].element, kBitmapLeafId, kBitmapLeafPosition, kBitmapLeafPaint);
    const auto* bitmap_leaf = std::get_if<graphics::BitmapElementPayload>(&tree.nodes[4].element.payload);
    ASSERT_NE(bitmap_leaf, nullptr);
    EXPECT_EQ(bitmap_leaf->bitmap, &kBitmap);
    EXPECT_FALSE(tree.nodes[4].first_child.has_value());
    EXPECT_FALSE(tree.nodes[4].next_sibling.has_value());
}

TEST(ElementTreeBuilder, SingleLeafRootSpansOnlyUsedStorage)
{
    std::array<graphics::ElementTreeNode, 5> storage{};
    graphics::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(builder.add_terminal(make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize)));

    const graphics::ElementTree tree = builder.build();
    EXPECT_EQ(tree.root, graphics::ElementNodeIndex{0});
    EXPECT_EQ(tree.nodes.size(), 1u);
    EXPECT_LT(tree.nodes.size(), storage.size());
    EXPECT_EQ(tree.nodes.data(), storage.data());
    expect_element(tree.nodes[0].element, kLeafId, kLeafPosition, kLeafPaint);
}

TEST(ElementTreeBuilder, RejectsSecondRoot)
{
    const auto root =
        make_container(kRootId, kRootPosition, kRootPaint, graphics::StackDirection::TopToBottom);
    const auto leaf = make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize);
    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};

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
    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};

    bool callback_ran = false;
    EXPECT_FALSE(builder.add_container(leaf, [&](auto&) { callback_ran = true; }));
    EXPECT_FALSE(callback_ran);
    EXPECT_FALSE(builder.add_terminal(leaf));
}

TEST(ElementTreeBuilder, RejectsWhenNodeStorageIsExhausted)
{
    const auto root =
        make_container(kRootId, kRootPosition, kRootPaint, graphics::StackDirection::TopToBottom);
    const auto leaf = make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize);
    std::array<graphics::ElementTreeNode, 1> storage{};
    graphics::ElementTreeBuilder builder{storage};

    EXPECT_FALSE(builder.add_container(root, [&](auto& children) {
        EXPECT_FALSE(children.add_terminal(leaf));
    }));
    EXPECT_FALSE(builder.add_terminal(leaf));
}

TEST(ElementTreeBuilder, RejectsContainerPassedToAddTerminal)
{
    const auto root =
        make_container(kRootId, kRootPosition, kRootPaint, graphics::StackDirection::TopToBottom);
    const auto leaf = make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize);
    std::array<graphics::ElementTreeNode, 4> storage{};
    graphics::ElementTreeBuilder builder{storage};

    EXPECT_FALSE(builder.add_terminal(root));
    EXPECT_FALSE(builder.add_terminal(leaf));
}

TEST(ElementTreeBuilder, BuildReturnsRootAndUsedNodesAfterSuccess)
{
    std::array<graphics::ElementTreeNode, 6> storage{};
    graphics::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(builder.add_container(
        make_container(kRootId, kRootPosition, kRootPaint, graphics::StackDirection::LeftToRight),
        [](auto& children) {
            EXPECT_TRUE(children.add_terminal(make_rectangle(kLeafId, kLeafPosition, kLeafPaint, kLeafSize)));
            EXPECT_TRUE(children.add_terminal(
                make_text(kNestedLeafId, kNestedLeafPosition, kNestedLeafPaint, kNestedLeafText)));
        }));

    const graphics::ElementTree tree = builder.build();
    EXPECT_EQ(tree.root, graphics::ElementNodeIndex{0});
    EXPECT_EQ(tree.nodes.size(), 3u);
    EXPECT_LT(tree.nodes.size(), storage.size());
    EXPECT_EQ(tree.nodes.data(), storage.data());
    expect_optional_index(tree.nodes[0].first_child, graphics::ElementNodeIndex{1});
    expect_optional_index(tree.nodes[1].next_sibling, graphics::ElementNodeIndex{2});
    EXPECT_FALSE(tree.nodes[2].next_sibling.has_value());
}
