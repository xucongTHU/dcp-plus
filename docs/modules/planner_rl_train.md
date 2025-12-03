# 路径规划强化学习训练系统

该目录包含导航规划模块的强化学习训练和评估系统。它实现了一个近端策略优化(PPO)算法来训练路径规划策略。

## 目录结构

```
training/
├── config/                 # 配置文件
├── models/                 # 训练好的模型权重
├── benchmark/              # 基准测试和对比工具
├── utils/                  # 工具函数
├── results/                # 评估结果
├── environment.py          # 环境实现
├── curriculum_train.py     # 课程学习训练脚本
├── planner_rl_train.py     # 标准训练脚本
├── planner_rl_eval.py      # 标准评估脚本
├── enhanced_eval.py        # 增强评估（详细指标）
├── final_evaluation.py     # 特定任务评估
├── settings.py             # 配置设置
└── README.md               # 本文档
```

## 快速开始

### 先决条件

确保已安装所需的依赖项：

```bash
pip install torch numpy matplotlib pyyaml
```

### 训练模型

使用课程学习训练新的PPO模型（推荐）：

```bash
python curriculum_train.py --config config/advanced_ppo_config.yaml
```

标准训练：
```bash
python planner_rl_train.py --config config/advanced_ppo_config.yaml
```

附加选项：
- `--stages N`：课程学习阶段数
- `--episodes N`：覆盖训练episode数

### 评估模型

在特定任务上评估训练好的模型：

```bash
python final_evaluation.py --model-path models/curriculum_final_weights.pth --config config/advanced_ppo_config.yaml
```

标准评估：

```bash
python planner_rl_eval.py --model-path models/curriculum_final_weights.pth --config config/advanced_ppo_config.yaml
```

带有详细指标的增强评估：

```bash
python enhanced_eval.py --model-path models/curriculum_final_weights.pth --config config/advanced_ppo_config.yaml
```

选项：
- `--episodes N`：评估episode数
- `--output FILE`：将结果保存到JSON文件

### 运行基准测试

对模型进行基准测试和比较：

```bash
python benchmark/benchmark_planner.py --model-path models/ppo_weights.pth --config config/ppo_train_config.yaml
```

## 配置

训练配置由[config/](file:///workspaces/ad_data_closed_loop/training/config/)目录中的YAML文件控制。推荐配置是[advanced_ppo_config.yaml](file:///workspaces/ad_data_closed_loop/training/config/advanced_ppo_config.yaml)。

关键参数包括：
- `state_dim`：状态空间维度（位置x,y为2）
- `action_dim`：可能的动作数（4方向移动为4）
- `hidden_dim`：神经网络中隐藏层的大小
- `learning_rate`：优化器的学习率
- `gamma`：未来奖励的折扣因子
- `lam`：广义优势估计的Lambda参数
- `epsilon`：PPO裁剪参数
- `epochs`：每次迭代的更新轮数
- `batch_size`：更新的小批量大小
- `episodes`：总训练episode数
- `max_steps`：每个episode的最大步数
- `entropy_coef`：熵奖励系数（探索）

## 训练方法

我们使用课程学习来有效训练模型：
1. **阶段1**：简单的2x2环境，目标在附近
2. **阶段2**：5x5环境，目标距离中等
3. **阶段3**：10x10环境，目标较远
4. **阶段4**：15x15环境，目标距离远
5. **阶段5**：完整的20x20环境，随机位置

这种渐进式方法帮助模型在处理复杂任务之前学习基本的导航技能。

## 环境类型

提供两种环境类型：

1. **简单环境**：无障碍物的网格世界
2. **复杂环境**：随机放置障碍物的网格世界

## 模型架构

PPO实现使用Actor-Critic架构：
- 用于特征提取的共享ReLU隐藏层
- 策略（actor）和价值（critic）估计的独立头
- 用于动作概率分布的Softmax激活

## 结果

评估结果以JSON格式保存，包含：
- 每个案例的指标（奖励、步数、成功、路径）
- 汇总统计（成功率、总案例数）

模型保存为PyTorch状态字典：
- `curriculum_final_weights.pth`：最终训练的模型
- `curriculum_stage_N_weights.pth`：每个课程学习阶段的中间模型