//
// Created by xucong on 25-6-12.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <string>
#include <memory>
#include <vector>
#include "nlohmann/json.hpp"

#include "trigger_engine/idl/dcl_trigger.h"
#include "trigger_engine/strategy_parser/strategy_config.h"
#include "common/config/app_config.h"
#include "recorder/common/diskspace_checker.hpp"

namespace dcl {
namespace recorder {

class DataStorage {
public:
    DataStorage() = default;
    ~DataStorage() = default;

    bool Init(const std::shared_ptr<void>& node, 
              const trigger::StrategyConfig& config);
    
    void AddTrigger(const trigger::TriggerContext& context);
    
    void storeData(const struct DataPoint& data_point);
    
    bool SaveToFile(const std::string& filepath);
    
    bool LoadFromFile(const std::string& filepath);

private:
    std::shared_ptr<void> node_;
    trigger::StrategyConfig config_;
};

} // namespace recorder
} // namespace dcl

#endif // DATA_STORAGE_H
