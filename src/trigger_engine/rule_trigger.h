//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#pragma once

#include "trigger_base.h"
#include "trigger_engine/common/trigger_checker.h"
#include "state_machine/state_machine.h"

#include <unordered_map>
#include <string>
#include <functional>
#include <memory>

namespace dcl {
namespace trigger {


class RuleTrigger : public TriggerBase {
public:
    RuleTrigger();
    ~RuleTrigger() override = default;

    bool proc() override;
    bool checkCondition() override;
    void registerVariableGetter(const std::string& var_name, 
                                std::function<TriggerConditionChecker::Value()> getter) override;
    void OnMessageReceived(const std::string& topic, const TRawMessagePtr& subject) override;                            

private:
    TriggerConditionChecker trigger_checker_;
    SystemState current_state_;
    std::unordered_map<std::string, std::function<TriggerConditionChecker::Value()>> variable_getters_;
    
    mutable std::unordered_map<std::string, TriggerConditionChecker::Value> current_variables_;
};

} 
}