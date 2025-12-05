// costmap.h
#ifndef COSTMAP_H
#define COSTMAP_H

#include <vector>
#include <cmath>

// State结构体，用于表示标准化后的特征向量
struct State {
    std::vector<double> features;  // 标准化后的特征向量
    
    // 默认构造函数
    State() = default;
    
    // 构造函数，接受特征向量
    explicit State(const std::vector<double>& feats) : features(feats) {}
    
    // 获取特征维度
    size_t getFeatureDim() const { return features.size(); }
    
    // 访问特定特征
    double operator[](size_t index) const { 
        return (index < features.size()) ? features[index] : 0.0; 
    }
    
    // 设置特征值
    void setFeature(size_t index, double value) {
        if (index >= features.size()) {
            features.resize(index + 1, 0.0);
        }
        features[index] = value;
    }
    
    // 添加特征到末尾
    void addFeature(double value) {
        features.push_back(value);
    }
    
    // 清空所有特征
    void clear() {
        features.clear();
    }
    
    // 获取特征数量
    size_t size() const {
        return features.size();
    }
};

struct Point {
    double x, y;
    Point(double x = 0, double y = 0) : x(x), y(y) {}
    
    // 转换为State对象的方法
    State toState() const {
        std::vector<double> features = {x, y};
        return State(features);
    }
    
    // 根据地图尺寸转换为标准化State对象
    State toNormalizedState(int map_width, int map_height) const {
        std::vector<double> features;
        // 添加标准化坐标
        features.push_back(x / map_width);
        features.push_back(y / map_height);
        // 填充其余特征为0，满足最小state_dim要求
        features.resize(24, 0.0);
        return State(features);
    }
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