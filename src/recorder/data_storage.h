//
// Created by xucong on 25-6-12.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include "nlohmann/json.hpp"

// #include "data_upload/DataReporter.h"
#include "recorder/common/diskspace_checker.hpp"
#include "recorder/common/file_compress.h"
#include "recorder/common/file_roller.h"
// #include "trigger/base/TriggerBase.h"
// #include "cyber_recorder/CyberRecorder.h"
#include "recorder/rscl_recorder.h"
// #include "common/time/Timer.h"
#include "common/utils/utils.h"
#include "common/config/app_config.h"


namespace dcl {
namespace recorder {

enum class CollectionStatus : int32_t {
    None = 0,
    Collecting,     // 采集中
    Completed       // 采集完成
};

class DataStorage {
public:
    DataStorage() = default;
    virtual ~DataStorage() = default;

    bool Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node,
              const strategy::StrategyConfig& strategy_config);

    // bool Init(stoic::cm::NodeHandle& nh,
    //           const common::AppConfigData::DataStorage& config,
    //           const strategy::StrategyConfig& strategy_config,
    //           std::shared_ptr<uploader::DataReporter> data_reporter);

    bool Start();
    bool Stop();
    void AddTrigger(const trigger::TriggerContextPtr& trigger);
    std::shared_ptr<RsclRecorder> GetRsclRecorder() const { return rscl_recorder_; }

private:
    bool saveTriggerInfoJson(std::string& output_json_filename, const trigger::TriggerContext& current_trigger);
    bool compressFiles(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath);
    bool handleTrigger(const trigger::TriggerContextPtr& current_trigger);
    bool checkDiskSpace() const;

private:
    std::shared_ptr<senseAD::rscl::comm::Node> node_{nullptr};
    strategy::StrategyConfig strategy_config_;
    std::shared_ptr<strategy::Strategy> strategy_{nullptr};
    std::unique_ptr<FileRoller> fileRoller;
    std::shared_ptr<DiskSpaceChecker> diskSpaceChecker;
    // std::shared_ptr<uploader::DataReporter> data_reporter_;
    std::shared_ptr<RsclRecorder> rscl_recorder_;


    std::string dataPath;
    std::thread rollThread;
    std::atomic<bool> isRollThreadRunning{false};
    std::chrono::seconds rollInterval{60}; 
    mutable std::mutex rollMutex;
    std::mutex triggerMutex;
    std::condition_variable cv_;

    bool debug = false;

    std::atomic<CollectionStatus> collectionStatus_{CollectionStatus::None};
    std::atomic<bool> stop_{false};
    std::queue<trigger::TriggerContextPtr> triggerList_; 
    // trigger::TriggerContext current_trigger_;
    double bag_distance = 0.0;

    uint64_t last_trigger_finish_time;
};

} 
} 

#endif // DATA_STORAGE_H
