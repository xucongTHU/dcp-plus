//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#pragma once

#include "channel/observer.h"
#include "trigger_engine/common/trigger_checker.h"

#include <unordered_map>
#include <string>
#include <functional>

namespace dcl {
namespace trigger {


class RuleTrigger : public Observer {
public:
    RuleTrigger();
    ~RuleTrigger() override = default;

    bool proc();
    bool checkCondition();
    std::string getTriggerName() const { return trigger_name_; }
    // void OnMessageReceived(const std::string& topic, const TRawMessagePtr& msg) override;
    void registerVariableGetter(const std::string& var_name, 
                                std::function<TriggerConditionChecker::Value()> getter);

private:
    std::string trigger_id_;
    std::string trigger_name_;
    std::string trigger_condition_;
    std::string business_type_;
    TriggerConditionChecker trigger_checker_;
    
    std::unordered_map<std::string, std::function<TriggerConditionChecker::Value()>> variable_getters_;
    
    mutable std::unordered_map<std::string, TriggerConditionChecker::Value> current_variables_;
};

REGISTER_TRIGGER(RuleTrigger)

} 
} 