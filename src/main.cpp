// main.cpp - Main entry point for AD Data Closed Loop System
#include "data_collection_planner.h"

#include "common/log/logger.h"
#include "auth_manager/auth_manager.h"
#include <iostream>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <cstdio>

// 安全获取密码输入
std::string getPasswordInput(const std::string& prompt) {
    std::cout << prompt;

    std::string password;
    char ch;

    struct termios old_termios;
    tcgetattr(STDIN_FILENO, &old_termios);

    struct termios new_termios = old_termios;
    new_termios.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    while ((ch = getchar()) != '\n') {
        if (ch == 127 || ch == 8) {
            if (!password.empty()) {
                password.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
        } else {
            password += ch;
            // std::cout << "*";
            std::cout.flush();
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);

    std::cout << std::endl;
    return password;
}

int main() {
    try {
        AuthManager& auth = AuthManager::getInstance();

        std::cout << "EdgeInsight DCP Plus Service - Authorization Required" << std::endl;
        std::cout << "Enter username: ";
        std::string username;
        std::getline(std::cin, username);
        std::string password = getPasswordInput("Enter password: ");

        std::string token = auth.login(username, password);

        // 验证授权令牌
        if (!auth.validateToken(token)) {
            std::cout << "Invalid username or password. Access denied." << std::endl;
            return -1;
        }

        // 检查用户权限
        if (!auth.hasPermission(token, "data_collection")) {
            std::cout << "User does not have permission for data collection. Access denied." << std::endl;
            return -1;
        }

        std::cout << "Authorization successful!" << std::endl;

        // Initialize logging
        dcp::common::Logger::instance()->Init(dcp::common::LOG_TO_CONSOLE | dcp::common::LOG_TO_FILE, LOG_LEVEL_INFO,
                               "/tmp/ad_data_closed_loop.log", "/tmp/ad_data_closed_loop.csv");

        AD_INFO(Main, "Starting Data Collection Planner (DCP) System");

        // Create data collection planner
        dcp::DataCollectionPlanner collector;

        // Initialize the system
        if (!collector.initialize()) {
            AD_ERROR(Main, "Failed to initialize data collection planner");
            dcp::common::Logger::instance()->Uninit();
            auth.logout(token); // 注销会话
            return -1;
        }

        // Set mission area based on PRD: configurable 2D grid (default 20x20)
        MissionArea mission(Point(50.0, 50.0), 10.0);
        collector.setMissionArea(mission);

        // Plan data collection mission
        auto mission_path = collector.planDataCollectionMission();

        if (mission_path.empty()) {
            AD_WARN(Main, "No valid path planned for data collection");
            dcp::common::Logger::instance()->Uninit();
            auth.logout(token); // 注销会话
            return -1;
        }

        // Execute data collection
        collector.executeDataCollection(mission_path);

        // Report coverage metrics
        collector.reportCoverageMetrics();

        // Analyze data and update planner weights
        collector.analyzeAndExportWeights();

        // Upload collected data
        collector.uploadCollectedData();

        AD_INFO(Main, "Data Collection Mission Completed");
        auth.logout(token);
        return 0;

    } catch (const std::exception& e) {
        AD_ERROR(Main, "Exception occurred: " + std::string(e.what()));
        dcp::common::Logger::instance()->Uninit();
        return -1;
    } catch (...) {
        AD_ERROR(Main, "Unknown exception occurred");
        dcp::common::Logger::instance()->Uninit();
        return -1;
    }
}