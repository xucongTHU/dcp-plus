# Data Value Evaluation Module

The Data Value Evaluation Module provides a framework for assessing the value of data items in the AD EdgeInsight system using a mathematical formula approach. It supports both fixed rule-based evaluation and AI-based evaluation to determine whether data should be collected during the standard phase.

## Formula-Based Evaluation

The module implements the following formula for data value calculation:

**V_data = w1 * V_rule + w2 * V_model + w3 * V_distribution**

Where:
- **V_rule**: 规则价值 (交叉口、行人出现、夜间雨天)
- **V_model**: 模型价值 (感知模型不确定性高 数据更有价值)
- **V_distribution**: 数据分布价值 (历史少采的场景 价值高；重复场景 价值低)
- **w1, w2, w3**: 权重 (固定值，按经验设定或仿真验证)

Default weights: w1=0.4, w2=0.4, w3=0.2

## Overview

The module consists of the following components:

- **DataValueEvaluator**: Core evaluator that uses different strategies to assess data value
- **DataValueTrigger**: Triggers data collection based on value assessment
- **DataValueModule**: Main integration class that combines evaluator and trigger

## Features

- **Formula-Based Evaluation**: Implements the mathematical formula V_data = w1*V_rule + w2*V_model + w3*V_distribution
- **Multiple Evaluation Strategies**:
  - Fixed Value Strategy: Uses the formula with configurable weights
  - AI Evaluation Strategy: Uses machine learning models to predict data value

- **Standard Phase Collection**: Determines if data should be collected during the standard phase based on value thresholds

- **Configurable Weights**: Adjustable weights for different value components

## Key Classes

### DataItem
Represents a data item to be evaluated, containing:
- `data_id`: Unique identifier for the data item
- `data_type`: Type of data (e.g., sensor_data, lidar_pointcloud, camera_image)
- `scenario_type`: Scenario type (e.g., intersection, highway, urban_road)
- `has_pedestrian`: Whether pedestrians are present
- `has_vehicle_interaction`: Whether there are vehicle interactions
- `weather_condition`: Weather conditions (sunny, rainy, foggy, night)
- `model_uncertainty`: Model uncertainty value
- `is_rare_scenario`: Whether this is a rare scenario
- `location`: Location information
- Other standard fields (size, creation time, metadata, etc.)

### DataValueMetrics
Contains the evaluation results:
- `rule_value`: V_rule component (rule-based value)
- `model_value`: V_model component (model-based value)
- `distribution_value`: V_distribution component (distribution-based value)
- `w1, w2, w3`: Weights used in the formula
- `total_value`: Final calculated value using the formula
- Other standard metrics (quality, relevance, etc.)

## Usage

### Basic Evaluation
```cpp
data_value::DataValueModule module;
module.initialize();

data_value::DataItem data_item;
// ... populate data_item fields including scenario-specific ones ...

data_value::DataValueMetrics metrics = module.evaluate_data(data_item);
bool should_collect = module.should_collect_data(data_item, 0.5); // 0.5 threshold
```

### Scenario-Specific Fields for Value Calculation
```cpp
data_item.scenario_type = "intersection";          // 交叉口场景 - 高价值
data_item.has_pedestrian = true;                   // 有行人 - 高价值
data_item.has_vehicle_interaction = true;          // 有车辆交互 - 中等价值
data_item.weather_condition = "rainy";             // 雨天 - 高价值
data_item.model_uncertainty = 0.7;                 // 模型不确定性高 - 高价值
data_item.is_rare_scenario = true;                 // 稀有场景 - 高价值
```

### Setting Up Data Collection Trigger
```cpp
auto& trigger = module.get_trigger();
trigger.set_collection_callback([](const data_value::DataItem& item, const data_value::DataValueMetrics& metrics) {
    // Implement data collection logic here
    std::cout << "Collecting data: " << item.data_id << std::endl;
    std::cout << "Value breakdown - Rule: " << metrics.rule_value 
              << ", Model: " << metrics.model_value 
              << ", Distribution: " << metrics.distribution_value << std::endl;
});

trigger.evaluate_and_trigger(data_item, 0.5);
```

## Value Calculation Components

### Rule Value (V_rule)
- Scenario complexity (intersection, crosswalk, highway_merge have high values)
- Pedestrian presence (high value when pedestrians detected)
- Vehicle interactions (medium value for interactions)
- Weather conditions (rainy, foggy, night, snow have higher values)

### Model Value (V_model)
- Based on model uncertainty
- Higher uncertainty means more valuable data for model improvement
- Directly uses the `model_uncertainty` field from DataItem

### Distribution Value (V_distribution)
- Based on data rarity/distribution
- Higher value for rare scenarios
- Lower value for common scenarios (like highway driving)

## Integration with Data Collection System

The module is designed to work seamlessly with the existing data collection system in AD EdgeInsight, using extended data structures that include scenario-specific fields for value calculation.

## Configuration

The module supports configurable weights for the formula:

- Default weights: w1=0.4, w2=0.4, w3=0.2
- Can be customized during FixedValueStrategy construction
- Different evaluation strategies can be added or removed at runtime