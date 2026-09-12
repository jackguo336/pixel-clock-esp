#include <array>
#include <cstdint>
#include <type_traits>
#include <variant>

#include "display/elements.hpp"
#include "display/scene_data.hpp"
#include "display/widgets.hpp"
#include "gtest/gtest.h"

namespace {

constexpr display::ElementId kResolvedId{.value = 12};
constexpr display::Position kResolvedPosition{.x = 4, .y = 6};
constexpr display::SolidPaint kResolvedPaint{.color = {.red = 9, .green = 18, .blue = 27}};
constexpr uint16_t kResolvedHeight = 3;
constexpr uint32_t kSceneRevision = 7;

class RevisionBoxWidget final : public display::Widget {
public:
    [[nodiscard]] display::Element resolve(const display::SceneData& scene_data) const override
    {
        return display::Element{
            .id = kResolvedId,
            .position = kResolvedPosition,
            .paint = kResolvedPaint,
            .payload = display::FilledRectangleElement{
                .size =
                    {
                        .width = static_cast<uint16_t>(scene_data.revision),
                        .height = kResolvedHeight,
                    },
            },
        };
    }
};

}  // namespace

TEST(Widget, IsAbstract)
{
    static_assert(std::is_abstract_v<display::Widget>);
}

TEST(Widget, ResolveReadsSceneDataAndReturnsElementVisuals)
{
    const RevisionBoxWidget widget;
    const display::SceneData scene_data{.revision = kSceneRevision};

    const display::Element element = widget.resolve(scene_data);

    EXPECT_EQ(element.id.value, kResolvedId.value);
    EXPECT_EQ(element.position.x, kResolvedPosition.x);
    EXPECT_EQ(element.position.y, kResolvedPosition.y);

    const auto* solid_paint = std::get_if<display::SolidPaint>(&element.paint);
    ASSERT_NE(solid_paint, nullptr);
    EXPECT_EQ(solid_paint->color.red, kResolvedPaint.color.red);
    EXPECT_EQ(solid_paint->color.green, kResolvedPaint.color.green);
    EXPECT_EQ(solid_paint->color.blue, kResolvedPaint.color.blue);

    const auto* rectangle = std::get_if<display::FilledRectangleElement>(&element.payload);
    ASSERT_NE(rectangle, nullptr);
    EXPECT_EQ(rectangle->size.width, static_cast<uint16_t>(kSceneRevision));
    EXPECT_EQ(rectangle->size.height, kResolvedHeight);
}

TEST(ContainerChild, StoresElementAndWidgetIdsInOrder)
{
    constexpr display::ElementId kFirstElement{.value = 3};
    constexpr display::WidgetId kWidget{.value = 8};
    constexpr display::ElementId kSecondElement{.value = 5};
    const std::array<display::ContainerChild, 3> stored{kFirstElement, kWidget, kSecondElement};

    const display::ContainerElement container{
        .layout_direction = display::LayoutDirection::LeftToRight,
        .children = stored,
    };

    ASSERT_EQ(container.children.size(), 3u);

    const auto* first = std::get_if<display::ElementId>(&container.children[0]);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->value, kFirstElement.value);

    const auto* widget_id = std::get_if<display::WidgetId>(&container.children[1]);
    ASSERT_NE(widget_id, nullptr);
    EXPECT_EQ(widget_id->value, kWidget.value);

    const auto* second = std::get_if<display::ElementId>(&container.children[2]);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(second->value, kSecondElement.value);
}
