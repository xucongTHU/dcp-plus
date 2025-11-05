// data_collection_planner.cpp
#include "data_collection_planner.h"
#include <iostream>
#include <fstream>
#include <random>
#include <yaml-cpp/yaml.h>

DataCollectionPlanner::DataCollectionPlanner(const std::string& config_file) {
    nav_planner_ = std::make_unique<NavPlannerNode>(config_file);
    mission_area_ = MissionArea(Point(50.0, 50.0), 30.0); // Default mission area
}

bool DataCollectionPlanner::initialize() {
    LogUtils::log(LogUtils::INFO, "Initializing Data Collection Planner");
    
    if (!nav_planner_->initialize()) {
        LogUtils::log(LogUtils::ERROR, "Failed to initialize navigation planner");
        return false;
    }
    
    LogUtils::log(LogUtils::INFO, "Data Collection Planner initialized successfully");
    return true;
}

void DataCollectionPlanner::setMissionArea(const MissionArea& area) {
    mission_area_ = area;
    nav_planner_->setGoalPosition(area.center);
    LogUtils::log(LogUtils::INFO, "Mission area set to center: " + 
                 LogUtils::formatPoint(area.center) + ", radius: " + 
                 std::to_string(area.radius));
}

std::vector<Point> DataCollectionPlanner::planDataCollectionMission() {
    LogUtils::log(LogUtils::INFO, "Planning data collection mission");
    
    // Plan global path using navigation planner
    std::vector<Point> collection_path = nav_planner_->planGlobalPath();
    
    // Optimize waypoints for data collection
    std::vector<Point> optimized_waypoints;
    for (const auto& point : collection_path) {
        Point optimal_point = nav_planner_->optimizeNextWaypoint();
        optimized_waypoints.push_back(optimal_point);
    }
    
    LogUtils::log(LogUtils::INFO, "Data collection mission planned with " + 
                 std::to_string(optimized_waypoints.size()) + " waypoints");
    
    return optimized_waypoints;
}

void DataCollectionPlanner::executeDataCollection(const std::vector<Point>& path) {
    LogUtils::log(LogUtils::INFO, "Executing data collection along path with " + 
                 std::to_string(path.size()) + " waypoints");
    
    // Simulate data collection at each waypoint
    std::vector<DataPoint> new_data;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    StateInfo prev_state;
    
    for (size_t i = 0; i < path.size(); ++i) {
        const Point& waypoint = path[i];
        
        // Update planner's current position
        nav_planner_->setCurrentPosition(waypoint);
        
        // Simulate data collection
        DataPoint data_point(waypoint, "sensor_data_" + std::to_string(i), i);
        new_data.push_back(data_point);
        
        // Simulate state information for reward calculation
        StateInfo current_state;
        current_state.visited_new_sparse = (dis(gen) > 0.7); // 30% chance of visiting sparse area
        current_state.trigger_success = (dis(gen) > 0.5);   // 50% chance of successful trigger
        current_state.collision = (dis(gen) < 0.05);        // 5% chance of collision
        current_state.distance_to_sparse = dis(gen) * 10.0; // Random distance to sparse area
        
        // Compute reward for this action
        double reward = nav_planner_->computeStateReward(prev_state, current_state);
        LogUtils::log(LogUtils::INFO, "Waypoint " + std::to_string(i) + 
                     " reward: " + std::to_string(reward));
        
        prev_state = current_state;
    }
    
    // Update with new data
    updateWithNewData(new_data);
    
    LogUtils::log(LogUtils::INFO, "Data collection completed with " + 
                 std::to_string(new_data.size()) + " data points collected");
}

void DataCollectionPlanner::updateWithNewData(const std::vector<DataPoint>& new_data) {
    LogUtils::log(LogUtils::INFO, "Updating planner with " + 
                 std::to_string(new_data.size()) + " new data points");
    
    // Add new data points to planner and local storage
    for (const auto& data_point : new_data) {
        nav_planner_->addDataPoint(data_point.position);
        collected_data_.push_back(data_point);
    }
    
    // Update costmap with new statistics
    nav_planner_->updateCostmapWithStatistics();
    
    // Update coverage metrics (simplified)
    std::vector<std::pair<int, int>> visited_cells;
    for (const auto& data_point : new_data) {
        auto grid_coords = PlannerUtils::worldToGrid(data_point.position, 1.0);
        visited_cells.push_back(grid_coords);
    }
    nav_planner_->updateCoverageMetrics(visited_cells);
    
    LogUtils::log(LogUtils::INFO, "Planner updated with new data");
}

void DataCollectionPlanner::reportCoverageMetrics() {
    const CoverageMetric& metrics = nav_planner_->getCoverageMetric();
    LogUtils::log(LogUtils::INFO, "=== Coverage Metrics Report ===");
    LogUtils::log(LogUtils::INFO, "Total coverage: " + 
                 std::to_string(metrics.getCoverageRatio() * 100) + "%");
    LogUtils::log(LogUtils::INFO, "Sparse area coverage: " + 
                 std::to_string(metrics.getSparseCoverageRatio() * 100) + "%");
    LogUtils::log(LogUtils::INFO, "Total cells: " + 
                 std::to_string(metrics.getTotalCells()));
    LogUtils::log(LogUtils::INFO, "Visited cells: " + 
                 std::to_string(metrics.getVisitedCells()));
    LogUtils::log(LogUtils::INFO, "Total sparse cells: " + 
                 std::to_string(metrics.getTotalSparseCells()));
    LogUtils::log(LogUtils::INFO, "Visited sparse cells: " + 
                 std::to_string(metrics.getVisitedSparseCells()));
    LogUtils::log(LogUtils::INFO, "===============================");
}

void DataCollectionPlanner::analyzeAndExportWeights() {
    LogUtils::log(LogUtils::INFO, "Analyzing collected data and updating planner weights");
    
    // Compute density map from collected data
    auto density_map = DataCollectionAnalyzer::computeDensityMap(collected_data_);
    
    // Detect sparse regions
    auto sparse_zones = DataCollectionAnalyzer::detectSparseRegions(density_map);
    
    // Get current weights from planner
    DataCollectionAnalyzer::PlannerWeights current_weights;
    // In a real implementation, we would retrieve these from the planner
    
    // Adjust cost weights based on analysis
    auto new_weights = DataCollectionAnalyzer::adjustCostWeights(sparse_zones, current_weights);
    
    // Save to planner configuration file
    DataCollectionAnalyzer::saveToPlannerConfig(new_weights, 
        "/workspaces/ad_data_closed_loop/infra/navigation_planner/config/planner_weights.yaml");
    
    // Notify planner to reload configuration
    nav_planner_->reloadConfiguration();
    
    LogUtils::log(LogUtils::INFO, "Planner weights updated and configuration reloaded");
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
        out << YAML::Key << "grid_resolution" << YAML::Value << weights.grid_resolution;
        out << YAML::EndMap;
        
        std::ofstream fout(config_path);
        fout << out.c_str();
        fout.close();
        
        LogUtils::log(LogUtils::INFO, "Planner weights saved to " + config_path);
    } catch (const std::exception& e) {
        LogUtils::log(LogUtils::ERROR, "Failed to save planner weights: " + std::string(e.what()));
    }
}