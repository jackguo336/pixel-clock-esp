#include "font_manager.hpp"
#include "gtest/gtest.h"

TEST(FontManager, InstanceReturnsTheSameObject)
{
    graphics::FontManager& first = graphics::FontManager::instance();
    graphics::FontManager& second = graphics::FontManager::instance();
    EXPECT_EQ(&first, &second);
}

TEST(FontManager, UnknownFontIsRejected)
{
    graphics::FontManager& manager = graphics::FontManager::instance();
    constexpr auto unknown = static_cast<graphics::FontId>(255);
    EXPECT_EQ(manager.load(unknown), graphics::FontLoadStatus::UnknownFont);
    EXPECT_FALSE(manager.is_loaded(unknown));
    EXPECT_FALSE(manager.font(unknown).is_valid());
    EXPECT_NE(&manager.font(graphics::FontId::English7x3), &manager.font(unknown));
}
