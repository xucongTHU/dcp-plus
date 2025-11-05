# Navigation Planner Module for Autonomous Driving

## costmap
This implementation provides the core functionality for a costmap that can be adjusted based on data density statistics, which is a key component of the data closed loop system described in the documentation. The key features include:

1. CostMap class that maintains a 2D grid of cells with cost and data density values
2. Parameter configuration for sparse threshold, exploration bonus, and redundancy penalty
3. Data statistics update method that processes data points and calculates density
4. Cost adjustment based on the data density to encourage exploration of sparse areas
5. Boundary checking to ensure safe access to cells
This implementation aligns with the documentation's description of how the system dynamically adjusts the costmap based on historical data collection patterns, encouraging exploration of sparse areas while avoiding redundant data collection in high-density areas.

## planner_rl
These implementations provide the core functionality for the reinforcement learning-based navigation planner:

1. **Reward System** (planner_reward.h/cpp):
- Implements the reward function as described in the documentation
- Provides positive rewards for visiting sparse areas (+10) and successful triggers (+0.5)
- Applies penalties for collisions (-1.0) and step costs (-0.01)
- Includes optional shaped reward based on distance to sparse cells
2. **Route Optimization** (planner_route_optimize.h/cpp):
- Implements the optRoute function that adjusts costs based on data density
- Decreases costs in sparse areas to encourage exploration
- Increases costs in high-density areas to discourage redundancy
- Includes a placeholder for A* path planning algorithm
The implementations align with the documentation's description of how the system encourages data collection in sparse regions while avoiding ineffective movements and collisions. The parameters used (sparse_threshold, exploration_bonus, redundancy_penalty) match those defined in the planner_weights.yaml configuration file.

## sampler
These implementations provide the core functionality for the sampler components:

1. **Coverage Metrics** (coverage_metric.h/cpp):
- Tracks and calculates various coverage metrics as described in the documentation:
- Overall grid coverage ratio
- Sparse sample coverage ratio
- Updates metrics based on visited cells and data density information
- Provides methods to query coverage statistics
2. **Sampling Optimizer** (sampling_optimizer.h/cpp):

- Optimizes the next sampling point based on multiple factors:
  - Exploration weight (encouraging visits to sparse areas)
  - Efficiency weight (preferring closer cells)
  - Redundancy penalty (discouraging visits to high-density areas)
  - Cost considerations from the costmap
- Finds the nearest sparse cell to guide exploration
- Calculates sampling scores to determine optimal next sampling points
These components work together to ensure effective data collection by maximizing coverage of sparse areas while minimizing redundant data collection in already well-sampled regions. They directly support the AB testing metrics mentioned in the documentation such as coverage ratios and route efficiency.

## semantics
These implementations provide a complete semantic processing framework for the navigation planner:

1. **Semantic Map** (semantic_map.h/cpp):
- Represents semantic objects in the environment with types, positions, and properties
- Provides methods to query objects by type or location
- Calculates semantic costs for path planning
2. **Semantic Constraints** (semantic_constraint.h/cpp):
- Checks paths and points for semantic constraint violations
- Handles traffic rules and data collection constraints
- Applies semantic penalties to costmaps
3. **Semantic Filter** (semantic_filter.h/cpp):
- Filters semantic objects by confidence thresholds
- Filters objects by type specifications
- Allows dynamic configuration of filtering criteria
These components work together to incorporate semantic understanding into the navigation planning process, ensuring the vehicle respects traffic rules, avoids obstacles, and prioritizes data collection in designated zones.

## utils
These utility implementations provide essential helper functions for the navigation planner:

1. **Geometric Utilities**:
- Distance calculations (Euclidean, Manhattan, point-to-segment)
- Coordinate transformations (world to grid and vice versa)
- Path operations (length calculation, resampling, smoothing)
- Angle normalization and interpolation
2. **File I/O Utilities**:
- Parameter loading and saving to/from YAML files
- File existence checks and directory creation
- Text file reading and writing
3. **Validation Utilities**:
- Path validity checking against costmaps
- Point comparison with tolerance
4. **Logging Utilities**:
- Multi-level logging system (debug, info, warn, error)
- Point formatting for logs
These utilities form the foundation for many operations in the navigation planner, providing reusable functions that simplify the implementation of more complex algorithms and ensure consistency across the system.

## navigation_planner
This implementation provides the main navigation planner node that integrates all the components of the system:

1. **Central Coordination**: The NavPlannerNode class serves as the central coordinator for all navigation planning components.

2. **Configuration Management**: Loads and manages parameters from the YAML configuration file, supporting dynamic reloading.

3. **Path Planning**: Implements both global and local path planning functions that utilize the route planner and costmap.

4. **Data Collection Optimization**: Uses the sampling optimizer to determine optimal waypoints for data collection missions.

5. **Validation and Metrics**: Validates paths against constraints and updates coverage metrics based on visited areas.

6. **Reward Computation**: Integrates with the reward calculator to evaluate state transitions.

7. **Dynamic Updates**: Supports reloading of configuration parameters and updating costmaps with new data statistics.

This main node ties together all the previously implemented components into a cohesive navigation planning system that supports the data closed-loop functionality described in the documentation. It handles the complete workflow from configuration loading, path planning, constraint checking, to coverage metric updates and reward computation.