//
// Created by xucong on 25-5-7.
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "app_config.h"
#include <fstream>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <filesystem>

namespace dcp {
namespace common {

AppConfig& AppConfig::getInstance() {
    static AppConfig instance;
    return instance;
}

bool AppConfig::Init(const std::string& filePath) {
    if (!checkFileExists(filePath)) {
        std::cerr << "Error: Configuration file does not exist." << std::endl;
        return false;
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Failed to open configuration file." << std::endl;
        return false;
    }

    std::string jsonString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (!CheckValid(jsonString)) {
        std::cerr << "Error: Configuration file is invalid." << std::endl;
        return false;
    }

    std::cout << "AppConfig CheckValid is true"  << std::endl;

    configData = nlohmann::json::parse(jsonString);
    // DataStorage
    parsedConfig.dataStorage.rollingDeleteThreshold = (int8_t)configData["dataStorage"]["rollingDeleteThreshold"];
    parsedConfig.dataStorage.rollInterval = (int8_t)configData["dataStorage"]["rollInterval"];
    parsedConfig.dataStorage.bagInterval = (int8_t)configData["dataStorage"]["bagInterval"];
    parsedConfig.dataStorage.capacityMb = (uint64_t)configData["dataStorage"]["capacityMb"];
    parsedConfig.dataStorage.requriedSpaceMb = (uint64_t)configData["dataStorage"]["requiredSpaceMb"];
    parsedConfig.dataStorage.storagePaths["bagPath"] = configData["dataStorage"]["storagePaths"]["bagPath"];
    parsedConfig.dataStorage.storagePaths["encPath"] = configData["dataStorage"]["storagePaths"]["encPath"];

    // DataProto
    parsedConfig.dataProto.vin = configData["dataProto"]["vin"];
    parsedConfig.dataProto.software_version = configData["dataProto"]["software_version"];
    parsedConfig.dataProto.hardware_version = configData["dataProto"]["hardware_version"];
    parsedConfig.dataProto.device = configData["dataProto"]["device"];
    parsedConfig.dataProto.device_id = configData["dataProto"]["device_id"];
    parsedConfig.dataProto.mqtt.broker = configData["dataProto"]["mqtt"]["broker"];
    parsedConfig.dataProto.mqtt.broker_ssl = configData["dataProto"]["mqtt"]["broker_ssl"];
    parsedConfig.dataProto.mqtt.username = configData["dataProto"]["mqtt"]["username"];
    parsedConfig.dataProto.mqtt.password = configData["dataProto"]["mqtt"]["password"];
    parsedConfig.dataProto.mqtt.upTopic = configData["dataProto"]["mqtt"]["upTopic"];
    parsedConfig.dataProto.mqtt.downTopic = configData["dataProto"]["mqtt"]["downTopic"];

    // DataUpload
    parsedConfig.dataUpload.retryCount = (int8_t)configData["dataUpload"]["retryCount"];
    parsedConfig.dataUpload.retryIntervalSec = (int64_t)configData["dataUpload"]["retryIntervalSec"];
    parsedConfig.dataUpload.uploadFileIntervalMs = (int64_t)configData["dataUpload"]["uploadFileIntervalMs"];
    parsedConfig.dataUpload.uploadFileSliceIntervalMs = (int64_t)configData["dataUpload"]["uploadFileSliceIntervalMs"];
    parsedConfig.dataUpload.uploadFileSliceSizeMb = (size_t)configData["dataUpload"]["uploadFileSliceSizeMb"];
    parsedConfig.dataUpload.clientCertPath = configData["dataUpload"]["clientCertPath"];
    parsedConfig.dataUpload.clientKeyPath = configData["dataUpload"]["clientKeyPath"];
    parsedConfig.dataUpload.caCertPath = configData["dataUpload"]["caCertPath"];
    parsedConfig.dataUpload.gateway = std::string(configData["dataUpload"]["gateway"]);
    parsedConfig.dataUpload.fileRecordPath = configData["dataUpload"]["fileRecordPath"];
    parsedConfig.dataUpload.filenameRegex = std::string(configData["dataUpload"]["filenameRegex"]);
    parsedConfig.dataUpload.uploadPaths.emplace("encPath", configData["dataUpload"]["uploadPaths"]["encPath"]);
    parsedConfig.dataUpload.rsa_pub_key_path = configData["dataUpload"]["publicKeyPath"];
    parsedConfig.dataUpload.watch_dir = configData["dataUpload"]["uploadPaths"]["bagPath"];
    parsedConfig.dataUpload.enc_dir = configData["dataUpload"]["uploadPaths"]["encPath"];

    // Log
    parsedConfig.log.logLevel = configData["log"]["LOG_level"];
    parsedConfig.log.logPattern = configData["log"]["LOG_pattern"];
    parsedConfig.log.logPath = configData["log"]["LOG_path"];
    parsedConfig.log.logBasename = configData["log"]["LOG_basename"];

    // Debug
    parsedConfig.debug.closeMqttSsl = configData["debug"]["closeMqttSsl"];
    parsedConfig.debug.closeDataReporter = configData["debug"]["closeDataReporter"];
    parsedConfig.debug.closeDataStorage = configData["debug"]["closeDataStorage"];
    parsedConfig.debug.closeDataEnc = configData["debug"]["closeDataEnc"];
    parsedConfig.debug.closeDataUpload = configData["debug"]["closeDataUpload"];
    parsedConfig.debug.deleteFileAfterDataUpload = configData["debug"]["deleteFileAfterDataUpload"];
    parsedConfig.debug.closeLogUpload = configData["debug"]["closeLogUpload"];
    parsedConfig.debug.cloudtimeOutMs = configData["debug"]["cloudtimeOutMs"];

    return true;
}

AppConfigData AppConfig::GetConfig() const {
    return parsedConfig;
}

bool AppConfig::CheckValid(const std::string& jsonString) {
    try {
        nlohmann::json jsonData = nlohmann::json::parse(jsonString);
        return checkJsonFormat(jsonData);
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        return false;
    }
}

bool AppConfig::checkFileExists(const std::string& filePath) const {
    return std::filesystem::exists(filePath);
}

bool AppConfig::checkJsonFormat(const nlohmann::json& jsonData) const {
    if (!jsonData.contains("dataStorage") || !jsonData.contains("dataUpload")||
        !jsonData.contains("dataProto")|| !jsonData.contains("log") ||
        !jsonData.contains("debug")) {
        return false;
    }

    // DataStorage
    if (!jsonData["dataStorage"].contains("rollingDeleteThreshold") || !jsonData["dataStorage"].contains("rollInterval") ||
        !jsonData["dataStorage"].contains("bagInterval") || !jsonData["dataStorage"].contains("storagePaths") ||
        !jsonData["dataStorage"].contains("capacityMb") || !jsonData["dataStorage"].contains("requiredSpaceMb")) {
        return false;
    }

    // DataProto
    if (!jsonData["dataProto"].contains("vin") || !jsonData["dataProto"].contains("software_version") ||
        !jsonData["dataProto"].contains("hardware_version") || !jsonData["dataProto"].contains("device") ||
        !jsonData["dataProto"].contains("device_id") || !jsonData["dataProto"].contains("mqtt")) {
        return false;
    }

    // DataUpload
    if (!jsonData["dataUpload"].contains("retryCount") || !jsonData["dataUpload"].contains("retryIntervalSec") ||
        !jsonData["dataUpload"].contains("uploadFileIntervalMs") || !jsonData["dataUpload"].contains("uploadFileSliceIntervalMs") ||
        !jsonData["dataUpload"].contains("uploadFileSliceSizeMb") || !jsonData["dataUpload"].contains("clientCertPath") ||
        !jsonData["dataUpload"].contains("clientKeyPath") || !jsonData["dataUpload"].contains("caCertPath") ||
        !jsonData["dataUpload"].contains("gateway") || !jsonData["dataUpload"].contains("fileRecordPath") ||
        !jsonData["dataUpload"].contains("uploadPaths")) {
        return false;
    }

    // Log
    if (!jsonData["log"].contains("LOG_level") || !jsonData["log"].contains("LOG_pattern") ||
        !jsonData["log"].contains("LOG_path") || !jsonData["log"].contains("LOG_basename")) {
        return false;
    }

    // Debug
    if (!jsonData["debug"].contains("closeMqttSsl") || !jsonData["debug"].contains("closeDataReporter") ||
        !jsonData["debug"].contains("closeDataStorage") || !jsonData["debug"].contains("closeDataEnc") ||
        !jsonData["debug"].contains("closeDataUpload") || !jsonData["debug"].contains("deleteFileAfterDataUpload") ||
        !jsonData["debug"].contains("closeLogUpload") || !jsonData["debug"].contains("cloudtimeOutMs")) {
        return false;
    }

    return true;
}


} 
} 
