# ad_data_closed_loop

## architecture
```
|-- ad_data_closed_loop
    ├── CHANGELOG.md
    ├── CONTRIBUTING.md
    ├── LICENSE
    ├── README.md
    ├── docs
    │   ├── architecture.md
    │   ├── data_spec.md
    │   ├── deployment.md
    │   ├── modules
    │   │   ├── data_processor_spec.md
    │   │   └── navigation_planner.md
    │   ├── new_features.md
    │   └── new_features_cn.md
    ├── ops
    │   ├── deploy_edge.sh
    │   └── deploy_mdel.sh
    ├── release.conf.json
    ├── requirements.txt
    ├── src
    │   ├── CMakeLists.txt
    │   ├── Makefile
    │   ├── data_collection
    │   │   ├── 3rdparty
    │   │   ├── CMakeLists.txt
    │   │   ├── LICENSE
    │   │   ├── Makefile
    │   │   ├── README.md
    │   │   ├── ad_comm
    │   │   ├── ad_msgs
    │   │   ├── cmake
    │   │   ├── config
    │   │   ├── dag
    │   │   ├── docker
    │   │   ├── release.conf.json
    │   │   ├── resource
    │   │   └── src
    │   ├── data_collection_planner.cpp
    │   ├── data_collection_planner.h
    │   ├── main.cpp
    │   └── navigation_planner
    │       ├── config
    │       ├── costmap
    │       ├── nav_planner_node.cpp
    │       ├── nav_planner_node.h
    │       ├── rl
    │       ├── sampler
    │       ├── semantics
    │       └── utils
    ├── tests
    ├── tools
    │   └── bag_inspect.py
    └── training
        ├── ad_algorithm_train.py
        ├── benchmark
        ├── datasets
        ├── models
        ├── planner_rl_train.py
        ├── settings.py
        └── utils
```