#pragma once

#include "platform/message.hpp"

namespace platform {

class Component {
public:
    virtual ~Component() = default;

    virtual const char* name() const = 0;
    virtual void start() {}
    virtual void stop() {}
    virtual void on_event(const Event& event) { (void)event; }
    virtual void on_command(const Command& command) { (void)command; }
};

}  // namespace platform
