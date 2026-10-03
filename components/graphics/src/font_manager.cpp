#include "font_manager.hpp"

#include "bitmap_file_loader.hpp"

namespace graphics {
namespace {

const Font kEmptyFont{};

}  // namespace

FontManager& FontManager::instance()
{
    static FontManager manager;
    return manager;
}

bool FontManager::is_known(FontId id)
{
    return static_cast<std::size_t>(id) < kFontCount;
}

FontLoadStatus FontManager::load()
{
    FontLoadStatus first_failure = FontLoadStatus::Ok;
    for (std::size_t index = 0; index < kFontCount; ++index) {
        const FontLoadStatus status = load(static_cast<FontId>(index));
        if (first_failure == FontLoadStatus::Ok && status != FontLoadStatus::Ok) {
            first_failure = status;
        }
    }
    return first_failure;
}

FontLoadStatus FontManager::load(FontId id)
{
    if (!is_known(id)) {
        return FontLoadStatus::UnknownFont;
    }

    const std::size_t index = static_cast<std::size_t>(id);
    ManagedFont& managed_font = fonts_[index];
    if (managed_font.loaded) {
        return FontLoadStatus::Ok;
    }

    MutableBitmapFile destination{};
    destination.size = managed_font.config.bitmap_size();
    destination.pixels = managed_font.pixels;

    const BitmapFileLoader loader;
    const BitmapLoadStatus load_status = loader.load(managed_font.config.bitmap_path, destination);
    if (load_status != BitmapLoadStatus::Ok) {
        return FontLoadStatus::BitmapLoadFailed;
    }

    managed_font.font = Font{
        .config = managed_font.config,
        .bitmap = destination.as_read_only(),
    };
    if (!managed_font.font.is_valid()) {
        managed_font.font = Font{};
        return FontLoadStatus::InvalidBitmap;
    }

    managed_font.loaded = true;
    return FontLoadStatus::Ok;
}

bool FontManager::is_loaded(FontId id) const
{
    if (!is_known(id)) {
        return false;
    }
    return fonts_[static_cast<std::size_t>(id)].loaded;
}

const Font& FontManager::font(FontId id) const
{
    if (!is_known(id)) {
        return kEmptyFont;
    }
    return fonts_[static_cast<std::size_t>(id)].font;
}

}  // namespace graphics
