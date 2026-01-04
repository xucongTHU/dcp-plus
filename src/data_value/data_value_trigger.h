#ifndef DATA_VALUE_TRIGGER_H
#define DATA_VALUE_TRIGGER_H

#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include "data_value_evaluator.h"

namespace data_value {

// Callback type for when data collection should be triggered
using DataCollectionCallback = std::function<void(const DataItem& data_item, const DataValueMetrics& metrics)>;

class DataValueTrigger {
public:
    DataValueTrigger();
    ~DataValueTrigger();
    
    // Set the callback function to be called when data should be collected
    void set_collection_callback(DataCollectionCallback callback);
    
    // Evaluate data and trigger collection if value is above threshold
    void evaluate_and_trigger(const DataItem& data_item, double min_value_threshold = 0.5);
    
    // Set the minimum value threshold for triggering data collection
    void set_min_value_threshold(double threshold);
    
    // Get the current minimum value threshold
    double get_min_value_threshold() const;
    
    // Manually trigger data collection for a specific item
    void trigger_collection(const DataItem& data_item);
    
    // Start the trigger system
    void start();
    
    // Stop the trigger system
    void stop();

private:
    std::unique_ptr<DataValueEvaluator> evaluator_;
    DataCollectionCallback collection_callback_;
    double min_value_threshold_;
    std::atomic<bool> running_;
    std::mutex callback_mutex_;
};

} // namespace data_value

#endif // DATA_VALUE_TRIGGER_H