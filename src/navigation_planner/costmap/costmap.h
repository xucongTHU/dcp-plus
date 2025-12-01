// costmap.h
#ifndef COSTMAP_H
#define COSTMAP_H

#include <vector>
#include <cmath>

struct Point {
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
};

struct Cell {
    int x, y;
    double cost;
    double data_density;
    
    Cell(int x = 0, int y = 0, double cost = 0.0, double density = 0.0) 
        : x(x), y(y), cost(cost), data_density(density) {}
};

namespace dcl::planner {

class CostMap {
private:
    std::vector<std::vector<Cell>> cells;
    int width, height;
    double resolution; // meters per cell
    double sparse_threshold;
    double exploration_bonus;
    double redundancy_penalty;

public:
    CostMap(int width, int height, double resolution);
    
    void setParameters(double sparse_threshold, double exploration_bonus, double redundancy_penalty);
    
    void updateWithDataStatistics(const std::vector<Point>& data_points);
    
    void adjustCostsBasedOnDensity();
    
    double getDataDensity(int x, int y) const;
    
    void setCellCost(int x, int y, double cost);
    
    double getCellCost(int x, int y) const;
    
    bool isValidCell(int x, int y) const;
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    double getResolution() const { return resolution; }
    
    // Getters for parameters
    double getSparseThreshold() const { return sparse_threshold; }
    double getExplorationBonus() const { return exploration_bonus; }
    double getRedundancyPenalty() const { return redundancy_penalty; }
};

} // namespace dcl::planner
#endif // COSTMAP_H