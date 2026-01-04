#ifndef DATA_VALUE_MODULE_H
#define DATA_VALUE_MODULE_H

#include "data_value_evaluator.h"
#include "data_value_trigger.h"

namespace data_value {

// Main module class that integrates data value evaluation and triggering
class DataValueModule {
public:
    DataValueModule();
    ~DataValueModule();
    
    // Initialize the module with default settings
    bool initialize();
    
    // Get reference to the evaluator
    DataValueEvaluator& get_evaluator();
    
    // Get reference to the trigger
    DataValueTrigger& get_trigger();
    
    // Convenience method to evaluate a data item
    DataValueMetrics evaluate_data(const DataItem& data_item);
    
    // Convenience method to check if data should be collected during standard phase
    bool should_collect_data(const DataItem& data_item, double min_value_threshold = 0.5);
    
    // Start the module
    void start();
    
    // Stop the module
    void stop();

private:
    std::unique_ptr<DataValueEvaluator> evaluator_;
    std::unique_ptr<DataValueTrigger> trigger_;
    bool initialized_;
};

} // namespace data_value

#endif // DATA_VALUE_MODULE_H