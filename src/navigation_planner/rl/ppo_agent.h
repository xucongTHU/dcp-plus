// ppo_agent.h
#ifndef PPO_AGENT_H
#define PPO_AGENT_H

#include <vector>
#include <memory>
#include "../costmap/costmap.h"

// ONNX Runtime headers
#ifdef HAVE_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

// Forward declarations for ONNX Runtime
namespace Ort {
class Env;
class Session;
class Allocator;
struct SessionOptions;
}

namespace dcl::planner {

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
    
    // ONNX Runtime components for inference
#ifdef HAVE_ONNXRUNTIME
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::unique_ptr<Ort::Allocator> allocator_;
#endif
    
    // Network dimensions
    // state_dim_: 输入状态维度，根据 MODEL_SPEC.md 规范:
    // state = [
    //   norm_lat, norm_lon,            # 归一化坐标 (2个值)
    //   heatmap_summary(16 values),    # 局部热力图池化向量 (16个值)
    //   last_n_actions(4),             # 历史动作 one-hot 或 embeddings (4个值)
    //   remaining_budget_norm,         # 时间/距离 (1个值)
    //   local_traffic_density,         # 局部交通密度 (1个值)
    //   ...                            # 其他特征
    // ]
    // 最小 state_dim_ 为 24 (2+16+4+1+1)
    int state_dim_;
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
    int selectAction(const State& state, bool deterministic = false);
    
    /**
     * @brief Compute action probabilities for given state
     * @param state Input state
     * @return Action probabilities
     */
    std::vector<double> getActionProbabilities(const Point& state);
    std::vector<double> getActionProbabilities(const State& state);
    
    /**
     * @brief Compute state value
     * @param state Input state
     * @return State value
     */
    double getValue(const Point& state);
    double getValue(const State& state);
    
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
     * @brief Load an ONNX model for inference
     * @param filepath Path to the ONNX model
     * @return True if successful
     */
    bool loadOnnxModel(const std::string& filepath);
    
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
    
    /**
     * @brief Set state dimension
     * @param dim New state dimension
     */
    void setStateDim(int dim) { state_dim_ = dim; }
    
    /**
     * @brief Get state dimension
     * @return Current state dimension
     */
    int getStateDim() const { return state_dim_; }
};

} // namespace dcl::planner

#endif // PPO_AGENT_H