// planner_route_optimize.h
#ifndef PLANNER_ROUTE_OPTIMIZE_H
#define PLANNER_ROUTE_OPTIMIZE_H

#include <vector>
#include <memory>
#include "../costmap/costmap.h"
#include "ppo_agent.h"

namespace dcp {
namespace planner {

// Forward declarations
class PPOAgent;
struct MapData {
    int width, height;
    double resolution;
    
    MapData(int w = 0, int h = 0, double res = 1.0) 
        : width(w), height(h), resolution(res) {}
    
    CostMap toCostMap() const {
        return CostMap(width, height, resolution);
    }
};

struct DataStats {
    double sparse_threshold;
    double exploration_bonus;
    double redundancy_penalty;
    
    DataStats(double threshold = 0.2, double bonus = 0.5, double penalty = 0.4)
        : sparse_threshold(threshold), exploration_bonus(bonus), redundancy_penalty(penalty) {}
    
    double dataDensity(const Point& position) const {
        // Simplified implementation
        // Unused parameter warning fixed by adding (void) cast
        (void)position;
        return 0.0;
    }
};

class RoutePlanner {
private:
    double threshold_sparse;
    double exploration_bonus;
    double redundancy_penalty;
    
    std::unique_ptr<PPOAgent> ppo_agent_;

public:
    RoutePlanner(double sparse_threshold = 0.2, 
                double exploration_bonus = 0.5,
                double redundancy_penalty = 0.4);
    
    /**
     * @brief Optimize route based on map data and data statistics
     * @param map Current map data including costmap
     * @param stats Data statistics including density information
     */
    void optRoute(const MapData& map, const DataStats& stats);
    
    /**
     * @brief Compute path using A* algorithm
     * @param costmap Costmap with adjusted costs
     * @param start Start position
     * @param goal Goal position
     * @return Planned path
     */
    std::vector<Point> computeAStarPath(const CostMap& costmap, 
                                       const Point& start, 
                                       const Point& goal);
                                       
    /**
     * @brief Compute path using PPO-based reinforcement learning
     * @param costmap Costmap with adjusted costs
     * @param start Start position
     * @param goal Goal position
     * @return Planned path
     */
    std::vector<Point> computePPOPath(const CostMap& costmap,
                                     const Point& start,
                                     const Point& goal);
                                       
    // Setters for parameters
    void setSparseThreshold(double threshold) { threshold_sparse = threshold; }
    void setExplorationBonus(double bonus) { exploration_bonus = bonus; }
    void setRedundancyPenalty(double penalty) { redundancy_penalty = penalty; }
    
    /**
     * @brief Set PPO agent for RL-based planning
     * @param agent PPO agent
     */
    void setPPOAgent(std::unique_ptr<PPOAgent> agent) { 
        ppo_agent_ = std::move(agent); 
    }
    
    /**
     * @brief Get PPO agent
     * @return Pointer to PPO agent
     */
    PPOAgent* getPPOAgent() { 
        return ppo_agent_.get(); 
    }
};

} // namespace planner
} // namespace dcp

#endif // PLANNER_ROUTE_OPTIMIZE_H