# 自动驾驶场景实现

本文档详细描述了自动驾驶系统中各类场景触发条件实现。

## 1. 变道场景 (Lane Change)

### 1.1 左绕障变道 (left_nudge)

在效率变道的基础上，自车轨迹终点处于当前自车车道，注意绕障不变更灯。
```angular
1. 计算 efficiency_lane_change:
    - 安全间隙判断
    - 左右车道速度/成本计算
    - 返回 true/false

2. 计算 ego_trajectory_end_in_current_lane:
    - 获取轨迹终点
    - 判断是否在当前车道范围内

3. 计算 changing_light_for_obstacle:
    - 感知障碍物
    - 判断是否需要变道开灯

4. 触发条件:
    trigger = efficiency_lane_change 
           && ego_trajectory_end_in_current_lane 
           && !changing_light_for_obstacle
```
```cpp
// 左绕障效率变道触发器函数
bool checkLeftObstacleLaneChange(
    double v_ego,
    double d_front_left, double d_rear_left,
    double avg_speed_left_lane,
    double d_end, double lane_center_d, double lane_width,
    bool obstacle_detected,
    bool obstacle_requires_lane_change)
{
    // 1️⃣ efficiency_lane_change（左侧绕障版）
    bool safe_left_gap = (d_front_left > v_ego*1.5) && (d_rear_left > v_ego*1.5); // 安全距离阈值
    bool efficient_left = (avg_speed_left_lane > v_ego + 1.0);                     // 左车道更高效
    bool efficiency_lane_change = safe_left_gap && efficient_left;

    // 2️⃣ ego_trajectory_end_in_current_lane
    bool ego_trajectory_end_in_current_lane = (abs(d_end - lane_center_d) <= lane_width/2);

    // 3️⃣ changing_light_for_obstacle
    bool changing_light_for_obstacle = obstacle_detected && obstacle_requires_lane_change;

    // 4️⃣ 触发条件
    bool trigger = efficiency_lane_change
                 && ego_trajectory_end_in_current_lane
                 && !changing_light_for_obstacle;

    return trigger;
}



```

### 1.2 右绕障变道 (right_nudge)

与左绕障变道类似。
```cpp
// 右绕障效率变道触发器函数
bool checkRightObstacleLaneChange(
    double v_ego,
    double d_front_right, double d_rear_right,
    double avg_speed_right_lane,
    double d_end, double lane_center_d, double lane_width,
    bool obstacle_detected,
    bool obstacle_requires_lane_change)
{
    // 1️⃣ efficiency_lane_change（右侧绕障版）
    // 安全 gap：右前、右后距离满足阈值
    bool safe_right_gap = (d_front_right > v_ego*1.5) && (d_rear_right > v_ego*1.5); 
    // 效率判断：右车道速度更高
    bool efficient_right = (avg_speed_right_lane > v_ego + 1.0);                     
    bool efficiency_lane_change = safe_right_gap && efficient_right;

    // 2️⃣ ego_trajectory_end_in_current_lane
    // 保证绕障后轨迹终点仍在当前车道
    bool ego_trajectory_end_in_current_lane = (abs(d_end - lane_center_d) <= lane_width/2);

    // 3️⃣ changing_light_for_obstacle
    // 判断绕障是否需要开灯变道
    bool changing_light_for_obstacle = obstacle_detected && obstacle_requires_lane_change;

    // 4️⃣ 触发条件
    bool trigger = efficiency_lane_change
                 && ego_trajectory_end_in_current_lane
                 && !changing_light_for_obstacle;

    return trigger;
}

```

### 1.3 左变道

触发条件：
- 存在左车道线(line_start_x20)
- 左车道线是可跨越车道线(虚线)
- 存在变道点(固定距离轨迹和车道线交点)
- 变道点x位置小于轨迹的一半左右(防止变道太晚)
- 轨迹终点小于车道线终点(防止路口导航变道误判)
- 不能跨多个车道线
```cpp
// 左车道变道触发器函数
bool checkLeftLaneChangeTrigger(
    bool has_left_lane_line,
    bool left_lane_line_is_dashed,
    const std::vector<Point>& lane_change_points, // 变道点列表
    double min_lane_change_x,
    double max_lane_change_x,
    double trajectory_end_d,      // 轨迹终点横向位置
    double lane_center_d,         // 左车道中心d
    double lane_width,            // 单车道宽度
    double trajectory_min_d,      // 轨迹最小d
    double trajectory_max_d       // 轨迹最大d
)
{
    // 1️⃣ 左车道线存在且是虚线
    bool lane_line_ok = has_left_lane_line && left_lane_line_is_dashed;

    // 2️⃣ 存在变道点
    bool has_lane_change_point = !lane_change_points.empty();

    // 3️⃣ 变道点 x 位置合适（取第一个点作为示例）
    bool lane_change_point_x_valid = false;
    if(has_lane_change_point)
    {
        double x = lane_change_points.front().x;
        lane_change_point_x_valid = (x >= min_lane_change_x) && (x <= max_lane_change_x);
    }

    // 4️⃣ 轨迹终点小于车道线终点
    bool trajectory_end_less_than_lane_end = (trajectory_end_d < lane_center_d + lane_width/2);

    // 5️⃣ 不跨越多车道
    double trajectory_lateral_span = trajectory_max_d - trajectory_min_d;
    bool not_crossing_multiple_lanes = (trajectory_lateral_span <= lane_width);

    // 6️⃣ 组合触发条件
    bool trigger = lane_line_ok
                 && has_lane_change_point
                 && lane_change_point_x_valid
                 && trajectory_end_less_than_lane_end
                 && not_crossing_multiple_lanes;

    return trigger;
}

```
### 1.4 右变道

触发条件：
- 存在右车道线(line_start_x20)
- 右车道线是可跨越车道线(虚线)
- 存在变道点(固定距离轨迹和车道线交点)
- 变道点x位置小于轨迹的一半左右(防止变道太晚)
- 轨迹终点小于车道线终点(防止路口导航变道误判)
- 不能跨多个车道线
```cpp
// 右车道变道触发器函数
bool checkRightLaneChangeTrigger(
    bool has_right_lane_line,
    bool right_lane_line_is_dashed,
    const std::vector<Point>& lane_change_points, // 变道点列表
    double min_lane_change_x,
    double max_lane_change_x,
    double trajectory_end_d,      // 轨迹终点横向位置
    double lane_center_d,         // 右车道中心d
    double lane_width,            // 单车道宽度
    double trajectory_min_d,      // 轨迹最小d
    double trajectory_max_d       // 轨迹最大d
)
{
    // 1️⃣ 右车道线存在且是虚线
    bool lane_line_ok = has_right_lane_line && right_lane_line_is_dashed;

    // 2️⃣ 存在变道点
    bool has_lane_change_point = !lane_change_points.empty();

    // 3️⃣ 变道点 x 位置合适（取第一个点作为示例）
    bool lane_change_point_x_valid = false;
    if(has_lane_change_point)
    {
        double x = lane_change_points.front().x;
        lane_change_point_x_valid = (x >= min_lane_change_x) && (x <= max_lane_change_x);
    }

    // 4️⃣ 轨迹终点小于车道线终点
    bool trajectory_end_less_than_lane_end = (trajectory_end_d < lane_center_d + lane_width/2);

    // 5️⃣ 不跨越多车道
    double trajectory_lateral_span = trajectory_max_d - trajectory_min_d;
    bool not_crossing_multiple_lanes = (trajectory_lateral_span <= lane_width);

    // 6️⃣ 组合触发条件
    bool trigger = lane_line_ok
                 && has_lane_change_point
                 && lane_change_point_x_valid
                 && trajectory_end_less_than_lane_end
                 && not_crossing_multiple_lanes;

    return trigger;
}

```

### 1.5 左效率变道

在左变道基础上增加以下判断逻辑：
- 存在自车道的cipv
- 判断cipv是否小于未来自车最大速度
```cpp
// 左变道 + CIPV 触发器函数
bool checkLeftLaneChangeWithCIPV(
    bool left_lane_change,        // 左变道前提条件
    bool has_ego_lane_cipv,      // 自车道前车是否存在
    double cipv_speed,            // CIPV 当前速度
    double max_future_ego_speed   // 自车未来最大速度
)
{
    // 1️⃣ 左变道判断
    left_lane_change = checkLeftLaneChangeTrigger(...);

    // 2️⃣ CIPV 速度判断
    bool cipv_less_than_max_future_ego_speed = has_ego_lane_cipv && (cipv_speed < max_future_ego_speed);

    // 3️⃣ 组合触发条件
    bool trigger = left_lane_change
                 && has_ego_lane_cipv
                 && cipv_less_than_max_future_ego_speed;

    return trigger;
}

```

### 1.6 右效率变道

在右变道基础上增加以下判断逻辑：
- 存在自车道的cipv
- 判断cipv是否小于未来自车最大速度
```cpp
// 右变道 + CIPV 触发器函数
bool checkRightLaneChangeWithCIPV(
    bool right_lane_change,       // 右变道前提条件
    bool has_ego_lane_cipv,       // 自车道前车是否存在
    double cipv_speed,            // CIPV 当前速度
    double max_future_ego_speed   // 自车未来最大速度
)
{
    // 1️⃣ 右变道判断
    right_lane_change = checkRightLaneChangeTrigger(...);
    
    // 2️⃣ CIPV 速度判断
    bool cipv_less_than_max_future_ego_speed = has_ego_lane_cipv && (cipv_speed < max_future_ego_speed);

    // 3️⃣ 组合触发条件
    bool trigger = right_lane_change
                 && has_ego_lane_cipv
                 && cipv_less_than_max_future_ego_speed;

    return trigger;
}

```

## 2. 转弯场景 (Turn)

### 2.1 无交互左转

触发条件：
- 自车需要处于路口状态（左右车道线短于轨迹，或者自车前后一定范围内存在停止线，可以避免环岛等错误场景但是可能误杀很多无停止线的转弯场景）
- 自车的横摆角速度（yaw_raw）大于0.2（rad）
- 自车的结尾帧yaw大于0.3（rad）
- 车道线末端和轨迹末端夹角大于15度(排除曲率弯)
- 自车中点的yaw>0.1且与末端yaw反向

### 2.2 无交互右转

触发条件：
- 自车需要处于路口状态（左右车道线短于轨迹，或者自车前后一定范围内存在停止线）
- 自车的横摆角速度（yaw_raw）大于0.2（rad）
- 自车的结尾帧yaw小于-0.3（rad）
- 车道线末端和轨迹末端夹角大于15度(排除曲率弯)
- 自车中点的yaw>0.1且与末端yaw反向

### 2.3 有交互左转

在无交互左转基础上增加交互逻辑判断：
- 根据他车与自车未来轨迹点之间的距离（是否小于4m，并且自车时刻晚于他车时刻，不超过4s）
- 根据polygon判断是否存在交集
- 考虑他车是否减速

### 2.4 有交互右转

在无交互右转基础上增加交互逻辑判断：
- 根据他车与自车未来轨迹点之间的距离（是否小于4m，并且自车时刻晚于他车时刻，不超过4s）
- 根据polygon判断是否存在交集
- 考虑他车是否减速

### 2.5 左转弯静止

在左转基础上，判断自车是否静止：速度小于0.01m/s，触发回传时间点静止

### 2.6 右转弯静止

在右转基础上，判断自车是否静止：速度小于0.01m/s，触发回传时间点静止

### 2.7 左转交互静止

在左转交互基础上，判断自车是否静止：速度小于0.01m/s

### 2.8 右转交互静止

在右转交互基础上，判断自车是否静止：速度小于0.01m/s

## 3. 静止场景 (Standstill)

触发条件：
- 自车速度小于0.1m/s
- 单次采集90s
- 触发间隔在10分钟

## 4. 交互场景 (Interaction)

### 4.1 它车横穿交互

触发条件：
- 满足交互条件
- 它车轨迹和自车轨迹最小距离小于4m
- 自车比它车后到交互点且不能后到太久
- 它车和自车bbx有交叠
- 在满足交互条件上,它车的heading和自车的heading在17-162度之间

### 4.2 它车急减速交互

触发条件：
- 同上交互条件
- 在满足交互条件上,它车存在急减速min_agent_hist_acc < -1

## 5. 偏离回正场景 (Deviation Correction)

触发条件：
- 不能处于转弯／曲率弯道／路口场景
- 自车前3/8轨迹有压线趋势, 自车后半段轨迹居中行驶

## 6. 大曲率弯道场景 (High Curvature Turn)

触发条件：
- 自车速度大于5m/s
- 自车左右都存在车道线，且自车未来轨迹在两条车道线之间，自车位于两条车道线内
- 车道线的c2,c3大于0.001
- 自车未来yaw大于0.02
- 自车不压线

## 7. 加减速场景 (Acceleration/Deceleration)

### 7.1 刹车

触发条件：
- 转向稳定(固定距离轨迹曲率判断)并且自车不压线后
- 自车加速度小于-0.15，且存在交互他车

### 7.2 刹停

触发条件：
- 转向稳定(固定距离轨迹曲率判断)并且自车不压线后
- 固定时间轨迹中，速度包含0.01m/s以下轨迹点

### 7.3 加速

触发条件：
- 转向稳定(固定距离轨迹曲率判断)并且自车不压线后
- 自车速度大于3/s，且最大加速度大于0.15

### 7.4 安全加速 (safe_acc)

触发条件：
- 无交互情况下
- 如果自车速度在3m/s ~ 8m/s范围内提速到10m/s以上

## 8. 车道保持场景 (Lane Keeping)

### 8.1 普通车道保持

触发条件：
- 自车速度大于5m/s
- 自车左右都存在车道线，且自车未来轨迹在两条车道线之间，自车位于两条车道线中间
- 自车不压线

### 8.2 过路口车道保持

触发条件：
- 自车速度大于5m/s
- 自车在路口附近
- 自车未来一段时间的轨迹，被夹在两段远处平行车道线之间

## 9. 路口场景 (Intersection)

### 9.1 接近路口

触发条件：
- 有红绿灯信号（表示在路口附近）
- 固定时间轨迹和任意水平车道线保持1m以上的距离
- 自车速度大于5m/s

## 10. 分合流场景 (Merge/Diverge)

### 10.1 分流

触发条件：
- 自车向分流车道变道
- 左侧分流：
  - 自车存在左车道线
  - 左车道线起点<分流车道线起点<左车道线终点
  - 分流车道线起点>0
  - 分流车道线起点距左车道线距离dis < dis_th
  - 分流车道线起点方向和左车道线终点方向夹角>theta_th
- 右侧分流：
  - 自车存在右车道线
  - 右车道线起点<分流车道线起点<右车道线终点
  - 分流车道线起点>0
  - 分流车道线起点距右车道线距离dis < dis_th
  - 分流车道线起点方向和右车道线终点方向夹角>theta_th
- 注意：当前场景判断逻辑不覆盖路口少变多类型的分流

### 10.2 合流

触发条件：
- 自车存在左右车道线
- 旁车道线起点<合流车道线终点<旁车道线终点
- 合流线终点>0
- 合流车道线终点距旁车道线距离dis<dis_th
- 合流车道线终点方向合旁车道线起点方向夹角>theta_th
- 注意：当前场景判断逻辑不覆盖路口多变少类型的分流

## 11. 环岛场景 (Roundabout)

触发条件：
- 自车靠近环岛