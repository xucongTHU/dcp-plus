//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#pragma once

#include "trigger_base.h"
#include "trigger/common/trigger_checker.h"
#include "state_machine/state_machine.h"

#include <unordered_map>
#include <string>
#include <functional>
#include <memory>

namespace dcp::trigger
{

class RuleTrigger : public TriggerBase {
public:
    RuleTrigger();
    ~RuleTrigger() override = default;

    bool proc() override;
    bool checkCondition() override;
    void registerVariableGetter(const std::string& var_name,
                                std::function<TriggerChecker::Value()> getter) override;
    void OnMessageReceived(const std::string& topic, const rclcpp::SerializedMessage& subject) override;

private:
    TriggerChecker trigger_checker_;
    SystemState current_state_;
    std::unordered_map<std::string, std::function<TriggerChecker::Value()>> variable_getters_;

    mutable std::unordered_map<std::string, TriggerChecker::Value> current_variables_;
};

}
