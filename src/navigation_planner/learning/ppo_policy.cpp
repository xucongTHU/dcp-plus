// ppo_policy.cpp
#include "ppo_policy.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include "data_collection/common/log/logger.h"

// ONNX Runtime headers
#ifdef HAVE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace dcp::planner {

PPOPolicy::PPOPolicy(const PPOConfig& config)
    : config_(config), state_dim_(24), action_dim_(4),  // Default state_dim set to 24, matching minimum specification
      total_reward_(0.0), episode_count_(0) {
#ifdef HAVE_ONNXRUNTIME
    // Initialize ONNX Runtime environment
    // This creates the ONNX Runtime environment for model inference
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "");
    // Session will be created when loading model
    session_ = nullptr;
#endif
    
    AD_INFO(PLANNER, "PPO Agent initialized with state_dim=%d, action_dim=%d", state_dim_, action_dim_);
}

int PPOPolicy::selectAction(const Point& state, bool deterministic) {
    // Convert Point to State for compatibility
    // This is a simplified state representation, containing only coordinate information
    std::vector<double> features = {state.x, state.y};
    features.resize(state_dim_, 0.0);  // Fill to standard dimension
    State state_vec(features);
    return selectAction(state_vec, deterministic);
}

int PPOPolicy::selectAction(const State& state, bool deterministic) {
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

std::vector<double> PPOPolicy::getActionProbabilities(const Point& state) {
    // Convert Point to State for compatibility
    std::vector<double> features = {state.x, state.y};
    features.resize(state_dim_, 0.0);  // Fill to standard dimension
    State state_vec(features);
    return getActionProbabilities(state_vec);
}

#ifdef HAVE_ONNXRUNTIME
std::pair<Ort::Value, Ort::Value> PPOPolicy::runInference(const State& state) {
    if (!session_) {
        throw std::runtime_error("ONNX session is not initialized");
    }
    
    // Create input tensor
    // Ensure input feature dimension matches state_dim_
    std::vector<float> input_data(state.features.begin(), state.features.end());
    if (input_data.size() != static_cast<size_t>(state_dim_)) {
        AD_WARN(PLANNER, "Input state dimension mismatch. Expected: %d, Got: %lu. Padding with zeros.",
                state_dim_, input_data.size());
        
        // Adjust input data dimension to match state_dim_
        if (input_data.size() < static_cast<size_t>(state_dim_)) {
            input_data.resize(state_dim_, 0.0f);
        } else {
            input_data.resize(state_dim_);
        }
    }
    
    std::vector<int64_t> input_shape = {1, static_cast<int64_t>(state_dim_)};
    
    // Setup input tensor
    // Using CPU memory info with default allocator
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());
    
    // Setup input/output names according to model spec
    // Input node name: "input"
    // Output node names: "output_policy" and "output_value"
    std::vector<const char*> input_names = {"input"};
    std::vector<const char*> output_names = {"output_policy", "output_value"};
    
    // Run inference
    // AD_WARN(PLANNER, "Running inference with input shape [%ld, %ld]", input_shape[0], input_shape[1]);
    auto output_tensors = session_->Run(
        Ort::RunOptions{nullptr}, 
        input_names.data(), 
        &input_tensor, 
        1, 
        output_names.data(), 
        2);
        
    // Move the tensors out of the vector
    Ort::Value policy_tensor = std::move(output_tensors[0]);
    Ort::Value value_tensor = std::move(output_tensors[1]);
    
    return std::make_pair(std::move(policy_tensor), std::move(value_tensor));
}
#endif

std::vector<double> PPOPolicy::getActionProbabilities(const State& state) {
    // If we have a trained model, use it for inference
#ifdef HAVE_ONNXRUNTIME
    if (session_) {
        try {
            // Run inference to get policy and value tensors
            // Model outputs unprocessed logits that need to be converted to probabilities
            auto tensors = runInference(state);
            
            // Extract logits from output tensor (model outputs logits, not probabilities)
            float* logits_data = tensors.first.GetTensorMutableData<float>();
            
            // Apply softmax to convert logits to probabilities
            // This follows the specification that model outputs logits and vehicle side applies softmax
            std::vector<double> probs(action_dim_, 0.0);
            double max_logit = *std::max_element(logits_data, logits_data + action_dim_);
            double sum_exp = 0.0;
            
            // Compute softmax
            for (int i = 0; i < action_dim_; i++) {
                probs[i] = std::exp(static_cast<double>(logits_data[i]) - max_logit);
                sum_exp += probs[i];
            }
            
            // Normalize
            for (int i = 0; i < action_dim_; i++) {
                probs[i] /= sum_exp;
            }

            return probs;
        } catch (const Ort::Exception& e) {
            AD_ERROR(PLANNER, "ONNX model inference error: %s", e.what());
        }
    } else {
        AD_WARN(PLANNER, "Session is not available, using fallback method");
    }
#endif

    // Fallback to random probabilities if no model or error occurred
    // This provides a degradation mechanism when model loading fails
    std::vector<double> probs(action_dim_, 1.0 / action_dim_);
    AD_INFO(PLANNER, "PPO Agent selected action probabilities: %s", probs.data());
    return probs;
}

double PPOPolicy::getValue(const Point& state) {
    // Convert Point to State for compatibility
    std::vector<double> features = {state.x, state.y};
    features.resize(state_dim_, 0.0);  // Fill to standard dimension
    State state_vec(features);
    return getValue(state_vec);
}

double PPOPolicy::getValue(const State& state) {
    // If we have a trained model, use it for inference
#ifdef HAVE_ONNXRUNTIME
    if (session_) {
        AD_WARN(PLANNER, "Session is available, performing value inference");
        try {
            // Run inference to get policy and value tensors
            auto tensors = runInference(state);
            
            // Extract value from output tensor
            // Value output shape is [batch_size, 1] according to specification
            float* value_data = tensors.second.GetTensorMutableData<float>();
            
            // Convert tensor to scalar value
            double value = static_cast<double>(value_data[0]);
            AD_WARN(PLANNER, "Value inference completed successfully, value: %f", value);
            return value;
        } catch (const Ort::Exception& e) {
            AD_ERROR(PLANNER, "ONNX model inference error: %s", e.what());
        }
    } else {
        AD_WARN(PLANNER, "Session is not available for value inference, using fallback method");
    }
#endif
    
    // Fallback to zero value if no model or error occurred
    // This provides a degradation mechanism when model loading fails
    AD_WARN(PLANNER, "Using fallback value (0.0)");
    return 0.0;
}

void PPOPolicy::update(const std::vector<Trajectory>& trajectories) {
    AD_INFO(PLANNER, "Updating PPO agent with %lu trajectories", trajectories.size());
    
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
    
    AD_INFO(PLANNER, "PPO agent updated. Total episodes: %d, Total reward: %.2f", episode_count_, total_reward_);
}

bool PPOPolicy::saveWeights(const std::string& filepath) {
    // Not implemented for ONNX Runtime - models are loaded, not saved
    AD_WARN(PLANNER, "Saving weights not implemented for ONNX Runtime");

    return false;
}

bool PPOPolicy::loadWeights(const std::string& filepath) {
    // Try to load as ONNX model first
    return loadOnnxModel(filepath);
}

bool PPOPolicy::loadOnnxModel(const std::string& filepath) {
#ifdef HAVE_ONNXRUNTIME
    try {
        // Create session with default options
        // This loads the ONNX model for inference using ONNX Runtime
        session_ = std::make_unique<Ort::Session>(*env_, filepath.c_str(), Ort::SessionOptions{});
        
        AD_INFO(PLANNER, "ONNX model loaded successfully from %s", filepath.c_str());
        return true;
    } catch (const Ort::Exception& e) {
        AD_ERROR(PLANNER, "Failed to load ONNX model from %s: %s", filepath.c_str(), e.what());
        return false;
    }
#else
    AD_WARN(PLANNER, "ONNX Runtime not enabled, cannot load model from %s", filepath.c_str());
    return false;
#endif
}

void PPOPolicy::resetStatistics() {
    total_reward_ = 0.0;
    episode_count_ = 0;
}

void PPOPolicy::updateConfigFromParameters(const std::map<std::string, double>& parameters) {
    // Update PPO configuration from parameter map
    auto it = parameters.find("ppo_config_learning_rate");
    if (it != parameters.end()) {
        config_.learning_rate = it->second;
    }
    
    it = parameters.find("ppo_config_gamma");
    if (it != parameters.end()) {
        config_.gamma = it->second;
    }
    
    it = parameters.find("ppo_config_gae_lambda");
    if (it != parameters.end()) {
        config_.lam = it->second;
    }
    
    it = parameters.find("ppo_config_clip_epsilon");
    if (it != parameters.end()) {
        config_.clip_epsilon = it->second;
    }
    
    it = parameters.find("ppo_config_entropy_coef");
    if (it != parameters.end()) {
        config_.entropy_coef = it->second;
    }
    
    it = parameters.find("ppo_config_value_loss_coef");
    if (it != parameters.end()) {
        config_.value_loss_coef = it->second;
    }
    
    it = parameters.find("ppo_config_batch_size");
    if (it != parameters.end()) {
        config_.batch_size = static_cast<int>(it->second);
    }
    
    it = parameters.find("ppo_config_epochs");
    if (it != parameters.end()) {
        config_.epochs = static_cast<int>(it->second);
    }
    
    it = parameters.find("ppo_config_max_training_steps");
    if (it != parameters.end()) {
        config_.max_training_steps = static_cast<int>(it->second);
    }
    
    it = parameters.find("ppo_config_max_episode_steps");
    if (it != parameters.end()) {
        config_.max_episode_steps = static_cast<int>(it->second);
    }
}

} // namespace dcp::planner