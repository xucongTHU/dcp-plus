# Path Planning Reinforcement Learning Training System

This directory contains the reinforcement learning training and evaluation system for the navigation planner module. It implements a Proximal Policy Optimization (PPO) algorithm for training path planning policies.

## Directory Structure

```
training/
├── config/                 # Configuration files
├── models/                 # Trained model weights
├── benchmark/              # Benchmark and comparison tools
├── utils/                  # Utility functions
├── results/                # Evaluation results
├── environment.py          # Environment implementations
├── curriculum_train.py     # Curriculum learning training script
├── planner_rl_train.py     # Standard training script
├── planner_rl_eval.py      # Standard evaluation script
├── enhanced_eval.py        # Enhanced evaluation with detailed metrics
├── final_evaluation.py     # Evaluation on specific tasks
├── settings.py             # Configuration settings
└── README.md               # This file
```

## Getting Started

### Prerequisites

Make sure you have installed the required dependencies:

```bash
pip install torch numpy matplotlib pyyaml
```

### Training a Model

To train a new PPO model with curriculum learning (recommended):

```bash
python curriculum_train.py --config config/advanced_ppo_config.yaml
```

For standard training:
```bash
python planner_rl_train.py --config config/advanced_ppo_config.yaml
```

Additional options:
- `--stages N`: Number of curriculum stages (for curriculum learning)
- `--episodes N`: Override the number of training episodes

### Evaluating a Model

To evaluate a trained model on specific tasks:

```bash
python final_evaluation.py --model-path models/curriculum_final_weights.pth --config config/advanced_ppo_config.yaml
```

For standard evaluation:

```bash
python planner_rl_eval.py --model-path models/curriculum_final_weights.pth --config config/advanced_ppo_config.yaml
```

Enhanced evaluation with detailed metrics:

```bash
python enhanced_eval.py --model-path models/curriculum_final_weights.pth --config config/advanced_ppo_config.yaml
```

Options:
- `--episodes N`: Number of evaluation episodes
- `--output FILE`: Save results to a JSON file

### Running Benchmarks

To benchmark and compare models:

```bash
python benchmark/benchmark_planner.py --model-path models/ppo_weights.pth --config config/ppo_train_config.yaml
```

## Configuration

The training configuration is controlled by YAML files in the [config/](file:///workspaces/ad_data_closed_loop/training/config/) directory. The recommended configuration is [advanced_ppo_config.yaml](file:///workspaces/ad_data_closed_loop/training/config/advanced_ppo_config.yaml).

Key parameters include:
- `state_dim`: Dimension of the state space (2 for x,y position)
- `action_dim`: Number of possible actions (4 for 4-directional movement)
- `hidden_dim`: Size of hidden layers in the neural network
- `learning_rate`: Learning rate for the optimizer
- `gamma`: Discount factor for future rewards
- `lam`: Lambda parameter for Generalized Advantage Estimation
- `epsilon`: PPO clipping parameter
- `epochs`: Number of update epochs per iteration
- `batch_size`: Mini-batch size for updates
- `episodes`: Total number of training episodes
- `max_steps`: Maximum steps per episode
- `entropy_coef`: Coefficient for entropy bonus (exploration)

## Training Approach

We use curriculum learning to train the model effectively:
1. **Stage 1**: Simple 2x2 environment with nearby goal
2. **Stage 2**: 5x5 environment with medium-distance goal
3. **Stage 3**: 10x10 environment with farther goal
4. **Stage 4**: 15x15 environment with distant goal
5. **Stage 5**: Full 20x20 environment with random positions

This progressive approach helps the model learn basic navigation skills before tackling complex tasks.

## Environment Types

Two environment types are available:

1. **Simple Environment**: Grid world without obstacles
2. **Complex Environment**: Grid world with randomly placed obstacles

## Model Architecture

The PPO implementation uses an Actor-Critic architecture with:
- Shared ReLU hidden layers for feature extraction
- Separate heads for policy (actor) and value (critic) estimation
- Softmax activation for action probability distribution

## Results

Evaluation results are saved in JSON format containing:
- Per-case metrics (reward, steps, success, path)
- Summary statistics (success rate, total cases)

Models are saved as PyTorch state dictionaries:
- `curriculum_final_weights.pth`: Final trained model
- `curriculum_stage_N_weights.pth`: Intermediate models from each curriculum stage