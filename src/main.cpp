// main.cpp
#include "data_collection_planner.h"
#include "state_machine.h"
#include "data_collection/src/common/logger/Logger.h"
#include <iostream>
#include <getopt.h>

using namespace dcl;
using namespace dcl::planner;
using namespace dcl::state_machine;
using namespace dcl::logger;
using namespace dcl::data_storage;
using namespace dcl::data_upload;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "Options:\n"
              << "  -h, --help              Show this help message\n"
              << "  -a, --algorithm ALG     Planning algorithm (astar or ppo)\n"
              << "  -w, --weights PATH      Path to PPO weights file\n"
              << "  -c, --config PATH       Path to planner configuration file\n"
              << std::endl;
}

// Main
int main(int argc, char* argv[]) {
    // Default values
    std::string algorithm = "astar";
    std::string ppo_weights_path = "/workspaces/ad_data_closed_loop/training/models/ppo_weights.pth";
    std::string config_path = "/workspaces/ad_data_closed_loop/src/navigation_planner/config/planner_weights.yaml";
    
    // Parse command line arguments
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"algorithm", required_argument, 0, 'a'},
        {"weights", required_argument, 0, 'w'},
        {"config", required_argument, 0, 'c'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "ha:w:c:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'a':
                algorithm = optarg;
                break;
            case 'w':
                ppo_weights_path = optarg;
                break;
            case 'c':
                config_path = optarg;
                break;
            default:
                printUsage(argv[0]);
                return -1;
        }
    }
    
    // Initialize logger
    Logger::instance()->Init(LOG_TO_CONSOLE | LOG_TO_FILE, LOG_LEVEL_INFO, 
                            "/tmp/data_collection.log", "/tmp/data_collection.csv");
    
    LogUtils::setLogLevel(LogUtils::INFO);
    LogUtils::log(LogUtils::INFO, "Starting Data Collection with Navigation Planner Integration");
    LogUtils::log(LogUtils::INFO, "Using algorithm: " + algorithm);
    
    try {
        // Create data collection planner
        auto collector = std::make_shared<DataCollectionPlanner>(config_path);
        
        // Create navigation planner
        auto nav_planner = std::make_shared<NavPlannerNode>(config_path);
        
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
        
        // Configure algorithm
        if (algorithm == "ppo") {
            LogUtils::log(LogUtils::INFO, "Enabling PPO-based path planning");
            nav_planner->setUsePPO(true);
            if (!nav_planner->loadPPOWeights(ppo_weights_path)) {
                LogUtils::log(LogUtils::WARN, "Failed to load PPO weights, falling back to A*");
                nav_planner->setUsePPO(false);
            }
        } else {
            LogUtils::log(LogUtils::INFO, "Using A*-based path planning");
            nav_planner->setUsePPO(false);
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