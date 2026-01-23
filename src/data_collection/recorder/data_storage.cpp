//
// Created by xucong on 25-6-12.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "data_storage.h"
#include "common/log/logger.h"
#include "common/utils/utils.h"

namespace dcp::recorder {

namespace fs = std::filesystem;

constexpr float kDiskThreshold = 90.0f;
constexpr uint64_t kDefaultDataSizeBytes = 1024 * 1024; // 1GB

bool DataStorage::Init(const std::shared_ptr<rclcpp::Node>& node, const trigger::StrategyConfig& strategy_config)
{
    node_ = node;
    auto appconfig = common::AppConfig::getInstance().GetConfig();
    config_ = strategy_config;

    auto it  = appconfig.dataStorage.storagePaths.find("bagPath");
    if (it != appconfig.dataStorage.storagePaths.end()) {
        data_path_ = it->second;
    }
    data_path_ = data_path_.empty() ? "./data" : data_path_;

    // 创建数据目录（如果不存在）
    if (!fs::exists(data_path_)) {
        if (!fs::create_directories(data_path_)) {
            std::cerr << "Failed to create data directory: " << data_path_ << std::endl;
            return false;
        }
    }

    disk_space_checker_ = std::make_shared<DiskSpaceChecker>();
    if (!disk_space_checker_) {
        AD_ERROR(DataStorage, "DiskSpaceChecker make_unique failed.");
        return false;
    }
    disk_space_checker_->setThreshold(90.0f);

    file_roller_ = std::make_unique<FileRoller>();
    if (!file_roller_) {
        AD_ERROR(DataStorage, "FileRoller make_unique failed.");
        return false;
    }

    for (const auto& k: config_.strategies) {
        if (k.trigger.enabled)
            strategy_ = std::make_shared<trigger::Strategy>(k);
    }

    if(!strategy_)
    {
        AD_ERROR(DataStorage,"no own strategy");
        return false;
    }

    ros2bag_recorder_ = std::make_shared<Ros2BagRecorder>(node_);
    ros2bag_recorder_->Init();
    last_trigger_timestamp_ = common::GetCurrentTimestamp();

    return true;
}


bool DataStorage::save_json(std::string& output_json_filename, const trigger::TriggerContext& current_trigger)
{
    auto appconfig = common::AppConfig::getInstance().GetConfig();
    nlohmann::json json;
    json["city"] = "WuHan";
    json["day_night"] = "day";
    json["dev_project"] = "dongfengL29Pro";
    json["shadow_tag_info"]["businessType"] = current_trigger.businessType;
    json["shadow_tag_info"]["triggerId"] = current_trigger.triggerId;
    json["shadow_tag_info"]["timeStamp"] = common::UnixSecondsToString(current_trigger.triggerTimestamp/1e6);
    json["shadow_tag_info"]["forward_time"] = strategy_->mode.cacheMode.forwardCaptureDurationSec;
    json["shadow_tag_info"]["backward_time"] = strategy_->mode.cacheMode.backwardCaptureDurationSec;
    json["shadow_tag_info"]["triggerDesc"] = current_trigger.triggerDesc;
    json["is_cloud_upload"] = !appconfig.debug.closeDataUpload;

    std::ofstream ofs(output_json_filename);
    if (ofs.is_open()){
        ofs << json.dump(4)  <<std::endl;
        ofs.close();
        return true;
    }
    else{
        AD_ERROR(DataStorage, "file: %s open error", output_json_filename.c_str());
        return false;
    }
}

bool DataStorage::handle_trigger(const trigger::TriggerContext& trigger)
{
    auto appconfig = common::AppConfig::getInstance().GetConfig();
    const float currentUsage = disk_space_checker_->getUsagePercentage(data_path_);
    if (disk_space_checker_->isOverThreshold(data_path_)) {
        AD_WARN(DataStorage, "Disk space is insufficient! Current usage: %f%, unable to start collection", currentUsage);
        // return false;
    }

    // std::string vin_id = data_reporter_->vin;
    // data_reporter_->getCollectBagDistance(bag_distance);
    uint64_t now = common::GetCurrentTimestamp();
    if ((now - trigger.triggerTimestamp) >= 0.01*1e9) return false;
    std::string filepath = data_path_ +
        common::MakeRecorderFileName(trigger.triggerId, trigger.businessType, trigger.triggerTimestamp/1e9);
    ros2bag_recorder_->TriggerRecord(trigger.triggerTimestamp, filepath);
    AD_INFO(DataStorage, "Trigger Recorder path:%s, Trigger ID: %s", filepath.c_str(), trigger.triggerId.c_str());

    std::vector<std::string> inputFilePaths;
    std::string output_json_filename = filepath;
    std::string output_lz4_filename = filepath;
    size_t start_pos = filepath.find("splite");
    if (start_pos != std::string::npos){
        output_json_filename.replace(start_pos,6,"json");
        output_lz4_filename.replace(start_pos,6,"tar.lz4");
    }

    AD_INFO(DataStorage, "========================================================");
    AD_INFO(DataStorage, "Shadow tag file :%s", output_json_filename.c_str());
    AD_INFO(DataStorage, "Shadow rsclbag file :%s", filepath.c_str());
    AD_INFO(DataStorage, "Shadow upload file :%s", output_lz4_filename.c_str());
    AD_INFO(DataStorage, "========================================================");

    save_json(output_json_filename, trigger);

    inputFilePaths.emplace_back(filepath);
    inputFilePaths.emplace_back(output_json_filename);
    if(compress_files(inputFilePaths, output_lz4_filename)) {
        double bag_capacity = 0;
        bag_capacity = static_cast<double>(fs::file_size(fs::path(output_lz4_filename)))/kDefaultDataSizeBytes;
        AD_INFO(DataStorage, "bag_capacity: %fM", bag_capacity);
        // data_reporter_->addCollectBagInfo(bag_distance, bag_capacity);
    }

    int cooldownDurationSec = strategy_->mode.cacheMode.cooldownDurationSec;
    auto time_since_last_finish = common::GetCurrentTimestamp() - last_trigger_timestamp_;

    uint64_t required_cooldown_us = cooldownDurationSec * 1e6;
    uint64_t remaining_cooldown = (time_since_last_finish < required_cooldown_us)
                                      ? (required_cooldown_us - time_since_last_finish)
                                      : 0;

    if(remaining_cooldown > 0){
        AD_INFO(DataStorage, "Cooling down, remaining: %.2f seconds", remaining_cooldown / 1e6);
        std::this_thread::sleep_for(std::chrono::microseconds(remaining_cooldown));
    }

    last_trigger_timestamp_ = common::GetCurrentTimestamp();
    AD_INFO(DataStorage, "Trigger finished at: %lld", last_trigger_timestamp_);

    return true;
}

void DataStorage::AddTrigger(const trigger::TriggerContext& context)
{
    {
        std::lock_guard<std::mutex> lock(trigger_mutex_);
        trigger_queue_.push(context);
    }
    cv_.notify_one();
}

bool DataStorage::Start() {
    while (!stop_.load()) {
        std::unique_lock<std::mutex> lock(trigger_mutex_);
        cv_.wait(lock, [&]{
            return !trigger_queue_.empty() || stop_.load();
        });

        if (stop_.load()) break;

        auto ctx = trigger_queue_.front();
        trigger_queue_.pop();

        AD_INFO(DataStorage, "Processed trigger - ID: %s, Timestamp: %lld",
                ctx.triggerId.c_str(),
                ctx.triggerTimestamp);
        lock.unlock();
        handle_trigger(ctx);
    }

    return true;
}

bool DataStorage::Stop() {
    AD_INFO(DataStorage, "Stop.");
    cv_.notify_all();

    return true;
}

bool DataStorage::check_disk_space()
{

    auto appconfig = common::AppConfig::getInstance().GetConfig();
    unsigned long long total, freeSpaceBytes;
    if (!disk_space_checker_->getDiskSpace(data_path_, total, freeSpaceBytes)) {
        throw std::runtime_error("Failed to get disk space");
    }
    const uint64_t freeSpaceMb = freeSpaceBytes / (1024 * 1024);
    return freeSpaceMb >= static_cast<uint64_t>(appconfig.dataStorage.requriedSpaceMb);
}

bool DataStorage::compress_files(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath) {
    if (inputFilePaths.empty()) {
        AD_ERROR(DataStorage, "Input file list is empty");
        return false;
    }

    for (const auto& path : inputFilePaths) {
        if (!fs::exists(path)) {
            AD_ERROR(DataStorage, "inputFilePath not exists: %s", path.c_str());
            return false;
        }
    }

    if (!check_disk_space()) {
        AD_ERROR(DataStorage,"DiskSpace is not enough to perform compression!!!" );
        return false;
    }

    auto ret = FileCompress::CompressFiles(inputFilePaths, outputFilePath);
    if (ret == FileCompress::ErrorCode::Success) {
        AD_INFO(DataStorage,"compressFiles success, outputFilePath: %s", outputFilePath.c_str());
        common::DeleteFiles(inputFilePaths);
        file_roller_->rollFiles();
    }

    return ret == FileCompress::ErrorCode::Success;
}

} 
