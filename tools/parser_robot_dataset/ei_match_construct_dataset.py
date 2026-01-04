import json
import os
import pyarrow.parquet as pq
import pyarrow as pa
import pandas as pd
import numpy as np
import glob
import shutil
from match_img_status import map_joint,all_joint,all_hand


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
                #print(video_path)
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
    print("parquet_info :state_dim and action_dim info ",state_dim,action_dim)
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
    #print("columns names :",parqut_df.columns)
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

def copy_2_task(tasks_jsonl,dst_dataset_dir):
    dst_meta_dir = os.path.join(dst_dataset_dir,"meta")
    if not os.path.exists(dst_meta_dir):
        os.makedirs(dst_meta_dir)
    print("step 1: copy tasks.json to data"    )
    dest_tasks_jsonl = os.path.join(dst_meta_dir,"tasks.jsonl")
    shutil.copy(tasks_jsonl, dest_tasks_jsonl)

def reformat_2_parquet(orig_parquet,dst_dir,ep_index,start_index=0,task_id=0):
    orig_table = pq.read_table(orig_parquet)
    parquet_file = pq.ParquetFile(orig_parquet)
    orig_data = parquet_file.read().to_pandas()
    message_cnt = len(orig_data["index"])
    episode_nm = "episode_{}".format(str(ep_index).zfill(6)) 
    dst_parquet = os.path.join(dst_dir,episode_nm+".parquet")
    episode_index = [ep_index for _ in range(message_cnt)]
    index = [i+start_index for i in range(message_cnt)]
    dst_table =   pa.table(
    { 
      "observation.state" : orig_data["observation.state"],
      "action"            : orig_data["action"],
      "timestamp"         : orig_data["timestamp"],
	 "annotation.human.action.task_description": orig_data["annotation.human.action.task_description"],
     "task_index"         : orig_data["task_index"],
     "annotation.human.validity": orig_data["annotation.human.validity"],
	 "episode_index"      : episode_index,
	 "index"              : index ,
	#  "next.reward"       : [0.0],
	#  "next.done"         : [False]
    })
    pq.write_table(dst_table, dst_parquet)
    return message_cnt

def parse_task_jsonl(task_json_path):
    task_info =[]
    with open(task_json_path, 'r') as file:
        for line in file:
        # 解析每一行
            data = json.loads(line)
            # 处理数据
            task_info.append(data["task"])
    return task_info

def construct_dataset(match_dir,out_dataset_dir):
    output_video_dir = os.path.join(out_dataset_dir,"videos","chunk-000","observation.images.ego_view")
    output_data_dir = os.path.join(out_dataset_dir,"data","chunk-000")
    output_meta_dir  = os.path.join(out_dataset_dir,"meta")
    os.makedirs(output_video_dir,exist_ok=True)
    os.makedirs(output_meta_dir,exist_ok=True)
    os.makedirs(output_data_dir,exist_ok=True)
    episodes_jsonl_file = os.path.join(out_dataset_dir,"meta","episodes.jsonl") 
    if os.path.exists(episodes_jsonl_file):
        os.remove(episodes_jsonl_file)
    start_index = 0
    total_info  = len(os.listdir(match_dir))
    for i, clip_nm in enumerate(sorted(os.listdir(match_dir))) :
        #clip_nm =r"rosbag2_2025_08_21-15_56_35"
        episode_index_info  = i

        ##videos 文件
        episode_nm = "episode_{}".format(str(episode_index_info).zfill(6)) 
        episode_video_nm =  episode_nm + ".mp4"
        parsed_clip_path = os.path.join(match_dir,clip_nm)
        file_mp4_nm =[file_mp4 for file_mp4 in os.listdir(parsed_clip_path)  if file_mp4.endswith(".mp4")][0]
        orig_mp4_path  = os.path.join(parsed_clip_path,file_mp4_nm)
        dst_mp4_path   = os.path.join(output_video_dir,episode_video_nm)
        shutil.copy(orig_mp4_path, dst_mp4_path)
        ###parquet 文件
        parquet_file_nm =[file_mp4 for file_mp4 in os.listdir(parsed_clip_path)  if file_mp4.endswith(".parquet")][0]
        orig_parquet_path  = os.path.join(parsed_clip_path,parquet_file_nm)
        message_cnt_info = reformat_2_parquet(orig_parquet_path,output_data_dir,episode_index_info,start_index ,task_id=0 )
        start_index = start_index+message_cnt_info
        

        ##    get task info 
        task_json_path = os.path.join(output_meta_dir,"tasks.jsonl")
        task_json_info = parse_task_jsonl(task_json_path)

        ## episodes.jsonl
        episodes_jsonl = {"episode_index": episode_index_info,
          #"tasks": ["pick the thread and grap demo", "valid"],
          "tasks": task_json_info, 
          "length": message_cnt_info, 
          "trajectory_id": clip_nm+"grap"}
        with open(episodes_jsonl_file, "a") as f:
            f.write(json.dumps(episodes_jsonl) + "\n") 
    
    modality_file_nm =[file_mp4 for file_mp4 in os.listdir(parsed_clip_path)  if file_mp4.endswith("modality.json")][0]
    orig_modalityt_path  = os.path.join(parsed_clip_path,modality_file_nm)
    dst_modality_path  =  os.path.join(output_meta_dir,modality_file_nm)
    shutil.copy(orig_modalityt_path, dst_modality_path)
    print("modality json file ",dst_modality_path)
    ## info.json         
    get_info_json(out_dataset_dir)
    ##  stats.json
    get_stat_json(out_dataset_dir)
    print("total info ",total_info)

    
if __name__ == "__main__":
    root_dataset_path = r'/data/cloud/clips/temp/20251120/match'
    dst_dataset_path = r'/data/cloud/clips/temp/20251120/dataset'
    task_json_file_path = r"/data/cloud/ros2/rosbag2_motor/temp/tasks.jsonl"
    copy_2_task(task_json_file_path,dst_dataset_path)
    construct_dataset(root_dataset_path,dst_dataset_path)

