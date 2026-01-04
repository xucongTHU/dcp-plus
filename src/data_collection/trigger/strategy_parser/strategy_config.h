//
// Created by xucong on 25-5-6.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>

namespace dcp {
namespace trigger {

struct Trigger {
    std::string triggerId;
    int8_t priority;
    bool enabled;
    std::string triggerCondition;
    std::string triggerDesc;
};

struct CacheMode {
    int forwardCaptureDurationSec;
    int backwardCaptureDurationSec;
    int cooldownDurationSec;
};

struct Mode {
    int triggerMode;
    CacheMode cacheMode;
};

struct Channel {
    std::string topic;
    std::string type;
    int originalFrameRate;
    int capturedFrameRate;
};

struct Dds {
    std::vector<Channel> channels;
};

struct Strategy {
    std::string businessType;
    Trigger trigger;
    Mode mode;
    bool enableMasking;
    Dds dds;
    std::unordered_map<std::string, std::string> upload;
};

struct StrategyConfig {
    std::string configId;
    int strategyId;
    std::vector<Strategy> strategies;
};

}
}
