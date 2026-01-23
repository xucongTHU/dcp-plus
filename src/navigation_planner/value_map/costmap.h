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
    
    // 构造函数，使用PRD文档中的规范创建24维状态向量
    State(double norm_lat, double norm_lon, 
          const std::vector<double>& heatmap_summary, 
          const std::vector<double>& last_n_actions,
          double remaining_budget_norm, 
          double local_traffic_density) : features() {
        // 添加归一化坐标 (2维)
        features.push_back(norm_lat);
        features.push_back(norm_lon);
        
        // 添加热力图摘要 (16维) - 需要确保有16个值
        for (size_t i = 0; i < 16; ++i) {
            if (i < heatmap_summary.size()) {
                features.push_back(heatmap_summary[i]);
            } else {
                features.push_back(0.0);  // 填充缺失值
            }
        }
        
        // 添加历史动作 (4维)
        for (size_t i = 0; i < 4; ++i) {
            if (i < last_n_actions.size()) {
                features.push_back(last_n_actions[i]);
            } else {
                features.push_back(0.0);  // 填充缺失值
            }
        }
        
        // 添加剩余预算归一化值 (1维)
        features.push_back(remaining_budget_norm);
        
        // 添加局部交通密度 (1维)
        features.push_back(local_traffic_density);
        
        // 确保总维度为24
        features.resize(24, 0.0);
    }
    
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

namespace dcp::planner {

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

} // namespace dcp::planner
#endif // COSTMAP_H