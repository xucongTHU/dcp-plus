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
python tools/train_ppo_model.py -o models/ppo_weights.txt -e 1000
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