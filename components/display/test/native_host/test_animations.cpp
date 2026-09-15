#include <array>
#include <variant>

#include "display/animations.hpp"
#include "display/elements.hpp"
#include "gtest/gtest.h"

TEST(AnimationTiming, DefaultsToNoDelaysNoIntervalNoTransitionAndOnce)
{
    const display::AnimationTiming timing{};

    EXPECT_EQ(timing.delay_us, 0);
    EXPECT_EQ(timing.interval_us, 0);
    EXPECT_EQ(timing.transition_duration_us, 0);
    EXPECT_EQ(timing.repeat, display::RepeatMode::Once);
}

TEST(Animation, DefaultsToFlashPayload)
{
    const display::Animation animation{};

    EXPECT_NE(std::get_if<display::FlashAnimation>(&animation.payload), nullptr);
}

TEST(Element, HasNoAnimationsByDefault)
{
    const display::Element element{};

    EXPECT_TRUE(element.animations.empty());
}

TEST(Element, MultipleElementsShareImmutableAnimationStorage)
{
    const display::Animation animation{
        .timing =
            {
                .delay_us = 1000,
                .interval_us = 250000,
                .transition_duration_us = 50000,
                .repeat = display::RepeatMode::Loop,
            },
        .payload = display::FlashAnimation{},
    };
    const std::array<display::Animation, 1> animations{animation};

    const display::Element first{
        .id = {.value = 1},
        .animations = animations,
    };
    const display::Element second{
        .id = {.value = 2},
        .animations = animations,
    };

    ASSERT_EQ(first.animations.size(), 1u);
    ASSERT_EQ(second.animations.size(), 1u);
    EXPECT_EQ(first.animations.data(), animations.data());
    EXPECT_EQ(second.animations.data(), animations.data());
    EXPECT_EQ(first.animations.data(), second.animations.data());
    EXPECT_EQ(first.animations[0].timing.interval_us, animation.timing.interval_us);
    EXPECT_EQ(second.animations[0].timing.repeat, display::RepeatMode::Loop);
    EXPECT_NE(std::get_if<display::FlashAnimation>(&first.animations[0].payload), nullptr);
}

TEST(SequenceAnimation, FramesReferenceElementIds)
{
    const std::array<display::ElementId, 2> frames{
        display::ElementId{.value = 3},
        display::ElementId{.value = 5},
    };
    const display::SequenceAnimation sequence{
        .frames = frames,
        .transition = display::TransitionKind::Fade,
    };
    const display::Animation animation{
        .payload = sequence,
    };

    const auto* payload = std::get_if<display::SequenceAnimation>(&animation.payload);
    ASSERT_NE(payload, nullptr);
    ASSERT_EQ(payload->frames.size(), 2u);
    EXPECT_EQ(payload->frames.data(), frames.data());
    EXPECT_EQ(payload->frames[0].value, 3);
    EXPECT_EQ(payload->frames[1].value, 5);
    EXPECT_EQ(payload->transition, display::TransitionKind::Fade);
}
