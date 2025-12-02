//
// Created by xucong on 25-4-2.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "strategy_parser.h"

namespace dcl {
namespace trigger {

bool StrategyParser::LoadConfigFromFile(const std::string &file_path, StrategyConfig &conf) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open JSON config file: " << file_path << std::endl;
        return false;
    }

    std::string jsonString((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));
    file.close();

    if (!CheckValid(jsonString)) {
        std::cerr << "JSON file is invalid." << std::endl;
        return false;
    }

    nlohmann::json jsonData = nlohmann::json::parse(jsonString);
    ParseJsonConfig(jsonData, conf);

    return true;
}

void StrategyParser::ParseJsonConfig(const nlohmann::json &jsonData, StrategyConfig &config) {
    config.configId = jsonData["configId"];
    config.strategyId = jsonData["strategyId"];

    for (const auto& strategyJson : jsonData["strategies"]) {
        Strategy st;
        st.businessType = strategyJson["businessType"];
        st.trigger.triggerName = strategyJson["trigger"]["triggerName"];
        st.trigger.triggerId = strategyJson["trigger"]["triggerId"];
        st.trigger.priority = strategyJson["trigger"]["priority"];
        st.trigger.enabled = strategyJson["trigger"]["enabled"];
        st.trigger.triggerCondition =  strategyJson["trigger"] ["triggerCondition"];

        // parse mode
        st.mode.triggerMode = strategyJson["mode"]["triggerMode"];
        st.mode.cacheMode.forwardCaptureDurationSec =  strategyJson["mode"] ["cacheMode" ]["forwardCaptureDurationSec"];
        st.mode.cacheMode.backwardCaptureDurationSec = strategyJson["mode"] ["cacheMode" ]["backwardCaptureDurationSec"];
        st.mode.cacheMode.cooldownDurationSec = strategyJson["mode"] ["cacheMode" ]["cooldownDurationSec"];

        //
        st.enableMasking = strategyJson["enableMasking"];

        //parse channels
        for (const auto& channelJson : strategyJson["channels"]["dds"]) {
            Channel channel;
            channel.topic = channelJson["topic"];
            channel.type = channelJson["type"];
            channel.originalFrameRate = channelJson["originalFrameRate"];
            channel.capturedFrameRate = channelJson["capturedFrameRate"];
            st.dds.channels.emplace_back(channel);
        }

        config.strategies.push_back(st);
    }
}

bool StrategyParser::CheckValid(const std::string &jsonString) {
    try {
        nlohmann::json jsonData = nlohmann::json::parse(jsonString);
        if (!jsonData.contains("configId") || !jsonData.contains("strategyId") || !jsonData.contains("strategies")) { return false; }

        for (const auto& stJson : jsonData["strategies"]) {
            if (!stJson.contains("trigger") || !stJson.contains("mode") ||
                !stJson.contains("enableMasking") || !stJson.contains("channels")) {
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

bool StrategyParser::getTriggerType(const std::string& filepath, std::vector<std::string>& trigger_vec) { 
    // 读取storage配置文件
    std::ifstream storage_config_file(filepath);
    if (!storage_config_file.is_open())
    {
        std::cout << "Could not open the storage config file!" << std::endl;
        return false;
    }

    // 获取有效的trigger name
    json storage_config;
    storage_config_file >> storage_config;
    if(!(storage_config.contains("trigger_info_array"))) {
        std::cout << "storage config file contains no trigger info array!" << std::endl;
        return false; 
    }
    if (storage_config["trigger_info_array"].is_array()) {  
        for (const auto& trigger_info: storage_config["trigger_info_array"]) {
            trigger_vec.push_back((std::string)trigger_info["trigger_name"]);  
        } 
    }
    return true;
}

bool StrategyParser::CheckMessage(const json &j, std::vector<std::string>& trigger_vec, int8_t& bag_duration)
{
    if(!(j.contains("configId") && j.contains("strategies"))) return false; 
    if(!(j["strategies"].is_array() && j["strategies"].size() > 0)) return false; 
    for (const auto& strategy: j["strategies"]) {
        if(!(strategy.contains("trigger") && strategy.contains("mode") 
                && strategy.contains("rollingDeleteThreshold")
                && strategy.contains("enableMasking")
                && strategy.contains("channels"))) return false;    
        if(!(strategy["trigger"].contains("triggerName")
                && strategy["trigger"].contains("triggerId")
                && strategy["trigger"].contains("priority")
                && strategy["trigger"].contains("enabled")
                && strategy["trigger"].contains("triggerCondition"))) return false; 
        
        std::cout << "judge:" << strategy["trigger"]["triggerName"] << "in trigger list\n";   
        
        // 查找触发器名称是否存在于列表中
        auto it = std::find(trigger_vec.begin(), trigger_vec.end(), strategy["trigger"]["triggerName"]);
        if (it == trigger_vec.end()) {
            std::cout << "'" <<  strategy["trigger"]["triggerName"] << "' not exists in trigger list\n";   
            return false;   
        } 

        if(!(strategy["mode"].contains("triggerMode")
                && strategy["mode"].contains("cacheMode"))) return false;  
        if(!(strategy["mode"]["cacheMode"].contains("forwardCaptureDurationSec")
                && strategy["mode"]["cacheMode"].contains("backwardCaptureDurationSec")
                && strategy["mode"]["cacheMode"].contains("cooldownDurationSec"))){
                    return false; 
                }    
        else {
            bag_duration = static_cast<int>(strategy["mode"]["cacheMode"]["forwardCaptureDurationSec"]) +  
            static_cast<int>(strategy["mode"]["cacheMode"]["backwardCaptureDurationSec"]);   
        }   
        if(!(strategy["channels"].contains("dds")
                && strategy["channels"]["dds"].is_array())) return false;
        for (const auto& channel: strategy["channels"]["dds"]) {
            if(!(channel.contains("topic")
                && channel.contains("type")
                && channel.contains("originalFrameRate")
                && channel.contains("capturedFrameRate"))) return false; 
        }   
    }   
    return true;   
}

} 
} 
