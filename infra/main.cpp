#include "data_collection_planner.h"

// Main
int main() {
    LogUtils::setLogLevel(LogUtils::INFO);
    LogUtils::log(LogUtils::INFO, "Starting Data Collection with Navigation Planner Integration");
    
    // Create data collection planner
    DataCollectionPlanner collector;
    
    // Initialize the system 
    if (!collector.initialize()) {
        LogUtils::log(LogUtils::ERROR, "Failed to initialize data collection planner");
        return -1;
    }
    
    // Set mission area
    MissionArea mission(Point(50.0, 50.0), 40.0);
    collector.setMissionArea(mission);
    
    // Plan data collection mission
    auto mission_path = collector.planDataCollectionMission();
    
    // Execute data collection
    collector.executeDataCollection(mission_path);
    
    // Report coverage metrics
    collector.reportCoverageMetrics();
    
    // Analyze data and update planner weights
    collector.analyzeAndExportWeights();
    
    LogUtils::log(LogUtils::INFO, "Data Collection Mission Completed");
    return 0;
}