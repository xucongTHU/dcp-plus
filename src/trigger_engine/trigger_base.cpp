//
// Created by xucong on 25-2-10.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "trigger_base.h"

namespace dcl {
namespace trigger {

bool TriggerBase::init(const std::string& triggerId, const StrategyConfig& strategyConfig) {
    for (const auto& st : strategyConfig.strategies) {
        if (st.trigger.triggerId == triggerId) {
            trigger_obj_ = std::make_unique<Trigger>(st.trigger);
            break;
        }
    }

    if (!trigger_obj_) {
        AD_ERROR(TriggerBase, "Trigger object not found for trigger ID: %s", triggerId.c_str());
        return false;
    }

    return true;
}

}
}
