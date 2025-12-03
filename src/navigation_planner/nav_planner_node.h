// nav_planner_node.h
#ifndef NAV_PLANNER_NODE_H
#define NAV_PLANNER_NODE_H

#include <vector>
#include <string>
#include <memory>
#include <map>

// Include all component headers
#include "costmap/costmap.h"
#include "rl/planner_route_optimize.h"
#include "rl/planner_reward.h"
#include "rl/ppo_agent.h"
#include "sampler/coverage_metric.h"
#include "sampler/sampling_optimizer.h"
#include "semantics/semantic_map.h"
#include "semantics/semantic_constraint.h"
#include "semantics/semantic_filter.h"
#include "utils/planner_utils.h"

namespace dcl::planner {

class NavPlannerNode {
private:
    // Core components
    std::unique_ptr<CostMap> costmap_;
    std::unique_ptr<RoutePlanner> route_planner_;
    std::unique_ptr<SamplingOptimizer> sampling_optimizer_;
    std::unique_ptr<SemanticMap> semantic_map_;
    std::unique_ptr<SemanticConstraintChecker> constraint_checker_;
    std::unique_ptr<SemanticFilter> semantic_filter_;
    std::unique_ptr<CoverageMetric> coverage_metric_;
    
    // Configuration
    std::string config_file_path_;
    std::map<std::string, double> planner_parameters_;
    
    // State variables
    Point current_position_;
    Point goal_position_;
    std::vector<Point> global_path_;
    std::vector<Point> local_path_;
    
    // Data collection statistics
    std::vector<Point> collected_data_points_;
    
    // RL parameters
    bool use_ppo_;  // Flag to enable PPO-based planning
    
public:
    NavPlannerNode(const std::string& config_file = 
                   "/workspaces/ad_data_closed_loop/infra/navigation_planner/config/planner_weights.yaml");
    
    ~NavPlannerNode() = default;
    
    /**
     * @brief Initialize the navigation planner node
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Load configuration parameters from YAML file
     * @return true if loading successful, false otherwise
     */
    bool loadConfiguration();
    
    /**
     * @brief Update the costmap with new data statistics
     */
    void updateCostmapWithStatistics();
    
    /**
     * @brief Plan global path from current position to goal
     * @return Planned path
     */
    std::vector<Point> planGlobalPath();
    
    /**
     * @brief Plan local path/replan as needed
     * @return Local path
     */
    std::vector<Point> planLocalPath();
    
    /**
     * @brief Optimize next waypoint for data collection
     * @return Next optimal waypoint
     */
    Point optimizeNextWaypoint();
    
    /**
     * @brief Check if planned path satisfies all constraints
     * @param path Path to validate
     * @return true if path is valid, false otherwise
     */
    bool validatePath(const std::vector<Point>& path);
    
    /**
     * @brief Update coverage metrics based on visited locations
     * @param visited_cells List of visited cell coordinates
     */
    void updateCoverageMetrics(const std::vector<std::pair<int, int>>& visited_cells);
    
    /**
     * @brief Compute reward for current state transition
     * @param prev_state_info Information about previous state
     * @param new_state_info Information about new state
     * @return Computed reward
     */
    double computeStateReward(const StateInfo& prev_state_info, 
                             const StateInfo& new_state_info);
    
    /**
     * @brief Reload planner configuration (e.g., when weights file is updated)
     */
    void reloadConfiguration();
    
    /**
     * @brief Add a new data point to collected data
     */
    void addDataPoint(const Point& point);
    
    /**
     * @brief Enable or disable PPO-based planning
     */
    void setUsePPO(bool use_ppo) { use_ppo_ = use_ppo; }
    
    /**
     * @brief Load PPO weights from file
     */
    bool loadPPOWeights(const std::string& filepath);
    
    /**
     * @brief Save PPO weights to file
     */
    bool savePPOWeights(const std::string& filepath);
    
    /**
     * @brief Get current coverage metrics
     */
    const CoverageMetric& getCoverageMetric() const { return *coverage_metric_; }
    
    /**
     * @brief Get current position
     */
    const Point& getCurrentPosition() const { return current_position_; }
    
    /**
     * @brief Set current position
     */
    void setCurrentPosition(const Point& position) { current_position_ = position; }
    
    /**
     * @brief Get goal position
     */
    const Point& getGoalPosition() const { return goal_position_; }
    
    /**
     * @brief Set goal position
     */
    void setGoalPosition(const Point& position) { goal_position_ = position; }
    
    /**
     * @brief Get global path
     */
    const std::vector<Point>& getGlobalPath() const { return global_path_; }
    
    /**
     * @brief Get local path
     */
    const std::vector<Point>& getLocalPath() const { return local_path_; }
};

} // namespace dcl::planner
#endif // NAV_PLANNER_NODE_H