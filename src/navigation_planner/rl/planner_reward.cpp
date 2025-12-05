// planner_reward.cpp
#include "planner_reward.h"
#include <cmath>

namespace dcl::planner {
double RewardCalculator::computeReward(const StateInfo& prev_state_info, 
                                      const StateInfo& new_state_info) {
    double reward = 0.0;
    
    // Reward for visiting new sparse areas
    if (new_state_info.visited_new_sparse) {
        reward += 10.0;  // Large positive reward as mentioned in docs
    }
    
    // Reward for successful trigger (useful data collection)
    if (new_state_info.trigger_success) {
        reward += 0.5;   // Additional reward as mentioned in docs
    }
    
    // Penalty for collision or entering obstacle
    if (new_state_info.collision) {
        reward -= 1.0;   // Large penalty as mentioned in docs
    }
    
    // Small step penalty to encourage shorter paths
    reward -= 0.01;
    
    // Optionally add shaped reward based on distance to nearest sparse cell
    // This encourages moving toward sparse areas even before reaching them
    reward += computeShapedReward(new_state_info.distance_to_sparse);
    
    return reward;
}

double RewardCalculator::computeShapedReward(double distance_to_sparse) {
    // Exponential decay reward - closer to sparse area gives higher reward
    // This is one possible implementation of shaped reward
    if (distance_to_sparse <= 0.0) {
        return 0.0;
    }
    return 2.0 * exp(-0.1 * distance_to_sparse);
}
}