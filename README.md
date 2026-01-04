# DCP-Plus

**DCP-Plus: Learning Data Collection Policies for Self-Evolving Embodied Agents**

DCP-Plus is a research and system framework that formulates **data collection as a first-class planning problem** for embodied intelligence systems. Instead of passively recording data or relying on hand-crafted triggers, DCP-Plus learns *how data should be collected* to maximize downstream learning progress under system resource constraints.

---

## What is DCP?

**DCP** stands for **Data Collection as Planning**.

The key idea behind DCP is to treat data acquisition not as a static logging process, but as a **decision-making and planning problem**. At each timestep, an embodied agent decides *when*, *what*, and *how* to collect data based on environmental context, trajectory history, learning needs, and system resources.

**DCP-Plus** extends this idea by introducing learning-based decision-making and closed-loop feedback from downstream model training.

---

## Key Features

- Learning-based data collection using reinforcement learning (PPO)
- Data value estimation driven by downstream training gains
- End-to-end closed-loop learning between data generation and model improvement
- Resource-aware recording under disk and bandwidth constraints
- Edge–cloud architecture for scalable deployment
- ROS2-compatible design for embodied agents

---

## System Overview

DCP-Plus is implemented as a distributed system with a clear separation between **edge-side execution** and **cloud-side learning**:

- **Edge (robot / vehicle side)**  
  Executes the task policy and runs a lightweight data collection policy online.

- **Cloud (training side)**  
  Performs data value learning, policy optimization, and training evaluation.

The data collection policy operates independently of task execution, enabling seamless integration with existing navigation or manipulation stacks.

---

## Getting Started
Prerequisites

- Ubuntu 22.04
- ROS2 (Humble recommended)
- C++17
- Onnxruntime1.10.0
- Python 3.10+