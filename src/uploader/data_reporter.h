//
// Created by xucong on 25-7-7.
// Copyright (c) 2025 T3CAIC Group Limited. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef DATA_REPORTER_H
#define DATA_REPORTER_H


#include "ad_rscl/ad_rscl.h"
#include "ad_msg_idl/ad_vehicle/vehicle.capnp.h"
#include "ad_msg_idl/ad_sensor/sensor.capnp.h"

#include "data_proto/mqtt_wrapper.h"
#include "common/log/logger.h"
#include "common/data.h"
#include "common/utils/utils.h"
// #include "common/time/Timer.h"
// #include "strategy/StrategyParser.h"
// #include "strategy/StrategyConfig.h"
#include "channel/observer.h"

#include "nlohmann/json.hpp"
#include <mqtt/client.h>
#include <string>
#include <iostream>
#include <thread>
#include <chrono>


namespace dcl {
namespace uploader {

using json = nlohmann::json;
using namespace dcl::common;

class DataReporter : public Observer {
  public:
    // std::shared_ptr<MqttCallback> callback;
    std::string vin;
    bool collect_state = false;
    strategy::CacheMode cache_mode_ = {};
    std::shared_ptr<data_proto::MqttWrapper> mqttWrapper_;

    DataReporter() = default;
    ~DataReporter() = default;

    bool Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node, strategy::CacheMode& mode);
    bool Start();

    // void publishUploadMessage(dcl::common::FileUploadProgress &upload_progress);
    // void getCollectBagDistance(double& bag_distance);
    // void addCollectBagInfo(double bag_distance, double bag_capacity);
    // void addUploadBagInfo(double bag_distance, double bag_capacity);

  private:
     /**
     * @brief mqtt初始化
     */
    bool MqttInit();

    /**
     * @brief 回调函数，用于处理接收到的智能驾驶算法消息。
     */
    void onMessageReceived(const std::string& topic, const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg) override;
    void UpdateVehicleInfo(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg);
    // void UpdatePoseInfo(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg);
    // void UpdateImageInfo(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg);
    // void UpdatePcdInfo(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg);

    /**
     * @brief 获取系统信息、上报的请求头。
     */
    // void getSysInfo(SysInfo& sys_info);
    // void getObuInfo(OBUStateInfo& obu_info);
    // void getJsonHeader(json& reportJson, std::string& msgType);
    // void getFrameRate(SysInfo& sys_info);
    // void frameCounterReset();

    /**
     * @brief 发送系统状态、GPS、采集状态等JSON 消息, 并发布到 MQTT 主题。
     */
    // void publishSysMessage(const SysInfo &sys_info);
    // void publishObuMessage(const OBUStateInfo &obu_info);
    // void publishCollectMessage(const CollectInfo &collect_info);



    /**
     * @brief mqtt处理任务下发和配置下发
     */
    void handleTaskAndConfigMessage(const std::string& topic, const std::string& msg);
    void handleTaskMessage(const std::string &msg, const std::string &msgId, const std::string &vin,
                           const std::string &devId, const std::string &hardwareVersion, const std::string &softwareVersion);
    void handleConfigMessage(const std::string &msg, const std::string &msgId, const std::string &vin, const std::string &devId);
    void handleTokenResponse(const std::string& contentStr,const std::string& token_path);
    bool checkConfigValid(const json &j);
    void publishTaskMessage(const std::string &msgId, const std::string &vin,const std::string &device, const std::string &sendId,
                            int sendType, const std::string &taskId, const std::string &devId, const std::string &hardwareVersion, const std::string &softwareVersion);

  private:
    std::string device_id;
    std::string taskId;
    std::string upload_topic;
    std::string download_topic;
    std::string device;
    std::string software_version;
    std::string hardware_version;
    std::mutex mutex_;
    std::shared_ptr<senseAD::rscl::comm::Node> node_ptr_{nullptr};

    senseAD::msg::vehicle::VehicleReport::Reader latest_chassis_;
    senseAD::msg::sensor::Gnss::Reader latest_inspva_;

    double distance = 0.0;
    uint64_t last_time_;
    std::map<std::string, int> frame_counter_;
    std::vector<std::string> sensor_list = {"canbus", "inspva", "img","pc"} ;

    std::vector<std::string> trigger_vec;
    CollectInfo collect_info = {};

};

}
}


#endif //DATA_REPORTER_H
