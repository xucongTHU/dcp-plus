# CHANGELOG

#  (2025-12-03)


### Features

* **architecture:** add submodule and update project structure ([1fcdc5b](https://github.com/xucongTHU/ad_data_closed_loop/commit/1fcdc5b196d0f29e2d2fc882295e80444cd2c7b9))
* **data-processor:** remove costmap_builder and feature_alignment modules ([b831e7a](https://github.com/xucongTHU/ad_data_closed_loop/commit/b831e7a5a0e0ce4b6a9cfd64801849120ddd68b7))
* **logger:** migrate logging macros to AD_* format ([25e2577](https://github.com/xucongTHU/ad_data_closed_loop/commit/25e257759fdf8a468e324d3b53d65a509d3985db))
* **navigation-planner:** add PPO-based path planning support ([b26ba05](https://github.com/xucongTHU/ad_data_closed_loop/commit/b26ba053de393ecf279dd9473015648d77be516b))
* **planner:** implement navigation planning core, RL route optimization, and planning utils ([5ff2fcb](https://github.com/xucongTHU/ad_data_closed_loop/commit/5ff2fcb82d8bf499bb3c26bcb35a1ed6d7f99f51))
* **state-machine:** integrate state machine for data collection and navigation coordination ([c09fb7a](https://github.com/xucongTHU/ad_data_closed_loop/commit/c09fb7a89f962769ae068ffd8335eda7fdcdd90a))
* **training:** implement advanced PPO training with configurable network and environment ([864865d](https://github.com/xucongTHU/ad_data_closed_loop/commit/864865d2052ee08edd0ec5cbad52d27e9910d43c))
* **utils:** add microsecond timestamp utility function ([86f1030](https://github.com/xucongTHU/ad_data_closed_loop/commit/86f1030199780ce85d6ab834e18e3d9f96aac3ea))





#  (2025-12-03)


### Features

* **architecture:** add submodule and update project structure ([1fcdc5b](https://github.com/xucongTHU/ad_data_closed_loop/commit/1fcdc5b196d0f29e2d2fc882295e80444cd2c7b9))
* **data-processor:** remove costmap_builder and feature_alignment modules ([b831e7a](https://github.com/xucongTHU/ad_data_closed_loop/commit/b831e7a5a0e0ce4b6a9cfd64801849120ddd68b7))
* **logger:** migrate logging macros to AD_* format ([25e2577](https://github.com/xucongTHU/ad_data_closed_loop/commit/25e257759fdf8a468e324d3b53d65a509d3985db))
* **navigation-planner:** add PPO-based path planning support ([b26ba05](https://github.com/xucongTHU/ad_data_closed_loop/commit/b26ba053de393ecf279dd9473015648d77be516b))
* **planner:** implement navigation planning core, RL route optimization, and planning utils ([5ff2fcb](https://github.com/xucongTHU/ad_data_closed_loop/commit/5ff2fcb82d8bf499bb3c26bcb35a1ed6d7f99f51))
* **state-machine:** integrate state machine for data collection and navigation coordination ([c09fb7a](https://github.com/xucongTHU/ad_data_closed_loop/commit/c09fb7a89f962769ae068ffd8335eda7fdcdd90a))
* **training:** implement advanced PPO training with configurable network and environment ([864865d](https://github.com/xucongTHU/ad_data_closed_loop/commit/864865d2052ee08edd0ec5cbad52d27e9910d43c))
* **utils:** add microsecond timestamp utility function ([86f1030](https://github.com/xucongTHU/ad_data_closed_loop/commit/86f1030199780ce85d6ab834e18e3d9f96aac3ea))





#  (2025-12-03)


### Features

* **architecture:** add submodule and update project structure ([1fcdc5b](https://github.com/xucongTHU/ad_data_closed_loop/commit/1fcdc5b196d0f29e2d2fc882295e80444cd2c7b9))
* **data-processor:** remove costmap_builder and feature_alignment modules ([b831e7a](https://github.com/xucongTHU/ad_data_closed_loop/commit/b831e7a5a0e0ce4b6a9cfd64801849120ddd68b7))
* **logger:** migrate logging macros to AD_* format ([25e2577](https://github.com/xucongTHU/ad_data_closed_loop/commit/25e257759fdf8a468e324d3b53d65a509d3985db))
* **navigation-planner:** add PPO-based path planning support ([b26ba05](https://github.com/xucongTHU/ad_data_closed_loop/commit/b26ba053de393ecf279dd9473015648d77be516b))
* **planner:** implement navigation planning core, RL route optimization, and planning utils ([5ff2fcb](https://github.com/xucongTHU/ad_data_closed_loop/commit/5ff2fcb82d8bf499bb3c26bcb35a1ed6d7f99f51))
* **state-machine:** integrate state machine for data collection and navigation coordination ([c09fb7a](https://github.com/xucongTHU/ad_data_closed_loop/commit/c09fb7a89f962769ae068ffd8335eda7fdcdd90a))
* **training:** implement advanced PPO training with configurable network and environment ([864865d](https://github.com/xucongTHU/ad_data_closed_loop/commit/864865d2052ee08edd0ec5cbad52d27e9910d43c))
* **utils:** add microsecond timestamp utility function ([86f1030](https://github.com/xucongTHU/ad_data_closed_loop/commit/86f1030199780ce85d6ab834e18e3d9f96aac3ea))





## (2025-12-02)

### Refactor

#### refactor(trigger): restructure trigger engine components
- Improve `TriggerManager` with thread-safe accessors and scheduler integration
- Migrate variable getters in `TriggerManager` to use `MessageProvider`

---

## (2025-12-01)

### Features

#### feat(logger): migrate logging macros to AD_* format
- Replace `LOG_*` macros with `AD_*` macros across multiple modules
- Update logger calls in `ChannelManager`, `MessageProvider`, `DataStorage`,
  `RsclRecorder`, and `StateMachine` components
- Change log levels and formatting for consistency

#### feat(utils): add microsecond timestamp utility function
- Implement `GetCurrentTimestampUs()` in `utils.cpp`
- Add function declaration in `utils.h`
- Provide helper for microsecond precision time measurement

#### feat(state-machine): integrate state machine for data collection and navigation coordination
- Added `state_machine` module with 8 states and 12 transition events
- Integrated `DataCollectionPlanner`, `NavPlannerNode`, and `DataStorage` components
- Updated `main.cpp` to use state machine driven workflow
- Enhanced logging with `AD_INFO/AD_ERROR` macros
- Extended `CMakeLists.txt` to include new source files
- Improved data collection execution with real module integration
- Added upload functionality and coverage reporting
- Refactored namespaces and include paths for better modularity

---

## (2025-11-07)

### Chore

#### chore(vscode): update VS Code settings for better C++ development experience
- Added file associations for C++ standard library headers
- Configured C++ IntelliSense, formatting, and inlay hints settings
- Updated `cmake.sourceDirectory` path for correct project root detection

---

## (2025-11-05)

### Docs / Architecture

#### docs(architecture): update project structure and documentation files

#### feat(data-processor): remove costmap_builder and feature_alignment modules
- Removed `costmap_builder.py` and `feature_alignment.py` modules
- Deprecated:
  - Costmap generation and data density analysis
  - Cross-modality feature alignment and ICP-based registration
  - Visualization and configuration utilities tied to these modules
- Simplifies data processing pipeline, removing dependencies on
  `scipy`, `matplotlib`, and `opencv-python`

#### refactor(submodule): relocate data_collection from infra → src
- Moved `infra/data_collection` to `src/data_collection` for clearer project hierarchy
- Aligns submodule structure with standard source tree conventions
- Updated CMake and dependency paths accordingly

---

## (2025-11-04)

### Features

#### feat(planner): implement navigation planning core, RL route optimization, and planning utils
- Add `NavPlannerNode`: costmap integration, route planner, sampling optimizer,
  semantic map & constraint checking, dynamic config reload, data-point collection
  and statistics
- Implement planner weights loading from YAML and document planner configuration parameters
- Add path planning & validation features including coverage metrics,
  path resampling, smoothing, length calculation, pose interpolation and angle normalization
- Integrate RL support: `RoutePlanner` (A* placeholder), sampling-based cost adjustments
  using data-density stats, `RewardCalculator` with shaped rewards, exploration bonus,
  and redundancy penalty
- Provide comprehensive planning utilities: coordinate transforms,
  distance functions, YAML param load/save, file I/O, and configurable logging
- Chore: update docs and README — fix `ad_shadowmode` URL, document new costmap/semantics/RL
  file structure, and update data_processor docs to include new components

---

## (2025-10-23)

### Docs / Architecture

#### docs: add Synaptix AI Proprietary License

#### feat(architecture): add submodule and update project structure
- Add `infra/data_collection` submodule from external repository
- Add navigation planner and data processing modules structure
- Add config, docker, docs, infra, training, ops, tests, tools component directories
- Update `README.md` with detailed project architecture diagram

---

## (2025-10-21)

### Docs

#### docs: add ad data_closed_loop new features documents

---

## (2025-10-17)

### Initial Commit

---
