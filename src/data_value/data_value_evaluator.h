#ifndef DATA_VALUE_EVALUATOR_H
#define DATA_VALUE_EVALUATOR_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace data_value {

// Structure to represent data value metrics
struct DataValueMetrics {
    double base_value = 0.0;
    double quality_score = 0.0;
    double relevance_score = 0.0;
    double timeliness_score = 0.0;
    double completeness_score = 0.0;
    
    // Components for the fixed value formula
    double rule_value = 0.0;      // V_rule: 规则价值 (交叉口、行人出现、夜间雨天)
    double model_value = 0.0;     // V_model: 模型价值 (感知模型不确定性高 数据更有价值)
    double distribution_value = 0.0; // V_distribution: 数据分布价值 (历史少采的场景 价值高；重复场景 价值低)
    
    // Weights for the formula
    double w1 = 0.4;  // Weight for rule value
    double w2 = 0.4;  // Weight for model value
    double w3 = 0.2;  // Weight for distribution value
    
    double total_value = 0.0;
    std::chrono::system_clock::time_point evaluation_time;
};

// Structure to represent data item to be evaluated
struct DataItem {
    std::string data_id;
    std::string data_type;
    std::string source;
    size_t size_bytes = 0;
    std::chrono::system_clock::time_point creation_time;
    std::chrono::system_clock::time_point last_access_time;
    std::string content_metadata;
    std::vector<uint8_t> data_content;  // For AI analysis if needed
    
    // Additional fields based on existing data collection system
    std::string vin;
    std::string task_id;
    
    // Scenario-specific fields for value calculation
    std::string scenario_type;          // 场景类型 (如: 交叉口, 高速, 城市道路)
    bool has_pedestrian = false;        // 是否有行人
    bool has_vehicle_interaction = false; // 是否有车辆交互
    std::string weather_condition;      // 天气条件 (晴天, 雨天, 雾天, 夜间)
    double model_uncertainty = 0.0;     // 模型不确定性
    bool is_rare_scenario = false;      // 是否为稀有场景
    std::string driving_mode;           // 驾驶模式
    std::string location;               // 位置信息
};

// Interface for data value evaluation strategies
class DataValueStrategy {
public:
    virtual ~DataValueStrategy() = default;
    virtual DataValueMetrics evaluate(const DataItem& data_item) const = 0;
    virtual std::string get_strategy_name() const = 0;
};

// Fixed value strategy - assigns values based on the formula: V_data = w1*V_rule + w2*V_model + w3*V_distribution
class FixedValueStrategy : public DataValueStrategy {
public:
    explicit FixedValueStrategy(
        double w1 = 0.4, 
        double w2 = 0.4, 
        double w3 = 0.2
    );
    DataValueMetrics evaluate(const DataItem& data_item) const override;
    std::string get_strategy_name() const override { return "FixedValueStrategy"; }

private:
    double w1_;  // Weight for rule value
    double w2_;  // Weight for model value
    double w3_;  // Weight for distribution value
    
    // Calculate rule-based value (V_rule)
    double calculate_rule_value(const DataItem& data_item) const;
    
    // Calculate model-based value (V_model)
    double calculate_model_value(const DataItem& data_item) const;
    
    // Calculate distribution-based value (V_distribution)
    double calculate_distribution_value(const DataItem& data_item) const;
};

// AI-based evaluation strategy - uses ML models to determine value
class AIEvaluationStrategy : public DataValueStrategy {
public:
    AIEvaluationStrategy();
    DataValueMetrics evaluate(const DataItem& data_item) const override;
    std::string get_strategy_name() const override { return "AIEvaluationStrategy"; }

private:
    // Placeholder for ML model or AI service integration
    bool initialize_model();
    double predict_value(const DataItem& data_item) const;
};

// Main data value evaluator class
class DataValueEvaluator {
public:
    DataValueEvaluator();
    ~DataValueEvaluator();

    // Add a strategy to the evaluator
    void add_strategy(std::unique_ptr<DataValueStrategy> strategy);

    // Evaluate data value using all registered strategies
    std::vector<std::pair<std::string, DataValueMetrics>> evaluate_all_strategies(const DataItem& data_item) const;

    // Evaluate using a specific strategy by name
    DataValueMetrics evaluate_with_strategy(const DataItem& data_item, const std::string& strategy_name) const;

    // Get the best value among all strategies
    DataValueMetrics get_best_value(const DataItem& data_item) const;

    // Register data for standard phase collection based on value threshold
    bool should_collect_during_standard_phase(const DataItem& data_item, double min_value_threshold = 0.5) const;

private:
    std::vector<std::unique_ptr<DataValueStrategy>> strategies_;
    std::unordered_map<std::string, size_t> strategy_index_map_;
};

} // namespace data_value

#endif // DATA_VALUE_EVALUATOR_H