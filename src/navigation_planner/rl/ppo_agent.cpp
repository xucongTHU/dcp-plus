// ppo_agent.cpp
#include "ppo_agent.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include "common/log/logger.h"

// ONNX Runtime headers
#ifdef HAVE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace dcl::planner {

PPOAgent::PPOAgent(const PPOConfig& config)
    : config_(config), state_dim_(24), action_dim_(4),  // 默认state_dim设为24，符合规范中的最小值
      total_reward_(0.0), episode_count_(0) {
    // Initialize ONNX Runtime environment
    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "");
    allocator_ = std::make_unique<Ort::Allocator>();
    
    AD_INFO(PLANNER, "PPO Agent initialized with state_dim=%d, action_dim=%d", state_dim_, action_dim_);
}

int PPOAgent::selectAction(const Point& state, bool deterministic) {
    // Convert Point to State for compatibility
    // 这是一个简化的状态表示，只包含坐标信息
    std::vector<double> features = {state.x, state.y};
    features.resize(state_dim_, 0.0);  // 填充至标准维度
    State state_vec(features);
    return selectAction(state_vec, deterministic);
}

int PPOAgent::selectAction(const State& state, bool deterministic) {
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
    // Convert Point to State for compatibility
    std::vector<double> features = {state.x, state.y};
    features.resize(state_dim_, 0.0);  // 填充至标准维度
    State state_vec(features);
    return getActionProbabilities(state_vec);
}

std::vector<double> PPOAgent::getActionProbabilities(const State& state) {
    // If we have a trained model, use it for inference
    if (session_) {
        try {
            // Create input tensor
            // 确保输入特征维度与state_dim_一致
            std::vector<float> input_data(state.features.begin(), state.features.end());
            if (input_data.size() != static_cast<size_t>(state_dim_)) {
                AD_WARN(PLANNER, "Input state dimension mismatch. Expected: %d, Got: %lu. Padding with zeros.",
                        state_dim_, input_data.size());
                
                // 调整输入数据维度以匹配state_dim_
                if (input_data.size() < static_cast<size_t>(state_dim_)) {
                    input_data.resize(state_dim_, 0.0f);
                } else {
                    input_data.resize(state_dim_);
                }
            }
            
            std::vector<int64_t> input_shape = {1, static_cast<int64_t>(state_dim_)};
            
            // Setup input tensor
            auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
                memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());
            
            // Setup input/output names
            Ort::AllocatorWithDefaultOptions allocator;
            std::vector<const char*> input_names = {"input"};
            std::vector<const char*> output_names = {"output_policy", "output_value"};
            
            // Run inference
            auto output_tensors = session_->Run(
                Ort::RunOptions{nullptr}, 
                input_names.data(), 
                &input_tensor, 
                1, 
                output_names.data(), 
                2);
            
            // Extract logits from output tensor (model outputs logits, not probabilities)
            float* logits_data = output_tensors[0].GetTensorMutableData<float>();
            auto logits_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
            
            // Apply softmax to convert logits to probabilities
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
    }
    
    // Fallback to random probabilities if no model or error occurred
    std::vector<double> probs(action_dim_, 1.0 / action_dim_);
    return probs;
}

double PPOAgent::getValue(const Point& state) {
    // Convert Point to State for compatibility
    std::vector<double> features = {state.x, state.y};
    features.resize(state_dim_, 0.0);  // 填充至标准维度
    State state_vec(features);
    return getValue(state_vec);
}

double PPOAgent::getValue(const State& state) {
    // If we have a trained model, use it for inference
    if (session_) {
        try {
            // Create input tensor
            // 确保输入特征维度与state_dim_一致
            std::vector<float> input_data(state.features.begin(), state.features.end());
            if (input_data.size() != static_cast<size_t>(state_dim_)) {
                AD_WARN(PLANNER, "Input state dimension mismatch. Expected: %d, Got: %lu. Padding with zeros.",
                        state_dim_, input_data.size());
                
                // 调整输入数据维度以匹配state_dim_
                if (input_data.size() < static_cast<size_t>(state_dim_)) {
                    input_data.resize(state_dim_, 0.0f);
                } else {
                    input_data.resize(state_dim_);
                }
            }
            
            std::vector<int64_t> input_shape = {1, static_cast<int64_t>(state_dim_)};
            
            // Setup input tensor
            auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
                memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());
            
            // Setup input/output names
            Ort::AllocatorWithDefaultOptions allocator;
            std::vector<const char*> input_names = {"input"};
            std::vector<const char*> output_names = {"output_policy", "output_value"};
            
            // Run inference
            auto output_tensors = session_->Run(
                Ort::RunOptions{nullptr}, 
                input_names.data(), 
                &input_tensor, 
                1, 
                output_names.data(), 
                2);
            
            // Extract value from output tensor
            float* value_data = output_tensors[1].GetTensorMutableData<float>();
            
            // Convert tensor to scalar value
            return static_cast<double>(value_data[0]);
        } catch (const Ort::Exception& e) {
            AD_ERROR(PLANNER, "ONNX model inference error: %s", e.what());
        }
    }
    
    // Fallback to zero value if no model or error occurred
    return 0.0;
}

void PPOAgent::update(const std::vector<Trajectory>& trajectories) {
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

bool PPOAgent::saveWeights(const std::string& filepath) {
    // Not implemented for ONNX Runtime - models are loaded, not saved
    AD_WARN(PLANNER, "Saving weights not implemented for ONNX Runtime");

    return false;
}

bool PPOAgent::loadWeights(const std::string& filepath) {
    // Try to load as ONNX model first
    return loadOnnxModel(filepath);
}

bool PPOAgent::loadOnnxModel(const std::string& filepath) {
    try {
        // Create session options
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
        
        // Create session
        session_ = std::make_unique<Ort::Session>(*env_, filepath.c_str(), session_options);
        
        AD_INFO(PLANNER, "ONNX model loaded from %s", filepath.c_str());
        return true;
    } catch (const Ort::Exception& e) {
        AD_ERROR(PLANNER, "Failed to load ONNX model: %s", e.what());
        return false;
    }
}

void PPOAgent::resetStatistics() {
    total_reward_ = 0.0;
    episode_count_ = 0;
}

} // namespace dcl::planner