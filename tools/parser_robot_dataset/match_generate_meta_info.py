import json
import os
import pyarrow.parquet as pq
import pandas as pd
import numpy as np
import glob
from match_img_joint_info import map_joint,all_joint,all_hand


def get_task_instruction(task_json_path: str) -> dict:
    """Get task language instruction"""
    with open(task_json_path, "r") as f:
        task_info = json.load(f)
    task_name = task_info[0]["task_name"]
    task_init_scene = task_info[0]["init_scene_text"]
    task_instruction = f"{task_name}.{task_init_scene}"
    print(f"Get Task Instruction <{task_instruction}>")
    return task_instruction

def get_info_json(dataset_path):
    chunks_path = os.path.join(dataset_path,"data")
    total_chunks =  len([chunk_dir for chunk_dir in os.listdir(chunks_path) if chunk_dir.startswith("chunk")])
    
    total_episodes =[]
    for _, _, filenames in os.walk(chunks_path):
        for fiel in  filenames:
            if fiel.endswith(".parquet"):
                total_episodes.append(fiel)
    total_episodes_cnt =  len(total_episodes)
    total_frames = get_frame_total(dataset_path)    
    video_path = os.path.join(dataset_path,"videos")
    total_videos =[]
    for _, _, filenames in os.walk(video_path):
        for fiel in  filenames:
            if fiel.endswith(".mp4"):
                total_videos.append(fiel)
    total_videos_cnt =  len(total_videos)
    names_info =[]
    for sig_body in all_joint:
        names_info.extend(map_joint[sig_body])
    for sig_body in all_hand:
        names_info.extend(map_joint[sig_body])

    state_shape,action_shape = get_parquet_info(dataset_path)
    FEATURES = {   
    "observation.state": {
        "dtype": "float64",
        "shape": [state_shape],
        "names":names_info
    },
    "action": {
        "dtype": "float64",
        "shape": [action_shape],
        "names":names_info
    },
    "timestamp": {
            "dtype": "float64",
            "shape": [
                1
            ]
        },
    "annotation.human.action.task_description": {
            "dtype": "int64",
            "shape": [
                1
            ]
        },
    "task_index": {
            "dtype": "int64",
            "shape": [
                1
            ]
        },
    "annotation.human.validity": {
            "dtype": "int64",
            "shape": [
                1
            ]
        },
     "episode_index": {
            "dtype": "int64",
            "shape": [
                1
            ]
        },
    "index": {
            "dtype": "int64",
            "shape": [
                1
            ]
        },
    "observation.images.ego_view": {
        "dtype": "video",
        "shape": [480, 640, 3],
        "names": ["height", "width", "channel"],
        "video_info": {
            "video.fps": 30.0,
            "video.codec": "h264",
            "video.pix_fmt": "yuv420p",
            "video.is_depth_map": False,
            "has_audio": False,
           },
       },

    }
    
    infos = {
        "codebase_version": "v2.0",
        "robot_type": "Ti5robot",
        "total_episodes": total_episodes_cnt,
        "total_frames": total_frames,
        "total_tasks": 2,
        "total_videos": total_videos_cnt,
        "total_chunks": total_chunks,
        "chunks_size": total_episodes_cnt,
        "fps": 30.0,
        "splits": {
            "train": "0:100"
        },
        "data_path": "data/chunk-{episode_chunk:03d}/episode_{episode_index:06d}.parquet",
        "video_path": "videos/chunk-{episode_chunk:03d}/{video_key}/episode_{episode_index:06d}.mp4",
        "features": FEATURES  
    }
    info_path = os.path.join(dataset_path,"meta","info.json")
    print("info_path ",info_path)
    with open(info_path, "w") as f:
        f.write(json.dumps(infos,indent=2) ) 

import cv2
def get_frame_total(root_data_dir):
    # 视频文件路径
    all_frame = 0
    videos_top_path = os.path.join(root_data_dir,"videos")
    for dirpath, dirnames, filenames in os.walk(videos_top_path):
        for fiel in  filenames:
            if fiel.endswith(".mp4"):
                video_path = os.path.join(dirpath,fiel)
                print(video_path)
                cap = cv2.VideoCapture(video_path)
                frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT)) 
                all_frame =all_frame+frame_count
    cap.release()
    return all_frame
 
def get_parquet_info(root_data_dir):
    parquet = os.path.join(root_data_dir,"data","chunk-000","episode_000000.parquet")
    parquet_file = pq.ParquetFile(parquet)
    data = parquet_file.read().to_pandas()
    state_dim = len(data["observation.state"][0])
    action_dim = len(data["action"][0])
    print("data",state_dim,action_dim)
    return state_dim,action_dim

def set_default( obj):
    if isinstance(obj, np.integer):
        return int(obj)
    elif isinstance(obj, np.floating):
        return float(obj)
    else :
        return obj


def get_stat_json(dataset_path) :
    files = glob.glob('{}/data/**/*.parquet'.format(dataset_path))
    parqut_df = pd.concat([pd.read_parquet(fp) for fp in files])
    print("columns names :",parqut_df.columns)
    stat_info ={}

    nestd_df = ["observation.state" , "action"]
    for nestd in  nestd_df:
        
        explored_df = pd.DataFrame(zip(*parqut_df[nestd]))
        # print(df.columns ,df[r"observation.state"].apply(max))
        max_value = explored_df.apply(max,axis=1).tolist()
        min_value = explored_df.apply(min,axis=1).tolist()
        mean_value = explored_df.apply(np.mean,axis=1).tolist()
        std_value =explored_df.apply(np.std,axis=1).tolist()
        q1_value = explored_df.apply(lambda x: x.quantile(0.1),axis=1).tolist()
        q99_value =explored_df.apply(lambda x: x.quantile(0.99),axis=1).tolist()
        #nestd = "action" if nestd== "observation.action" else nestd
        stat_info[nestd] = {}
        stat_info[nestd]["max"]=max_value
        stat_info[nestd]["min"]=min_value
        stat_info[nestd]["mean"]=mean_value
        stat_info[nestd]["std"]=std_value
        stat_info[nestd]["q01"]=q1_value
        stat_info[nestd]["q99"]=q99_value

    stat_nm = [ 'timestamp',  "task_index" , "episode_index" , "index"] 
    for sig_sta in  stat_nm: 
        stat_info[sig_sta] = {}  
        # parqut_df[sig_sta] = parqut_df[sig_sta].astype(int) if isinstance(parqut_df[sig_sta], np.integer)  else  parqut_df[sig_sta]
        # parqut_df[sig_sta] = parqut_df[sig_sta].astype(float) if isinstance(parqut_df[sig_sta], np.floating)  else  parqut_df[sig_sta]
        max_value  = parqut_df[sig_sta].max()
        min_value  = parqut_df[sig_sta].min()
        mean_value = parqut_df[sig_sta].mean()
        std_value  = parqut_df[sig_sta].std()
        q1_value   = parqut_df[sig_sta].quantile(0.1)
        q99_value  = parqut_df[sig_sta].quantile(0.99)
        #print(type(max_value),type(min_value),type(mean_value),type(std_value),type(q1_value),type(q99_value))
        stat_info[sig_sta]["max"]=set_default(max_value)
        stat_info[sig_sta]["min"]=set_default(min_value)
        stat_info[sig_sta]["mean"]=set_default(mean_value)
        stat_info[sig_sta]["std"]=set_default(std_value)
        stat_info[sig_sta]["q01"]=set_default(q1_value)
        stat_info[sig_sta]["q99"]=set_default(q99_value)

    fixed_stat_nm =["annotation.human.action.task_description",]
    for sig_fixed_sta in  fixed_stat_nm: 
        stat_info[sig_fixed_sta] = {} 
        stat_info[sig_fixed_sta]["max"]=0.0
        stat_info[sig_fixed_sta]["min"]=0.0
        stat_info[sig_fixed_sta]["mean"]=0.0
        stat_info[sig_fixed_sta]["std"]=0.0
        stat_info[sig_fixed_sta]["q01"]=0.0
        stat_info[sig_fixed_sta]["q99"]=0.0

    stat_path = os.path.join(dataset_path,"meta","stats.json")
    print("stats_file_path ",stat_path)
    with open(stat_path, "w") as f:
        f.write(json.dumps(stat_info,indent=2) ) 

    
if __name__ == "__main__":
    root_dataset_path = r'/data/cloud/ros2/rosbag2_motor/batch_01_select/dataset'
    get_info_json(root_dataset_path)
    get_stat_json(root_dataset_path)
