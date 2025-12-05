# 导航规划器实现详解

## 1. 整体架构

导航规划器模块是自动驾驶数据闭环系统的核心组件之一，负责路径规划和数据采集优化。它集成了多个子模块，形成了一个完整的路径规划和数据采集系统：

- NavPlannerNode：中央协调器，整合所有导航规划组件
- RoutePlanner：路径规划器，支持A*和PPO两种算法
- CostMap：成本地图，根据数据密度动态调整成本
- SamplingOptimizer：采样优化器，优化下一个数据采集点
- SemanticMap：语义地图，处理环境中的语义信息
- CoverageMetric：覆盖率度量，计算各种覆盖率指标

## 2. 核心组件

### 2.1 NavPlannerNode（导航规划节点）

NavPlannerNode 是整个导航规划系统的中枢，负责协调各个组件的工作：

- 初始化所有子模块（成本地图、路径规划器、采样优化器等）
- 加载和管理来自YAML配置文件的参数
- 实现全局和局部路径规划功能
- 验证路径约束
- 更新覆盖率度量
- 计算状态转移奖励
- 支持动态重载配置参数
- 支持PPO权重的加载和保存

### 2.2 RoutePlanner（路径规划器）

RoutePlanner 实现了两种路径规划算法：

- A*算法：经典的寻路算法，用于基础路径规划
- PPO算法：基于强化学习的路径规划算法

路径规划器还实现了基于数据密度统计的成本调整功能：

- 对稀疏区域降低成本（鼓励探索）
- 对高密度区域增加成本（避免冗余）

### 2.3 PPOAgent（PPO代理）

PPOAgent 是强化学习算法的具体实现：

- 实现了简化版的PPO算法
- 包含演员（Actor）和评论家（Critic）神经网络
- 支持动作选择和状态价值计算
- 支持权重的保存和加载

### 2.4 CostMap（成本地图）

CostMap 维护一个二维网格，每个单元格包含成本和数据密度值：

- 根据收集的数据点更新统计数据
- 根据数据密度调整成本
- 支持边界检查以确保安全访问单元格

### 2.5 RewardCalculator（奖励计算器）

RewardCalculator 实现了强化学习的奖励函数：

- 访问新稀疏区域给予+10的奖励
- 成功触发数据采集给予+0.5的奖励
- 发生碰撞给予-1.0的惩罚
- 每步给予-0.01的小惩罚以鼓励短路径
- 可选地基于到最近稀疏单元格的距离提供形状奖励

## 3. 工作流程

### Overview

This document describes the cloud-edge architecture for reinforcement learning-based path planning. The system consists of two main components:

1. **Cloud-side Training**: Training PPO models using collected data
2. **Edge-side Inference**: Deploying trained models for real-time path planning

### Architecture

```
┌─────────────────────┐                    ┌─────────────────────┐
│     Cloud Side      │    ONNX Model      │     Edge Side       │
│                     │    (Export)        │                     │
│  ┌──────────────┐   │    Format          │   ┌─────────────┐   │
│  │   Training   │───┼────────────────────┼──▶│  Inference  │   │
│  │  (Python)    │   │                    │   │ (C++/ONNX RT) │
│  └──────────────┘   │                    │   └─────────────┘   │
│          │          │                    │          │          │
│          ▼          │                    │          ▼          │
│  ┌──────────────┐   │    Data Logs       │   ┌─────────────┐   │
│  │ Data Storage │   │    (Telemetry)     │   │    PPO      │   │
│  └──────────────┘◀──┼────────────────────┼───┤  Agent      │   │
│                     │                    │   └─────────────┘   │
└─────────────────────┘                    └─────────────────────┘
```

### Cloud-Side Training

The cloud-side component is responsible for training the PPO models using data collected from edge devices:

1. Collect telemetry data from vehicles
2. Train PPO models using curriculum learning
3. Export trained models as ONNX files

#### Model Training Process

1. **Data Collection**: Vehicles collect path planning experiences during operation
2. **Data Upload**: Telemetry data is uploaded to the cloud
3. **Model Training**: PPO models are trained using the collected data
4. **Model Export**: Trained models are exported as ONNX files

#### Training Commands

```bash
cd training
python curriculum_train.py --config config/advanced_ppo_config.yaml
```

### Edge-Side Inference

The edge-side component performs real-time inference using the deployed models:

1. Load ONNX models using ONNX Runtime
2. Perform inference for state-action selection
3. Execute path planning decisions

#### Inference Process

1. **Model Loading**: Load ONNX model at startup
2. **State Observation**: Gather current state information
3. **Action Selection**: Use model to select optimal action
4. **Execution**: Execute the selected action

#### Integration with Other Modules

The PPO agent integrates with other modules in the navigation planner:

1. **RoutePlanner**: Uses PPO agent for reinforcement learning-based path planning
2. **CostMap**: Provides cost information for state representation
3. **Trigger Engine**: Determines when to record data based on exploration needs
4. **Ring Buffer**: Stores recent experiences for potential retraining

### Model Format

Models are exported in ONNX format for easy deployment:

1. **Format**: ONNX (.onnx files)
2. **Serialization**: Contains both network architecture and weights
3. **Portability**: Can be loaded directly in ONNX Runtime without Python dependencies

### Implementation Details

#### ONNX Runtime Integration

The PPO agent uses ONNX Runtime for inference:

1. **Model Loading**: Models are loaded using `Ort::Session`
2. **Inference**: Forward passes are performed using the loaded session
3. **Fallback**: Random action selection if model loading fails

#### State Representation

States are represented as feature vectors with normalized values according to the following specification:

```
state = [
  norm_lat, norm_lon,            // 归一化坐标
  heatmap_summary(16 values),    // 局部热力图池化向量
  last_n_actions(4),             // 历史动作 one-hot 或 embeddings
  remaining_budget_norm,         // 时间/距离
  local_traffic_density,         // scalar
  ...                            // 总长度 = state_dim
]
```

Where:
- norm_lat, norm_lon: Normalized latitude and longitude coordinates (range [0, 1])
- heatmap_summary: 16-value vector representing pooled local heatmap features
- last_n_actions: One-hot encoding or embeddings of the last 4 actions
- remaining_budget_norm: Normalized remaining time/distance budget (range [0, 1])
- local_traffic_density: Local traffic density scalar value (range [0, 1])

Minimum state_dim is 24 (2 + 16 + 4 + 1 + 1).

#### Action Space

The action space consists of 4 discrete movements:
1. Right (x+1, y)
2. Up (x, y+1)
3. Left (x-1, y)
4. Down (x, y-1)

### Expected ONNX Model Format

The ONNX model should have:
- **Input**: Named "input" with shape [batch_size, state_dim] 
  - state_dim >= 24 for full specification compliance
- **Outputs**: 
  - "output_policy" with shape [batch_size, action_dim] containing logits (action_dim=4)
  - "output_value" with shape [batch_size, 1] (state value)

### Deployment Process

1. **Train Model**: Train PPO model in the cloud
2. **Export Model**: Export as ONNX using `torch.onnx.export()` or similar
3. **Deploy Model**: Transfer to edge device
4. **Load Model**: Load using ONNX Runtime at startup
5. **Run Inference**: Perform real-time inference

### Benefits of Cloud-Edge Architecture

1. **Scalable Training**: Leverage cloud computing resources for training
2. **Low Latency**: Real-time inference on edge devices
3. **Bandwidth Efficient**: Only transfer model weights, not raw data
4. **Privacy Preserving**: Keep raw data on edge devices
5. **Continuous Improvement**: Regular model updates from cloud