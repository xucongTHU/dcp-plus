# AD_EdgeInsight

## overview

```angular2svg
+-------------------+              +---------------------+            +--------------------------+
|   车端/机器人         | <---MQTT-- |   云端参数服务/API    | <---S3--> |  云端批量分析 / DB          |
|  (RSCL/ROS2 Runtime) |           |  (REST / MQTT)      |           |  (Heatmap, Stats)         |
|  - RL Inference      |           |  - 接收热图/权重      |           |  - 自动生成 planner weights |
|  - Local Logger      |           |  - 下发 planner.yaml |           |  - 模型仓库（artifacts）    |
+-------------------+              +---------------------+            +--------------------------+
^   |                                ^
|   | 上传 RSCL/ROS2 bag              | 分析后下发参数 / 模型
|   v                                |
Local recorder ----> object storage (aws-s3/tencent-cos)
```

## build
```
git clone https://gitlab.t3caic.com/icr11/dataengine/data-infra/Aurora/ad_edgeinsight.git
cd ad_edgeinsight
colcon build --base-paths src/data_collection
cd build && cmake .. && make -j8
```
