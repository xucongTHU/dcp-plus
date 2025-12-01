//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#ifndef TRIGGER_MANAGER_H
#define TRIGGER_MANAGER_H

#include "rule_trigger.h"
#include "strategy_config.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>

namespace dcl {
namespace trigger {


class TriggerManager {
public:
    TriggerManager() = default;
    ~TriggerManager() = default;

    void InitTriggerChecker(std::shared_ptr<RuleTrigger> trigger);
    void InitTriggerScehduler(std::shared_ptr<RuleTrigger> trigger);

    std::shared_ptr<RuleTrigger> createTrigger(const Strategy& strategy);

    std::shared_ptr<RuleTrigger> getTrigger(const std::string& trigger_id) const;

private:
    std::unordered_map<std::string, 
        std::function<std::function<TriggerConditionChecker::Value()>()>> variable_getter_factories_;
    
    std::unordered_map<std::string, std::shared_ptr<RuleTrigger>> triggers_;
};

} 
} 

#endif //TRIGGER_MANAGER_H