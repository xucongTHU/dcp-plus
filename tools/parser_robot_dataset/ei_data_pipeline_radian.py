import os ,subprocess
from ei_parse_bag_radian import parserManager
from ei_match_img_topic import process_package_data
from ei_match_construct_dataset import copy_2_task,construct_dataset


if __name__ =="__main__":
    root_batch = r"/data/cloud/clips/temp/20251222"
    parsed_sub_dir = "parser_robot_dataset"
    matched_sub_dir = "match"
    data_nm = os.path.basename(root_batch)
    for bag_nm in  os.listdir(root_batch):
        ####if  bag_nm.startswith("rosbag2_"):
        if  bag_nm.startswith("1_"):
            bag_path = os.path.join(root_batch,bag_nm)
            custome_parser = parserManager(bag_path)
            print("bag_path",bag_path)
            custome_parser.parser_rosbag2_all(sub_dir_nm=parsed_sub_dir)
    root_dir_exp = os.path.join(root_batch,parsed_sub_dir)
    total = len(os.listdir(root_dir_exp))
    for i, bag_nm in enumerate(sorted(os.listdir(root_dir_exp))) :
        new_bag_path = os.path.join(root_dir_exp,bag_nm)
        print("new_bag_path",i,total,new_bag_path)
        ####process_package_data(new_bag_path)
        process_package_data(new_bag_path,parser_dir_nm=parsed_sub_dir,matched_dir_nm=matched_sub_dir)
    ###生成目标    
    root_dataset_path = os.path.join(root_batch,matched_sub_dir)
    dst_batch_path = os.path.join(root_batch,data_nm)  
    dst_dataset_path = os.path.join(dst_batch_path,"grab_sprite_degree") 
    task_json_file_path = r"/data/cloud/ros2/rosbag2_motor/temp/tasks.jsonl"
    copy_2_task(task_json_file_path,dst_dataset_path)
    construct_dataset(root_dataset_path,dst_dataset_path)
    dst_obs_path = " obs://fxqc/common/robot/teleop/temp/"
    upload_cmd = "obsutil cp -r -f {0} {1}".format(dst_batch_path,dst_obs_path)
    print("upload_cmd ",upload_cmd)
    #subprocess.run([upload_cmd],  shell=True, encoding="utf-8",check=True,executable="/bin/bash")