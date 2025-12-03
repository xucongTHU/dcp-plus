# Autonomous Driving Data Closed Loop System

## Overview

The AD Data Closed Loop system is designed for autonomous driving data collection, processing, and model iteration. It enables automated workflows for data gathering, uploading, training, and algorithm updates through a closed-loop process that continuously improves autonomous driving models.

## Features

- **Data Collection Control**: Event-triggered or periodic data recording and storage
- **Edge-side Data Processing**: On-vehicle data filtering, encryption, and compression
- **Navigation Planning**: Reinforcement learning-based path planning capabilities
- **Data Upload Management**: Supports MQTT/HTTP protocols for cloud uploading
- **Offline Training Support**: Includes RL training scripts for model iteration
- **State Machine Management**: Controls data collection lifecycle state transitions

## Key Components

### Navigation Planner
The navigation planner module provides intelligent path planning capabilities using either traditional A* algorithm or advanced PPO (Proximal Policy Optimization) reinforcement learning algorithm.

#### A* Algorithm
A* is a classic pathfinding algorithm that works well for finding the shortest path in a known environment with static obstacles. It's reliable and computationally efficient, making it suitable for real-time applications.

#### PPO Algorithm
PPO (Proximal Policy Optimization) is a state-of-the-art reinforcement learning algorithm that learns optimal policies through interaction with the environment. It can adapt to dynamic conditions and optimize for long-term rewards rather than just shortest paths.

The PPO implementation includes:
- Actor-Critic architecture with configurable network depth
- Generalized Advantage Estimation (GAE) for better policy updates
- Curriculum learning approach for progressive training
- Configurable hyperparameters for fine-tuning

### Data Collection Planner
Coordinates data collection missions, integrating with the navigation planner to optimize paths for data coverage and exploration of sparse areas.

### State Machine
Manages the overall system state, transitioning between idle, planning, executing, and uploading states.

## Getting Started

### Prerequisites

- GCC/G++ >= 11.4
- CMake >= 3.20
- Make
- Python 3.10+

### Building

```bash
cd /workspaces/ad_data_closed_loop
make tgz_thor
```

Or using CMake:

```bash
mkdir build && cd build && cmake .. && make
```

## Usage

### Running with A* Algorithm (Default)

```bash
./ad_data_closed_loop
```

### Running with PPO Algorithm

First, train a PPO model (typically done in the cloud):

```bash
cd training
python curriculum_train.py --config config/advanced_ppo_config.yaml
```

Then run the system with the PPO algorithm:

```bash
./ad_data_closed_loop --algorithm ppo --weights models/ppo_weights.txt
```

### Command Line Options

- `-h, --help`: Show help message
- `-a, --algorithm ALG`: Planning algorithm (astar or ppo)
- `-w, --weights PATH`: Path to PPO weights file
- `-c, --config PATH`: Path to planner configuration file

## System Architecture

```
+---------------------+
|   Application Layer |
| - State Machine     |
| - Main Entry        |
+----------+----------+
           |
+----------v----------+
|   Module Layer      |
| - Data Collection   |
| - Navigation Planner|
+----------+----------+
           |
+----------v----------+
|   Communication     |
| - ad_comm (ROS/Apollo)|
+----------+----------+
           |
+----------v----------+
|   Data & Storage    |
| - Bag files, Upload |
+---------------------+
```

## Training Process

1. **Data Collection**: Vehicle collects data during operation using A* or PPO algorithms
2. **Data Upload**: Collected data is uploaded to the cloud for processing
3. **Model Training**: PPO model is trained in the cloud using the collected data
4. **Model Deployment**: Trained model weights are deployed to vehicles
5. **Inference**: Vehicles use the trained PPO model for improved path planning

This creates a closed-loop system where real-world experience continuously improves the planning algorithm.

### Reinforcement Learning Implementation Details

The reinforcement learning component is implemented in the [training/](file:///workspaces/ad_data_closed_loop/training/) directory with the following features:

#### Environment
- Grid-world environment with customizable size (default 20x20)
- Two environment types: simple (no obstacles) and complex (with obstacles)
- 4-directional movement actions (up, right, down, left)
- Reward function based on distance to goal with time penalties

#### Training Approach
- **Curriculum Learning**: Progressive training starting from simple tasks to complex ones:
  1. Stage 1: 2x2 environment with nearby goal
  2. Stage 2: 5x5 environment with medium-distance goal
  3. Stage 3: 10x10 environment with farther goal
  4. Stage 4: 15x15 environment with distant goal
  5. Stage 5: Full 20x20 environment with random positions
  
- **PPO Algorithm**: Uses Proximal Policy Optimization with:
  - Actor-Critic architecture
  - Generalized Advantage Estimation (GAE)
  - Configurable network layers and hyperparameters
  - Entropy bonus for exploration

#### Model Architecture
- Configurable shared ReLU hidden layers for feature extraction
- Separate heads for policy (actor) and value (critic) estimation
- Softmax activation for action probability distribution
- Dropout layers for regularization

#### Configuration
Training is controlled by YAML configuration files in the [training/config/](file:///workspaces/ad_data_closed_loop/training/config/) directory:
- [advanced_ppo_config.yaml](file:///workspaces/ad_data_closed_loop/training/config/advanced_ppo_config.yaml): Recommended configuration with tuned hyperparameters
- [example_ppo_config.yaml](file:///workspaces/ad_data_closed_loop/training/config/example_ppo_config.yaml): Basic example configuration
- [optimized_ppo_config.yaml](file:///workspaces/ad_data_closed_loop/training/config/optimized_ppo_config.yaml): Configuration with optimized hyperparameters

Key hyperparameters include:
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

### Training Scripts
Several scripts are available for training and evaluation:
- [curriculum_train.py](file:///workspaces/ad_data_closed_loop/training/curriculum_train.py): Main training script using curriculum learning (recommended)
- [planner_rl_train.py](file:///workspaces/ad_data_closed_loop/training/planner_rl_train.py): Standard PPO training script
- [planner_rl_eval.py](file:///workspaces/ad_data_closed_loop/training/planner_rl_eval.py): Basic model evaluation
- [enhanced_eval.py](file:///workspaces/ad_data_closed_loop/training/enhanced_eval.py): Detailed model evaluation with metrics
- [final_evaluation.py](file:///workspaces/ad_data_closed_loop/training/final_evaluation.py): Evaluation on specific tasks to demonstrate learning
- [benchmark/benchmark_planner.py](file:///workspaces/ad_data_closed_loop/training/benchmark/benchmark_planner.py): Performance benchmarking

### Results
Training results are saved in the [training/models/](file:///workspaces/ad_data_closed_loop/training/models/) directory:
- `curriculum_final_weights.pth`: Final trained model
- `curriculum_stage_N_weights.pth`: Intermediate models from each curriculum stage

Evaluation results are saved in JSON format in the [training/results/](file:///workspaces/ad_data_closed_loop/training/results/) directory.

## Configuration

Main configuration files:
- `config/planner_weights.yaml`: Navigation planner weights
- `config/default_strategy_config.json`: Data collection strategy
- `config/app_config.json`: Application configuration

## Directory Structure

- `src/`: Source code
- `config/`: Configuration files
- `models/`: Trained model weights
- `tools/`: Utility scripts
- `training/`: Training scripts and utilities
- `3rdparty/`: Third-party libraries

## Contributing

Please read CONTRIBUTING.md for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the MIT License - see the LICENSE file for details.