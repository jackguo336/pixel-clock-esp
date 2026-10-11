#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>

#include "element_tree.hpp"
#include "elements.hpp"
#include "view_data.hpp"
#include "views.hpp"
#include "gtest/gtest.h"

namespace {

constexpr graphics::ElementId kOuterId{.value = 19};
constexpr graphics::ElementId kRootId{.value = 20};
constexpr graphics::ElementId kBoxId{.value = 21};
constexpr graphics::ElementId kLabelId{.value = 22};
constexpr graphics::Position kOuterPosition{.x = 0, .y = 1};
constexpr graphics::Position kRootPosition{.x = 1, .y = 2};
constexpr graphics::Position kBoxPosition{.x = 3, .y = 4};
constexpr graphics::Position kLabelPosition{.x = 5, .y = 6};
constexpr graphics::SolidPaint kOuterPaint{.color = {.red = 10, .green = 20, .blue = 30}};
constexpr graphics::SolidPaint kRootPaint{.color = {.red = 9, .green = 18, .blue = 27}};
constexpr graphics::SolidPaint kBoxPaint{.color = {.red = 8, .green = 16, .blue = 24}};
constexpr graphics::SolidPaint kLabelPaint{.color = {.red = 7, .green = 14, .blue = 21}};
constexpr uint16_t kBoxHeight = 3;
constexpr uint32_t kViewRevision = 7;
constexpr std::string_view kLabelText = "revision";

graphics::Element make_outer_container()
{
    return graphics::Element{
        .id = kOuterId,
        .position = kOuterPosition,
        .paint = graphics::Paint{kOuterPaint},
        .payload = graphics::ContainerElementPayload{.layout_direction = graphics::StackDirection::LeftToRight},
    };
}

graphics::Element make_root_container()
{
    return graphics::Element{
        .id = kRootId,
        .position = kRootPosition,
        .paint = graphics::Paint{kRootPaint},
        .payload = graphics::ContainerElementPayload{.layout_direction = graphics::StackDirection::TopToBottom},
    };
}

graphics::Element make_revision_box(uint32_t revision)
{
    return graphics::Element{
        .id = kBoxId,
        .position = kBoxPosition,
        .paint = graphics::Paint{kBoxPaint},
        .payload = graphics::FilledRectangleElementPayload{
            .size =
                {
                    .width = static_cast<uint16_t>(revision),
                    .height = kBoxHeight,
                },
        },
    };
}

graphics::Element make_label()
{
    return graphics::Element{
        .id = kLabelId,
        .position = kLabelPosition,
        .paint = graphics::Paint{kLabelPaint},
        .payload = graphics::TextElementPayload{.text = kLabelText, .font = nullptr},
    };
}

class RevisionTreeView final : public graphics::View {
public:
    [[nodiscard]] bool build(
        const graphics::ViewData& view_data,
        graphics::ElementTreeBuilder& builder) const override
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

class HostedRevisionTreeView final : public graphics::View {
public:
    [[nodiscard]] bool build(
        const graphics::ViewData& view_data,
        graphics::ElementTreeBuilder& builder) const override
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

class TwoRootView final : public graphics::View {
public:
    [[nodiscard]] bool build(
        const graphics::ViewData&,
        graphics::ElementTreeBuilder& builder) const override
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

class NonContainerView final : public graphics::View {
public:
    [[nodiscard]] bool build(
        const graphics::ViewData&,
        graphics::ElementTreeBuilder& builder) const override
    {
        return builder.add_container(make_label(), [](auto&) {});
    }
};

class OversizedTreeView final : public graphics::View {
public:
    [[nodiscard]] bool build(
        const graphics::ViewData& view_data,
        graphics::ElementTreeBuilder& builder) const override
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
    static_assert(std::is_abstract_v<graphics::View>);
}

TEST(View, BuildReadsViewDataAndAddsScopedElements)
{
    const RevisionTreeView view;
    const graphics::ViewData view_data{.revision = kViewRevision};
    std::array<graphics::ElementTreeNode, 8> storage{};
    graphics::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(view.build(view_data, builder));

    const graphics::ElementTree tree = builder.build();
    ASSERT_EQ(tree.nodes.size(), 3u);
    EXPECT_EQ(tree.root, 0);
    EXPECT_EQ(tree.nodes[0].element.id.value, kRootId.value);
    ASSERT_TRUE(tree.nodes[0].first_child.has_value());
    EXPECT_EQ(*tree.nodes[0].first_child, 1);
    ASSERT_TRUE(tree.nodes[1].next_sibling.has_value());
    EXPECT_EQ(*tree.nodes[1].next_sibling, 2);

    const auto* box = std::get_if<graphics::FilledRectangleElementPayload>(&tree.nodes[1].element.payload);
    ASSERT_NE(box, nullptr);
    EXPECT_EQ(box->size.width, static_cast<uint16_t>(kViewRevision));
    EXPECT_EQ(box->size.height, kBoxHeight);

    const auto* label = std::get_if<graphics::TextElementPayload>(&tree.nodes[2].element.payload);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->text, kLabelText);
    EXPECT_EQ(label->font, nullptr);
}

TEST(View, BuildCanBeCalledInsideContainer)
{
    const HostedRevisionTreeView view;
    const graphics::ViewData view_data{.revision = kViewRevision};
    std::array<graphics::ElementTreeNode, 8> storage{};
    graphics::ElementTreeBuilder builder{storage};

    ASSERT_TRUE(view.build(view_data, builder));

    const graphics::ElementTree tree = builder.build();
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

    const auto* box = std::get_if<graphics::FilledRectangleElementPayload>(&tree.nodes[2].element.payload);
    ASSERT_NE(box, nullptr);
    EXPECT_EQ(box->size.width, static_cast<uint16_t>(kViewRevision));
    EXPECT_EQ(box->size.height, kBoxHeight);

    const auto* label = std::get_if<graphics::TextElementPayload>(&tree.nodes[3].element.payload);
    ASSERT_NE(label, nullptr);
    EXPECT_EQ(label->text, kLabelText);
    EXPECT_EQ(label->font, nullptr);
}

TEST(View, BuildPropagatesBuilderFailures)
{
    const graphics::ViewData view_data{.revision = kViewRevision};

    {
        const TwoRootView view;
        std::array<graphics::ElementTreeNode, 4> storage{};
        graphics::ElementTreeBuilder builder{storage};
        EXPECT_FALSE(view.build(view_data, builder));
    }

    {
        const NonContainerView view;
        std::array<graphics::ElementTreeNode, 4> storage{};
        graphics::ElementTreeBuilder builder{storage};
        EXPECT_FALSE(view.build(view_data, builder));
    }

    {
        const OversizedTreeView view;
        std::array<graphics::ElementTreeNode, 2> storage{};
        graphics::ElementTreeBuilder builder{storage};
        EXPECT_FALSE(view.build(view_data, builder));
    }
}
