#include "data_value_trigger.h"
#include <iostream>

namespace data_value {

DataValueTrigger::DataValueTrigger() 
    : evaluator_(std::make_unique<DataValueEvaluator>())
    , min_value_threshold_(0.5)
    , running_(false) {}

DataValueTrigger::~DataValueTrigger() {
    stop();
}

void DataValueTrigger::set_collection_callback(DataCollectionCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    collection_callback_ = std::move(callback);
}

void DataValueTrigger::evaluate_and_trigger(const DataItem& data_item, double min_value_threshold) {
    // Evaluate the data item using the evaluator
    DataValueMetrics best_metrics = evaluator_->get_best_value(data_item);
    
    // Check if the value is above the threshold
    if (best_metrics.total_value >= min_value_threshold) {
        {
            std::lock_guard<std::mutex> lock(callback_mutex_);
            if (collection_callback_) {
                // Trigger data collection with the evaluated metrics
                collection_callback_(data_item, best_metrics);
            }
        }
        
        std::cout << "Data collection triggered for item " << data_item.data_id 
                  << " with total value " << best_metrics.total_value
                  << " (rule: " << best_metrics.rule_value 
                  << ", model: " << best_metrics.model_value 
                  << ", distribution: " << best_metrics.distribution_value << ")" << std::endl;
    } else {
        std::cout << "Data collection skipped for item " << data_item.data_id 
                  << " with total value " << best_metrics.total_value 
                  << " (below threshold " << min_value_threshold << ")" << std::endl;
    }
}

void DataValueTrigger::set_min_value_threshold(double threshold) {
    min_value_threshold_ = threshold;
}

double DataValueTrigger::get_min_value_threshold() const {
    return min_value_threshold_;
}

void DataValueTrigger::trigger_collection(const DataItem& data_item) {
    // Evaluate the data item
    DataValueMetrics metrics = evaluator_->get_best_value(data_item);
    
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        if (collection_callback_) {
            collection_callback_(data_item, metrics);
        }
    }
    
    std::cout << "Manual data collection triggered for item " << data_item.data_id 
              << " with total value " << metrics.total_value << std::endl;
}

void DataValueTrigger::start() {
    running_ = true;
    std::cout << "DataValueTrigger started with threshold " << min_value_threshold_ << std::endl;
}

void DataValueTrigger::stop() {
    running_ = false;
    std::cout << "DataValueTrigger stopped" << std::endl;
}

} // namespace data_value