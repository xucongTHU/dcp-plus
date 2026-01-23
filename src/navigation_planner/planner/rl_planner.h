// rl_planner.h
#ifndef RL_PLANNER_H
#define RL_PLANNER_H

#include <vector>
#include <string>
#include <memory>
#include <map>

// Include all component headers
#include "../value_map/costmap.h"
#include "../rl_policy/route_optimize.h"
#include "../rl_policy/reward_calculator.h"
#include "../rl_policy/ppo_agent.h"
#include "../value_map/coverage_metric.h"
#include "../value_map/sampling_optimizer.h"
#include "../safety_layer/semantic_map.h"
#include "../safety_layer/semantic_constraint.h"
#include "../safety_layer/semantic_filter.h"
#include "../utils/planner_utils.h"
#include "planner_base.hpp"

namespace dcp::planner {

class RLPlanner : public PlannerBase {
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
    std::string model_file_;
    std::map<std::string, double> planner_parameters_;
    
    // State variables
    Point current_position_;
    Point goal_position_;
    std::vector<Point> planner_path_;
    
    // Data collection statistics
    std::vector<Point> collected_data_points_;
    
    // RL parameters
    bool use_ppo_;  // Flag to enable PPO-based planning
    
public:
    RLPlanner(const std::string& model_file, const std::string& config_file);
    
    ~RLPlanner() override = default;

    // Implementation of PlannerBase interface
    void reset() override;
    Trajectory plan(const PlannerInput& input) override;
    
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
     * @brief Load PPO weights from file
     */
    bool loadPPOWeights(const std::string& filepath);
    
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
     * @brief Get the route planner instance for direct access
     */
    RoutePlanner* getRoutePlanner() { return route_planner_.get(); }

};

} // namespace dcp::planner
#endif // RL_PLANNER_H