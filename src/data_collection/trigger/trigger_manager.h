#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <shared_mutex>
#include "channel/message_provider.h"
#include "trigger/strategy_parser/strategy_parser.h"
#include "trigger_base.h"
#include "priority_scheduler/priority_scheduler.h"

namespace dcp::trigger{

class TriggerManager {
public:
    TriggerManager() = default;
    ~TriggerManager() = default;

    bool initTriggerChecker(std::shared_ptr<TriggerBase> trigger);
    bool initScheduler(const StrategyConfig& strategy_config, const std::shared_ptr<Scheduler>& scheduler);

    bool initialize();
    bool shouldTrigger(const Point& position) { return true; }
    bool isInSparseArea(const Point& position) { return true; }
    double getDistanceToNearestSparseArea(const Point& position) {return 0.0; }

    std::shared_ptr<TriggerBase> createTrigger(const std::string& trigger_id);
    std::shared_ptr<TriggerBase> getTrigger(const std::string& trigger_id) const;

    bool processScheduler();

private:
    // StrategyConfig config_;
    std::unordered_map<std::string,
                       std::function<std::function<TriggerChecker::Value()>()>> variable_getter_factories_;

    std::unordered_map<std::string, std::shared_ptr<TriggerBase>> triggers_;
    std::shared_ptr<channel::MessageProvider> message_provider_;
    std::unordered_map<std::string, std::shared_ptr<TriggerBase>> trigger_instances_;
    std::shared_ptr<Scheduler> scheduler_;
    StrategyConfig strategy_config_;
    mutable std::shared_mutex mutex_;
};

}
