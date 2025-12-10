// data_collection_planner.cpp
#include "data_collection_planner.h"
#include <iostream>
#include <fstream>
#include <random>
#include <yaml-cpp/yaml.h>
#include "common/log/logger.h"

namespace dcl {
DataCollectionPlanner::DataCollectionPlanner(const std::string& model_file, const std::string& config_file) {
    AD_INFO(DataCollectionPlanner, "Creating DataCollectionPlanner with model_file: %s, config_file: %s", 
            model_file.c_str(), config_file.c_str());
    nav_planner_ = std::make_unique<planner::NavPlannerNode>(model_file, config_file);
    // data_storage_ = std::make_unique<recorder::DataStorage>();
    // data_uploader_ = std::make_unique<uploader::DataUploader>();
    // trigger_manager_ = std::make_unique<trigger::TriggerManager>();
    // strategy_parser_ = std::make_unique<trigger::StrategyParser>();
    mission_area_ = MissionArea(Point(50.0, 50.0), 30.0); // Default mission area
    AD_INFO(DataCollectionPlanner, "DataCollectionPlanner constructor completed");
}

bool DataCollectionPlanner::initialize() {
    AD_INFO(DataCollectionPlanner, "Initializing Data Collection Planner");

    // Load strategy configuration
    // trigger::StrategyConfig s_config_;
    // if (!strategy_parser_->LoadConfigFromFile("/workspaces/ad_data_closed_loop/config/default_strategy_config.json", config_)) {
    //     AD_ERROR(DataCollectionPlanner, "Failed to load strategy configuration, using defaults");
    // }

    // const auto& data_upload_config = common::AppConfig::getInstance().GetConfig().dataUpload;
    
    if (!nav_planner_->initialize()) {
        AD_ERROR(DataCollectionPlanner, "Failed to initialize navigation planner");
        return false;
    }
    
    // // Initialize data collection components
    // if (!data_storage_->Init(node_, s_config_)) {
    //     AD_ERROR(DataCollectionPlanner, "Failed to initialize data storage");
    //     return false;
    // }
    
    // if (!data_uploader_->Init(data_upload_config)) {
    //     AD_ERROR(DataCollectionPlanner, "Failed to initialize data uploader");
    //     return false;
    // }
    
    // if (!trigger_manager_->initialize()) {
    //     Logger::instance()->Log(LOG_LEVEL_ERROR, "DATA_COLLECTION", "Failed to initialize trigger manager");
    //     return false;
    // }
    
    AD_INFO(DataCollectionPlanner, "Data Collection Planner initialized successfully");
    return true;
}

void DataCollectionPlanner::setMissionArea(const MissionArea& area) {
    AD_INFO(DataCollectionPlanner, "Setting mission area");
    AD_WARN(DataCollectionPlanner, "New mission area - center: (%s, %s), radius: %s", 
             std::to_string(area.center.x).c_str(), std::to_string(area.center.y).c_str(), 
             std::to_string(area.radius).c_str());
    
    mission_area_ = area;
    nav_planner_->setGoalPosition(area.center);

    AD_INFO(DataCollectionPlanner, "Mission area set to center: (%s, %s), radius: %s", 
            std::to_string(area.center.x).c_str(), std::to_string(area.center.y).c_str(), 
            std::to_string(area.radius).c_str());
}

std::vector<Point> DataCollectionPlanner::planDataCollectionMission() {
    AD_INFO(DataCollectionPlanner, "Planning data collection mission");
    
    // Plan global path using navigation planner
    std::vector<Point> collection_path = nav_planner_->planGlobalPath();
    
    AD_WARN(DataCollectionPlanner, "Received collection path with %s points from navigation planner", 
             std::to_string(collection_path.size()).c_str());
    
    // Apply data collection strategy to optimize waypoints
    std::vector<Point> optimized_waypoints;
    
    if (!collection_path.empty()) {
        // Use the navigation planner's sampling optimizer to find optimal waypoints
        Point current_pos = nav_planner_->getCurrentPosition();
        
        // For each segment of the path, find optimal data collection points
        for (size_t i = 0; i < collection_path.size(); ++i) {
            // Check if we should collect data at this point based on strategy
            // if (trigger_manager_ && trigger_manager_->shouldTrigger(collection_path[i])) {   //TODO
                Point optimal_point = nav_planner_->optimizeNextWaypoint();
                optimized_waypoints.push_back(optimal_point);
                
                // Update current position for next optimization
                nav_planner_->setCurrentPosition(optimal_point);
            // }
        }
        
        // Restore original position
        nav_planner_->setCurrentPosition(current_pos);
    }
    
    AD_INFO(DataCollectionPlanner, "Data collection mission planned with %s waypoints", std::to_string(optimized_waypoints.size()).c_str());
    
    return optimized_waypoints;
}

void DataCollectionPlanner::executeDataCollection(const std::vector<Point>& path) {
    AD_INFO(DataCollectionPlanner, "Executing data collection along path with %s waypoints", std::to_string(path.size()).c_str());
    
    if (path.empty()) {
        AD_WARN(DataCollectionPlanner, "Empty path provided for data collection");
        return;
    }
    
    // Execute data collection at each waypoint using real data collection modules
    std::vector<DataPoint> new_data;
    
    for (size_t i = 0; i < path.size(); ++i) {
        const Point& waypoint = path[i];
        
        // Update planner's current position
        nav_planner_->setCurrentPosition(waypoint);
        
        // Trigger data collection based on strategy
        // if (trigger_manager_ && trigger_manager_->shouldTrigger(waypoint)) {
        //     AD_INFO(DataCollectionPlanner, "Triggering data collection at waypoint %s", std::to_string(i).c_str());
            
        //     // Create trigger context for data collection
        //     auto trigger_context = std::make_shared<dcl::trigger::TriggerContext>();
        //     trigger_context->position.x = waypoint.x;
        //     trigger_context->position.y = waypoint.y;
        //     trigger_context->triggerTimestamp = static_cast<uint64_t>(std::time(nullptr));
            
        //     // Trigger data collection using data storage module
        //     data_storage_->AddTrigger(*trigger_context);
            
        //     // Collect real sensor data using data collection modules
        //     std::string sensor_data = "collected_data_at_" + std::to_string(waypoint.x) + "_" + std::to_string(waypoint.y);
            
        //     // Create data point with real sensor data
        //     DataPoint data_point(waypoint, sensor_data, static_cast<double>(std::time(nullptr)));
        //     new_data.push_back(data_point);
            
        //     // Store data locally
        //     data_storage_->storeData(data_point);
            
        //     // Compute reward for this action
        //     StateInfo current_state;
        //     current_state.visited_new_sparse = trigger_manager_->isInSparseArea(waypoint);
        //     current_state.trigger_success = !sensor_data.empty();
        //     current_state.collision = false; // Would be determined by real sensor data
        //     current_state.distance_to_sparse = trigger_manager_->getDistanceToNearestSparseArea(waypoint);
            
        //     // In a real implementation, we would track previous state to compute reward
        //     double reward = nav_planner_->computeStateReward(StateInfo(), current_state);
        //     AD_INFO(DataCollectionPlanner, "Waypoint %s reward: %s", std::to_string(i).c_str(), std::to_string(reward).c_str());
        // }
    }
    
    // Update with new data
    updateWithNewData(new_data);
    
    AD_INFO(DataCollectionPlanner, "Data collection completed with %s data points collected", std::to_string(new_data.size()).c_str());
}

void DataCollectionPlanner::updateWithNewData(const std::vector<DataPoint>& new_data) {
    AD_INFO(DataCollectionPlanner, "Updating planner with %s new data points", std::to_string(new_data.size()).c_str());
    
    if (new_data.empty()) {
        AD_WARN(DataCollectionPlanner, "No new data points to update");
        return;
    }
    
    // Add new data points to planner and local storage
    for (const auto& data_point : new_data) {
        nav_planner_->addDataPoint(data_point.position);
        collected_data_.push_back(data_point);
    }
    
    // Update costmap with new statistics
    nav_planner_->updateCostmapWithStatistics();
    
    // Update coverage metrics based on actual visited cells
    std::vector<std::pair<int, int>> visited_cells;
    double resolution = 1.0; // Default resolution
    
    for (const auto& data_point : new_data) {
        auto grid_coords = dcl::planner::PlannerUtils::worldToGrid(data_point.position, resolution);
        visited_cells.push_back(grid_coords);
    }
    nav_planner_->updateCoverageMetrics(visited_cells);
    
    AD_INFO(DataCollectionPlanner, "Planner updated with new data");
}

void DataCollectionPlanner::uploadCollectedData() {
    AD_INFO(DataCollectionPlanner, "Uploading collected data to cloud");
    
    // // Upload data using data uploader
    // if (data_uploader_->Start()) {
    //     AD_INFO(DataCollectionPlanner, "Data uploaded successfully");
    //     // Clear local data after successful upload
    //     collected_data_.clear();
    // } else {
    //     AD_INFO(DataCollectionPlanner, "Failed to upload data");
    // }
}

void DataCollectionPlanner::reportCoverageMetrics() {
    AD_INFO(DataCollectionPlanner, "Reporting coverage metrics");
    
    const dcl::planner::CoverageMetric& coverage = nav_planner_->getCoverageMetric();
    
    AD_INFO(DataCollectionPlanner, "Total cells: %s", std::to_string(coverage.getTotalCells()).c_str());
    AD_INFO(DataCollectionPlanner, "Visited cells: %s", std::to_string(coverage.getVisitedCells()).c_str());
    AD_INFO(DataCollectionPlanner, "Coverage ratio: %s", std::to_string(coverage.getCoverageRatio()).c_str());
    AD_INFO(DataCollectionPlanner, "Sparse coverage ratio: %s", std::to_string(coverage.getSparseCoverageRatio()).c_str());
}

void DataCollectionPlanner::analyzeAndExportWeights() {
    AD_INFO(DataCollectionPlanner, "Analyzing collected data and exporting weights");
    AD_WARN(DataCollectionPlanner, "Collected data size: %s points", std::to_string(collected_data_.size()).c_str());
    
    if (collected_data_.empty()) {
        AD_WARN(DataCollectionPlanner, "No collected data available for analysis");
        return;
    }
    
    // Compute density map from collected data
    auto heatmap = DataCollectionAnalyzer::computeDensityMap(collected_data_);
    
    // Detect sparse regions
    auto sparse_regions = DataCollectionAnalyzer::detectSparseRegions(heatmap);
    
    // Load current weights from configuration
    DataCollectionAnalyzer::PlannerWeights current_weights;
    
    // Adjust weights based on sparse zones
    auto adjusted_weights = DataCollectionAnalyzer::adjustCostWeights(sparse_regions, current_weights);
    
    // Save to planner configuration
    DataCollectionAnalyzer::saveToPlannerConfig(adjusted_weights, 
        "/workspaces/ad_data_closed_loop/config/planner_weights.yaml");
    
    // Reload configuration in navigation planner
    nav_planner_->reloadConfiguration();
    
    AD_INFO(DataCollectionPlanner, "Weights analysis and export completed");
}

// DataCollectionAnalyzer implementation
DataCollectionAnalyzer::Heatmap DataCollectionAnalyzer::computeDensityMap(
    const std::vector<DataPoint>& data_points, int grid_width, int grid_height, double resolution) {
    
    Heatmap heatmap(grid_width, grid_height, resolution);
    
    // Initialize density values
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            heatmap.density_values[y][x] = 0.0;
        }
    }
    
    // Calculate density based on data points
    for (const auto& point : data_points) {
        int cell_x = static_cast<int>(point.position.x / resolution);
        int cell_y = static_cast<int>(point.position.y / resolution);
        
        if (cell_x >= 0 && cell_x < grid_width && cell_y >= 0 && cell_y < grid_height) {
            heatmap.density_values[cell_y][cell_x] += 1.0;
        }
    }
    
    // Normalize densities
    double max_density = 0.0;
    for (int y = 0; y < grid_height; y++) {
        for (int x = 0; x < grid_width; x++) {
            max_density = std::max(max_density, heatmap.density_values[y][x]);
        }
    }
    
    if (max_density > 0.0) {
        for (int y = 0; y < grid_height; y++) {
            for (int x = 0; x < grid_width; x++) {
                heatmap.density_values[y][x] /= max_density;
            }
        }
    }
    
    return heatmap;
}

std::vector<DataCollectionAnalyzer::Region> DataCollectionAnalyzer::detectSparseRegions(
    const Heatmap& heatmap, double sparse_threshold) {
    
    std::vector<Region> sparse_regions;
    
    for (int y = 0; y < heatmap.height; y++) {
        for (int x = 0; x < heatmap.width; x++) {
            if (heatmap.density_values[y][x] < sparse_threshold) {
                Point center(x * heatmap.resolution, y * heatmap.resolution);
                Region region(center, heatmap.resolution, true);
                sparse_regions.push_back(region);
            }
        }
    }
    
    return sparse_regions;
}

DataCollectionAnalyzer::PlannerWeights DataCollectionAnalyzer::adjustCostWeights(
    const std::vector<Region>& sparse_zones, const PlannerWeights& current_weights) {
    
    PlannerWeights adjusted_weights = current_weights;
    
    // If we have many sparse zones, increase exploration bonus
    if (sparse_zones.size() > 100) {
        adjusted_weights.exploration_bonus = std::min(1.0, current_weights.exploration_bonus * 1.2);
    }
    // If we have few sparse zones, decrease exploration bonus
    else if (sparse_zones.size() < 50) {
        adjusted_weights.exploration_bonus = std::max(0.1, current_weights.exploration_bonus * 0.8);
    }
    
    // Adjust redundancy penalty based on coverage
    double coverage_ratio = static_cast<double>(sparse_zones.size()) / 
                           (sparse_zones.size() > 0 ? sparse_zones.size() : 1);
    adjusted_weights.redundancy_penalty = 0.3 + coverage_ratio * 0.3;
    
    return adjusted_weights;
}

void DataCollectionAnalyzer::saveToPlannerConfig(const PlannerWeights& weights, 
                                                const std::string& config_path) {
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "sparse_threshold" << YAML::Value << weights.sparse_threshold;
        out << YAML::Key << "exploration_bonus" << YAML::Value << weights.exploration_bonus;
        out << YAML::Key << "redundancy_penalty" << YAML::Value << weights.redundancy_penalty;
        // out << YAML::Key << "grid_resolution" << YAML::Value << weights.grid_resolution;
        out << YAML::EndMap;
        
        std::ofstream fout(config_path);
        fout << out.c_str();
        fout.close();
        
        AD_INFO(DataCollectionPlanner, "Planner weights saved to %s", config_path.c_str());
    } catch (const std::exception& e) {
        AD_ERROR(DataCollectionPlanner, "Failed to save planner weights: %s", e.what());
    }
}

} //namespace dcl