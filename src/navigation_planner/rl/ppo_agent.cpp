// ppo_agent.cpp
#include "ppo_agent.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include "../../common/log/logging.h"

namespace dcl::planner::rl {

PPOAgent::PPOAgent(const PPOConfig& config)
    : config_(config), state_dim_(2), hidden_dim_(64), action_dim_(4),
      total_reward_(0.0), episode_count_(0) {
    // Initialize actor network with random weights
    actor_network_.resize(hidden_dim_);
    for (int i = 0; i < hidden_dim_; i++) {
        actor_network_[i].resize(state_dim_);
        for (int j = 0; j < state_dim_; j++) {
            // Random initialization
            actor_network_[i][j] = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
        }
    }
    
    // Initialize critic network with random weights
    critic_network_.resize(hidden_dim_);
    for (int i = 0; i < hidden_dim_; i++) {
        critic_network_[i].resize(state_dim_);
        for (int j = 0; j < state_dim_; j++) {
            // Random initialization
            critic_network_[i][j] = (static_cast<double>(rand()) / RAND_MAX) * 2.0 - 1.0;
        }
    }
    
    LOG(INFO) << "PPO Agent initialized with state_dim=" << state_dim_ 
              << ", action_dim=" << action_dim_ << ", hidden_dim=" << hidden_dim_;
}

int PPOAgent::selectAction(const Point& state, bool deterministic) {
    auto probs = getActionProbabilities(state);
    
    if (deterministic) {
        // Select action with highest probability
        return std::distance(probs.begin(), std::max_element(probs.begin(), probs.end()));
    } else {
        // Sample action according to probabilities
        std::random_device rd;
        std::mt19937 gen(rd());
        std::discrete_distribution<> dis(probs.begin(), probs.end());
        return dis(gen);
    }
}

std::vector<double> PPOAgent::getActionProbabilities(const Point& state) {
    // Simplified neural network forward pass
    std::vector<double> hidden(hidden_dim_, 0.0);
    
    // Linear transformation from input to hidden layer
    for (int i = 0; i < hidden_dim_; i++) {
        hidden[i] += actor_network_[i][0] * state.x + actor_network_[i][1] * state.y;
    }
    
    // Apply ReLU activation
    for (int i = 0; i < hidden_dim_; i++) {
        hidden[i] = std::max(0.0, hidden[i]);
    }
    
    // Output layer - map to action probabilities
    std::vector<double> logits(action_dim_, 0.0);
    for (int a = 0; a < action_dim_; a++) {
        for (int i = 0; i < hidden_dim_; i++) {
            logits[a] += hidden[i]; // Simplified mapping
        }
    }
    
    // Apply softmax to get probabilities
    double max_logit = *std::max_element(logits.begin(), logits.end());
    double sum_exp = 0.0;
    for (double& logit : logits) {
        logit = std::exp(logit - max_logit);
        sum_exp += logit;
    }
    
    for (double& prob : logits) {
        prob /= sum_exp;
    }
    
    return logits;
}

double PPOAgent::getValue(const Point& state) {
    // Simplified critic network forward pass
    double value = 0.0;
    
    // Linear transformation
    for (int i = 0; i < hidden_dim_; i++) {
        value += critic_network_[i][0] * state.x + critic_network_[i][1] * state.y;
    }
    
    return value;
}

void PPOAgent::update(const std::vector<Trajectory>& trajectories) {
    LOG(INFO) << "Updating PPO agent with " << trajectories.size() << " trajectories";
    
    // This is a simplified version of PPO update
    // In a full implementation, this would involve:
    // 1. Computing advantages using GAE
    // 2. Performing multiple epochs of optimization
    // 3. Updating both actor and critic networks
    
    // For now, we'll just increment the episode counter and total reward
    for (const auto& traj : trajectories) {
        episode_count_++;
        for (double reward : traj.rewards) {
            total_reward_ += reward;
        }
    }
    
    LOG(INFO) << "PPO agent updated. Total episodes: " << episode_count_ 
              << ", Total reward: " << total_reward_;
}

bool PPOAgent::saveWeights(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG(ERROR) << "Failed to open file for saving weights: " << filepath;
        return false;
    }
    
    // Save actor network
    file << "ActorNetwork:\n";
    for (const auto& row : actor_network_) {
        for (double weight : row) {
            file << weight << " ";
        }
        file << "\n";
    }
    
    // Save critic network
    file << "CriticNetwork:\n";
    for (const auto& row : critic_network_) {
        for (double weight : row) {
            file << weight << " ";
        }
        file << "\n";
    }
    
    file.close();
    LOG(INFO) << "PPO weights saved to " << filepath;
    return true;
}

bool PPOAgent::loadWeights(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG(ERROR) << "Failed to open file for loading weights: " << filepath;
        return false;
    }
    
    std::string line;
    std::getline(file, line); // Skip "ActorNetwork:" header
    
    // Load actor network
    for (auto& row : actor_network_) {
        for (double& weight : row) {
            file >> weight;
        }
    }
    
    std::getline(file, line); // Skip empty line
    std::getline(file, line); // Skip "CriticNetwork:" header
    
    // Load critic network
    for (auto& row : critic_network_) {
        for (double& weight : row) {
            file >> weight;
        }
    }
    
    file.close();
    LOG(INFO) << "PPO weights loaded from " << filepath;
    return true;
}

void PPOAgent::resetStatistics() {
    total_reward_ = 0.0;
    episode_count_ = 0;
}

} // namespace dcl::planner::rl