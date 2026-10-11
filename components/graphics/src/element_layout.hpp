#pragma once

#include <span>

#include "element_tree.hpp"

namespace graphics {

// Writes layout for every node reachable from root.
void lay_out_element_tree(std::span<ElementTreeNode> nodes, ElementNodeIndex root);

}  // namespace graphics
