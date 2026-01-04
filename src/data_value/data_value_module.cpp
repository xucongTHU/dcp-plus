#include "data_value_module.h"
#include <iostream>

namespace data_value {

DataValueModule::DataValueModule() 
    : evaluator_(std::make_unique<DataValueEvaluator>())
    , trigger_(std::make_unique<DataValueTrigger>())
    , initialized_(false) {}

DataValueModule::~DataValueModule() = default;

bool DataValueModule::initialize() {
    // Additional initialization can be done here
    initialized_ = true;
    std::cout << "DataValueModule initialized successfully" << std::endl;
    return initialized_;
}

DataValueEvaluator& DataValueModule::get_evaluator() {
    if (!evaluator_) {
        evaluator_ = std::make_unique<DataValueEvaluator>();
    }
    return *evaluator_;
}

DataValueTrigger& DataValueModule::get_trigger() {
    if (!trigger_) {
        trigger_ = std::make_unique<DataValueTrigger>();
    }
    return *trigger_;
}

DataValueMetrics DataValueModule::evaluate_data(const DataItem& data_item) {
    if (!evaluator_) {
        std::cerr << "Evaluator not available" << std::endl;
        DataValueMetrics default_metrics;
        default_metrics.evaluation_time = std::chrono::system_clock::now();
        return default_metrics;
    }
    
    return evaluator_->get_best_value(data_item);
}

bool DataValueModule::should_collect_data(const DataItem& data_item, double min_value_threshold) {
    if (!evaluator_) {
        return false;
    }
    
    DataValueMetrics metrics = evaluator_->get_best_value(data_item);
    return metrics.total_value >= min_value_threshold;
}

void DataValueModule::start() {
    if (trigger_) {
        trigger_->start();
    }
    std::cout << "DataValueModule started" << std::endl;
}

void DataValueModule::stop() {
    if (trigger_) {
        trigger_->stop();
    }
    std::cout << "DataValueModule stopped" << std::endl;
}

} // namespace data_value