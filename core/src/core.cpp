#include "core/core.h"
#include <iostream>

namespace core {
    void CoreModule::init() {
        std::cout << "[core] initialized" << std::endl;
    }
}
