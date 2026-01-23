// planner_reward.h
#ifndef PLANNER_REWARD_H
#define PLANNER_REWARD_H

#include <vector>

struct StateInfo {
    bool visited_new_sparse;        // 访问新稀疏区域
    bool trigger_success;           // 数据收集成功
    bool collision;                 // 碰撞
    bool reached_goal;              // 到达目标
    bool on_efficient_path;         // 在高效路径上
    bool visited_before;            // 重复访问
    double distance_to_sparse;      // 到稀疏区域的距离
    double distance_to_target;      // 到目标的距离
    double path_efficiency;         // 路径效率
    int steps_taken;                // 已采取的步数
    int total_visited_count;        // 总访问次数（用于重复惩罚）
    
    StateInfo() : visited_new_sparse(false), trigger_success(false), 
                  collision(false), reached_goal(false), 
                  on_efficient_path(true), visited_before(false),
                  distance_to_sparse(0.0), distance_to_target(0.0), 
                  path_efficiency(1.0), steps_taken(0), total_visited_count(1) {}
};

namespace dcp::planner {

class RewardCalculator {
public:
    /**
     * @brief Compute reward based on state transitions and information
     * @param prev_state_info Information about previous state
     * @param new_state_info Information about new state
     * @return Computed reward value
     */
    static double computeReward(const StateInfo& prev_state_info, 
                               const StateInfo& new_state_info);
                               
    /**
     * @brief Compute shaped reward based on distance to nearest sparse cell
     * @param distance_to_sparse Distance to nearest sparse cell
     * @return Shaped reward component
     */
    static double computeShapedReward(double distance_to_sparse);
};

} // namespace dcp::planner

#endif // PLANNER_REWARD_H