// main.cpp
#include "data_collection_planner.h"
#include "state_machine.h"
#include "data_collection/src/common/logger/Logger.h"


using namespace dcl;
using namespace dcl::planner;
using namespace dcl::state_machine;
using namespace dcl::logger;
using namespace dcl::data_storage;
using namespace dcl::data_upload;

// Main
int main() {
    // Initialize logger
    Logger::instance()->Init(LOG_TO_CONSOLE | LOG_TO_FILE, LOG_LEVEL_INFO, 
                            "/tmp/data_collection.log", "/tmp/data_collection.csv");
    
    LogUtils::setLogLevel(LogUtils::INFO);
    LogUtils::log(LogUtils::INFO, "Starting Data Collection with Navigation Planner Integration");
    
    try {
        // Create data collection planner
        auto collector = std::make_shared<DataCollectionPlanner>();
        
        // Create navigation planner
        auto nav_planner = std::make_shared<NavPlannerNode>(
            "/workspaces/ad_data_closed_loop/infra/navigation_planner/config/planner_weights.yaml");
        
        // Create data storage
        auto data_storage = std::make_shared<DataStorage>();
        
        // Create state machine
        auto state_machine = std::make_shared<StateMachine>();
        state_machine->setDataCollectionPlanner(collector);
        state_machine->setNavPlanner(nav_planner);
        state_machine->setDataStorage(data_storage);
        
        // Initialize the system 
        if (!state_machine->initialize()) {
            LogUtils::log(LogUtils::ERROR, "Failed to initialize state machine");
            Logger::instance()->Uninit();
            return -1;
        }
        
        // Set mission area
        MissionArea mission(Point(50.0, 50.0), 40.0);
        collector->setMissionArea(mission);
        
        // Start mission planning
        state_machine->handleEvent(StateEvent::PLAN_REQUEST);
        
        // Simulate navigation and data collection
        // In a real implementation, this would be handled by the navigation system
        for (int i = 0; i < 10; i++) {
            state_machine->handleEvent(StateEvent::WAYPOINT_REACHED);
        }
        
        LogUtils::log(LogUtils::INFO, "Data Collection Mission Completed");
        Logger::instance()->Uninit();
        return 0;
        
    } catch (const std::exception& e) {
        LogUtils::log(LogUtils::ERROR, "Exception occurred: " + std::string(e.what()));
        Logger::instance()->Uninit();
        return -1;
    } catch (...) {
        LogUtils::log(LogUtils::ERROR, "Unknown exception occurred");
        Logger::instance()->Uninit();
        return -1;
    }
}