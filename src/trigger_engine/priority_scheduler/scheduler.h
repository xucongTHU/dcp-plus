//
// Created by xucong on 25-6-27.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <memory>
#include <string>
// #include "trigger/base/TriggerBase.h"
#include "../strategy_config.h"
#include "../trigger_base.h"

namespace dcl {
namespace trigger {

enum class TaskState { WAITING, RUNNING, PAUSED, FINISHED };

struct TriggerTask {
    std::string triggerName;
    std::string triggerId;
    int8_t priority;
    std::shared_ptr<TriggerBase> trigger;
    StrategyConfig strategyConfig;
    bool cancelled = false;
    TaskState state = TaskState::WAITING;
    int retry_count = 0;
    int max_retries;
    std::chrono::steady_clock::time_point last_attempt_time;
    std::chrono::milliseconds retry_interval;
};

class Scheduler {
public:
    virtual ~Scheduler() = default;
    virtual void AddTask(TriggerTask task) = 0;
    virtual void StartScheduling() = 0;
};

} 
} 

#endif // SCHEDULER_H
