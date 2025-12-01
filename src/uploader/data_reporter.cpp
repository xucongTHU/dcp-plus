//
// Created by xucong on 25-7-7.
// Copyright (c) 2025 T3CAIC Group Limited. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "data_reporter.h"

#include <cmath>
#include "common/config/app_config.h"

namespace dcl {
namespace uploader {

static const char* LOG_TAG = "DATA_REPORTER";
static inline const std::string OBU161 = "obu161";
static inline const std::string OBU14 = "obu14";
static inline const std::string OBU02 = "obu02";
static inline const std::string RSU02 = "rsu02";
static inline const std::string OBU01 = "obu01";
static inline const std::string OT103 = "103";
static inline const std::string OT104 = "104";
static inline const std::string OT105 = "105";
static inline const std::string OT0013 = "0013";
static inline const std::string OT0014 = "0014";


bool DataReporter::Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node, strategy::CacheMode& mode) {
    node_ptr_ = node;
    cache_mode_ = mode;
    last_time_ = common::MonoTime();
    // frameCounterReset();

    bool ok = MqttInit();
    // CHECK_AND_RETURN(ok, DataReporter, "MqttInit failed", false);
    return true;
}

void DataReporter::OnMessageReceived(const std::string& topic, const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg) {
    if (topic == "/canbus/vehicle_report") {
        UpdateVehicleInfo(msg);
    }

    //TODO
}


void DataReporter::UpdateVehicleInfo(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_counter_["canbus"]++;

    kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
            (msg->Bytes()), msg->ByteSize());
    ::capnp::FlatArrayMessageReader reader(vi_kjarr);
    latest_chassis_ = reader.getRoot<senseAD::msg::vehicle::VehicleReport>();
    
    // if(frame_counter_["canbus"] % 100 == 0) { 
    //     LOG_INFO("[canbus info] vehicle_speed: %f",
    //         latest_chassis_.getChassis().getVehicleMps());
    //     LOG_INFO("[canbus info] brake_pedal_state: %d",
    //         latest_chassis_.getBrake().getParkingBrakeActual());
    //     LOG_INFO("[canbus info] gear_info: %d",
    //         static_cast<uint8_t>(latest_chassis_.getGear().getActual()));   
    // }
    
}

// void DataReporter::callbackPos(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     frame_counter_["inspva"]++;

//     kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
//             (msg->Bytes()), msg->ByteSize());
//     ::capnp::FlatArrayMessageReader reader(vi_kjarr);
//     latest_inspva_ = reader.getRoot<senseAD::msg::sensor::Gnss>();
    
//     if(frame_counter_["inspva"] % 100 == 0) { 
//         LOG_INFO("[gnss info] latitude: %lf, longitude: %lf, height: %lf",
//             latest_inspva_.getLatitude(), latest_inspva_.getLongitude(), latest_inspva_.getAltitude());
//     }
// }

// void DataReporter::callbackImg(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     frame_counter_["img"]++;
// }

// void DataReporter::callbackPcdFrame(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>> msg) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     frame_counter_["pc"]++;
// }

// void DataReporter::frameCounterReset() {
//     for (const auto& sensor_name : sensor_list) {
//         frame_counter_[sensor_name] = 0;
//     }
// }

// void DataReporter::getCollectBagDistance(double& bag_distance) {
//     int bag_duration = cache_mode_.forwardCaptureDurationSec + cache_mode_.backwardCaptureDurationSec;
//     bag_distance = latest_chassis_.getChassis().getVehicleMps() * bag_duration;
// }

// void DataReporter::addCollectBagInfo(double bag_distance, double bag_capacity) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     int bag_duration = cache_mode_.forwardCaptureDurationSec + cache_mode_.backwardCaptureDurationSec;
//     collect_info.distanceCollect += bag_distance;
//     collect_info.durationCollect += bag_duration;
//     collect_info.triggercountCollect += 1 ;
//     collect_info.bagcapacityCollect += bag_capacity;
// }

// void DataReporter::addUploadBagInfo(double bag_distance, double bag_capacity) {
//     std::lock_guard<std::mutex> lock(mutex_);
//     int bag_duration = cache_mode_.forwardCaptureDurationSec + cache_mode_.backwardCaptureDurationSec;
//     collect_info.distanceUpload += bag_distance;
//     collect_info.durationUpload += bag_duration;
//     collect_info.triggercountUpload += 1 ;
//     collect_info.bagcapacityUpload += bag_capacity;
// }

// void DataReporter::getFrameRate(SysInfo& sys_info) {
//     auto curMsgTime = common::MonoTime();
//     auto deltaTime = curMsgTime - last_time_;
//     for (const auto& [sensor_name, frame_count] : frame_counter_) {
//         std::cout << "sensor_name: " << sensor_name << ", frame_count: " << frame_count << std::endl;
//         auto frame_ratio = frame_count / ToSecond(deltaTime);
//         if(sensor_name == "canbus") {
//             sys_info.canbusFrameRateException = frame_ratio ;
//         }else if (sensor_name == "pc") {
//             sys_info.pcFrameRateException = frame_ratio ;
//         }else if (sensor_name == "img") {
//             sys_info.imageFrameRateException = frame_ratio ;
//         }else if (sensor_name == "inspva") {
//             sys_info.insFrameRateException = frame_ratio ;
//         }
//     }
//     last_time_ = curMsgTime;
//     frameCounterReset();
// }

// void DataReporter::getObuInfo(OBUStateInfo& obu_info) {
//     // std::lock_guard<std::mutex> lock(mutex_);
//     obu_info.vin = vin;
//     obu_info.longitude = std::to_string(latest_inspva_.getLatitude());
//     obu_info.latitude = std::to_string(latest_inspva_.getLongitude());
//     obu_info.altitude = std::to_string(latest_inspva_.getAltitude());
//     obu_info.speed = std::to_string(latest_chassis_.getChassis().getVehicleMps() * 3.6);
//     auto light_state = latest_chassis_.getBeam();
//     if(light_state.getTurnLeftLamp() || light_state.getTurnRightLamp() || light_state.getHarzardLamp()){
//         obu_info.light = "ON";   
//     }
//     else{   
//         obu_info.light = "OFF";   
//     }   
//     obu_info.brakeState = static_cast<int32_t>(latest_chassis_.getBrake().getParkingBrakeActual());
//     obu_info.gearState = std::to_string(static_cast<uint8_t>(latest_chassis_.getGear().getActual()));  
//     obu_info.wheelAngle = std::to_string(latest_chassis_.getSteering().getAngleActual());
//     obu_info.collectState = collect_state;
// }

// void DataReporter::getSysInfo(SysInfo& sys_info) {
//     CPUData oldData = readCPUData();
//     std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for 1 second
//     CPUData newData = readCPUData();
//     double cpuUsage = calculateCPUPercentage(oldData, newData);
//     sys_info.cpuUsage = cpuUsage;
//     // sys_info.cpuUsage = std::round(cpuUsage * 100) / 100;
//     LOG_INFO("cpuUsage get: %f", sys_info.cpuUsage);

//     // sys_info.taskId = callback->task_id_;
//     getMemData(sys_info);
//     getSpaceData(sys_info, "/home/nvidia/userdata");
//     getFrameRate(sys_info);
// }


// void DataReporter::getJsonHeader(json& reportJson, std::string& msgType) {
//     std::string token = common::readFileToString(std::string(common::getInstallRootPath()) + "/config/token");
//     // reportJson["devId"] = device_id;
//     reportJson["devId"] = "V-Box2103010456";
//     reportJson["oemId"] = 1;
//     reportJson["devType"] = 11;
//     reportJson["verType"] = "OBU-MQTT";
//     reportJson["version"] = "v1.3";
//     reportJson["msgType"] = msgType;
//     reportJson["msgId"] = getRandMsgID();
//     reportJson["timeStamp"] = generateTimestamp();
//     reportJson["requester"] = 2;
//     reportJson["isEncrypt"] = 0;
//     reportJson["token"] = token;
//     std::cout << "token: " << token << std::endl;
//     reportJson["sign"] = "";
// }

// void DataReporter::publishUploadMessage(dcl::common::FileUploadProgress &upload_progress)
// {
//     // 构建 JSON 消息
//     json reportJson;
//     std::string msgType = "obu111";
//     getJsonHeader(reportJson, msgType);

//     json contentJson;
//     contentJson["vin"] = vin;
//     contentJson["sn"] = device_id;
//     contentJson["status"] = json::array();

//     json uploadJson;
//     uploadJson["bizType"] = "b001";
//     // uploadJson["status"] = json::array();

//     upload_progress.vin = vin;
//     json statusJson = upload_progress.to_json();
//     uploadJson["bizId"] = statusJson["fileUuid"];
//     std::string timestamp = common::get14DigitTimestamp();
//     statusJson["batchId"] =  "P"+ timestamp + "-" + vin.substr(vin.length() - 6);
//     // statusJson["taskId"] = callback->task_id_;
//     statusJson["taskId"] = taskId;
//     // common::jsonFormater(uploadJson, statusJson);
//     // contentJson["status"].emplace_back(statusJson);
//     uploadJson["status"] = statusJson;
//     contentJson["status"].emplace_back(uploadJson);
//     reportJson["content"] = contentJson.dump();

//     std::string payload = reportJson.dump();

//     LOG_INFO("publish upload message: \n%s", payload.c_str());

//     mqttWrapper_->Publish(upload_topic, payload);
// }


bool DataReporter::checkConfigValid(const json& jsonData) {
    try {
        if (!jsonData.contains("configId") || !jsonData.contains("strategyId") || !jsonData.contains("strategies")) { return false; }

        for (const auto& stJson : jsonData["strategies"]) {
            if (!stJson.contains("trigger") || !stJson.contains("mode") ||
                !stJson.contains("enableMasking") || !stJson.contains("channels") ||
                !stJson.contains("businessType")) {
                return false;
            }

            // trigger
            if (!stJson["trigger"].contains("triggerName") || !stJson["trigger"].contains("triggerId") ||
                !stJson["trigger"].contains("priority") || !stJson["trigger"].contains("enabled") || !stJson["trigger"].contains("triggerCondition")) {
                return false;
            }

            // channels
            for (const auto& channelJson : stJson["channels"]["dds"]) {
                if (!channelJson.contains("topic") || !channelJson.contains("type") ||
                    !channelJson.contains("originalFrameRate") || !channelJson.contains("capturedFrameRate")) {
                    return false;
                }
            }

        }
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}


void DataReporter::handleConfigMessage(const std::string &msg, const std::string &msgId, const std::string &vin, const std::string &devId)
{
    try
    {
        std::cout << "msg " << msg << std::endl;
        json rawJson = json::parse(msg);
        std::cout << "rawJson " << rawJson << std::endl;
        // std::string contentStr = rawJson["content"].get<std::string>();
        json paras = rawJson["paras"];
        LOG_INFO("paras: %s", paras.dump().c_str());
        std::string token = common::readFileToString(std::string(common::getInstallRootPath()) + "/config/token");
        // 生成消息ID（确保content和msgId使用相同的ID
        std::string contentToken = trim(token);
        LOG_INFO("contentToken: %s", contentToken.c_str());
        // 创建content JSON
        json content;
        content["msg"] = "";
        content["msgId"] = msgId;
        content["msgType"] = OBU161;
        content["vin"] = vin;
        content["device"] = device;
        content["softwareVersion"] = software_version;
        content["hardwareVersion"] = hardware_version;
        content["sendId"] = msgId;
        // 检查 paras 是否存在且是数组
        if (!rawJson.contains("paras") || !rawJson["paras"].is_array())
        {
            LOG_ERROR("Error: 'paras' is missing or not an array");
        }

        // 检查 paras 数组是否为空
        if (paras.empty())
        {
            LOG_ERROR("Error: 'paras' array is empty");
        }
        // 获取 paras 数组的第一个元素
        json first_para = paras[0];
        LOG_ERROR("first_para: %s", first_para.dump().c_str());
        if (!(first_para.contains("configId") && first_para.contains("strategyId")))
        {
            std::cerr << "Error: [configId] and [strategyId] not found." << std::endl;
        }
        std::string configId = first_para["configId"].get<std::string>();
        int strategyId = first_para["strategyId"].get<int>();
        // 创建最内层的resultContent JSON
        json resultContent;
        resultContent["sendId"] = configId;
        resultContent["vin"] = vin;
        resultContent["device"] = device;
        resultContent["device_id"] = device_id;
        resultContent["softwareVersion"] = software_version;
        resultContent["hardwareVersion"] = hardware_version;
        resultContent["status"] = 1;
        content["resultContent"] = resultContent.dump(); // 将JSON转为字符串

        if (!checkConfigValid(first_para)) {
            LOG_ERROR("Error: wrong config json format!");
            content["status"] = "2";
            content["error"] = "wrong format!";
            return;
        }

        saveDataToFile(first_para.dump(), std::string(getInstallRootPath()) + "/config/strategy_config.json");
        content["status"] = "1";
        content["error"] = "success";

        // 创建外层JSON
        json response;
        response["content"] = content.dump(); // 将content JSON转为字符串
        response["devId"] = devId;
        response["devType"] = 11;
        response["isEncrypt"] = 0;
        response["msgId"] = msgId;
        response["msgType"] = OBU161;
        response["oemId"] = 1;
        response["requester"] = 2;
        response["timeStamp"] = generateTimestamp();
        response["token"] = contentToken;
        response["verType"] = "OBU-MQTT";
        response["version"] = "v1.3";

        std::string payload = response.dump();

        LOG_INFO("response: %s", payload.c_str());

        mqttWrapper_->Publish(upload_topic, payload);
        LOG_INFO("配置响应消息发布成功");
    }
    catch (const mqtt::exception &e)
    {
        LOG_ERROR("消息发布失败: %s", e.what());
        return;
    }
}

void DataReporter::handleTokenResponse(const std::string& contentStr, const std::string& token_path) {
    try {
            json contentJson = json::parse(contentStr);
            // 打印content字段内容
            std::cout << "\n=== content字段 ===" << std::endl;
            std::cout << "expireTime: " << contentJson["expireTime"].get<long long>() << std::endl;
            std::cout << "msg: " << contentJson["msg"].get<std::string>() << std::endl;
            std::cout << "msgId: " << contentJson["msgId"].get<std::string>() << std::endl;
            std::cout << "resultCode: " << contentJson["resultCode"].get<std::string>() << std::endl;
            std::cout << "token: " << contentJson["token"].get<std::string>() << std::endl;
            const std::string token = contentJson["token"].get<std::string>();
            common::saveDataToFile(token, token_path);
    }
    catch (const std::exception& e) {
        std::cerr << "Token处理失败: " << e.what() << std::endl;
    }

}

// 性能日志记录
void DataReporter::publishTaskMessage(const std::string &msgId, const std::string &vin,const std::string &device, const std::string &sendId,
                                      int sendType, const std::string &taskId, const std::string &devId, const std::string &hardwareVersion, const std::string &softwareVersion)
{
    std::string token = common::readFileToString(std::string(common::getInstallRootPath()) + "/config/token");
    std::string contentToken = trim(token);
    // 创建最内层的resultContent JSON
    json resultContent;
    // std::string msgId = "msgId";
    resultContent["sendId"] = sendId;
    resultContent["taskId"] = taskId;
    resultContent["sendType"] = sendType;
    resultContent["softwareVersion"] = softwareVersion;
    resultContent["hardwareVersion"] = hardwareVersion;
    resultContent["status"] = 1;
    resultContent["device"] = device;
    resultContent["device_id"] = devId;
    resultContent["vin"] = vin;
    resultContent["error"] = 1;

    // 创建content JSON
    json content;
    content["msg"] = "";
    content["msgId"] = msgId;
    content["msgType"] = OBU161;
    content["resultContent"] = resultContent.dump(); // 将JSON转为字符串

    // 创建外层JSON
    json response;
    response["content"] = content.dump(); // 将content JSON转为字符串
    response["devId"] = devId;
    response["devType"] = 11;
    response["isEncrypt"] = 0;
    response["msgId"] = msgId;
    response["msgType"] = OBU161;
    response["oemId"] = 1;
    response["requester"] = 2;
    response["timeStamp"] = generateTimestamp();
    response["token"] = contentToken;
    response["verType"] = "OBU-MQTT";
    response["version"] = "v1.3";
    std::cout << "response " << response.dump() << "status_topic_"<< upload_topic << std::endl;
    // 3. 发布响应
    try
    {
        std::string payload = response.dump();
        mqttWrapper_->Publish(upload_topic, payload);
        LOG_INFO("配置响应消息发布成功");
    }
    catch (const mqtt::exception &e)
    {
        LOG_ERROR("消息发布失败: %s", e.what());
    }
}

void DataReporter::handleTaskMessage(const std::string &msg, const std::string &msgId, const std::string &vin, const std::string &devId,
                                     const std::string &hardwareVersion, const std::string &softwareVersion)
{
    json outer = json::parse(msg);
    json content = json::parse(outer["content"].get<std::string>());

    try {
        if (!content.contains("paras") || content["paras"].empty()) {
            std::cerr << "无效的 paras 字段" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "paras字段错误: " << e.what() << std::endl;
    }

    std::string orderType = content["orderType"].get<std::string>();
    std::cout << "orderType " << orderType << std::endl;
    const json &paras = content["paras"][0];
    std::cout << "paras.dump() " << paras.dump(2) << std::endl;

    // 1.task start
    if (orderType == OT104 || orderType == OT105) {
        std::string sendId = paras["sendId"];
        int sendType = paras["sendType"];
        // int strategyId = paras["strategyId"];
        taskId = paras["taskId"];
        std::string taskIdPath = std::string(getInstallRootPath()) + "/config/" + taskId + ".json";
        saveDataToFile(paras.dump(), taskIdPath);

        //send message
        publishTaskMessage(msgId,vin, devId, sendId, sendType, taskId, devId, hardwareVersion, softwareVersion);
        //
        if (orderType == OT104) {
            std::vector<int> startDate = paras["startDate"];
            std::vector<int> endDate = paras["endDate"];
        }
    }
}

void DataReporter::handleTaskAndConfigMessage(const std::string& topic, const std::string& msg){
    auto start_time = std::chrono::steady_clock::now();
    const std::string rawMessage = msg;
    LOG_INFO("handleTaskAndConfigMessage: %s", rawMessage.c_str());
    try
    {
        json rawJson = json::parse(rawMessage);
        // 打印外层字段
        std::cout << "=== 下行消息字段 ===" << std::endl;
        std::cout << "devId: " << rawJson["devId"].get<std::string>() << std::endl;
        std::cout << "oemId: " << rawJson["oemId"].get<int>() << std::endl;
        std::cout << "devType: " << rawJson["devType"].get<int>() << std::endl;
        std::cout << "verType: " << rawJson["verType"].get<std::string>() << std::endl;
        std::cout << "version: " << rawJson["version"].get<std::string>() << std::endl;
        std::cout << "msgType: " << rawJson["msgType"].get<std::string>() << std::endl;
        std::cout << "msgId: " << rawJson["msgId"].get<std::string>() << std::endl;
        std::cout << "timeStamp: " << rawJson["timeStamp"].get<long long>() << std::endl;
        std::cout << "requester: " << rawJson["requester"].get<int>() << std::endl;
        std::cout << "isEncrypt: " << rawJson["isEncrypt"].get<int>() << std::endl;
        std::cout << "sign: " << rawJson["sign"].get<std::string>() << std::endl;

        std::cout << "=== 下行消息 context字段 ===" << std::endl;
        std::cout << "context: \n" << rawJson["content"].get<std::string>() << std::endl;
        std::string contentStr = rawJson["content"].get<std::string>();

        //TODO
        std::string msgType = rawJson["msgType"].get<std::string>();
        std::string devId = rawJson["devId"].get<std::string>();
        std::string msgId = rawJson["msgId"].get<std::string>();
        nlohmann::json contentJson = nlohmann::json::parse(contentStr);
        std::string orderType;

        //TODO 下行消息逻辑处理
        if (contentJson.contains("orderType")) {
            std::cout << "orderType 存在，值为: " << contentJson["orderType"] << std::endl;
            orderType = contentJson["orderType"];
        } else {
            std::cout << "orderType 不存在" << std::endl;
        }

        // 1. token消息处理
        if (msgType == OBU02 || msgType == RSU02) {
            std::string token_path = std::string(common::getInstallRootPath()) + "/config/token";
            handleTokenResponse(contentStr,token_path);
        } else if (msgType == OBU161) {
        // 2. 配置消息处理
            if (orderType == OT103) {
                std::cout << "handleConfigMessage" << std::endl;
                handleConfigMessage(contentStr, msgId, Vin(), devId);
            }
        // 3. 任务消息处理
            else {
                std::cout << "handleTaskMessage" << std::endl;
                handleTaskMessage(rawMessage, msgId, Vin(), devId, hardware_version, software_version);
            }
        }
        //TODO迭代三
        // else if (msgType == OBU14) {
        //     if (orderType == OT0013 || orderType == OT0014) {
        //         handleTaskMessage(rawMessage, msgId,Vin(), devId, hardware_version, software_version);
        //     } else {
        //         std::cerr << "未知消息类型: " << orderType << std::endl;
        //     }
        else {
            std::cerr << "未知消息类型: " << msgType << std::endl;
        }
    } catch (const json::exception &e) {
        std::cerr << "JSON解析错误: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "消息处理异常: " << e.what() << std::endl;
    } catch (const std::ios_base::failure &e) {
        std::cerr << "文件IO错误: " << e.what() << std::endl;
    }

    // 性能日志记录
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    LOG_INFO("消息处理耗时: %d ms", duration.count());
}

// void DataReporter::publishSysMessage(const SysInfo &sys_info)
// {
//     // 构建 JSON 消息
//     json reportJson;
//     std::string msgType = "obu111";
//     getJsonHeader(reportJson, msgType);

//     json contentJson;
//     contentJson["vin"] = vin;
//     contentJson["sn"] = device_id;
//     contentJson["status"] = json::array();

//     json uploadJson;
//     uploadJson["bizType"] = "b002";
//     uploadJson["bizId"] = vin;
//     uploadJson["status"] = json::array();
//     std::cout << "sys info: " << sys_info.cpuUsage << std::endl;
//     json statusJson = sys_info.to_json();
//     std::cout << "sys info json: " << statusJson["cpuUsage"] << std::endl;
//     statusJson["taskId"] = taskId;
//     uploadJson["status"] = statusJson;
//     contentJson["status"].emplace_back(uploadJson);
//     reportJson["content"] = contentJson.dump();
//     std::string payload = reportJson.dump();

//     LOG_INFO("[Mem] context: \n%s",  reportJson.dump(2).c_str());

//     mqttWrapper_->Publish(upload_topic, payload);
// }

// void DataReporter::publishObuMessage(const OBUStateInfo &obu_info)
// {
//     // 构建 JSON 消息
//     json reportJson;
//     std::string msgType = "obu11";
//     getJsonHeader(reportJson, msgType);

//     json contentJson = obu_info.to_json();
//     reportJson["content"] = contentJson.dump();
//     std::string payload = reportJson.dump();

//     LOG_INFO("[Obu] context: \n%s", reportJson.dump(2).c_str());

//     mqttWrapper_->Publish(upload_topic, payload);
// }

// void DataReporter::publishCollectMessage(const CollectInfo &collect_info)
// {
//         // 构建 JSON 消息
//     json reportJson;
//     std::string msgType = "obu111";
//     getJsonHeader(reportJson, msgType);

//     json contentJson;
//     contentJson["vin"] = vin;
//     contentJson["sn"] = device_id;
//     contentJson["status"] = json::array();

//     json uploadJson;
//     uploadJson["bizType"] = "b003";
//     uploadJson["bizId"] = vin;
//     // uploadJson["status"] = json::array();

//     json statusJson = collect_info.to_json();
//     // common::jsonFormater(uploadJson, statusJson);
//     // statusJson["taskId"] = common::get14DigitTimestamp();
//     statusJson["taskId"] = taskId;
//     uploadJson["status"] = statusJson;
//     contentJson["status"].emplace_back(uploadJson);

//     // contentJson["status"].emplace_back(statusJson);
//     reportJson["content"] = contentJson.dump();
//     std::string payload = reportJson.dump();

//     LOG_INFO("[Collect] context: \n%s", reportJson.dump(2).c_str());

//     mqttWrapper_->Publish(upload_topic, payload);
// }

bool DataReporter::Start() {
    while (true) {
        {
            // std::lock_guard<std::mutex> lock(mutex_);
            // SysInfo sys_info = {};
            // getSysInfo(sys_info);
            // publishSysMessage(sys_info);
            // OBUStateInfo obu_info = {};
            // getObuInfo(obu_info);
            // publishObuMessage(obu_info);
            // collect_info.vin = vin;
            // publishCollectMessage(collect_info);
            // collect_info = {};
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return true;
}

bool DataReporter::MqttInit() {
    try
    {
        // 读取mqtt配置文件
        const auto& app_config = common::AppConfig::getInstance().GetConfig();
        //vehicle
        vin = app_config.dataProto.vin;
        device_id = app_config.dataProto.device_id;
        device = app_config.dataProto.device;
        software_version = app_config.dataProto.software_version;
        hardware_version = app_config.dataProto.hardware_version;

        // mqtt
        std::string mqtt_broker_ssl = app_config.dataProto.mqtt.broker_ssl;
        std::string mqtt_username = app_config.dataProto.mqtt.username;
        std::string mqtt_password = app_config.dataProto.mqtt.password;

        download_topic = app_config.dataProto.mqtt.downTopic;
        upload_topic = app_config.dataProto.mqtt.upTopic;
        std::string ca_cert = app_config.dataUpload.caCertPath;
        std::string client_cert = app_config.dataUpload.clientCertPath;
        std::string client_key = app_config.dataUpload.clientKeyPath;

        mqttWrapper_ = std::make_shared<data_proto::MqttWrapper>();
        if (!app_config.debug.closeMqttSsl) {
            mqttWrapper_->Init(mqtt_broker_ssl, "shadow_tbox_" + vin, mqtt_username, mqtt_password, ca_cert, client_cert, client_key);
        }
        mqttWrapper_->SetMessageCallback(
            std::bind(&DataReporter::handleTaskAndConfigMessage, this, std::placeholders::_1, std::placeholders::_2));
        auto ret = mqttWrapper_->Connect();
        if (ret != data_proto::MqttWrapper::ErrorCode::SUCCESS) {
            LOG_ERROR("MqttInit, Connect failed, ret: %d", ret);
            return false;
        } 

        LOG_INFO("MqttInit, Connect success");

        // Subscribe to topic
        LOG_INFO("MqttInit, Subscribing to topic: %s", download_topic.c_str());
        mqttWrapper_->Subscribe(download_topic, 1);

        // Publish message
        std::string payload = common::getTokenContent(device_id, vin);
        mqttWrapper_->Publish(upload_topic, payload);
        LOG_INFO("MqttInit, Published message to topic: %s\n pyload: %s", upload_topic.c_str(), payload.c_str());
    }
    catch (const mqtt::exception &exc)
    {
        std::cout << "Error: " << exc.what() << std::endl;
        return false;
    }
    return true;
}

} 
} 
