//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#include "rule_trigger.h"
#include "common/utils/utils.h"

namespace dcp {
namespace trigger {

RuleTrigger::RuleTrigger() 
    : current_state_(SystemState::IDLE) {
}

bool RuleTrigger::proc() {
    if (current_state_ == SystemState::TRIGGERED) {
        AD_WARN(RuleTrigger, "Already triggered, skipping.");
        return true;
    }

    bool condition_met = checkCondition();
    if (!condition_met) {
        // 条件不满足，重置状态为未触发
        if (current_state_ != SystemState::UNTRIGGERED) {
            current_state_ = SystemState::UNTRIGGERED;
            AD_INFO(RuleTrigger, "Condition not met, reset state to Untriggered.");
        }
        return false;
    }

    // 条件满足，执行触发逻辑
    TriggerContext context;
    context.triggerTimestamp = common::GetCurrentTimestamp();
    context.triggerId = trigger_obj_->triggerId;
    context.triggerDesc = trigger_obj_->triggerDesc;
    // context.businessType = trigger_obj_->businessType;
    // context.triggerState = SystemState::TRIGGERED;

    // 更新内部状态
    current_state_ = SystemState::TRIGGERED;
    return true;
}

bool RuleTrigger::checkCondition() {
    // 解析触发条件表达式
    if (!trigger_checker_.parse(trigger_obj_->triggerCondition)) {
        AD_ERROR(RuleTrigger, "Failed to parse condition: %s", 
                  trigger_checker_.lastError().c_str());
        return false;
    }
    current_variables_.clear();
    
    // 获取所有需要的变量值
    for (const auto& [var_name, getter] : variable_getters_) {
        try {
            current_variables_[var_name] = getter();
        } catch (const std::exception& e) {
            AD_ERROR(RuleTrigger, "Failed to get variable '%s': %s", 
                      var_name.c_str(), e.what());
            return false;
        }
    }

    // 检查条件
    bool result = trigger_checker_.executeCheck(current_variables_);
    
    return result;
}

void RuleTrigger::registerVariableGetter(const std::string& var_name, 
                                         std::function<TriggerChecker::Value()> getter) {
    variable_getters_[var_name] = std::move(getter);
}

} // namespace trigger
} // namespace shadow