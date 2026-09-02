#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace platform {

struct SpinLock {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
};

inline SemaphoreHandle_t as_sem(void* handle)
{
    return static_cast<SemaphoreHandle_t>(handle);
}

inline SpinLock* as_lock(void* handle)
{
    return static_cast<SpinLock*>(handle);
}

}  // namespace platform
