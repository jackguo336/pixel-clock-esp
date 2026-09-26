#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>

#include "display/element_tree.hpp"
#include "display/elements.hpp"
#include "display/view_data.hpp"
#include "display/views.hpp"
#include "gtest/gtest.h"

namespace {

constexpr display::ElementId kOuterId{.value = 19};
constexpr display::ElementId kRootId{.value = 20};
constexpr display::ElementId kBoxId{.value = 21};
constexpr display::ElementId kLabelId{.value = 22};
constexpr display::Position kOuterPosition{.x = 0, .y = 1};
constexpr display::Position kRootPosition{.x = 1, .y = 2};
constexpr display::Position kBoxPosition{.x = 3, .y = 4};
constexpr display::Position kLabelPosition{.x = 5, .y = 6};
constexpr display::SolidPaint kOuterPaint{.color = {.red = 10, .green = 20, .blue = 30}};
constexpr display::SolidPaint kRootPaint{.color = {.red = 9, .green = 18, .blue = 27}};
constexpr display::SolidPaint kBoxPaint{.color = {.red = 8, .green = 16, .blue = 24}};
constexpr display::SolidPaint kLabelPaint{.color = {.red = 7, .green = 14, .blue = 21}};
constexpr uint16_t kBoxHeight = 3;
constexpr uint32_t kViewRevision = 7;
constexpr std::string_view kLabelText = "revision";

display::Element make_outer_container()
{
    return display::Element{
        .id = kOuterId,
        .position = kOuterPosition,
        .paint = kOuterPaint,
        .payload = display::ContainerElementPayload{.layout_direction = display::StackDirection::LeftToRight},
    };
}

display::Element make_root_container()
{
    return display::Element{
        .id = kRootId,
        .position = kRootPosition,
        .paint = kRootPaint,
        .payload = display::ContainerElementPayload{.layout_direction = display::StackDirection::TopToBottom},
    };
}

display::Element make_revision_box(uint32_t revision)
{
    return display::Element{
        .id = kBoxId,
        .position = kBoxPosition,
        .paint = kBoxPaint,
        .payload = display::FilledRectangleElementPayload{
            .size =
                {
                    .width = static_cast<uint16_t>(revision),
                    .height = kBoxHeight,
                },
        },
    };
}

display::Element make_label()
{
    return display::Element{
        .id = kLabelId,
        .position = kLabelPosition,
        .paint = kLabelPaint,
        .payload = display::TextElementPayload{.text = kLabelText},
    };
}

class RevisionTreeView final : public display::View {
public:
    [[nodiscard]] bool build(
        const display::ViewData& view_data,
        display::ElementTreeBuilder& builder) const override
    {
        return builder.add_container(make_root_container(), [&](auto& children) {
            if (!children.add_terminal(make_revision_box(view_data.revision))) {
                return;
            }
            if (!children.add_terminal(make_label())) {
                return;
            }
        });
    }
};

class HostedRevisionTreeView final : public display::View {
public:
    [[nodiscard]] bool build(
        const display::ViewData& view_data,
        display::ElementTreeBuilder& builder) const override
    {
        return builder.add_container(make_outer_container(), [&](auto& children) {
            if (!nested_.build(view_data, children)) {
                return;
            }
        });
    }

private:
    RevisionTreeView nested_{};
};

class TwoRootView final : public display::View {
public:
    [[nodiscard]] bool build(
        const display::ViewData&,
        display::ElementTreeBuilder& builder) const override
    {
        if (!builder.add_terminal(make_revision_box(kViewRevision))) {
            return false;
        }
        if (!builder.add_terminal(make_label())) {
            return false;
        }
        return true;
    }
};

class NonContainerView final : public display::View {
public:
    [[nodiscard]] bool build(
        const display::ViewData&,
        display::ElementTreeBuilder& builder) const override
    {
        return builder.add_container(make_label(), [](auto&) {});
    }
};

class OversizedTreeView final : public display::View {
public:
    [[nodiscard]] bool build(
        const display::ViewData& view_data,
        display::ElementTreeBuilder& builder) const override
    {
        return builder.add_container(make_root_container(), [&](auto& children) {
            if (!children.add_terminal(make_revision_box(view_data.revision))) {
                return;
            }
            if (!children.add_terminal(make_label())) {
                return;
            }
        });
    }
};

}  // namespace

TEST(View, IsAbstract)
{
    static_assert(std::is_abstract_v<display::View>);
}

TEST(View, BuildReadsViewDataAndAddsScopedElements)
{
    const RevisionTreeView view;
    const display::ViewData view_data{.revision = kViewRevision};
    std::array<display::ElementTreeNode, 8> storage{};
    display::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(view.build(view_data, builder));

    const display::ElementTree tree = builder.get_tree();
    ASSERT_EQ(tree.nodes.size(), 3u);
    EXPECT_EQ(tree.root, 0);
    EXPECT_EQ(tree.nodes[0].element.id.value, kRootId.value);
    ASSERT_TRUE(tree.nodes[0].first_child.has_value());
    EXPECT_EQ(*tree.nodes[0].first_child, 1);
    ASSERT_TRUE(tree.nodes[1].next_sibling.has_value());
    EXPECT_EQ(*tree.nodes[1].next_sibling, 2);

    const auto* box = std::get_if<display::FilledRectangleElementPayload>(&tree.nodes[1].element.payload);
    ASSERT_NE(box, nullptr);
    EXPECT_EQ(box->size.width, static_cast<uint16_t>(kViewRevision));
    EXPECT_EQ(box->size.height, kBoxHeight);

    const auto* label = std::get_if<display::TextElementPayload>(&tree.nodes[2].element.payload);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->text, kLabelText);
}

TEST(View, BuildCanBeCalledInsideContainer)
{
    const HostedRevisionTreeView view;
    const display::ViewData view_data{.revision = kViewRevision};
    std::array<display::ElementTreeNode, 8> storage{};
    display::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(view.build(view_data, builder));

    const display::ElementTree tree = builder.get_tree();
    ASSERT_EQ(tree.nodes.size(), 4u);
    EXPECT_EQ(tree.root, 0);
    EXPECT_EQ(tree.nodes[0].element.id.value, kOuterId.value);
    ASSERT_TRUE(tree.nodes[0].first_child.has_value());
    EXPECT_EQ(*tree.nodes[0].first_child, 1);
    EXPECT_EQ(tree.nodes[1].element.id.value, kRootId.value);
    ASSERT_TRUE(tree.nodes[1].first_child.has_value());
    EXPECT_EQ(*tree.nodes[1].first_child, 2);
    ASSERT_TRUE(tree.nodes[2].next_sibling.has_value());
    EXPECT_EQ(*tree.nodes[2].next_sibling, 3);

    const auto* box = std::get_if<display::FilledRectangleElementPayload>(&tree.nodes[2].element.payload);
    ASSERT_NE(box, nullptr);
    EXPECT_EQ(box->size.width, static_cast<uint16_t>(kViewRevision));
    EXPECT_EQ(box->size.height, kBoxHeight);

    const auto* label = std::get_if<display::TextElementPayload>(&tree.nodes[3].element.payload);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->text, kLabelText);
}

TEST(View, BuildPropagatesBuilderFailures)
{
    const display::ViewData view_data{.revision = kViewRevision};

    {
        const TwoRootView view;
        std::array<display::ElementTreeNode, 4> storage{};
        display::ElementTreeBuilder builder{storage};
        EXPECT_FALSE(view.build(view_data, builder));
    }

    {
        const NonContainerView view;
        std::array<display::ElementTreeNode, 4> storage{};
        display::ElementTreeBuilder builder{storage};
        EXPECT_FALSE(view.build(view_data, builder));
    }

    {
        const OversizedTreeView view;
        std::array<display::ElementTreeNode, 2> storage{};
        display::ElementTreeBuilder builder{storage};
        EXPECT_FALSE(view.build(view_data, builder));
    }
}
