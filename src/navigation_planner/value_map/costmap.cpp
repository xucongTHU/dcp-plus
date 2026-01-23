// costmap.cpp
#include "costmap.h"
#include <algorithm>
#include <cstring>

namespace dcp::planner {
CostMap::CostMap(int width, int height, double resolution) 
    : width(width), height(height), resolution(resolution),
      sparse_threshold(0.2), exploration_bonus(0.5), redundancy_penalty(0.4) {
    cells.resize(height, std::vector<Cell>(width, Cell(0, 0, 0.0, 0.0)));
    
    // Initialize cell coordinates
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            cells[y][x] = Cell(x, y, 0.0, 0.0);
        }
    }
}

void CostMap::setParameters(double sparse_threshold, double exploration_bonus, double redundancy_penalty) {
    this->sparse_threshold = sparse_threshold;
    this->exploration_bonus = exploration_bonus;
    this->redundancy_penalty = redundancy_penalty;
}

void CostMap::updateWithDataStatistics(const std::vector<Point>& data_points) {
    // Reset data density
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            cells[y][x].data_density = 0.0;
        }
    }
    
    // Calculate density based on data points
    for (const auto& point : data_points) {
        int cell_x = static_cast<int>(point.x / resolution);
        int cell_y = static_cast<int>(point.y / resolution);
        
        if (isValidCell(cell_x, cell_y)) {
            cells[cell_y][cell_x].data_density += 1.0;
        }
    }
    
    // Normalize densities (optional)
    double max_density = 0.0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            max_density = std::max(max_density, cells[y][x].data_density);
        }
    }
    
    if (max_density > 0.0) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                cells[y][x].data_density /= max_density;
            }
        }
    }
}

void CostMap::adjustCostsBasedOnDensity() {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Adjust cost based on data density
            // Lower cost for sparse areas (encourage exploration)
            // Higher cost for dense areas (discourage redundancy)
            if (cells[y][x].data_density < sparse_threshold) {
                cells[y][x].cost -= exploration_bonus;
            } else {
                cells[y][x].cost += redundancy_penalty;
            }
        }
    }
}

double CostMap::getDataDensity(int x, int y) const {
    if (!isValidCell(x, y)) {
        return 0.0;
    }
    return cells[y][x].data_density;
}

void CostMap::setCellCost(int x, int y, double cost) {
    if (isValidCell(x, y)) {
        cells[y][x].cost = cost;
    }
}

double CostMap::getCellCost(int x, int y) const {
    if (!isValidCell(x, y)) {
        return 0.0;
    }
    return cells[y][x].cost;
}

bool CostMap::isValidCell(int x, int y) const {
    return (x >= 0 && x < width && y >= 0 && y < height);
}

} // namespace dcp::planner