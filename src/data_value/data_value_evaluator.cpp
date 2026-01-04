#include "data_value_evaluator.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace data_value {

// FixedValueStrategy implementation
FixedValueStrategy::FixedValueStrategy(double w1, double w2, double w3) 
    : w1_(w1), w2_(w2), w3_(w3) {}

double FixedValueStrategy::calculate_rule_value(const DataItem& data_item) const {
    double rule_value = 0.0;
    
    // V_rule components:
    // 1. Scenario type value
    if (data_item.scenario_type == "intersection" || 
        data_item.scenario_type == "crosswalk" || 
        data_item.scenario_type == "highway_merge") {
        rule_value += 0.3;  // High value for complex scenarios
    } else if (data_item.scenario_type == "urban_road") {
        rule_value += 0.2;  // Medium value
    } else if (data_item.scenario_type == "highway") {
        rule_value += 0.15; // Medium-low value
    }
    
    // 2. Pedestrian presence
    if (data_item.has_pedestrian) {
        rule_value += 0.25;  // High value when pedestrians are present
    }
    
    // 3. Vehicle interaction
    if (data_item.has_vehicle_interaction) {
        rule_value += 0.15;  // Medium value for vehicle interactions
    }
    
    // 4. Weather conditions
    if (data_item.weather_condition == "rainy") {
        rule_value += 0.1;
    } else if (data_item.weather_condition == "foggy") {
        rule_value += 0.15;
    } else if (data_item.weather_condition == "night") {
        rule_value += 0.12;
    } else if (data_item.weather_condition == "snow") {
        rule_value += 0.18;
    }
    
    // Ensure value is within [0, 1] range
    return std::min(1.0, rule_value);
}

double FixedValueStrategy::calculate_model_value(const DataItem& data_item) const {
    // V_model: Value based on model uncertainty
    // Higher uncertainty means more valuable data for model improvement
    
    // Directly use the model uncertainty value if provided
    if (data_item.model_uncertainty > 0) {
        return std::min(1.0, data_item.model_uncertainty);
    }
    
    // Default value if uncertainty is not provided
    return 0.3;
}

double FixedValueStrategy::calculate_distribution_value(const DataItem& data_item) const {
    // V_distribution: Value based on data distribution/rarity
    // Higher value for rare scenarios, lower for common ones
    
    if (data_item.is_rare_scenario) {
        return 0.8;  // High value for rare scenarios
    }
    
    // If not explicitly marked as rare, assign value based on location/commonness
    if (data_item.location.find("highway") != std::string::npos) {
        return 0.2;  // Highway data is common, low value
    } else if (data_item.location.find("city") != std::string::npos || 
               data_item.location.find("urban") != std::string::npos) {
        return 0.3;  // Urban data is somewhat common
    } else if (data_item.location.find("rural") != std::string::npos || 
               data_item.location.find("unusual") != std::string::npos) {
        return 0.6;  // Rural/unusual locations have higher value
    }
    
    return 0.4;  // Default value
}

DataValueMetrics FixedValueStrategy::evaluate(const DataItem& data_item) const {
    DataValueMetrics metrics;
    metrics.evaluation_time = std::chrono::system_clock::now();
    
    // Calculate individual components
    metrics.rule_value = calculate_rule_value(data_item);
    metrics.model_value = calculate_model_value(data_item);
    metrics.distribution_value = calculate_distribution_value(data_item);
    
    // Apply the formula: V_data = w1*V_rule + w2*V_model + w3*V_distribution
    metrics.total_value = w1_ * metrics.rule_value + 
                         w2_ * metrics.model_value + 
                         w3_ * metrics.distribution_value;
    
    // Calculate other metrics based on the formula result
    metrics.base_value = metrics.total_value * 0.7; // Base value as 70% of total
    metrics.quality_score = std::min(1.0, metrics.total_value * 1.1); // Quality score
    metrics.relevance_score = std::min(1.0, metrics.total_value); // Relevance
    metrics.timeliness_score = std::min(1.0, metrics.total_value * 0.9); // Timeliness
    metrics.completeness_score = std::min(1.0, metrics.total_value * 0.8); // Completeness
    
    // Store weights used in calculation
    metrics.w1 = w1_;
    metrics.w2 = w2_;
    metrics.w3 = w3_;
    
    return metrics;
}

// AIEvaluationStrategy implementation
AIEvaluationStrategy::AIEvaluationStrategy() {
    // Initialize the AI model or service
    initialize_model();
}

bool AIEvaluationStrategy::initialize_model() {
    // Placeholder for model initialization
    // In a real implementation, this would load an ML model or connect to an AI service
    std::cout << "Initializing AI evaluation model..." << std::endl;
    return true;
}

DataValueMetrics AIEvaluationStrategy::evaluate(const DataItem& data_item) const {
    DataValueMetrics metrics;
    metrics.evaluation_time = std::chrono::system_clock::now();
    
    // Use AI model to predict value
    double predicted_value = predict_value(data_item);
    
    // Calculate components based on AI prediction
    metrics.rule_value = predicted_value * 0.35;      // Rule value component
    metrics.model_value = predicted_value * 0.4;      // Model value component
    metrics.distribution_value = predicted_value * 0.25; // Distribution value component
    
    // Apply the weighted formula
    metrics.total_value = 0.4 * metrics.rule_value + 
                         0.4 * metrics.model_value + 
                         0.2 * metrics.distribution_value;
    
    // Set other metrics based on AI prediction
    metrics.base_value = metrics.total_value * 0.6; // Base value as 60% of total
    metrics.quality_score = std::min(1.0, metrics.total_value); // Quality score
    metrics.relevance_score = std::min(1.0, metrics.total_value * 1.2); // Relevance
    metrics.timeliness_score = std::min(1.0, metrics.total_value * 1.1); // Timeliness
    metrics.completeness_score = std::min(1.0, metrics.total_value * 0.9); // Completeness
    
    return metrics;
}

double AIEvaluationStrategy::predict_value(const DataItem& data_item) const {
    // Placeholder AI prediction logic
    // In a real implementation, this would use an actual ML model
    double value = 0.5; // Base value
    
    // Factor in scenario type
    if (data_item.scenario_type == "intersection" || 
        data_item.scenario_type == "crosswalk") {
        value += 0.2;
    } else if (data_item.scenario_type == "highway_merge") {
        value += 0.15;
    }
    
    // Factor in pedestrian detection
    if (data_item.has_pedestrian) {
        value += 0.15;
    }
    
    // Factor in vehicle interaction
    if (data_item.has_vehicle_interaction) {
        value += 0.1;
    }
    
    // Factor in weather conditions
    if (data_item.weather_condition == "rainy" || 
        data_item.weather_condition == "foggy" ||
        data_item.weather_condition == "snow") {
        value += 0.12;
    } else if (data_item.weather_condition == "night") {
        value += 0.08;
    }
    
    // Factor in model uncertainty
    if (data_item.model_uncertainty > 0.5) {
        value += 0.1 * data_item.model_uncertainty;
    }
    
    // Factor in rarity of scenario
    if (data_item.is_rare_scenario) {
        value += 0.15;
    }
    
    // Factor in data size (larger data might be more valuable)
    if (data_item.size_bytes > 1000000) { // > 1MB
        value += 0.05;
    } else if (data_item.size_bytes > 100000) { // > 100KB
        value += 0.02;
    }
    
    // Apply timeliness factor
    auto now = std::chrono::system_clock::now();
    auto age_seconds = std::chrono::duration_cast<std::chrono::seconds>(
        now - data_item.creation_time).count();
    
    if (age_seconds < 60) { // Less than 1 minute
        value += 0.05;
    }
    
    // Ensure value is within [0, 1] range
    return std::max(0.0, std::min(1.0, value));
}

// DataValueEvaluator implementation
DataValueEvaluator::DataValueEvaluator() {
    // Initialize with default strategies and weights based on the formula
    add_strategy(std::make_unique<FixedValueStrategy>(0.4, 0.4, 0.2));
    add_strategy(std::make_unique<AIEvaluationStrategy>());
}

DataValueEvaluator::~DataValueEvaluator() = default;

void DataValueEvaluator::add_strategy(std::unique_ptr<DataValueStrategy> strategy) {
    if (strategy) {
        strategies_.push_back(std::move(strategy));
        strategy_index_map_[strategies_.back()->get_strategy_name()] = strategies_.size() - 1;
    }
}

std::vector<std::pair<std::string, DataValueMetrics>> 
DataValueEvaluator::evaluate_all_strategies(const DataItem& data_item) const {
    std::vector<std::pair<std::string, DataValueMetrics>> results;
    
    for (const auto& strategy : strategies_) {
        DataValueMetrics metrics = strategy->evaluate(data_item);
        results.emplace_back(strategy->get_strategy_name(), metrics);
    }
    
    return results;
}

DataValueMetrics DataValueEvaluator::evaluate_with_strategy(
    const DataItem& data_item, const std::string& strategy_name) const {
    
    auto it = strategy_index_map_.find(strategy_name);
    if (it != strategy_index_map_.end()) {
        size_t index = it->second;
        if (index < strategies_.size()) {
            return strategies_[index]->evaluate(data_item);
        }
    }
    
    // Return default metrics if strategy not found
    DataValueMetrics default_metrics;
    default_metrics.evaluation_time = std::chrono::system_clock::now();
    return default_metrics;
}

DataValueMetrics DataValueEvaluator::get_best_value(const DataItem& data_item) const {
    auto all_results = evaluate_all_strategies(data_item);
    
    if (all_results.empty()) {
        DataValueMetrics default_metrics;
        default_metrics.evaluation_time = std::chrono::system_clock::now();
        return default_metrics;
    }
    
    // Find the strategy that gives the highest total value
    auto best_result = std::max_element(all_results.begin(), all_results.end(),
        [](const std::pair<std::string, DataValueMetrics>& a,
           const std::pair<std::string, DataValueMetrics>& b) {
            return a.second.total_value < b.second.total_value;
        });
    
    return best_result->second;
}

bool DataValueEvaluator::should_collect_during_standard_phase(
    const DataItem& data_item, double min_value_threshold) const {
    
    DataValueMetrics best_metrics = get_best_value(data_item);
    return best_metrics.total_value >= min_value_threshold;
}

} // namespace data_value