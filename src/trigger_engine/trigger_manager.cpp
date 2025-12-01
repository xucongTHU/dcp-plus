//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#include "trigger_manager.h"
#include "common/log/logger.h"
#include "trigger_engine/common/trigger_checker.h"

namespace dcl {
namespace trigger {

void TriggerManager::InitTriggerChecker(std::shared_ptr<RuleTrigger> trigger) {
    if (!trigger) return;
    trigger->registerVariableGetter("speed", []() -> TriggerConditionChecker::Value {
        return 65.0; // km/h
    });

    trigger->registerVariableGetter("distance_to_lead", []() -> TriggerConditionChecker::Value {
        return 15.0; // meters
    });

    trigger->registerVariableGetter("collision_risk", []() -> TriggerConditionChecker::Value {
        return true;
    });

    trigger->registerVariableGetter("takeover_requested", []() -> TriggerConditionChecker::Value {
        return true;
    });
}
std::shared_ptr<RuleTrigger> TriggerManager::createTrigger(const strategy::Strategy& strategy) {
    auto trigger = std::make_shared<RuleTrigger>();
    
    triggers_[strategy.triggerId] = trigger;
    
    LOG_INFO("Created RuleTrigger: %s", strategy.triggerId.c_str());
    return trigger;
}

std::shared_ptr<RuleTrigger> TriggerManager::getTrigger(const std::string& trigger_id) const {
    auto it = triggers_.find(trigger_id);
    if (it != triggers_.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace trigger
} // namespace shadow