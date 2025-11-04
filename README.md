# ad_data_closed_loop

## architecture
```
|-- ad_data_closed_loop
    |-- config
    |-- docker
    |-- docs
        |-- new_features_cn.md
        |-- new_featrures.md
    |-- infra
        |-- data_collector  //数据采集模块，使用ad_shadowmode<https://github.com/xucongTHU/ad_shadowmode.git>
        |-- navigation_planner  //数采路径规划模块
            |-- nav_planner_node.cpp
            |-- nav_planner_node.h
            |-- config
                |-- planner_weights.yaml
            |-- costmap //
                |-- costmap_layer.cpp
                |-- costmap_layer.h   
            |-- semantics //语义处理模块
                |-- semantic_filter.cpp
                |-- semantic_filter.h
                |-- semantic_constraint.cpp
                |-- semantic_constraint.h
                |-- semantic_map.cpp
                |-- semantic_map.h
            |-- utils //工具类
                |-- planner_utils.h
                |-- planner_utils.cpp            
            |-- rl //强化学习路径规划模块
                |-- planner_optRoute.cpp
                |-- planner_optRoute.h
                |-- planner_reward.cpp
                |-- planner_reward.h
                |-- planner_rl_agent.cpp
                |-- planner_rl_agent.h
        |-- data_processor  //数据处理模块
            |-- pointcloud_processor.py    //点云拼接\补偿\去畸变
            |-- pointcloud_camera_sync.py  //点云到相机时间同步
            |-- 
            |-- feature_alignment   //图像/点云对齐，拼接，补偿，去畸变
            |-- semantic_extrator   //语义信息提取
                |-- ground_extrator.py
                |-- extractor_utils.py
                |-- 
            |-- costmap_builder.py  //costmap生成，忽略
            |-- utils
            |-- visualizer
        
    |-- training
        |-- planner_rl_train.py
        |-- ad_algorithm_train.py
        |-- embodied_ai_algorithm_train.py
        |-- settings.py
        |-- utils
        |-- datasets
        |-- models
        |-- benchmark
    |-- ops //运维部署
        |-- ad_data_closed_loop_deploy.sh
        |-- ad_algorithm_model_deploy.sh
        |-- embodied_ai_algorithm_model_deploy.sh
    |-- tests
    |-- tools
    |-- requirements.txt
    |-- README.md
```