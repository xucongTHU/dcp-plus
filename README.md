# ad_data_closed_loop

## architecture
```
|-- ad_data_closed_loop
    ├── 3rdparty
    │   ├── Eigen
    │   ├── ThreadPool
    │   ├── curl
    │   ├── exprtk
    │   ├── manif
    │   ├── microtar
    │   ├── nlohmann
    │   ├── openssl
    │   ├── paho.mqtt.cpp
    │   ├── release.conf.json
    │   └── tl
    ├── CHANGELOG.md
    ├── CONTRIBUTING.md
    ├── LICENSE
    ├── README.md
    ├── cmake
    │   ├── cross_toolchains
    │   └── senseauto-tools.cmake
    ├── config
    │   ├── ai_shadow_trigger_conf.yaml
    │   ├── app_config.json
    │   ├── default_strategy_config.json
    │   ├── generic_trigger_example.yaml
    │   ├── log_config.json
    │   ├── optimized_trigger_example.yaml
    │   └── universal_trigger_example.yaml
    ├── docker
    │   ├── DOCKER_README.md
    │   ├── Dockerfile
    │   └── docker-compose.yml
    ├── docs
    │   ├── STATE_MACHINE_README.md
    │   ├── architecture_docs
    │   ├── deployment.md
    │   ├── images
    │   ├── modules
    │   ├── new_features_cn.md
    │   └── state_machine_usage.md
    ├── ops
    │   ├── deploy_edge.sh
    │   └── deploy_mdel.sh
    ├── release.conf.json
    ├── requirements.txt
    ├── resource
    │   ├── pki
    │   ├── scripts
    │   └── tar_extra
    ├── src
    │   ├── CMakeLists.txt
    │   ├── channel
    │   ├── codec
    │   ├── common
    │   ├── data_collection_planner.cpp
    │   ├── data_collection_planner.h
    │   ├── main.cpp
    │   ├── navigation_planner
    │   ├── recorder
    │   ├── state_machine
    │   ├── trigger_engine
    │   └── uploader
    ├── tests
    ├── tools
    │   ├── bag_inspect.py
    │   └── generate_changelog.sh
    └── training
        ├── ad_algorithm_train.py
        ├── benchmark
        ├── datasets
        ├── models
        ├── planner_rl_train.py
        ├── settings.py
        └── utils
```