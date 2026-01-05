// reward_calculator.cpp
#include "reward_calculator.h"
#include <cmath>

namespace dcp::planner {
double RewardCalculator::computeReward(const StateInfo& prev_state_info, 
                                      const StateInfo& new_state_info) {
    double reward = 0.0;
    
    // Distance-based reward: reward for getting closer to target
    // If distance to target decreased, give positive reward
    if (prev_state_info.distance_to_target > new_state_info.distance_to_target) {
        double distance_improvement = prev_state_info.distance_to_target - new_state_info.distance_to_target;
        reward += 5.0 * distance_improvement;  // Scale factor for distance improvement
    }
    
    // Time penalty: encourage efficient paths with fewer steps
    reward -= 0.01;  // Small penalty for each step to encourage efficiency
    
    // Goal reward: large reward for reaching the goal
    if (new_state_info.reached_goal) {
        reward += 50.0;  // Large reward for reaching the goal
    }
    
    // Collision penalty: large penalty for collisions
    if (new_state_info.collision) {
        reward -= 50.0;  // Large penalty for collision
    }
    
    // Data scarcity reward: reward for visiting sparse areas
    if (new_state_info.visited_new_sparse) {
        reward += 10.0;  // Reward for exploring sparse data areas
    }
    
    // Coverage reward: reward for exploration
    // This can be based on visiting new unexplored areas
    if (!new_state_info.visited_before) {
        reward += 2.0;  // Reward for visiting new areas
    }
    
    // Path efficiency penalty: penalize inefficient paths
    if (!new_state_info.on_efficient_path) {
        reward -= 5.0;  // Penalty for inefficient path
    }
    
    // Repetitive path penalty: penalize revisiting the same locations
    if (new_state_info.visited_before && new_state_info.total_visited_count > 3) {
        // More severe penalty if visiting the same location frequently
        reward -= 2.0 * (new_state_info.total_visited_count - 3);
    }
    
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