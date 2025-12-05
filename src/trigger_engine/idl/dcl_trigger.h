#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace dcl::trigger {

struct TriggerContext {
    std::string businessType;
    std::string triggerId;
    int64_t triggerTimestamp;
    std::string triggerDesc;
};

}