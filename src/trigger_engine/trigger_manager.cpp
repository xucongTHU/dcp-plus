//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#include "trigger_manager.h"
#include "common/log/logger.h"

namespace dcl {
namespace trigger {

bool TriggerManager::initTriggerChecker(std::shared_ptr<TriggerBase> trigger) {
    if (!trigger) return false;
    trigger->registerVariableGetter("speed", [this]() -> TriggerConditionChecker::Value {
        return message_provider_->getChassisVehicleMps(); // m/s
    });

    trigger->registerVariableGetter("automode", [this]() -> TriggerConditionChecker::Value {
        return message_provider_->getAutoModeEnable();
    });

    trigger->registerVariableGetter("gear", [this]() -> TriggerConditionChecker::Value {
        return message_provider_->getGear();
    });

    trigger->registerVariableGetter("aeb_decel_req", [this]() -> TriggerConditionChecker::Value {
        return message_provider_->getAebDecelReq();
    });
}

std::shared_ptr<TriggerBase> TriggerManager::createTrigger(const std::string& trigger_id) {
    std::unique_lock lock(mutex_);
    auto it = triggers_.find(trigger_id);
    return (it != triggers_.end()) ? it->second : nullptr;
}

std::shared_ptr<TriggerBase> TriggerManager::getTrigger(const std::string& trigger_id) const {
    std::shared_lock lock(mutex_);
    auto it = triggers_.find(trigger_id);
    return (it != triggers_.end()) ? it->second : nullptr;
}

bool TriggerManager::initScheduler(const StrategyConfig& strategy_config, const std::shared_ptr<Scheduler>& scheduler) {
    strategy_config_ = strategy_config;
    scheduler_ = scheduler;
    if (!scheduler_) {
        AD_ERROR(TriggerManager, "Scheduler is not initialized.");
        return false;
    }

    std::vector<std::pair<std::string, int>> enabled_triggers;
    int trigger_priority = std::numeric_limits<int>::max();

    if (enabled_triggers.empty())
    {
        AD_WARN(TriggerManager, "No enabled triggers found.");
    }

    {
        std::shared_lock lock(mutex_);
        for (const auto& s : strategy_config_.strategies) {
            if (s.trigger.enabled) {
                enabled_triggers.emplace_back(
                    s.trigger.triggerId,  
                    s.trigger.priority
                );
            }
        }
    }

    bool success = true;
    for (const auto& [id, priority] : enabled_triggers)
    {
        auto trigger = createTrigger(id);
        if (!trigger) {
            AD_ERROR(TriggerManager, "Trigger not found for %s", id.c_str());
            success = false;
            continue;
        }

        success = trigger->init(id, strategy_config_);
        CHECK_AND_RETURN(success, TriggerManager, "Trigger init failed", false);

        TriggerTask task;
        task.triggerId = id;
        task.priority = priority;
        task.trigger = std::move(trigger);
        task.strategyConfig = strategy_config_;
        scheduler_->AddTask(task);

        trigger_instances_[id] = std::dynamic_pointer_cast<TriggerBase>(task.trigger);
    }

    return success;
}

bool TriggerManager::processScheduler() {
    if (scheduler_) {
        scheduler_->StartScheduling();
    }
}

} // namespace trigger
} // namespace dcl