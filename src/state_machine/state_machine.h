#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <memory>
#include <string>
#include "../data_collection_planner.h"
#include "../navigation_planner/nav_planner_node.h"
#include "../recorder/DataStorage.h"

// 系统状态枚举
enum class SystemState {
    INITIALIZING,           // 初始化中
    IDLE,                   // 空闲状态
    PLANNING,               // 路径规划中
    NAVIGATING,             // 导航中
    TRIGGERED,              // 触发条件已满足
    UNTRIGGERED,            // 触发条件未满足
    DATA_COLLECTION,        // 数据采集中
    UPLOADING,              // 数据上传中
    ERROR,                  // 错误状态
    SHUTTING_DOWN           // 关闭中
};

// 状态转换事件枚举
enum class StateEvent {
    INIT_COMPLETE,          // 初始化完成
    PLAN_REQUEST,           // 规划请求
    PLAN_COMPLETE,          // 规划完成
    NAVIGATION_START,       // 开始导航
    WAYPOINT_REACHED,       // 到达路径点
    TRIGGERED,              // 触发条件满足
    UNTRIGGERED,            // 触发条件不满足
    DATA_COLLECTED,         // 数据采集完成
    UPLOAD_REQUEST,         // 上传请求
    UPLOAD_COMPLETE,        // 上传完成
    ERROR_OCCURRED,         // 发生错误
    RECOVERY_REQUEST,       // 恢复请求
    SHUTDOWN_REQUEST        // 关闭请求
};

namespace dcl::state_machine {

class StateMachine {
public:
    StateMachine();
    ~StateMachine() = default;

    // 初始化状态机
    bool initialize();
    
    // 处理状态事件
    void handleEvent(StateEvent event);
    
    // 获取当前状态
    SystemState getCurrentState() const { return current_state_; }
    
    // 设置数据采集规划器
    void setDataCollectionPlanner(std::shared_ptr<DataCollectionPlanner> planner);
    
    // 设置导航规划器
    void setNavPlanner(std::shared_ptr<NavPlannerNode> nav_planner);
    
    // 设置数据存储器
    void setDataStorage(std::shared_ptr<DataStorage> data_storage);
    
    // 状态转换日志
    void logStateTransition(SystemState from, SystemState to, StateEvent event);

private:
    // 状态处理函数
    void handleInitializing(StateEvent event);
    void handleIdle(StateEvent event);
    void handlePlanning(StateEvent event);
    void handleNavigating(StateEvent event);
    void handleDataCollection(StateEvent event);
    void handleUploading(StateEvent event);
    void handleError(StateEvent event);
    void handleShuttingDown(StateEvent event);
    
    // 状态转换辅助函数
    void transitionToState(SystemState new_state, StateEvent event);
    
    // 内部组件
    std::shared_ptr<DataCollectionPlanner> data_collection_planner_;
    std::shared_ptr<NavPlannerNode> nav_planner_;
    std::shared_ptr<DataStorage> data_storage_;
    
    // 当前状态
    SystemState current_state_;
    
    // 任务相关数据
    MissionArea current_mission_;
    std::vector<Point> current_path_;
    size_t current_waypoint_index_;
};

} // namespace dcl::state_machine

#endif // STATE_MACHINE_H