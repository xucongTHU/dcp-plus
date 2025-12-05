#pragma once

#include <string>
#include <vector>
#include <memory>
#include "nlohmann/json.hpp"

#include "common/config/app_config.h"
#include "../common/trigger_checker.h"

namespace dcl {
namespace trigger {

struct StrategyRule {
    std::string condition;
    std::string action;
    double priority;
};

struct StrategyConfig {
    std::vector<StrategyRule> rules;
    int max_concurrent_triggers;
};

class StrategyParser {
public:
    StrategyParser() = default;
    ~StrategyParser() = default;

    bool LoadConfigFromFile(const std::string& filepath, StrategyConfig& config);
    bool SaveConfigToFile(const std::string& filepath, const StrategyConfig& config);

private:
    bool ParseJsonConfig(const nlohmann::json& json_data, StrategyConfig& config);
};

} // namespace trigger
} // namespace dcl