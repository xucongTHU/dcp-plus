// nav_planner_node.cpp
#include "nav_planner_node.h"
#include <iostream>
#include <algorithm>

namespace dcl::planner {

NavPlannerNode::NavPlannerNode(const std::string& config_file)
    : config_file_path_(config_file)
    , current_position_(0.0, 0.0)
    , goal_position_(0.0, 0.0)
    , use_ppo_(false) {
    // Initialize with default values
    costmap_ = std::make_unique<CostMap>(100, 100, 1.0);
    route_planner_ = std::make_unique<RoutePlanner>();
    sampling_optimizer_ = std::make_unique<SamplingOptimizer>();
    semantic_map_ = std::make_unique<SemanticMap>(100, 100, 1.0);
    constraint_checker_ = std::make_unique<SemanticConstraintChecker>(*semantic_map_);
    semantic_filter_ = std::make_unique<SemanticFilter>();
    coverage_metric_ = std::make_unique<CoverageMetric>();
}

bool NavPlannerNode::initialize() {
    LogUtils::log(LogUtils::INFO, "Initializing Navigation Planner Node");
    
    // Load configuration
    if (!loadConfiguration()) {
        LogUtils::log(LogUtils::ERROR, "Failed to load configuration");
        return false;
    }
    
    // Initialize components with loaded parameters
    double sparse_threshold = planner_parameters_["sparse_threshold"];
    double exploration_bonus = planner_parameters_["exploration_bonus"];
    double redundancy_penalty = planner_parameters_["redundancy_penalty"];
    double grid_resolution = planner_parameters_["grid_resolution"];
    
    // Update costmap resolution
    // Note: In a real implementation, we would recreate the costmap with new dimensions
    // For simplicity, we'll just update the parameters
    costmap_->setParameters(sparse_threshold, exploration_bonus, redundancy_penalty);
    
    // Update route planner parameters
    route_planner_->setSparseThreshold(sparse_threshold);
    route_planner_->setExplorationBonus(exploration_bonus);
    route_planner_->setRedundancyPenalty(redundancy_penalty);
    
    // Update sampling optimizer parameters
    SamplingParams sampling_params;
    sampling_params.sparse_threshold = sparse_threshold;
    sampling_params.exploration_weight = exploration_bonus;
    sampling_params.redundancy_penalty = redundancy_penalty;
    sampling_optimizer_->updateParameters(sampling_params);
    
    // Update coverage metric
    coverage_metric_ = std::make_unique<CoverageMetric>(sparse_threshold);
    
    LogUtils::log(LogUtils::INFO, "Navigation Planner Node initialized successfully");
    return true;
}

bool NavPlannerNode::loadConfiguration() {
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

void NavPlannerNode::updateCostmapWithStatistics() {
    LogUtils::log(LogUtils::INFO, "Updating costmap with data statistics");
    
    // Update costmap with collected data points
    costmap_->updateWithDataStatistics(collected_data_points_);
    
    // Adjust costs based on density
    costmap_->adjustCostsBasedOnDensity();
    
    // Apply semantic constraints to costmap
    constraint_checker_->applyConstraintsToCostmap(*costmap_);
    
    LogUtils::log(LogUtils::INFO, "Costmap updated with statistics");
}

std::vector<Point> NavPlannerNode::planGlobalPath() {
    LogUtils::log(LogUtils::INFO, "Planning global path from " + 
                 LogUtils::formatPoint(current_position_) + " to " + 
                 LogUtils::formatPoint(goal_position_));
    
    // Choose planning algorithm based on configuration
    if (use_ppo_) {
        LogUtils::log(LogUtils::INFO, "Using PPO-based path planning");
        global_path_ = route_planner_->computePPOPath(*costmap_, current_position_, goal_position_);
    } else {
        LogUtils::log(LogUtils::INFO, "Using A*-based path planning");
        global_path_ = route_planner_->computeAStarPath(*costmap_, current_position_, goal_position_);
    }
    
    // Validate path
    if (!validatePath(global_path_)) {
        LogUtils::log(LogUtils::WARN, "Planned global path has constraint violations");
    }
    
    LogUtils::log(LogUtils::INFO, "Global path planned with " + 
                 std::to_string(global_path_.size()) + " waypoints");
    
    return global_path_;
}

std::vector<Point> NavPlannerNode::planLocalPath() {
    LogUtils::log(LogUtils::INFO, "Planning local path");
    
    // For demonstration, we'll just take a segment of the global path
    // In a real implementation, this would use local costmap and more sophisticated planning
    size_t segment_size = std::min(static_cast<size_t>(10), global_path_.size());
    local_path_.clear();
    
    for (size_t i = 0; i < segment_size; ++i) {
        local_path_.push_back(global_path_[i]);
    }
    
    LogUtils::log(LogUtils::INFO, "Local path planned with " + 
                 std::to_string(local_path_.size()) + " waypoints");
    
    return local_path_;
}

Point NavPlannerNode::optimizeNextWaypoint() {
    LogUtils::log(LogUtils::INFO, "Optimizing next waypoint for data collection");
    
    // Use sampling optimizer to find next optimal point
    Point next_waypoint = sampling_optimizer_->optimizeNextSample(*costmap_, current_position_);
    
    LogUtils::log(LogUtils::INFO, "Next optimal waypoint: " + LogUtils::formatPoint(next_waypoint));
    
    return next_waypoint;
}

bool NavPlannerNode::validatePath(const std::vector<Point>& path) {
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

void NavPlannerNode::updateCoverageMetrics(const std::vector<std::pair<int, int>>& visited_cells) {
    LogUtils::log(LogUtils::INFO, "Updating coverage metrics");
    
    coverage_metric_->updateCoverage(*costmap_, visited_cells);
    
    LogUtils::log(LogUtils::INFO, "Coverage ratio: " + 
                 std::to_string(coverage_metric_->getCoverageRatio()));
    LogUtils::log(LogUtils::INFO, "Sparse coverage ratio: " + 
                 std::to_string(coverage_metric_->getSparseCoverageRatio()));
}

double NavPlannerNode::computeStateReward(const StateInfo& prev_state_info, 
                                         const StateInfo& new_state_info) {
    LogUtils::log(LogUtils::INFO, "Computing state reward");
    
    double reward = RewardCalculator::computeReward(prev_state_info, new_state_info);
    
    LogUtils::log(LogUtils::INFO, "Computed reward: " + std::to_string(reward));
    
    return reward;
}

void NavPlannerNode::reloadConfiguration() {
    LogUtils::log(LogUtils::INFO, "Reloading configuration");
    
    if (loadConfiguration()) {
        // Reinitialize with new parameters
        initialize();
        LogUtils::log(LogUtils::INFO, "Configuration reloaded successfully");
    } else {
        LogUtils::log(LogUtils::ERROR, "Failed to reload configuration");
    }
}

void NavPlannerNode::addDataPoint(const Point& point) {
    collected_data_points_.push_back(point);
    LogUtils::log(LogUtils::INFO, "Added data point: " + LogUtils::formatPoint(point));
}

bool NavPlannerNode::loadPPOWeights(const std::string& filepath) {
    if (route_planner_->getPPOAgent()) {
        bool success = route_planner_->getPPOAgent()->loadWeights(filepath);
        if (success) {
            LogUtils::log(LogUtils::INFO, "PPO weights loaded from " + filepath);
            return true;
        } else {
            LogUtils::log(LogUtils::ERROR, "Failed to load PPO weights from " + filepath);
            return false;
        }
    }
    LogUtils::log(LogUtils::WARN, "PPO agent not available");
    return false;
}

bool NavPlannerNode::savePPOWeights(const std::string& filepath) {
    if (route_planner_->getPPOAgent()) {
        bool success = route_planner_->getPPOAgent()->saveWeights(filepath);
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

} // namespace dcl::planner