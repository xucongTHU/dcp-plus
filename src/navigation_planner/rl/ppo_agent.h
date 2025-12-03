// ppo_agent.h
#ifndef PPO_AGENT_H
#define PPO_AGENT_H

#include <vector>
#include <memory>
#include "../costmap/costmap.h"

namespace dcl::planner::rl {

struct PPOConfig {
    double learning_rate = 3e-4;
    double gamma = 0.99;           // Discount factor
    double lam = 0.95;             // GAE lambda
    double clip_epsilon = 0.2;     // PPO clip parameter
    double entropy_coef = 0.01;    // Entropy coefficient
    double value_loss_coef = 0.5;  // Value loss coefficient
    int batch_size = 64;
    int epochs = 10;
    
    PPOConfig() = default;
};

struct Trajectory {
    std::vector<Point> states;
    std::vector<int> actions;
    std::vector<double> rewards;
    std::vector<double> log_probs;
    std::vector<double> values;
    std::vector<bool> dones;
};

class PPOAgent {
private:
    PPOConfig config_;
    
    // Neural network parameters (simplified representation)
    // In a full implementation, these would be matrices and biases
    std::vector<std::vector<double>> actor_network_;
    std::vector<std::vector<double>> critic_network_;
    
    // Network dimensions
    int state_dim_;
    int hidden_dim_;
    int action_dim_;
    
    // Training statistics
    double total_reward_;
    int episode_count_;

public:
    PPOAgent(const PPOConfig& config = PPOConfig());
    
    /**
     * @brief Select action based on current state
     * @param state Current state representation
     * @param deterministic Whether to select deterministic action
     * @return Selected action index
     */
    int selectAction(const Point& state, bool deterministic = false);
    
    /**
     * @brief Compute action probabilities for given state
     * @param state Input state
     * @return Action probabilities
     */
    std::vector<double> getActionProbabilities(const Point& state);
    
    /**
     * @brief Compute state value
     * @param state Input state
     * @return State value
     */
    double getValue(const Point& state);
    
    /**
     * @brief Update agent based on collected trajectories
     * @param trajectories Batch of trajectories for training
     */
    void update(const std::vector<Trajectory>& trajectories);
    
    /**
     * @brief Save model weights to file
     * @param filepath Path to save weights
     * @return True if successful
     */
    bool saveWeights(const std::string& filepath);
    
    /**
     * @brief Load model weights from file
     * @param filepath Path to load weights from
     * @return True if successful
     */
    bool loadWeights(const std::string& filepath);
    
    /**
     * @brief Get current total reward
     */
    double getTotalReward() const { return total_reward_; }
    
    /**
     * @brief Get episode count
     */
    int getEpisodeCount() const { return episode_count_; }
    
    /**
     * @brief Reset training statistics
     */
    void resetStatistics();
};

} // namespace dcl::planner::rl

#endif // PPO_AGENT_H