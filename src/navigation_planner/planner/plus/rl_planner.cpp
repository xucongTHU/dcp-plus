// rl_planner.cpp
#include "rl_planner.h"
#include <iostream>
#include <algorithm>

namespace dcp::planner {

RLPlanner::RLPlanner(const std::string& model_file, const std::string& config_file)
    : config_file_path_(config_file)
    , model_file_(model_file)
    , current_position_(Point(0.0, 0.0))
    , goal_position_(Point(0.0, 0.0))
    , use_ppo_(true) {
    LogUtils::log(LogUtils::INFO, "Creating RLPlanner with model_file: " + model_file + ", config_file: " + config_file);
    // Initialize with default values based on PRD: configurable 2D grid (default 20x20)
    int default_map_width = 20;
    int default_map_height = 20;
    double default_resolution = 1.0;
    
    costmap_ = std::make_unique<CostMap>(default_map_width, default_map_height, default_resolution);
    route_planner_ = std::make_unique<RoutePlanner>();
    sampling_optimizer_ = std::make_unique<SamplingOptimizer>();
    semantic_map_ = std::make_unique<SemanticMap>(default_map_width, default_map_height, default_resolution);
    constraint_checker_ = std::make_unique<SemanticConstraintChecker>(*semantic_map_);
    semantic_filter_ = std::make_unique<SemanticFilter>();
    coverage_metric_ = std::make_unique<CoverageMetric>();

}

bool RLPlanner::initialize() {
    LogUtils::log(LogUtils::INFO, "Initializing Navigation Planner Node");
    
    // Load configuration
    if (!loadConfiguration()) {
        LogUtils::log(LogUtils::ERROR, "Failed to load configuration");
        return false;
    }

    // Load PPO weights if using PPO-based planning
    loadPPOWeights(model_file_);
    
    // Update costmap resolution
    auto sparse_it = planner_parameters_.find("sparse_threshold");
    auto exploration_it = planner_parameters_.find("exploration_bonus");
    auto redundancy_it = planner_parameters_.find("redundancy_penalty");

    if (sparse_it != planner_parameters_.end() && exploration_it != planner_parameters_.end() && redundancy_it != planner_parameters_.end()) {
        costmap_->setParameters(sparse_it->second, exploration_it->second, redundancy_it->second);
    }
    
    // Update route planner parameters
    if (sparse_it != planner_parameters_.end()) {
        route_planner_->setSparseThreshold(sparse_it->second);
    }
    if (exploration_it != planner_parameters_.end()) {
        route_planner_->setExplorationBonus(exploration_it->second);
    }
    if (redundancy_it != planner_parameters_.end()) {
        route_planner_->setRedundancyPenalty(redundancy_it->second);
    }
    
    // Update sampling optimizer parameters
    SamplingParams sampling_params;
    sampling_params.sparse_threshold = (sparse_it != planner_parameters_.end()) ? sparse_it->second : 0.2;
    sampling_params.exploration_weight = (exploration_it != planner_parameters_.end()) ? exploration_it->second : 1.0;
    sampling_params.redundancy_penalty = (redundancy_it != planner_parameters_.end()) ? redundancy_it->second : 5.0;

    auto efficiency_it = planner_parameters_.find("sampling_params_efficiency_weight");
    sampling_params.efficiency_weight = (efficiency_it != planner_parameters_.end()) ? efficiency_it->second : 0.5;

    sampling_optimizer_->updateParameters(sampling_params);
    
    // Update coverage metric
    coverage_metric_ = std::make_unique<CoverageMetric>(sampling_params.sparse_threshold);
    
    // Update PPO agent configuration from parameters
    if (route_planner_->getPPOPolicy()) {
        route_planner_->getPPOPolicy()->updateConfigFromParameters(planner_parameters_);
    }
    
    // Set use_ppo flag from configuration
    auto ppo_it = planner_parameters_.find("nav_planner_use_ppo");
    if (ppo_it != planner_parameters_.end()) {
        use_ppo_ = (ppo_it->second > 0.5); // Convert double to bool (1.0 = true, 0.0 = false)
    }
    
    LogUtils::log(LogUtils::INFO, "Navigation Planner Node initialized successfully");
    return true;
}

bool RLPlanner::loadConfiguration() {
    LogUtils::log(LogUtils::INFO, "Loading configuration from " + config_file_path_);
    
    if (!PlannerUtils::loadParametersFromYaml(config_file_path_, planner_parameters_)) {
        LogUtils::log(LogUtils::ERROR, "Failed to load parameters from YAML file");
        return false;
    }
    
    // Log loaded parameters
    for (const auto& param : planner_parameters_) {
        LogUtils::log(LogUtils::DEBUG, "Loaded parameter: " + param.first + " = " + 
                     std::to_string(param.second));
    }
    
    return true;
}

void RLPlanner::updateCostmapWithStatistics() {
    LogUtils::log(LogUtils::INFO, "Updating costmap with data statistics");
    
    // Update costmap with collected data points from trajectory
    costmap_->updateWithDataStatistics(collected_data_points_);
    
    // Adjust costs based on density
    costmap_->adjustCostsBasedOnDensity();
    
    // Apply semantic constraints to costmap
    constraint_checker_->applyConstraintsToCostmap(*costmap_);
    
    LogUtils::log(LogUtils::INFO, "Costmap updated with statistics");
}

Point RLPlanner::optimizeNextWaypoint() {
    LogUtils::log(LogUtils::INFO, "Optimizing next waypoint for data collection");
    
    // Use sampling optimizer to find next optimal point
    Point next_waypoint = sampling_optimizer_->optimizeNextSample(*costmap_, current_position_);
    
    LogUtils::log(LogUtils::INFO, "Next optimal waypoint: " + LogUtils::formatPoint(next_waypoint));
    
    return next_waypoint;
}

bool RLPlanner::validatePath(const std::vector<Point>& path) {
    if (path.empty()) {
        return false;
    }
    
    LogUtils::log(LogUtils::INFO, "Validating path with " + std::to_string(path.size()) + " waypoints");
    
    // Check path validity against costmap
    if (!PlannerUtils::isPathValid(path, *costmap_)) {
        LogUtils::log(LogUtils::WARN, "Path has collisions with obstacles");
        return false;
    }
    
    // Check semantic constraints
    auto violations = constraint_checker_->checkPathConstraints(path);
    if (!violations.empty()) {
        LogUtils::log(LogUtils::WARN, "Path violates " + std::to_string(violations.size()) + " constraints");
        for (const auto& violation : violations) {
            LogUtils::log(LogUtils::WARN, "Constraint violation: " + violation.description);
        }
        // Depending on severity, we might still consider the path valid
    }
    
    LogUtils::log(LogUtils::INFO, "Path validation completed");
    return true;
}

void RLPlanner::updateCoverageMetrics(const std::vector<std::pair<int, int>>& visited_cells) {
    LogUtils::log(LogUtils::INFO, "Updating coverage metrics");
    
    coverage_metric_->updateCoverage(*costmap_, visited_cells);
    
    LogUtils::log(LogUtils::INFO, "Coverage ratio: " + 
                 std::to_string(coverage_metric_->getCoverageRatio()));
    LogUtils::log(LogUtils::INFO, "Sparse coverage ratio: " + 
                 std::to_string(coverage_metric_->getSparseCoverageRatio()));
}

double RLPlanner::computeStateReward(const StateInfo& prev_state_info, 
                                         const StateInfo& new_state_info) {
    LogUtils::log(LogUtils::INFO, "Computing state reward");
    
    double reward = RewardCalculator::computeReward(prev_state_info, new_state_info);
    
    LogUtils::log(LogUtils::INFO, "Computed reward: " + std::to_string(reward));
    
    return reward;
}

void RLPlanner::reloadConfiguration() {
    LogUtils::log(LogUtils::INFO, "Reloading configuration");

    if (loadConfiguration()) {
        // Reinitialize with new parameters
        initialize();
        LogUtils::log(LogUtils::INFO, "Configuration reloaded successfully");
    } else {
        LogUtils::log(LogUtils::ERROR, "Failed to reload configuration");
    }
}

void RLPlanner::addDataPoint(const Point& point) {
    collected_data_points_.push_back(point);
    LogUtils::log(LogUtils::INFO, "Added data point: " + LogUtils::formatPoint(point));
}

bool RLPlanner::loadPPOWeights(const std::string& filepath) {
    LogUtils::log(LogUtils::INFO, "Loading PPO weights file from " + filepath);
    if (route_planner_->getPPOPolicy()) {
        bool success = route_planner_->getPPOPolicy()->loadWeights(filepath);
        if (success) {
            LogUtils::log(LogUtils::INFO, "PPO weights loaded successfully from " + filepath);
            return true;
        } else {
            LogUtils::log(LogUtils::ERROR, "Failed to load PPO weights from " + filepath);
            return false;
        }
    }
    LogUtils::log(LogUtils::WARN, "PPO agent not available");
    return false;
}

bool RLPlanner::savePPOWeights(const std::string& filepath) {
    if (route_planner_->getPPOPolicy()) {
        bool success = route_planner_->getPPOPolicy()->saveWeights(filepath);
        if (success) {
            LogUtils::log(LogUtils::INFO, "PPO weights saved to " + filepath);
            return true;
        } else {
            LogUtils::log(LogUtils::ERROR, "Failed to save PPO weights to " + filepath);
            return false;
        }
    }
    LogUtils::log(LogUtils::WARN, "PPO agent not available");
    return false;
}

void RLPlanner::reset() {
    // Reset planner state
    current_position_ = Point(0.0, 0.0);
    goal_position_ = Point(0.0, 0.0);
    planner_path_.clear();
    collected_data_points_.clear();
    
    LogUtils::log(LogUtils::INFO, "Navigation Planner Node reset");
}

Trajectory RLPlanner::plan(const PlannerInput& input) {
    // Set the current and goal positions from the input
    current_position_ = input.start;
    goal_position_ = input.goal;
    
    // Plan path using the navigation planner
    if (use_ppo_) {
        planner_path_ = route_planner_->computePPOPath(*costmap_, input.start, input.goal);
    } else {
        planner_path_ = route_planner_->computeAStarPath(*costmap_, input.start, input.goal);
    }
    
    // Create trajectory from the path
    Trajectory trajectory;
    trajectory.states = planner_path_;

    // Validate path
    if (!validatePath(planner_path_)) {
        LogUtils::log(LogUtils::WARN, "Planned path has constraint violations");
    }
    
    // // Calculate total cost
    // if (!path.empty()) {
    //     for (size_t i = 1; i < path.size(); ++i) {
    //         double segment_cost = PlannerUtils::euclideanDistance(path[i-1], path[i]);
    //         // trajectory.total_cost += segment_cost;//TODO
    //     }
    // }
    
    return trajectory;
}

} // namespace dcp::planner