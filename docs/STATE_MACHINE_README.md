# 状态机集成说明

## 功能概述

本次更新实现了状态机功能，用于整合data_collection和navigation_planner模块，通过状态机实现路径规划和数据采集的状态切换管理。

## 添加的文件

1. `state_machine.h` - 状态机类定义头文件
2. `state_machine.cpp` - 状态机类实现文件
3. 更新了`main.cpp`以演示状态机的使用
4. 更新了`CMakeLists.txt`以包含新文件

## 核心功能

### 状态管理
- 定义了8种系统状态（初始化、空闲、规划、导航、数据采集、上传、错误、关闭）
- 定义了12种状态转换事件
- 实现了完整的状态转换逻辑

### 组件集成
- 状态机集成了DataCollectionPlanner、NavPlannerNode和DataStorage组件
- 通过事件驱动的方式协调各组件工作

### 日志记录
- 所有状态转换都被记录到日志中，便于调试和监控

## 使用方法

在主程序中创建状态机实例，并设置相关组件：

```cpp
auto state_machine = std::make_shared<StateMachine>();
state_machine->setDataCollectionPlanner(collector);
state_machine->setNavPlanner(nav_planner);
state_machine->setDataStorage(data_storage);

// 初始化系统
state_machine->initialize();

// 发送事件驱动状态转换
state_machine->handleEvent(StateEvent::PLAN_REQUEST);
```

## 状态转换流程

1. 系统启动后进入INITIALIZING状态
2. 初始化完成后进入IDLE状态
3. 接收到PLAN_REQUEST事件后进入PLANNING状态
4. 规划完成后进入NAVIGATING状态
5. 在导航过程中根据条件进入DATA_COLLECTION状态
6. 数据采集完成后返回NAVIGATING状态
7. 任务完成后进入UPLOADING状态
8. 上传完成后回到IDLE状态

## 扩展性

该状态机设计具有良好的扩展性：
- 可以轻松添加新的状态和事件
- 状态转换逻辑清晰，易于维护
- 支持错误处理和系统恢复
- 支持系统正常关闭流程

## 编译说明

新添加的文件已更新到CMakeLists.txt中，执行以下命令即可编译：

```bash
cd /workspaces/ad_data_closed_loop/src
mkdir build && cd build
cmake ..
make
```