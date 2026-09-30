#include "font_manager.hpp"
#include "gtest/gtest.h"

TEST(FontManager, InstanceReturnsTheSameObject)
{
    display::FontManager& first = display::FontManager::instance();
    display::FontManager& second = display::FontManager::instance();
    EXPECT_EQ(&first, &second);
}

TEST(FontManager, UnknownFontIsRejected)
{
    display::FontManager& manager = display::FontManager::instance();
    constexpr auto unknown = static_cast<display::FontId>(255);
    EXPECT_EQ(manager.load(unknown), display::FontLoadStatus::UnknownFont);
    EXPECT_FALSE(manager.is_loaded(unknown));
    EXPECT_FALSE(manager.font(unknown).is_valid());
    EXPECT_NE(&manager.font(display::FontId::English7x3), &manager.font(unknown));
}
