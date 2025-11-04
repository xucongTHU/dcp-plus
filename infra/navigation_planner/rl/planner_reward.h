// planner_reward.h
#ifndef PLANNER_REWARD_H
#define PLANNER_REWARD_H

#include <vector>

struct StateInfo {
    bool visited_new_sparse;
    bool trigger_success;
    bool collision;
    double distance_to_sparse;
    
    StateInfo() : visited_new_sparse(false), trigger_success(false), 
                  collision(false), distance_to_sparse(0.0) {}
};

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

#endif // PLANNER_REWARD_H