// planner_route_optimize.cpp
#include "planner_route_optimize.h"
#include <iostream>
#include <queue>
#include <cmath>
#include <random>
#include <functional>


namespace dcp {
namespace planner {
RoutePlanner::RoutePlanner(double sparse_threshold, 
                          double exploration_bonus,
                          double redundancy_penalty)
    : threshold_sparse(sparse_threshold)
    , exploration_bonus(exploration_bonus)
    , redundancy_penalty(redundancy_penalty) {
    // Initialize PPO agent with default configuration
    PPOConfig config;
    ppo_agent_ = std::make_unique<PPOAgent>(config);
    
    // 设置默认state_dim为24，符合规范中的最小值
    if (ppo_agent_) {
        ppo_agent_->setStateDim(24);
    }
}

void RoutePlanner::optRoute(const MapData& map, const DataStats& stats) {
    CostMap costmap = map.toCostMap();
    
    // Adjust costs based on data density statistics
    for (int y = 0; y < costmap.getHeight(); y++) {
        for (int x = 0; x < costmap.getWidth(); x++) {
            Point position(x, y);
            double current_cost = costmap.getCellCost(x, y);
            
            // Adjust cost based on data density
            if (stats.dataDensity(position) < threshold_sparse) {
                // Decrease cost for sparse areas (encourage exploration)
                costmap.setCellCost(x, y, current_cost - exploration_bonus);
            } else {
                // Increase cost for high-density areas (encourage avoidance)
                costmap.setCellCost(x, y, current_cost + redundancy_penalty);
            }
        }
    }
    
    // Note: In a complete implementation, this would store the optimized route
    // For now, we're just demonstrating the cost adjustment logic
    std::cout << "Route optimized based on data statistics" << std::endl;
}

std::vector<Point> RoutePlanner::computeAStarPath(const CostMap& costmap, 
                                                 const Point& start, 
                                                 const Point& goal) {
    // This is a simplified placeholder for A* implementation
    // A complete implementation would include:
    // 1. Priority queue for open set
    // 2. Heuristic function (e.g., Euclidean distance)
    // 3. Path reconstruction from goal to start
    
    std::vector<Point> path;
    path.push_back(start);
    path.push_back(goal);
    
    // In a real implementation, this would contain the actual A* algorithm
    std::cout << "A* path computed from (" << start.x << "," << start.y 
              << ") to (" << goal.x << "," << goal.y << ")" << std::endl;
              
    return path;
}

std::vector<Point> RoutePlanner::computePPOPath(const CostMap& costmap,
                                               const Point& start,
                                               const Point& goal) {
    std::vector<Point> path;
    
    if (!ppo_agent_) {
        std::cerr << "PPO agent not initialized!" << std::endl;
        return path;
    }
    
    Point current_pos = start;
    path.push_back(current_pos);
    
    // Maximum steps as per PRD: 200 steps per episode
    const int max_steps = ppo_agent_->getMaxEpisodeSteps(); // Use config value which defaults to 200 per PRD
    int steps = 0;
    
    // Define action space as per PRD: forward, turn left, turn right, turn around
    // We need to track current orientation for directional actions
    int current_orientation = 0; // 0: right, 1: up, 2: left, 3: down
    
    // Define action transformations
    std::vector<std::function<Point(int)>> action_transforms = {
        // Action 0: Forward - move in current direction
        [](int orientation) -> Point {
            switch(orientation) {
                case 0: return Point(1, 0);  // Move right
                case 1: return Point(0, 1);  // Move up
                case 2: return Point(-1, 0); // Move left
                case 3: return Point(0, -1); // Move down
                default: return Point(0, 0); // No movement
            }
        },
        // Action 1: Turn left (counter-clockwise) - change orientation
        [](int orientation) -> Point {
            return Point(0, 0); // No movement, just change orientation
        },
        // Action 2: Turn right (clockwise) - change orientation
        [](int orientation) -> Point {
            return Point(0, 0); // No movement, just change orientation
        },
        // Action 3: Turn around (180 degrees) - change orientation
        [](int orientation) -> Point {
            return Point(0, 0); // No movement, just change orientation
        }
    };
    
    // Track last n actions (简化版本，只记录最后4个动作)
    std::vector<int> last_actions(4, 0);  // 初始化为0（假设动作0是默认动作）
    
    // Move until goal is reached or max steps exceeded
    while (steps < max_steps && 
           (std::abs(current_pos.x - goal.x) > 0.5 || std::abs(current_pos.y - goal.y) > 0.5)) {
        // Create a complex state representation according to the specification
        State state;
        
        // 1. 归一化坐标 (norm_lat, norm_lon)
        double norm_x = static_cast<double>(current_pos.x) / costmap.getWidth();
        double norm_y = static_cast<double>(current_pos.y) / costmap.getHeight();
        state.addFeature(norm_x);
        state.addFeature(norm_y);
        
        // 2. 热力图Summary (heatmap_summary, 16个值)
        // 简化实现：使用周围区域的数据密度作为热力图特征
        for (int i = 0; i < 16; i++) {
            // 模拟16个热力图特征值
            int offset_x = (i % 4) - 1;
            int offset_y = (i / 4) - 1;
            int check_x = static_cast<int>(current_pos.x) + offset_x;
            int check_y = static_cast<int>(current_pos.y) + offset_y;
            
            if (costmap.isValidCell(check_x, check_y)) {
                state.addFeature(costmap.getDataDensity(check_x, check_y));
            } else {
                state.addFeature(0.0);
            }
        }
        
        // 3. 历史动作 (last_n_actions, 4个值)
        for (int action : last_actions) {
            state.addFeature(static_cast<double>(action));
        }
        
        // 4. 剩余预算 (remaining_budget_norm)
        double remaining_budget = static_cast<double>(max_steps - steps) / max_steps;
        state.addFeature(remaining_budget);
        
        // 5. 局部交通密度 (local_traffic_density)
        // 简化实现：使用当前位置附近单元格的平均成本作为交通密度的代理
        double local_density = 0.0;
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int check_x = static_cast<int>(current_pos.x) + dx;
                int check_y = static_cast<int>(current_pos.y) + dy;
                if (costmap.isValidCell(check_x, check_y)) {
                    local_density += costmap.getCellCost(check_x, check_y);
                    count++;
                }
            }
        }
        if (count > 0) {
            local_density /= count;
        }
        state.addFeature(local_density);
        
        // Select action using PPO agent (deterministic for path planning)
        int action_idx = ppo_agent_->selectAction(state, true);
        
        // 更新历史动作队列
        last_actions.erase(last_actions.begin());
        last_actions.push_back(action_idx);
        
        // Apply action based on PRD specification: forward, turn left, turn right, turn around
        switch(action_idx) {
            case 0: // Forward
            {
                Point movement = action_transforms[0](current_orientation);
                Point next_pos(current_pos.x + movement.x, current_pos.y + movement.y);
                
                // Check if move is valid
                if (costmap.isValidCell(static_cast<int>(next_pos.x), static_cast<int>(next_pos.y))) {
                    current_pos = next_pos;
                    path.push_back(current_pos);
                }
                break;
            }
            case 1: // Turn left (counter-clockwise)
                current_orientation = (current_orientation + 1) % 4;
                break;
            case 2: // Turn right (clockwise)
                current_orientation = (current_orientation + 3) % 4; // Adding 3 is equivalent to subtracting 1
                break;
            case 3: // Turn around (180 degrees)
                current_orientation = (current_orientation + 2) % 4; // 180-degree turn
                break;
            default:
                break; // Invalid action, do nothing
        }
        
        steps++;
    }
    
    std::cout << "PPO path computed from (" << start.x << "," << start.y 
              << ") to (" << goal.x << "," << goal.y << ") with " 
              << path.size() << " waypoints" << std::endl;
              
    return path;
}

} // namespace planner
} // namespace dcp