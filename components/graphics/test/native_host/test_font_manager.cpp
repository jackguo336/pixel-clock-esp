#include <unistd.h>

#include "font_manager.hpp"
#include "gtest/gtest.h"

TEST(FontManager, InstanceIsUnique)
{
    display::FontManager& first = display::FontManager::instance();
    display::FontManager& second = display::FontManager::instance();
    EXPECT_EQ(&first, &second);
}

TEST(FontManager, MissingAtlasStaysUnloaded)
{
    if (access("/assets/font_en_7x3.bmp", F_OK) == 0) {
        GTEST_SKIP() << "built-in font atlas is present on this host";
    }

    display::FontManager& manager = display::FontManager::instance();
    EXPECT_EQ(manager.load(display::FontId::English7x3), display::FontLoadStatus::BitmapLoadFailed);
    EXPECT_FALSE(manager.is_loaded(display::FontId::English7x3));
    EXPECT_FALSE(manager.font(display::FontId::English7x3).is_valid());

    EXPECT_EQ(manager.load(), display::FontLoadStatus::BitmapLoadFailed);
    EXPECT_FALSE(manager.is_loaded(display::FontId::English7x3));
    EXPECT_FALSE(manager.font(display::FontId::English7x3).is_valid());
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
