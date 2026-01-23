//
// Created by xucong on 25-2-10.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef TRIGGER_BASE_H
#define TRIGGER_BASE_H

#include <string>
#include <memory>

#include "common/log/logger.h"
#include "strategy_parser/strategy_config.h"
#include "../msg/ad_trigger/dcp_trigger.h"
#include "common/trigger_checker.h"
#include "channel/observer.h"

namespace dcp::trigger {

/**
 * @brief Abstract base class for triggers.
 */
class TriggerBase : public channel::Observer {
public:
    TriggerBase() = default;
    virtual ~TriggerBase() = default;

    virtual bool init(const std::string& triggerId, const StrategyConfig& strategyConfig);
    virtual bool proc() = 0;
    virtual bool checkCondition() = 0;
    virtual void registerVariableGetter(const std::string& var_name,
                                        std::function<TriggerChecker::Value()> getter) = 0;

protected:
    std::unique_ptr<Trigger> trigger_obj_ = nullptr;

};

}

#endif //TRIGGER_BASE_H
