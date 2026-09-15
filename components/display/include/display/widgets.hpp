#pragma once

#include "display/elements.hpp"
#include "display/scene_data.hpp"

namespace display {

// Nonvisual producer of an Element. Widgets are stateless with respect to
// layout, paint, and animation; those properties belong to the returned
// Element.
//
// Views inside that Element (for example TextElement::text) must reference
// storage that outlives later scene resolution.
class Widget {
public:
    virtual ~Widget() = default;

    [[nodiscard]] virtual Element resolve(const SceneData& scene_data) const = 0;
};

}  // namespace display
