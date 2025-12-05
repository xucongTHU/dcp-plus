# 模型输入输出规范

## 概述

本文档详细说明了强化学习路径规划系统中云端训练模型与车端推理之间的接口规范。模型使用ONNX格式导出，在车端使用ONNX Runtime进行推理。

## 输入规范 [batch, state_dim]

### 输入格式
输入状态为标准化后的特征向量，总长度为state_dim：

```
state = [
  norm_lat, norm_lon,            // 归一化坐标 (索引0-1)
  heatmap_summary(16 values),    // 局部热力图池化向量 (索引2-17)
  last_n_actions(4),             // 历史动作 one-hot 或 embeddings (索引18-21)
  remaining_budget_norm,         // 时间/距离 (索引22)
  local_traffic_density,         // 局部交通密度 (索引23)
  ...                            // 其他特征 (索引24+)
]
```

具体字段说明：

1. **归一化坐标**
   - 索引0: norm_lat (纬度，标准化到[0,1]范围)
   - 索引1: norm_lon (经度，标准化到[0,1]范围)

2. **热力图Summary (Heatmap Summary)**
   - 索引2-17: 16个值表示局部热力图池化向量

3. **历史动作**
   - 索引18-21: 最近4个动作的one-hot编码或嵌入向量

4. **剩余预算**
   - 索引22: 剩余时间/距离预算（标准化到[0,1]范围）

5. **局部交通密度**
   - 索引23: 局部交通密度标量值（标准化到[0,1]范围）

6. **其他特征**
   - 索引24+: 可能包括其他环境特征、车辆状态等

## 输出规范

### 策略输出 [batch, action_dim]
- **action_dim = 4** （四种基本移动动作）
- 索引0: 向右移动 (x+1, y)
- 索引1: 向上移动 (x, y+1)
- 索引2: 向左移动 (x-1, y)
- 索引3: 向下移动 (x, y-1)

**注意**: 模型应输出logits而非概率分布，车端负责执行softmax操作。

### 价值输出 [batch, 1]
- 表示当前状态的价值评估（标量值）

## ONNX模型要求

### 输入节点
- **名称**: "input"
- **形状**: [batch_size, state_dim]
- **数据类型**: FLOAT

### 输出节点
1. **策略输出**
   - **名称**: "output_policy"
   - **形状**: [batch_size, action_dim]
   - **数据类型**: FLOAT
   - **内容**: 未经处理的logits

2. **价值输出**
   - **名称**: "output_value"
   - **形状**: [batch_size, 1]
   - **数据类型**: FLOAT
   - **内容**: 状态价值估计

## 车端处理流程

1. **输入准备**
   - 收集当前状态信息
   - 标准化所有特征到[0,1]范围（或根据需要到[-1,1]范围）
   - 构造输入张量

2. **模型推理**
   - 使用ONNX Runtime执行前向推理
   - 获取logits输出和价值输出

3. **策略处理**
   - 对logits应用softmax转换为概率分布
   - 根据应用场景选择动作：
     - 确定性策略：选择概率最高的动作（argmax）
     - 随机策略：根据概率分布采样

4. **价值使用**
   - 直接使用价值输出进行状态评估

## 示例

### 输入示例（完整格式）
```python
# 完整状态输入示例 (state_dim = 24)
input = [
    0.75, 0.32,                           # norm_lat, norm_lon
    0.1, 0.2, 0.3, 0.4, 0.5, 0.6,        # heatmap_summary 前6个值
    0.7, 0.8, 0.9, 1.0, 0.15, 0.25,      # heatmap_summary 中间6个值
    0.35, 0.45, 0.55, 0.65,              # heatmap_summary 后4个值
    0, 1, 0, 0,                           # last_n_actions (one-hot)
    0.67,                                 # remaining_budget_norm
    0.42                                  # local_traffic_density
]
shape = [1, 24]
```

### 输出示例
```python
# 策略logits输出
output_policy = [
    [2.1, 0.5, -1.2, 0.8]  // 未处理的logits
]
shape = [1, 4]

// 价值输出
output_value = [
    [0.85]  // 状态价值估计
]
shape = [1, 1]
```

### 车端处理示例
```cpp
// 接收到logits输出
float* logits = output_policy_data;

// 应用softmax转换为概率分布
std::vector<double> probs(4);
double max_logit = *std::max_element(logits, logits + 4);
double sum_exp = 0.0;

for (int i = 0; i < 4; i++) {
    probs[i] = std::exp(static_cast<double>(logits[i]) - max_logit);
    sum_exp += probs[i];
}

for (int i = 0; i < 4; i++) {
    probs[i] /= sum_exp;
}

// 选择动作
int action = std::distance(probs.begin(), std::max_element(probs.begin(), probs.end()));
```

## 注意事项

1. 所有输入特征必须进行标准化处理
2. 模型输出logits而非概率，增强数值稳定性
3. 车端负责执行softmax操作，提供策略灵活性
4. 模型必须通过ONNX Runtime加载和执行
5. 实现模型加载失败时的降级机制（如回退到规则策略）
6. 支持动态调整state_dim以适应不同的特征组合