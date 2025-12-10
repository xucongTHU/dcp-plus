// data_collection_planner.h
#ifndef DATA_COLLECTION_PLANNER_H
#define DATA_COLLECTION_PLANNER_H

#include <vector>
#include <string>
#include <memory>
#include "navigation_planner/nav_planner_node.h"
#include "navigation_planner/utils/planner_utils.h"

// Include data collection components
// #include "trigger_engine/strategy_parser/strategy_parser.h"
// #include "trigger_engine/trigger_manager.h"
// #include "recorder/data_storage.h"
// #include "uploader/data_uploader.h"

// Forward declarations for data collection components
struct DataPoint {
    Point position;
    std::string sensor_data;
    double timestamp;
    
    DataPoint(const Point& pos = Point(), const std::string& data = "", double time = 0.0)
        : position(pos), sensor_data(data), timestamp(time) {}
};

struct MissionArea {
    Point center;
    double radius;
    
    MissionArea(const Point& c = Point(), double r = 0.0) : center(c), radius(r) {}
};

namespace dcl {

class DataCollectionPlanner {
private:
    // std::shared_ptr<rscl::Node> node_;
    std::unique_ptr<planner::NavPlannerNode> nav_planner_;
    // std::unique_ptr<recorder::DataStorage> data_storage_;
    // std::unique_ptr<uploader::DataUploader> data_uploader_;
    // std::unique_ptr<trigger::TriggerManager> trigger_manager_;
    // std::unique_ptr<trigger::StrategyParser> strategy_parser_;
    // trigger::StrategyConfig strategy_config_;
    // common::AppConfigData config_;
    
    std::vector<DataPoint> collected_data_;
    MissionArea mission_area_;
    
public:
    DataCollectionPlanner(const std::string& model_file = "/workspaces/ad_data_closed_loop/training/models/planner_model.onnx",
                          const std::string& config_file = "/workspaces/ad_data_closed_loop/config/planner_weights.yaml");
    
    ~DataCollectionPlanner() = default;
    
    /**
     * @brief Initialize the data collection planner
     * @return true if initialization successful, false otherwise
     */
    bool initialize();
    
    /**
     * @brief Set the mission area for data collection
     */
    void setMissionArea(const MissionArea& area);
    
    /**
     * @brief Plan an optimal path for data collection mission
     * @return Path that optimizes data coverage
     */
    std::vector<Point> planDataCollectionMission();
    
    /**
     * @brief Execute data collection along a path using real data collection modules
     * @param path Path to follow for data collection
     */
    void executeDataCollection(const std::vector<Point>& path);
    
    /**
     * @brief Update planner with newly collected data
     * @param new_data Newly collected data points
     */
    void updateWithNewData(const std::vector<DataPoint>& new_data);
    
    /**
     * @brief Get coverage metrics for reporting
     */
    void reportCoverageMetrics();
    
    /**
     * @brief Analyze collected data and update planner weights
     */
    void analyzeAndExportWeights();
    
    /**
     * @brief Get collected data
     */
    const std::vector<DataPoint>& getCollectedData() const { return collected_data_; }
    
    /**
     * @brief Upload collected data to cloud
     */
    void uploadCollectedData();
};

// Data collection analyzer for processing collected data
class DataCollectionAnalyzer {
public:
    struct Heatmap {
        std::vector<std::vector<double>> density_values;
        int width, height;
        double resolution;
        
        Heatmap(int w = 0, int h = 0, double res = 1.0) 
            : width(w), height(h), resolution(res) {
            density_values.resize(h, std::vector<double>(w, 0.0));
        }
    };
    
    struct Region {
        Point center;
        double radius;
        bool is_sparse;
        
        Region(const Point& c = Point(), double r = 0.0, bool sparse = false)
            : center(c), radius(r), is_sparse(sparse) {}
    };
    
    struct PlannerWeights {
        double sparse_threshold;
        double exploration_bonus;
        double redundancy_penalty;
        // double grid_resolution;
        
        PlannerWeights(double threshold = 0.2, double bonus = 0.5, double penalty = 0.4)
            : sparse_threshold(threshold), exploration_bonus(bonus),
              redundancy_penalty(penalty) {}
    };
    
    /**
     * @brief Compute density map from collected data
     */
    static Heatmap computeDensityMap(const std::vector<DataPoint>& data_points, 
                                    int grid_width = 100, int grid_height = 100, 
                                    double resolution = 1.0);
    
    /**
     * @brief Detect sparse regions in heatmap
     */
    static std::vector<Region> detectSparseRegions(const Heatmap& heatmap, 
                                                  double sparse_threshold = 0.2);
    
    /**
     * @brief Adjust cost weights based on sparse zones
     */
    static PlannerWeights adjustCostWeights(const std::vector<Region>& sparse_zones,
                                          const PlannerWeights& current_weights);
    
    /**
     * @brief Save weights to planner configuration file
     */
    static void saveToPlannerConfig(const PlannerWeights& weights, 
                                   const std::string& config_path);
};

} // namespace dcl

#endif // DATA_COLLECTION_PLANNER_H