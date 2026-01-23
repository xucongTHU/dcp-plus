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
#include <queue>
#include "nlohmann/json.hpp"

#include "../msg/ad_trigger/dcp_trigger.h"
#include "ros2bag_recorder.h"
#include "common/config/app_config.h"
#include "diskspace_checker.hpp"
#include "file_roller.h"
#include "file_compress.h"

namespace dcp::recorder {

struct Point
{
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

class DataStorage {
public:
    DataStorage() = default;
    ~DataStorage() = default;

    bool Init(const std::shared_ptr<rclcpp::Node>& node,
              const trigger::StrategyConfig& strategy_config);

    bool Start();

    bool Stop();

    void AddTrigger(const trigger::TriggerContext& context);

    // void StoreData(const Point& data);

private:
    bool check_disk_space();

    bool compress_files(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath);

    bool save_json(std::string& output_json_filename,
                             const trigger::TriggerContext& current_trigger);

    bool handle_trigger(const trigger::TriggerContext& trigger);



private:
    std::shared_ptr<rclcpp::Node> node_;
    trigger::StrategyConfig config_;
    std::string data_path_;
    std::shared_ptr<DiskSpaceChecker> disk_space_checker_;
    std::unique_ptr<FileRoller> file_roller_;
    std::shared_ptr<trigger::Strategy> strategy_;

    std::shared_ptr<Ros2BagRecorder> ros2bag_recorder_;
    std::queue<trigger::TriggerContext> trigger_queue_;
    uint64_t last_trigger_timestamp_ = 0;
    std::mutex trigger_mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};

};

}

#endif // DATA_STORAGE_H
