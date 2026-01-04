#!/usr/bin/python3
# -*- coding: utf-8 -*-#

import os 
import shutil
import json
import math
import subprocess
import pyarrow as pa
import pyarrow.parquet as pq

from decimal import Decimal
## 配置时间的精确程度
from decimal import getcontext
work_context = getcontext()
work_context.prec = 9



all_joint =["left_arm","right_arm","waist","neck","left_leg","right_leg"]
all_hand =["left_hand","right_hand"]
map_joint ={
"left_arm"  :["L_SHOULDER_P","L_SHOULDER_R","L_SHOULDER_Y","L_ELBOW_Y","L_WRIST_Y","L_WRIST_R","L_WRIST_P"]
,"right_arm":["R_SHOULDER_P","R_SHOULDER_R","R_SHOULDER_Y","R_ELBOW_Y","R_WRIST_Y","R_WRIST_R","R_WRIST_P"]
,"waist"    :["WAIST_Y","WAIST_P","WAIST_R"]
,"neck"     :["NECK_Y","NECK_P","NECK_R"]
,"left_leg" :["L_HIP_P","L_HIP_R","L_HIP_Y","L_KNEE_P","L_ANKLE_P","L_ANKLE_R"]
,"right_leg":["R_HIP_P","R_HIP_R","R_HIP_Y","R_KNEE_P","R_ANKLE_P","R_ANKLE_R"]
,"left_hand" :["L_HAND_0","L_HAND_1","L_HAND_2","L_HAND_3","L_HAND_4","L_HAND_5"]
,"right_hand":["R_HAND_0","R_HAND_1","R_HAND_2","R_HAND_3","R_HAND_4","R_HAND_5"]
} 
sub_topic_nm = ['hand_command','hand_state','joint_command_upper_body','joint_state' ]

def find_match_joint_time(pcd_file, joint_time_list,match_gap=1):
    match_joint = None
    for joint_file_time in joint_time_list:
        joint_timestamp =  Decimal(joint_file_time)
        image_timestamp =  Decimal(pcd_file.split('/')[-1][0:-4])
        time_gap = abs(joint_timestamp - image_timestamp)
        if time_gap <= match_gap:
            match_joint = Decimal(joint_timestamp)
            match_gap = time_gap
            #print("match pose",pcd_file,match_gap)
    if not match_joint:
        print('未匹配pose', pcd_file,time_gap)
    return match_joint

def parse_joint_json(src_joint_path):
    jont_time_array =[]
    with open(src_joint_path,mode="r") as fobj:
        joint_array = json.load(fobj)
        for joint in joint_array:
            joint_time = joint.get("message_time")
            jont_time_array.append(joint_time)
    return jont_time_array


# 基本文件复制
def shutil_file_rename(src_dir,dst_dir):
    images = [ img  for img in sorted(os.listdir(src_dir)) if img.endswith(".jpg")] 
    if not os.path.exists(dst_dir):
        os.mkdir(dst_dir)
    for i, filename in enumerate(sorted(images)):
        src_image = os.path.join(src_dir,filename)
        dst_file_nm= "{}.png".format(str(i))
        dst_img   = os.path.join(dst_dir,dst_file_nm)
        shutil.copy(src_image, dst_img)      

import cv2  
def img_resize(input_file,out_put):
    img = cv2.imread(input_file)
    new_width, new_height = 640, 480
    resized = cv2.resize(img, (new_width, new_height))
    cv2.imwrite(out_put, resized, params=None)
    

def format_embodies_img_data(src_root_sub_dir,img_dir_name = r"image_raw"):
    ##图像内外参矩阵文件地址
    # join_sub_nm  = r'joint'
    img_sub_name = img_dir_name
    dst_sub_nm = r"images_format"
    ##图像     undistort_images
    img_path = os.path.join( src_root_sub_dir,img_sub_name)
    dst_img_path = os.path.join( src_root_sub_dir,dst_sub_nm)
    if not os.path.exists(dst_img_path):
        os.makedirs(dst_img_path)
    ##点云 bev_pcd compensate_pcd
    img_file_list =  [img_nm  for img_nm in sorted(os.listdir(img_path)) if img_nm.endswith("jpg") ]
    img_file_list.sort(key=lambda x: float(x.split('/')[-1][0:-4]))
    match_img_map =[]
    i = 0   
    for img_file  in img_file_list: 
        match_dict ={}   
        src_image = os.path.join(img_path,img_file)
        dst_file_nm= "{}.jpg".format(str(i))
        match_dict[str(i)]= img_file[:-4]
        dst_image = os.path.join(dst_img_path,dst_file_nm)
        img_resize(src_image, dst_image)
        #shutil.copy(src_image, dst_image)
        i =i+1  
        match_img_map.append(match_dict)
    join_file_out=os.path.join(src_root_sub_dir,"image"+"_map.json")
    with open(join_file_out, 'w') as f:
        json.dump(match_img_map, f, indent=2)  

def format_embodies_data(src_root_sub_dir,join_sub_nm,img_dir_name = r"image_raw"):
    ##图像内外参矩阵文件地址
    # join_sub_nm  = r'joint'
    img_sub_name = img_dir_name
    dst_sub_nm = r"images_format"
    ##图像     undistort_images
    img_path = os.path.join( src_root_sub_dir,img_sub_name)
    dst_img_path = os.path.join( src_root_sub_dir,dst_sub_nm)
    if not os.path.exists(dst_img_path):
        os.makedirs(dst_img_path)
    ##点云 bev_pcd compensate_pcd
    img_file_list =  [img_nm  for img_nm in sorted(os.listdir(img_path)) if img_nm.endswith("jpg") ]
    img_file_list.sort(key=lambda x: float(x.split('/')[-1][0:-4]))
    uniq_name = os.path.basename(src_root_sub_dir)
    joint_path = os.path.join( src_root_sub_dir,join_sub_nm,"joint.json")
    jont_times = parse_joint_json(joint_path)
    all_joint_times =  json.load(open(joint_path,mode="r") )
    match_time_tuple =[]
    joint_match_json = []
    i = 0
    for img_file  in img_file_list:   
        match_joint_time = find_match_joint_time(img_file,jont_times,match_gap=0.8)
        str_match_joint_time = str(match_joint_time)
        if match_joint_time is not None:
            match_time_tuple.append([img_file,match_joint_time])
            for sig  in all_joint_times:
                #print("sig",sig)
                if sig ["message_time"] == str_match_joint_time :
                    joint_info =  sig[str_match_joint_time]
                    status_info ={str_match_joint_time:joint_info}
                    joint_match_json.append(status_info)
                # else:
                #     print("joint_info",joint_info,sig ["message_time"])
        else:
            print("img_time",img_file,match_joint_time)
    join_file_out=os.path.join(src_root_sub_dir,join_sub_nm+"_match.json")
    with open(join_file_out, 'w') as f:
        json.dump(joint_match_json, f, indent=2)  
    print("len img_file_list",join_sub_nm,len(img_file_list),len(match_time_tuple))
    return match_time_tuple

# 图片文件列表
def img_2_h264_mp4(img_dir_path,out_dataset_dir,ep_index=1,video_fps=30):
    #images = [  img for img in sorted(os.listdir(img_dir_path)) if img.endswith(".png")]
    #images = [ os.path.join(img_dir_path,img)  for img in sorted(os.listdir(img_dir_path)) if img.endswith(".png")]  # 确保路径正确且图片存在
    episode_nm = "episode_{}".format(str(ep_index).zfill(6))
    ### 2.1生成视频
    episode_video_nm = episode_nm+".mp4"
    fps = video_fps  # 帧率
    # 打开文件（如果文件不存在将会被创建）
    output_dir = os.path.join(out_dataset_dir)
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    output_filename = os.path.join(output_dir,episode_video_nm)
    ffmpeg_cmd = 'cd {0} && ffmpeg -framerate {2}   -i  {1}/%d.jpg -c:v libx264 -pix_fmt yuv420p  -r {2} -y {3} '.format(output_dir,img_dir_path,str(fps),output_filename)  
    print("ffmpeg_cmd ",ffmpeg_cmd)
    # ffmpeg_cmd = 'cd {} && ffmpeg  -framerate, str(fps),'-c:v', 'libx264', '-pix_fmt', 'yuv420p', output_filename'.format(img_dir_path,stat_file_nm)
    subprocess.run([ffmpeg_cmd],  shell=True, encoding="utf-8",check=True,executable="/bin/bash")
    print("output_filename",output_filename)

def get_time_diff_by_img(img_json_file_path):
    timestamp_all =[]
    index_frame =[]
    with open(img_json_file_path,mode="r") as fobj:
        joint_array = json.load(fobj)
        start_time = Decimal(joint_array[0].get("0"))
        for i,joint in enumerate(joint_array) :
            index_frame.append(i)
            joint_time = joint[str(i)]
            diff_time = float(Decimal(joint_time) - start_time)
            timestamp_all.append(diff_time)
    return timestamp_all,index_frame


def get_match_action_state_dat(joint_state_json,hand_state_json):
    observation_joint_state =[]
    observation_hand_state =[]
    with open(joint_state_json,mode="r") as fobj,open(hand_state_json,mode="r") as fobj_h:
        joint_array = json.load(fobj)
        for joint in joint_array:
            joint_state_sig =[]
            joint_time = list(joint)[0]
            joint_detail = joint[joint_time] 
            for sif_body_info in all_joint:
                sig_joint_num = []
                for   sif_joint_info in map_joint[sif_body_info]:
                    sig_jont_status_info = joint_detail[sif_joint_info]
                    sig_joint_num.append(sig_jont_status_info)
                joint_state_sig.extend(sig_joint_num)  
            observation_joint_state.append(joint_state_sig)  
        joint_hand_array = json.load(fobj_h)
        for hand in joint_hand_array:
            hand_state_sig =[]
            joint_time = list(hand)[0]
            hand_detail = hand[joint_time]    
            for sif_hand_info in all_hand:
                sig_joint_num = []
                for  sif_joint_info in map_joint[sif_hand_info]:
                    sig_hand_status_info = hand_detail[sif_joint_info]
                    sig_joint_num.append(sig_hand_status_info)
                hand_state_sig.extend(sig_joint_num)
            observation_hand_state.append(hand_state_sig)
    print("observation_state :",len(observation_hand_state),len(observation_joint_state))
    observation_state = [a + b for a, b in zip(observation_joint_state, observation_hand_state)]
    return observation_state

def get_stat_action_state_step(joint_state_json,hand_state_json):
    modality_state ={}
    with open(joint_state_json,mode="r") as fobj,open(hand_state_json,mode="r") as fobj_h:
        joint_array = json.load(fobj)
        example_joint = joint_array[0]
        start_time = list(example_joint)[0]
        joint_detail = example_joint.get(start_time)
        start = 0
        for sif_body_info in all_joint:
            modality_state[sif_body_info] ={}
            modality_state[sif_body_info]["start"]=start
            sig_joint_num = [] 
            for  sif_joint_info in map_joint[sif_body_info]:
                sig_jont_status_info = joint_detail[sif_joint_info]
                sig_joint_num.append(sig_jont_status_info)
            start = start + len(sig_joint_num)
            modality_state[sif_body_info]["end"] = start
        joint_hand_array = json.load(fobj_h)
        example_hand = joint_hand_array[0]
        start_time = list(example_hand)[0]
        hand_detail = example_hand.get(start_time)
        for sif_body_info in all_hand:
            modality_state[sif_body_info] ={}
            modality_state[sif_body_info]["start"]=start
            sig_joint_num = [] 
            for  sif_joint_info in map_joint[sif_body_info]:
                sig_jont_status_info = hand_detail[sif_joint_info]
                sig_joint_num.append(sig_jont_status_info)
            start = start + len(sig_joint_num)
            modality_state[sif_body_info]["end"] = start      
    return modality_state

def json_2_parquet(parsed_json_dir,out_dataset_dir,ep_index=1,task_id=0 ):
    episode_nm = "episode_{}".format(str(ep_index).zfill(6))
    parque_file_nm = episode_nm+".parquet"

    observation_state =[]
    observation_action =[]
    img_map_json = os.path.join(parsed_json_dir,"image_map.json")
    timestamp,frame_index = get_time_diff_by_img(img_map_json)
    message_cnt = len(timestamp)
    episode_index = [ep_index for _ in range(message_cnt)]
    task_index    = [task_id for _ in range(message_cnt)]
    validity = [1 for _ in range(message_cnt)]
    task_description = [task_id for _ in range(message_cnt)]
    # ['hand_command','hand_state','joint_command_upper_body','joint_state' ]
    json_file_flag = "_match.json"
    joint_state_json = os.path.join(parsed_json_dir,sub_topic_nm[3]+json_file_flag)
    hand_state_json = os.path.join(parsed_json_dir,sub_topic_nm[1]+json_file_flag)
    observation_state = get_match_action_state_dat(joint_state_json,hand_state_json)

    joint_action_json = os.path.join(parsed_json_dir,sub_topic_nm[2]+json_file_flag)
    hand_action_json  = os.path.join(parsed_json_dir,sub_topic_nm[0]+json_file_flag)
    observation_action = get_match_action_state_dat(joint_action_json,hand_action_json)
    #print(observation_state)
    out_parque_dir = os.path.join(out_dataset_dir)
    if not os.path.exists(out_parque_dir):
        os.makedirs(out_parque_dir)
    output_filename = os.path.join(out_parque_dir,parque_file_nm)
    table = pa.table(
    { 
      "observation.state" : observation_state,
      "action":  observation_action,
      "timestamp"         : timestamp,
	 "annotation.human.action.task_description": task_description,
     "task_index"        : task_index,
     "annotation.human.validity": validity,
	 "episode_index"     : episode_index,
	 "index"             : frame_index,
	#  "next.reward"       : [0.0],
	#  "next.done"         : [False]
    })
    pq.write_table(table,output_filename )
    return message_cnt


def generate_modality_json(join_state_json,hand_state_json,join_action_json,hand_action_json,dst_dataset_dir):
    out_meta_dir = dst_dataset_dir
    if not os.path.exists(out_meta_dir):
        os.makedirs(out_meta_dir)
    modality_state  = get_stat_action_state_step(join_state_json,hand_state_json)
    modality_action = get_stat_action_state_step(join_action_json,hand_action_json)
    info={}
    info["state"] = modality_state
    #print("modality_state",modality_state)
    info["action"] = modality_action
    info["video"] = {
        "ego_view": {
            "original_key": "observation.images.ego_view"
        }
    }
    info["annotation"] = {
        "human.action.task_description": {},
        "human.validity": {}
    }  


    join_file_out = os.path.join(out_meta_dir,"modality.json")
    with open(join_file_out, 'w') as f:
        json.dump(info, f, indent=2)  
    print("modality ",join_file_out)



def process_package_data(rosbag_file_dir,parser_dir_nm="parser_robot_dataset",matched_dir_nm="match"):
    root_dir = os.path.dirname(os.path.dirname(rosbag_file_dir))
    clip_nm = os.path.basename(rosbag_file_dir)
    parsed_dir = os.path.join(root_dir,parser_dir_nm)
    dst_dataset_dir = os.path.join(root_dir,matched_dir_nm,clip_nm)
    if not os.path.exists(dst_dataset_dir):
        os.makedirs(dst_dataset_dir)
    print("step 2: dataset start"    )
 
    #clip_nm =r"rosbag2_2025_08_21-15_56_35"
    episode_index_info  = 0
    parsed_clip_path = os.path.join(parsed_dir,clip_nm) 
    print("clip_nm nm",clip_nm)     
    ### 1.整理和时间匹配数据 
    print("step 2.1: 1.整理和时间匹配数据 "    )
    print("parsed_clip_path :",parsed_clip_path)
    #format_embodies_img_data(parsed_clip_path) 
    format_embodies_img_data(parsed_clip_path,img_dir_name = r"image_rawcompressed")
    #sub_topic_nm   
    #join_sub_nm_info =['hand_command','hand_state','joint_command','joint_state' ]
    join_sub_nm_info =sub_topic_nm
    for sig_ontolo in join_sub_nm_info:
        format_embodies_data(parsed_clip_path,sig_ontolo,img_dir_name = r"image_rawcompressed")

    ### 2.生成目标数据集
    print("step 2.2: 1.生成目标数据集"    )
    format_dst_img_dir = os.path.join(parsed_clip_path,"images_format")
    ### 2.1生成视频
    print("step 2.2.: 2.生成视频"    )
    img_2_h264_mp4(format_dst_img_dir,dst_dataset_dir,ep_index=episode_index_info)
    ### 2.2 生成parquet 文件
    print("step 2.2.2: 3.生成parquet"    )
    message_cnt_info = json_2_parquet(parsed_clip_path,dst_dataset_dir,ep_index=episode_index_info,task_id=0 )
    print("step 2.3 生成meta数据"    )
    ### 2.3 生成meta数据
    joint_state_json_eg = os.path.join(parsed_clip_path,sub_topic_nm[3]+"_match.json")
    hand_state_json_eg = os.path.join(parsed_clip_path ,sub_topic_nm[1]+"_match.json")
    joint_acton_json_eg = os.path.join(parsed_clip_path,sub_topic_nm[2]+"_match.json")
    hand_action_json_eg = os.path.join(parsed_clip_path,sub_topic_nm[0]+"_match.json")

    print("step 2.3.1生成 modality_json"    )
    generate_modality_json(joint_state_json_eg,hand_state_json_eg,joint_acton_json_eg,hand_action_json_eg,dst_dataset_dir)

    print("end the process !")

if __name__ =="__main__":
    root_dir_exp = r"/data/cloud/clips/temp/20251121/parser_robot_dataset"
    total = len(os.listdir(root_dir_exp))
    for i, bag_nm in enumerate(sorted(os.listdir(root_dir_exp))) :
        new_bag_path = os.path.join(root_dir_exp,bag_nm)
        print("new_bag_path",i,total,new_bag_path)
        process_package_data(new_bag_path)
    
