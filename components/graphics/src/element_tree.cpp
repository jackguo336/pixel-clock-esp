#include "element_tree.hpp"

#include <cassert>
#include <limits>
#include <variant>

#include "element_layout.hpp"

namespace graphics {

ElementTreeBuilder::ElementTreeBuilder(std::span<ElementTreeNode> storage)
    : storage_(storage)
{
}

std::optional<ElementNodeIndex> ElementTreeBuilder::create_node(Element element)
{
    constexpr std::size_t kMaxNodeCount =
        static_cast<std::size_t>(std::numeric_limits<ElementNodeIndex>::max()) + 1;
    if (next_free_slot_index_ >= storage_.size() || next_free_slot_index_ >= kMaxNodeCount) {
        failed_to_add_element_ = true;
        return std::nullopt;
    }

    const auto index = static_cast<ElementNodeIndex>(next_free_slot_index_);
    ElementTreeNode& node = storage_[index];
    node.element = element;
    // Start with no children or sibling links for the newly created node.
    node.first_child.reset();
    node.next_sibling.reset();
    node.layout = ElementLayout{};
    ++next_free_slot_index_;
    return index;
}

void ElementTreeBuilder::link_node_to_tree(ElementNodeIndex index)
{
    // 1. If no parent, tree is empty, so set node as root.
    // 2. If parent exists and has no children, set node as first child.
    // 3. If parent exists and has children, set node as next sibling of last child.
    if (parent_.has_value()) {
        ElementTreeNode& parent_node = storage_[*parent_];
        if (!parent_node.first_child.has_value()) {
            parent_node.first_child = index;
        } else if (last_sibling_.has_value()) {
            storage_[*last_sibling_].next_sibling = index;
        }
        last_sibling_ = index;
    } else {
        root_ = index;
    }
}

bool ElementTreeBuilder::is_terminal_element(const Element& element)
{
    return std::get_if<TextElementPayload>(&element.payload) != nullptr
        || std::get_if<FilledRectangleElementPayload>(&element.payload) != nullptr
        || std::get_if<BitmapElementPayload>(&element.payload) != nullptr;
}

bool ElementTreeBuilder::add(Element element)
{
    if (failed_to_add_element_) {
        return false;
    }
    if (!parent_.has_value() && root_.has_value()) {
        failed_to_add_element_ = true;
        return false;
    }

    const std::optional<ElementNodeIndex> created_index = create_node(element);
    if (!created_index.has_value()) {
        return false;
    }
    link_node_to_tree(*created_index);
    return true;
}

bool ElementTreeBuilder::add_terminal(Element element)
{
    if (failed_to_add_element_) {
        return false;
    }
    if (!is_terminal_element(element)) {
        failed_to_add_element_ = true;
        return false;
    }
    return add(element);
}

ElementTree ElementTreeBuilder::build()
{
    assert(!failed_to_add_element_ && root_.has_value());
    lay_out_element_tree(storage_.first(next_free_slot_index_), *root_);
    return ElementTree{
        .root = *root_,
        .nodes = storage_.first(next_free_slot_index_),
    };
}

}  // namespace graphics
