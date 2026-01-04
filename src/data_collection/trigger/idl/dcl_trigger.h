#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace dcp::trigger {

struct TriggerContext {
    std::string businessType;
    std::string triggerId;
    int64_t triggerTimestamp;
    std::string triggerDesc;
};

}