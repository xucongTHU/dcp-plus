//
// Created by your name on 25-11-17.
// Copyright (c) 2025 T3CAIC. All rights reserved.
//

#ifndef TRIGGER_MANAGER_H
#define TRIGGER_MANAGER_H

#include "trigger_base.h"
#include "strategy_config.h"

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <shared_mutex>
#include "channel/message_provider.h"
#include "priority_scheduler/priority_scheduler.h"

namespace dcl {
namespace trigger {


class TriggerManager {
public:
    TriggerManager() = default;
    ~TriggerManager() = default;

    bool initTriggerChecker(std::shared_ptr<TriggerBase> trigger);
    bool initScheduler(const StrategyConfig& strategy_config, const std::shared_ptr<Scheduler>& scheduler);
    bool processScheduler();

    std::shared_ptr<TriggerBase> createTrigger(const std::string& trigger_id);
    std::shared_ptr<TriggerBase> getTrigger(const std::string& trigger_id) const;

private:
    std::unordered_map<std::string, 
        std::function<std::function<TriggerConditionChecker::Value()>()>> variable_getter_factories_;
    
    std::unordered_map<std::string, std::shared_ptr<TriggerBase>> triggers_;
    std::shared_ptr<dcl::channel::MessageProvider> message_provider_;
    std::unordered_map<std::string, std::shared_ptr<TriggerBase>> trigger_instances_;
    std::shared_ptr<Scheduler> scheduler_;
    StrategyConfig strategy_config_;
    mutable std::shared_mutex mutex_;
};

} 
} 

#endif //TRIGGER_MANAGER_H