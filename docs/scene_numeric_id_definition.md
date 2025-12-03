# 场景数字ID定义规范

本文档说明如何为自动驾驶场景定义纯数字ID，以便在触发器配置中使用。ID的定义基于触发事件的优先级、紧急程度和数据价值性。

## 数字ID编码规则

数字ID采用6位编码规则：PPTTTT

- PP (前2位): 优先级分类
  - 10-19: 紧急高价值场景（如紧急制动、严重事故等）
  - 20-29: 高优先级场景（如变道、转弯等主动驾驶行为）
  - 30-39: 中优先级场景（如车道保持、一般交互等）
  - 40-49: 低优先级场景（如静止、普通巡航等）
  
- TTTT (后4位): 场景类型编码

## 优先级分类详解

### 紧急高价值场景 (10-19)
这类场景通常涉及安全风险或极端情况，需要立即响应并具有极高的数据价值：

| 场景名称         | ID      | 说明                           |
|------------------|---------|--------------------------------|
| 紧急制动         | 100001  | Emergency braking              |
| 碰撞预警         | 100002  | Forward collision warning      |
| 急转弯           | 100003  | Sharp turn                     |
| 急变道           | 100004  | Emergency lane change          |
| 严重交互         | 100005  | Severe vehicle interaction     |

### 高优先级场景 (20-29)
这类场景代表常见的主动驾驶行为，对提升驾驶体验和安全性至关重要：

| 场景名称         | ID      | 说明                           |
|------------------|---------|--------------------------------|
| 变道             | 200000  | General lane change            |
| 左绕障变道       | 200001  | Left nudge                     |
| 右绕障变道       | 200002  | Right nudge                    |
| 左变道           | 200011  | Left lane change               |
| 右变道           | 200012  | Right lane change              |
| 左效率变道       | 200021  | Left efficiency lane change    |
| 右效率变道       | 200022  | Right efficiency lane change   |
| 转弯             | 210000  | General turn                   |
| 无交互左转       | 210001  | Non-interactive left turn      |
| 无交互右转       | 210002  | Non-interactive right turn     |
| 有交互左转       | 210011  | Interactive left turn          |
| 有交互右转       | 210012  | Interactive right turn         |
| 左转弯静止       | 210021  | Left turn standstill           |
| 右转弯静止       | 210022  | Right turn standstill          |
| 左转交互静止     | 210031  | Left turn interactive SS       |
| 右转交互静止     | 210032  | Right turn interactive SS      |
| 刹车             | 220001  | Braking                        |
| 加速             | 220002  | Acceleration                   |
| 安全加速         | 220003  | Safe acceleration              |

### 中优先级场景 (30-39)
这类场景常见但在正常驾驶中，对提升算法鲁棒性有一定价值：

| 场景名称         | ID      | 说明                           |
|------------------|---------|--------------------------------|
| 车道保持         | 300000  | General lane keeping           |
| 普通车道保持     | 300001  | Normal lane keeping            |
| 过路口车道保持   | 300002  | Intersection lane keeping      |
| 大曲率弯道       | 310000  | High curvature turn            |
| 大曲率弯道车道保持| 310001  | High curvature lane keeping    |
| 一般交互         | 320000  | General interaction            |
| 它车横穿交互     | 320001  | Other vehicle crossing         |
| 它车减速交互     | 320002  | Other vehicle deceleration     |
| 它车急减速交互   | 320003  | Other vehicle emergency brake  |
| 偏离回正         | 330001  | Deviation correction           |
| 分合流           | 340000  | Merge/diverge                  |
| 分流             | 340001  | Diverging                      |
| 合流             | 340002  | Merging                        |
| 环岛             | 350001  | Roundabout                     |
| 接近路口         | 360001  | Approaching intersection       |

### 低优先级场景 (40-49)
这类场景发生频繁但相对简单，主要用于数据补充和边界情况覆盖：

| 场景名称         | ID      | 说明                           |
|------------------|---------|--------------------------------|
| 静止             | 400001  | Standstill                     |
| 刹停             | 410001  | Full stop                      |

## ID编码优势

1. **层次清晰**：通过前两位数字可以快速识别场景的优先级
2. **便于排序**：数字ID天然有序，便于数据库查询和排序
3. **易于扩展**：每个优先级分类下有充足的ID空间用于扩展
4. **价值导向**：ID直接反映场景的重要性和数据价值
5. **紧急程度体现**：数值越小代表越紧急和重要

## 在配置文件中使用数字ID

在YAML配置文件中，triggerId字段使用数字ID：

```yaml
- businessType: "LaneChange"
  trigger:
    triggerId: 200001  # 左绕障变道，高优先级场景
    priority: 4
    enabled: true
    triggerCondition: "..."
    triggerDesc: "..."
```