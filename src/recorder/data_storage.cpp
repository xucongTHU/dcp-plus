//
// Created by xucong on 25-6-12.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "data_storage.h"

using namespace shadow;
using namespace dcl::common;
using namespace dcl::config;

namespace dcl {
namespace recorder {

static const char* LOG_TAG = "DataStorage";

namespace fs = std::filesystem;

constexpr float kDiskThreshold = 90.0f;
constexpr uint64_t kDefaultDataSizeBytes = 1024 * 1024 * 1024; // 1GB

bool DataStorage::Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node,
                       const strategy::StrategyConfig& strategy_config) {
    node_ = node;
    auto appconfig = AppConfig::getInstance().GetConfig();
    strategy_config_ = strategy_config;

    auto it  = appconfig.dataStorage.storagePaths.find("bagPath");
    if (it != appconfig.dataStorage.storagePaths.end()) {
        dataPath = it->second;
    }
    dataPath = dataPath.empty() ? "./data" : dataPath;

    // 创建数据目录（如果不存在）
    if (!fs::exists(dataPath)) {
        if (!fs::create_directories(dataPath)) {
            std::cerr << "Failed to create data directory: " << dataPath << std::endl;
            return false;
        }
    }

    diskSpaceChecker = std::make_shared<DiskSpaceChecker>();
    if (!diskSpaceChecker) {
        LOG_ERROR("DiskSpaceChecker make_unique failed.");
        return false;
    }
    diskSpaceChecker->setThreshold(90.0f);

    fileRoller = std::make_unique<FileRoller>();
    if (!fileRoller) {
        LOG_ERROR("FileRoller make_unique failed.");
        return false;
    }

    for (const auto& k: strategy_config_.strategies) {
        if (k.trigger.enabled)
            strategy_ = std::make_shared<strategy::Strategy>(k);
    }

    if(!strategy_)
    {
        LOG_ERROR("no own strategy");
        return false;
    }

    rscl_recorder_ = std::make_shared<RsclRecorder>(node_, strategy_);
    rscl_recorder_->Init();
    last_trigger_finish_time = common::Timer::now_us();

    collectionStatus_.store(CollectionStatus::Collecting, std::memory_order_relaxed);

    return true;
}


bool DataStorage::saveTriggerInfoJson(std::string& output_json_filename, 
                                      const trigger::TriggerContext& current_trigger){
    auto appconfig = AppConfig::getInstance().GetConfig();
    nlohmann::json json;
    json["city"] = "WuHan";
    json["day_night"] = "day";
    json["dev_project"] = "dongfengL29Pro";
    json["shadow_tag_info"]["businessType"] = current_trigger.businessType;
    json["shadow_tag_info"]["triggeId"] = current_trigger.triggerId;
    json["shadow_tag_info"]["triggerName"] = current_trigger.triggerName;
    json["shadow_tag_info"]["timeStamp"] = UnixSecondsToString(current_trigger.timeStamp/1e6);
    json["shadow_tag_info"]["forward_time"] = strategy_->mode.cacheMode.forwardCaptureDurationSec;
    json["shadow_tag_info"]["backward_time"] = strategy_->mode.cacheMode.backwardCaptureDurationSec;
    json["shadow_tag_info"]["triggeDesc"] = "testing AEB";
    json["is_cloud_upload"] = !appconfig.debug.closeDataUpload;
   
    std::ofstream ofs(output_json_filename);
    if (ofs.is_open()){
        ofs << json.dump(4)  <<std::endl;
        ofs.close();
        return true;    
    }
    else{
        std::cout << "file: "<< output_json_filename <<" open error"<< std::endl;
        return false;   
    }
}

bool DataStorage::handleTrigger(const trigger::TriggerContextPtr& trigger){
    auto appconfig = AppConfig::getInstance().GetConfig();
    const float currentUsage = diskSpaceChecker->getUsagePercentage(dataPath);
    if (diskSpaceChecker->isOverThreshold(dataPath)) {
        LOG_WARN("Disk space is insufficient! Current usage: %f%, unable to start collection", currentUsage);
        // return false; 
    }

    // std::string vin_id = data_reporter_->vin;
    // data_reporter_->getCollectBagDistance(bag_distance);
    uint64_t now = common::Timer::now_us();
    if ((now - trigger->timeStamp) >= 0.01*1e6) return false;
    std::string filepath = dataPath + "ActivelyReport" +
            MakeRecorderFileName(trigger->triggerId, trigger->businessType, trigger->timeStamp/1e6);
    std::string filepath_with_suffix = filepath + ".00000.rsclbag";

    LOG_INFO("Trigger Recorder path:%s, Trigger ID: %s", filepath_with_suffix.c_str(), trigger->triggerId.c_str());
    rscl_recorder_->TriggerRecord(trigger->timeStamp, filepath);
    std::string rsclbagfile = RenameRecordFile(filepath_with_suffix);

    std::vector<std::string> inputFilePaths;
    std::string output_json_filename = rsclbagfile;
    std::string output_lz4_filename = rsclbagfile;
    size_t start_pos = rsclbagfile.find("rsclbag");
    if (start_pos != std::string::npos){
        output_json_filename.replace(start_pos,7,"json");
        output_lz4_filename.replace(start_pos,7,"tar.lz4");
    }

    LOG_ERROR("========================================================");
    LOG_INFO("Shadow tag file :%s", output_json_filename.c_str());
    LOG_INFO("Shadow rsclbag file :%s", rsclbagfile.c_str());
    LOG_INFO("Shadow upload file :%s", output_lz4_filename.c_str());
    LOG_ERROR("========================================================");
    
    saveTriggerInfoJson(output_json_filename, *trigger);

    inputFilePaths.emplace_back(rsclbagfile);
    inputFilePaths.emplace_back(output_json_filename);
    // if(compressFiles(inputFilePaths, output_lz4_filename)) {
    //     double bag_capacity = 0;
    //     bag_capacity = static_cast<double>(fs::file_size(fs::path(output_lz4_filename)))/1024/1024;
    //     std::cerr << "bag_capacity:"  << bag_capacity << "M" << std::endl;
    //     // data_reporter_->addCollectBagInfo(bag_distance, bag_capacity);
    // }

    int cooldownDurationSec = strategy_->mode.cacheMode.cooldownDurationSec;
    auto time_since_last_finish = common::Timer::now_us() - last_trigger_finish_time;
    
    uint64_t required_cooldown_us = cooldownDurationSec * 1e6;
    uint64_t remaining_cooldown = (time_since_last_finish < required_cooldown_us)
                                ? (required_cooldown_us - time_since_last_finish)
                                : 0;

    if(remaining_cooldown > 0){
        LOG_INFO("Cooling down, remaining: %.2f seconds", remaining_cooldown / 1e6);
        std::this_thread::sleep_for(std::chrono::microseconds(remaining_cooldown));
    }

    last_trigger_finish_time = common::Timer::now_us();
    LOG_INFO("Trigger finished at: %lld", last_trigger_finish_time);                            

    return true;
}

void DataStorage::AddTrigger(const trigger::TriggerContextPtr& trigger) {
    {
        std::lock_guard<std::mutex> lock(triggerMutex);
        triggerList_.push(trigger);
    }
    cv_.notify_one(); 
}

bool DataStorage::Start() {
    while (!stop_.load() && collectionStatus_.load() == CollectionStatus::Collecting) {
        std::unique_lock<std::mutex> lock(triggerMutex);
        cv_.wait(lock, [&]{
            return !triggerList_.empty() || stop_.load();
        });

        if (stop_.load()) break;

        auto ctx = triggerList_.front();
        triggerList_.pop();
        
        LOG_INFO("Processed trigger - ID: %s, Name: %s, Timestamp: %lld", 
                 ctx->triggerId.c_str(), 
                 ctx->triggerName.c_str(), 
                 ctx->timeStamp);
        lock.unlock();
        handleTrigger(ctx);
    }

    return true;
}

bool DataStorage::Stop() {
    std::lock_guard<std::mutex> lock(rollMutex);

    if (collectionStatus_.load() != CollectionStatus::Collecting) {
        std::cerr << "Not in collection state, cannot stop." << std::endl;
        return false;
    }

    // 压缩剩余文件
    // if (!currentFiles_.empty()) {
    //     createNewCompressedFile();
    //     currentFiles_.clear();
    //     currentFileSize_ = 0;
    // }

    collectionStatus_.store(CollectionStatus::Completed, std::memory_order_relaxed);
    // currentCollectionName_.clear();
    std::cout << "Stop data collection" << std::endl;
    return true;
}

bool DataStorage::checkDiskSpace() const {
    auto appconfig = AppConfig::getInstance().GetConfig();
    unsigned long long total, freeSpaceBytes;
    if (!diskSpaceChecker->getDiskSpace(dataPath, total, freeSpaceBytes)) {
        throw std::runtime_error("Failed to get disk space");
    }
    const uint64_t freeSpaceMb = freeSpaceBytes / (1024 * 1024);
    return freeSpaceMb >= static_cast<uint64_t>(appconfig.dataStorage.requriedSpaceMb);
}

bool DataStorage::compressFiles(const std::vector<std::string>& inputFilePaths, const std::string& outputFilePath) {
    if (inputFilePaths.empty()) {
        std::cerr << "Error: Input file list is empty\n";
        return false;
    }
    
    for (const auto& path : inputFilePaths) {
        if (!fs::exists(path)) {
            LOG_ERROR("inputFilePath not exists: %s", path.c_str());
            return false;
        }
    }

    if (!checkDiskSpace()) {
        LOG_ERROR("DiskSpace is not enough to perform compression!!!" );
        return false;
    }

    auto ret = FileCompress::CompressFiles(inputFilePaths, outputFilePath);
    if (ret == FileCompress::ErrorCode::Success) {
        LOG_INFO("compressFiles success, outputFilePath: %s", outputFilePath.c_str());
        common::DeleteFiles(inputFilePaths);
        fileRoller->rollFiles();
    }

    return ret == FileCompress::ErrorCode::Success;
}

} 
} 
