#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace dcl::trigger {

enum class TriggerState : int32_t {
    None =0,
    UnTrigger,
    Triggered,
};

struct TriggerContext {
    std::string businessType;
    std::string triggerId;
    int64_t triggerTimestamp;
    std::string triggerName;
    // TriggerState triggerState;
};

}