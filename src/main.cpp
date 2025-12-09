// main.cpp - Main entry point for AD Data Closed Loop System
#include "data_collection_planner.h"

#include "common/log/logger.h"

int main() {
    try {
        // Initialize logging
        dcl::common::Logger::instance()->Init(dcl::common::LOG_TO_CONSOLE | dcl::common::LOG_TO_FILE, LOG_LEVEL_INFO, 
                               "/tmp/ad_data_closed_loop.log", "/tmp/ad_data_closed_loop.csv");

        AD_INFO(Main, "Starting AD Data Closed Loop System");
        
        // Create data collection planner
        dcl::DataCollectionPlanner collector;
        
        // Initialize the system 
        if (!collector.initialize()) {
            AD_ERROR(Main, "Failed to initialize data collection planner");
            dcl::common::Logger::instance()->Uninit();
            return -1;
        }
        
        // Set mission area
        MissionArea mission(Point(50.0, 50.0), 40.0);
        collector.setMissionArea(mission);
        
        // Plan data collection mission
        auto mission_path = collector.planDataCollectionMission();
        
        if (mission_path.empty()) {
            AD_WARN(Main, "No valid path planned for data collection");
            dcl::common::Logger::instance()->Uninit();
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
        dcl::common::Logger::instance()->Uninit();
        return 0;
        
    } catch (const std::exception& e) {
        AD_ERROR(Main, "Exception occurred: " + std::string(e.what()));
        dcl::common::Logger::instance()->Uninit();
        return -1;
    } catch (...) {
        AD_ERROR(Main, "Unknown exception occurred");
        dcl::common::Logger::instance()->Uninit();
        return -1;
    }
}