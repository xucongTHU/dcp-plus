#include "data_value_module.h"
#include <iostream>
#include <chrono>

int main() {
    // Create an instance of the data value module
    data_value::DataValueModule module;
    
    if (!module.initialize()) {
        std::cerr << "Failed to initialize DataValueModule" << std::endl;
        return -1;
    }
    
    // Create a sample data item with scenario-specific fields
    data_value::DataItem data_item;
    data_item.data_id = "sample_sensor_data_001";
    data_item.data_type = "sensor_data";
    data_item.source = "lidar_sensor_front";
    data_item.size_bytes = 1024000; // 1MB
    data_item.creation_time = std::chrono::system_clock::now();
    data_item.last_access_time = std::chrono::system_clock::now();
    data_item.content_metadata = "{\"sensor_type\":\"lidar\",\"frame_rate\":10,\"resolution\":\"1024x64\"}";
    data_item.vin = "1234567890ABCDEF";
    data_item.task_id = "task_001";
    
    // Set scenario-specific fields for the value calculation
    data_item.scenario_type = "intersection";          // 交叉口场景 - 高价值
    data_item.has_pedestrian = true;                   // 有行人 - 高价值
    data_item.has_vehicle_interaction = true;          // 有车辆交互 - 中等价值
    data_item.weather_condition = "rainy";             // 雨天 - 高价值
    data_item.model_uncertainty = 0.7;                 // 模型不确定性高 - 高价值
    data_item.is_rare_scenario = true;                 // 稀有场景 - 高价值
    data_item.location = "urban_intersection";         // 城市交叉口位置
    
    // Evaluate the data item using the fixed value formula
    data_value::DataValueMetrics metrics = module.evaluate_data(data_item);
    
    std::cout << "=== Data Value Evaluation Results ===" << std::endl;
    std::cout << "Data Item ID: " << data_item.data_id << std::endl;
    std::cout << "Scenario Type: " << data_item.scenario_type << std::endl;
    std::cout << "Has Pedestrian: " << (data_item.has_pedestrian ? "Yes" : "No") << std::endl;
    std::cout << "Weather: " << data_item.weather_condition << std::endl;
    std::cout << "Model Uncertainty: " << data_item.model_uncertainty << std::endl;
    std::cout << "Is Rare Scenario: " << (data_item.is_rare_scenario ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Value Components ===" << std::endl;
    std::cout << "Rule Value (V_rule): " << metrics.rule_value << " (交叉口、行人出现、夜间雨天)" << std::endl;
    std::cout << "Model Value (V_model): " << metrics.model_value << " (感知模型不确定性高 数据更有价值)" << std::endl;
    std::cout << "Distribution Value (V_distribution): " << metrics.distribution_value << " (历史少采的场景 价值高；重复场景 价值低)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Weights ===" << std::endl;
    std::cout << "w1 (Rule Weight): " << metrics.w1 << std::endl;
    std::cout << "w2 (Model Weight): " << metrics.w2 << std::endl;
    std::cout << "w3 (Distribution Weight): " << metrics.w3 << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== Final Results ===" << std::endl;
    std::cout << "Total Value: " << metrics.total_value << std::endl;
    std::cout << "Formula: V_data = w1*V_rule + w2*V_model + w3*V_distribution" << std::endl;
    std::cout << "Formula: V_data = " << metrics.w1 << "*" << metrics.rule_value 
              << " + " << metrics.w2 << "*" << metrics.model_value 
              << " + " << metrics.w3 << "*" << metrics.distribution_value 
              << " = " << metrics.total_value << std::endl;
    
    // Check if the data should be collected during standard phase
    bool should_collect = module.should_collect_data(data_item, 0.5); // 0.5 threshold
    std::cout << "Should collect during standard phase (threshold 0.5): " << (should_collect ? "Yes" : "No") << std::endl;
    
    // Test with different scenario (lower value)
    std::cout << std::endl << "=== Testing Low Value Scenario ===" << std::endl;
    data_value::DataItem low_value_item = data_item;
    low_value_item.scenario_type = "highway";          // 高速场景 - 低价值
    low_value_item.has_pedestrian = false;             // 无行人
    low_value_item.has_vehicle_interaction = false;    // 无车辆交互
    low_value_item.weather_condition = "sunny";        // 晴天
    low_value_item.model_uncertainty = 0.1;            // 低不确定性
    low_value_item.is_rare_scenario = false;           // 非稀有场景
    
    data_value::DataValueMetrics low_metrics = module.evaluate_data(low_value_item);
    std::cout << "Low value scenario total value: " << low_metrics.total_value << std::endl;
    std::cout << "Rule Value: " << low_metrics.rule_value 
              << ", Model Value: " << low_metrics.model_value 
              << ", Distribution Value: " << low_metrics.distribution_value << std::endl;
    
    bool should_collect_low = module.should_collect_data(low_value_item, 0.5);
    std::cout << "Should collect low value scenario: " << (should_collect_low ? "Yes" : "No") << std::endl;
    
    // Set up a callback for when data collection is triggered
    auto& trigger = module.get_trigger();
    trigger.set_collection_callback([](const data_value::DataItem& item, const data_value::DataValueMetrics& metrics) {
        std::cout << "\nData collection callback triggered for: " << item.data_id 
                  << " with total value: " << metrics.total_value
                  << " (rule: " << metrics.rule_value 
                  << ", model: " << metrics.model_value 
                  << ", distribution: " << metrics.distribution_value << ")" << std::endl;
        // Here you would implement the actual data collection logic
    });
    
    // Start the trigger system
    module.start();
    
    // Evaluate and potentially trigger collection for high-value item
    std::cout << "\n=== Testing High Value Item Collection Trigger ===" << std::endl;
    trigger.evaluate_and_trigger(data_item, 0.5);
    
    // Evaluate and potentially trigger collection for low-value item
    std::cout << "\n=== Testing Low Value Item Collection Trigger ===" << std::endl;
    trigger.evaluate_and_trigger(low_value_item, 0.5);
    
    // Stop the module
    module.stop();
    
    std::cout << "\nData value evaluation module example completed successfully!" << std::endl;
    std::cout << "The formula V_data = w1*V_rule + w2*V_model + w3*V_distribution was successfully demonstrated." << std::endl;
    
    return 0;
}