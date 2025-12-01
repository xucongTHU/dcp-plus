// planner_optRoute.h
#ifndef PLANNER_OPTROUTE_H
#define PLANNER_OPTROUTE_H

#include "../costmap/costmap.h"
#include <vector>


namespace dcl::planner {
struct MapData {
    CostMap costmap;
    
    MapData(int width, int height, double resolution) : costmap(width, height, resolution) {}
    
    CostMap toCostMap() const {
        return costmap;
    }
};

struct DataStats {
    std::vector<std::vector<double>> density_map;
    int width, height;
    
    DataStats(int width, int height) : width(width), height(height) {
        density_map.resize(height, std::vector<double>(width, 0.0));
    }
    
    double dataDensity(const Point& position) const {
        int x = static_cast<int>(position.x);
        int y = static_cast<int>(position.y);
        
        if (x >= 0 && x < width && y >= 0 && y < height) {
            return density_map[y][x];
        }
        return 0.0;
    }
};

class RoutePlanner {
private:
    double threshold_sparse;
    double exploration_bonus;
    double redundancy_penalty;

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
                                       
    // Setters for parameters
    void setSparseThreshold(double threshold) { threshold_sparse = threshold; }
    void setExplorationBonus(double bonus) { exploration_bonus = bonus; }
    void setRedundancyPenalty(double penalty) { redundancy_penalty = penalty; }
};

} // namespace dcl::planner

#endif // PLANNER_OPTROUTE_H