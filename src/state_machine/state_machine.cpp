#include "state_machine.h"
#include "common/log/Logger.h"
#include <iostream>

namespace dcl::state_machine {
StateMachine::StateMachine() 
    : current_state_(SystemState::INITIALIZING)
    , current_waypoint_index_(0) {
}

bool StateMachine::initialize() {
    AD_INFO(StateMachine, "Initializing state machine");
    
    if (!data_collection_planner_) {
        AD_ERROR(StateMachine, "Data collection planner not set");
        return false;
    }
    
    if (!nav_planner_) {
        AD_ERROR(StateMachine, "Navigation planner not set");
        return false;
    }
    
    if (!data_storage_) {
        AD_ERROR(StateMachine, "Data storage not set");
        return false;
    }
    
    // 初始化各组件
    if (!data_collection_planner_->initialize()) {
        AD_ERROR(StateMachine, "Failed to initialize data collection planner");
        transitionToState(SystemState::ERROR, StateEvent::ERROR_OCCURRED);
        return false;
    }
    
    if (!nav_planner_->initialize()) {
        AD_ERROR(StateMachine, "Failed to initialize navigation planner");
        transitionToState(SystemState::ERROR, StateEvent::ERROR_OCCURRED);
        return false;
    }
    
    AD_INFO(StateMachine, "State machine initialized successfully");
    transitionToState(SystemState::IDLE, StateEvent::INIT_COMPLETE);
    return true;
}

void StateMachine::handleEvent(StateEvent event) {
    switch (current_state_) {
        case SystemState::INITIALIZING:
            handleInitializing(event);
            break;
        case SystemState::IDLE:
            handleIdle(event);
            break;
        case SystemState::PLANNING:
            handlePlanning(event);
            break;
        case SystemState::NAVIGATING:
            handleNavigating(event);
            break;
        case SystemState::DATA_COLLECTION:
            handleDataCollection(event);
            break;
        case SystemState::UPLOADING:
            handleUploading(event);
            break;
        case SystemState::ERROR:
            handleError(event);
            break;
        case SystemState::SHUTTING_DOWN:
            handleShuttingDown(event);
            break;
    }
}

void StateMachine::handleInitializing(StateEvent event) {
    // 初始化状态只处理初始化完成事件
    if (event == StateEvent::INIT_COMPLETE) {
        transitionToState(SystemState::IDLE, event);
    } else {
        AD_WARN(StateMachine, "Unexpected event in INITIALIZING state");
    }
}

void StateMachine::handleIdle(StateEvent event) {
    switch (event) {
        case StateEvent::PLAN_REQUEST:
            AD_INFO(StateMachine, "Starting mission planning");
            transitionToState(SystemState::PLANNING, event);
            break;
        case StateEvent::UPLOAD_REQUEST:
            AD_INFO(StateMachine, "Starting data upload");
            transitionToState(SystemState::UPLOADING, event);
            break;
        case StateEvent::SHUTDOWN_REQUEST:
            AD_INFO(StateMachine, "Shutting down system");
            transitionToState(SystemState::SHUTTING_DOWN, event);
            break;
        default:
            AD_WARN(StateMachine, "Unexpected event in IDLE state");
            break;
    }
}

void StateMachine::handlePlanning(StateEvent event) {
    switch (event) {
        case StateEvent::PLAN_COMPLETE:
            AD_INFO(StateMachine, "Mission planning completed");
            transitionToState(SystemState::NAVIGATING, event);
            break;
        case StateEvent::ERROR_OCCURRED:
            AD_ERROR(StateMachine, "Error occurred during planning");
            transitionToState(SystemState::ERROR, event);
            break;
        default:
            AD_WARN(StateMachine, "Unexpected event in PLANNING state");
            break;
    }
}

void C::handleNavigating(StateEvent event) {
    switch (event) {
        case StateEvent::WAYPOINT_REACHED:
            AD_INFO(StateMachine, "Waypoint reached");
            // 检查是否需要采集数据
            if (current_waypoint_index_ < current_path_.size()) {
                const Point& waypoint = current_path_[current_waypoint_index_];
                // 这里应该使用真实的触发管理器来判断是否需要采集数据
                // 为简化，我们假设每5个点采集一次数据
                if (current_waypoint_index_ % 5 == 0) {
                    transitionToState(SystemState::DATA_COLLECTION, StateEvent::TRIGGERED);
                } else {
                    // 移动到下一个路径点
                    current_waypoint_index_++;
                }
            } else {
                // 路径完成，开始上传数据
                transitionToState(SystemState::UPLOADING, StateEvent::UPLOAD_REQUEST);
            }
            break;
        case StateEvent::ERROR_OCCURRED:
            AD_ERROR(StateMachine, "Error occurred during navigation");
            transitionToState(SystemState::ERROR, event);
            break;
        default:
            AD_WARN(StateMachine, "Unexpected event in NAVIGATING state");
            break;
    }
}

void C::handleDataCollection(StateEvent event) {
    switch (event) {
        case StateEvent::DATA_COLLECTED:
            AD_INFO(StateMachine, "Data collection completed");
            // 数据采集完成后继续导航
            current_waypoint_index_++;
            transitionToState(SystemState::NAVIGATING, event);
            break;
        case StateEvent::ERROR_OCCURRED:
            AD_ERROR(StateMachine, "Error occurred during data collection");
            transitionToState(SystemState::ERROR, event);
            break;
        default:
            AD_WARN(StateMachine, "Unexpected event in DATA_COLLECTION state");
            break;
    }
}

void StateMachine::handleUploading(StateEvent event) {
    switch (event) {
        case StateEvent::UPLOAD_COMPLETE:
            AD_INFO(StateMachine, "Data upload completed");
            transitionToState(SystemState::IDLE, event);
            break;
        case StateEvent::ERROR_OCCURRED:
            AD_ERROR(StateMachine, "Error occurred during data upload");
            transitionToState(SystemState::ERROR, event);
            break;
        default:
            AD_WARN(StateMachine, "Unexpected event in UPLOADING state");
            break;
    }
}

void StateMachine::handleError(StateEvent event) {
    switch (event) {
        case StateEvent::RECOVERY_REQUEST:
            AD_INFO(StateMachine, "Attempting system recovery");
            // 尝试恢复到空闲状态
            transitionToState(SystemState::IDLE, event);
            break;
        case StateEvent::SHUTDOWN_REQUEST:
            AD_INFO(StateMachine, "Shutting down due to error");
            transitionToState(SystemState::SHUTTING_DOWN, event);
            break;
        default:
            AD_WARN(StateMachine, "Unexpected event in ERROR state");
            break;
    }
}

void StateMachine::handleShuttingDown(StateEvent event) {
    // 关闭状态下不处理任何事件
    AD_WARN(StateMachine, "Event received during shutdown state, ignoring");
}

void StateMachine::transitionToState(SystemState new_state, StateEvent event) {
    SystemState old_state = current_state_;
    current_state_ = new_state;
    logStateTransition(old_state, new_state, event);
    
    // 根据新状态执行相应的动作
    switch (new_state) {
        case SystemState::PLANNING:
            // 执行路径规划
            if (data_collection_planner_) {
                current_path_ = data_collection_planner_->planDataCollectionMission();
                if (!current_path_.empty()) {
                    current_waypoint_index_ = 0;
                    handleEvent(StateEvent::PLAN_COMPLETE);
                } else {
                    handleEvent(StateEvent::ERROR_OCCURRED);
                }
            }
            break;
        case SystemState::DATA_COLLECTION:
            // 执行数据采集
            if (data_collection_planner_ && current_waypoint_index_ < current_path_.size()) {
                // 采集当前路径点的数据
                std::vector<Point> single_point_path = {current_path_[current_waypoint_index_]};
                data_collection_planner_->executeDataCollection(single_point_path);
                handleEvent(StateEvent::DATA_COLLECTED);
            }
            break;
        case SystemState::UPLOADING:
            // 执行数据上传
            if (data_collection_planner_) {
                data_collection_planner_->uploadCollectedData();
                handleEvent(StateEvent::UPLOAD_COMPLETE);
            }
            break;
        case SystemState::SHUTTING_DOWN:
            AD_INFO(StateMachine, "System shutdown complete");
            break;
        default:
            // 其他状态不需要特殊处理
            break;
    }
}

void StateMachine::logStateTransition(SystemState from, SystemState to, StateEvent event) {
    std::string from_str, to_str, event_str;
    
    // 状态字符串映射
    switch (from) {
        case SystemState::INITIALIZING: from_str = "INITIALIZING"; break;
        case SystemState::IDLE: from_str = "IDLE"; break;
        case SystemState::PLANNING: from_str = "PLANNING"; break;
        case SystemState::NAVIGATING: from_str = "NAVIGATING"; break;
        case SystemState::DATA_COLLECTION: from_str = "DATA_COLLECTION"; break;
        case SystemState::UPLOADING: from_str = "UPLOADING"; break;
        case SystemState::ERROR: from_str = "ERROR"; break;
        case SystemState::SHUTTING_DOWN: from_str = "SHUTTING_DOWN"; break;
    }
    
    switch (to) {
        case SystemState::INITIALIZING: to_str = "INITIALIZING"; break;
        case SystemState::IDLE: to_str = "IDLE"; break;
        case SystemState::PLANNING: to_str = "PLANNING"; break;
        case SystemState::NAVIGATING: to_str = "NAVIGATING"; break;
        case SystemState::DATA_COLLECTION: to_str = "DATA_COLLECTION"; break;
        case SystemState::UPLOADING: to_str = "UPLOADING"; break;
        case SystemState::ERROR: to_str = "ERROR"; break;
        case SystemState::SHUTTING_DOWN: to_str = "SHUTTING_DOWN"; break;
    }
    
    // 事件字符串映射
    switch (event) {
        case StateEvent::INIT_COMPLETE: event_str = "INIT_COMPLETE"; break;
        case StateEvent::PLAN_REQUEST: event_str = "PLAN_REQUEST"; break;
        case StateEvent::PLAN_COMPLETE: event_str = "PLAN_COMPLETE"; break;
        case StateEvent::NAVIGATION_START: event_str = "NAVIGATION_START"; break;
        case StateEvent::WAYPOINT_REACHED: event_str = "WAYPOINT_REACHED"; break;
        case StateEvent::TRIGGERED: event_str = "TRIGGERED"; break;
        case StateEvent::DATA_COLLECTED: event_str = "DATA_COLLECTED"; break;
        case StateEvent::UPLOAD_REQUEST: event_str = "UPLOAD_REQUEST"; break;
        case StateEvent::UPLOAD_COMPLETE: event_str = "UPLOAD_COMPLETE"; break;
        case StateEvent::ERROR_OCCURRED: event_str = "ERROR_OCCURRED"; break;
        case StateEvent::RECOVERY_REQUEST: event_str = "RECOVERY_REQUEST"; break;
        case StateEvent::SHUTDOWN_REQUEST: event_str = "SHUTDOWN_REQUEST"; break;
    }
    
    AD_INFO(StateMachine, "State transition: %s -> %s (event: %s)", from_str.c_str(), to_str.c_str(), event_str.c_str()); 
}

void StateMachine::setDataCollectionPlanner(std::shared_ptr<DataCollectionPlanner> planner) {
    data_collection_planner_ = planner;
}

void StateMachine::setNavPlanner(std::shared_ptr<NavPlannerNode> nav_planner) {
    nav_planner_ = nav_planner;
}

void StateMachine::setDataStorage(std::shared_ptr<DataStorage> data_storage) {
    data_storage_ = data_storage;
}

} // namespace dcl::state_machine