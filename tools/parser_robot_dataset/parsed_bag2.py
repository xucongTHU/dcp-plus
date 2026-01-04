
from pathlib import Path
import os,json
from cv_bridge import CvBridge
import cv2
from rosbags.typesys import Stores, get_types_from_msg, get_typestore
from rosbags.highlevel import AnyReader
from rosbags.rosbag2 import Reader

class parserManager:
    def __init__(self,bag_path):
        self.bag_path = bag_path

    def guess_msgtype(self,path: Path) -> str:
        """Guess message type name from path."""
        name = path.relative_to(path.parents[2]).with_suffix('')
        if 'msg' not in name.parts:
            name = name.parent / 'msg' / name.name
        return str(name)
	
    def rejister_msg(self,):
        typestore = get_typestore(Stores.ROS2_HUMBLE)
        add_types = {}
        ##  motor_msgs/msg/JointStateArray	  
        for pathstr in ['/opt/motor_msgs/msg/JointState.msg', '/opt/motor_msgs/msg/JointStateArray.msg',
                        '/opt/caicrobot_msgs/msg/JointState.msg', '/opt/caicrobot_msgs/msg/JointStateArray.msg',
                        '/opt/caicrobot_msgs/msg/JointCommand.msg','/opt/caicrobot_msgs/msg/JointCommandArray.msg']:
            msgpath = Path(pathstr)
            msgdef = msgpath.read_text(encoding='utf-8')
            add_types.update(get_types_from_msg(msgdef,self.guess_msgtype(msgpath)))
        typestore.register(add_types)
        return typestore

    def img_topic_type(self,):
        img_topic = ['/image_raw','/image_depth']  
        img_format =['bgr8','16UC1']  
        img_topic_map = dict(zip(img_topic,img_format))  
        return img_topic_map
    
    def parser_join(self,reader,bag_path_file,topic):
        ### topic  /joint_command_R485  /joint_state_R485 /joint_state
        out_dir_nm = topic.replace("/","")
        out_joint_dir =  os.path.join(os.path.dirname(bag_path_file), "parser_robot_dataset",os.path.basename(bag_path_file),out_dir_nm)
        if not os.path.exists(out_joint_dir):
            os.makedirs(out_joint_dir)  
        connections = [x for x in reader.connections if x.topic == topic]
        all_joint_info =[]
        print("start parse joint ...")
        for connection, timestamp, rawdata in reader.messages(connections=connections):
            msg = reader.deserialize(rawdata, connection.msgtype)
            #print(msg.header.frame_id,msg.header.stamp.sec,msg.header.stamp.nanosec)
            message_nan_time = str(msg.header.stamp.sec) + "."+ str(msg.header.stamp.nanosec).rjust(9, '0')
            #message_time = "{0}.{1}".format( str(timestamp)[0:10],str(timestamp)[10:] )
            message_time = message_nan_time
            #print("msg.states len",len(msg.states)) 
            status_info ={"message_time":message_time}
            joint_info ={}
            for sig_status in msg.states:
                 #joint_info[sig_status.name] =( sig_status.position,sig_status.velocity,sig_status.effort)
                 joint_info[sig_status.name] =( sig_status.position)
            status_info[message_time] =  joint_info
            all_joint_info.append(status_info)
        join_file_out=os.path.join(out_joint_dir,"joint.json")
        print('parse joint end,共提取joint 消息个数: %d' % len(all_joint_info),join_file_out)
        with open(join_file_out, 'w') as f:
            json.dump(all_joint_info, f, indent=2) 

    def parser_command_join(self,reader,bag_path_file,topic):
        out_dir_nm = topic.replace("/","")
        out_joint_dir =  os.path.join(os.path.dirname(bag_path_file), "parser_robot_dataset",os.path.basename(bag_path_file),out_dir_nm)
        if not os.path.exists(out_joint_dir):
            os.makedirs(out_joint_dir)  
        connections = [x for x in reader.connections if x.topic == topic]
        all_joint_info =[]
        print("start parse joint ...")
        for connection, timestamp, rawdata in reader.messages(connections=connections):
            msg = reader.deserialize(rawdata, connection.msgtype)
            message_nan_time = str(msg.header.stamp.sec) + "."+ str(msg.header.stamp.nanosec).rjust(9, '0')
            message_time = message_nan_time
            status_info ={"message_time":message_time}
            joint_info ={}
            for sig_status in msg.commands:
                # joint_info[sig_status.name] =( sig_status.position,sig_status.velocity,sig_status.effort,
                # sig_status.kp ,sig_status.kd,sig_status.control_mode)
                 joint_info[sig_status.name] =( sig_status.position)
            status_info[message_time] =  joint_info
            all_joint_info.append(status_info)
        join_file_out=os.path.join(out_joint_dir,"joint.json")
        print('parse joint end,共提取joint 消息个数: %d' % len(all_joint_info),join_file_out)
        with open(join_file_out, 'w') as f:
            json.dump(all_joint_info, f, indent=2) 


    def parse_img(self,reader,bag_path_file,img_topic):
        topics_map = self.img_topic_type()
        if img_topic in topics_map.keys():
            out_topic_nm = img_topic.replace("/","")
            img_connections = [x for x in reader.connections if x.topic == img_topic]
            index_img = 0
            bridge = CvBridge()
            out_img_dir =  os.path.join(os.path.dirname(bag_path_file), "parser_robot_dataset",os.path.basename(bag_path_file),out_topic_nm)
            if not os.path.exists(out_img_dir):
                os.makedirs(out_img_dir)
            print("start parse img ...")
            for connection, timestamp, rawdata in reader.messages(connections=img_connections):
                msg = reader.deserialize(rawdata, connection.msgtype)
                cv_image = bridge.imgmsg_to_cv2(msg,topics_map[img_topic])
                out_img_nm = os.path.join(out_img_dir,  "{0}.{1}.jpg".format(str(timestamp)[0:10],str(timestamp)[10:].rjust(9, '0')))
                cv2.imwrite(out_img_nm, cv_image)
                index_img += 1
            print('parser_robot_dataset image end,共提取图像个数: %d' % index_img,out_img_dir)
        else:
            print("not match img topic  {}".format(topics_map))

    def parser_rosbag2_all(self,):
        custorm_typestore = self.rejister_msg()
        bag_path_file = self.bag_path
        bagpath = Path(bag_path_file)
        
        # Create reader instance and open for reading.
        message_topics = []
        with AnyReader([bagpath], default_typestore=custorm_typestore) as reader:
            for connection in reader.connections:
                message_topics.append(connection.topic)
                print(connection.topic, connection.msgtype)
            joint_topics = [   "/hand_state", "/joint_state"]
            #joint_topics = [  "/joint_state"]
            for joint_topic in joint_topics:
                if joint_topic in message_topics:
                    self.parser_join(reader,bag_path_file,joint_topic)
            joint_command_topics = [  "/joint_command_upper_body","/hand_command"]
            for joint_cmd_topic in joint_command_topics:
                if joint_cmd_topic in message_topics:
                    self.parser_command_join(reader,bag_path_file,joint_cmd_topic)
            img_topics = ['/image_raw']
            for img_topic in img_topics:
                if img_topic in message_topics:
                    self.parse_img(reader,bag_path_file,img_topic)

if __name__ =="__main__":
    root_batch = r"/data/cloud/ros2/rosbag2_motor/batch_01_select"
    for bag_nm in  os.listdir(root_batch):
        if  bag_nm.startswith("rosbag2_"):
            bag_path = os.path.join(root_batch,bag_nm)
            custome_parser = parserManager(bag_path)
            custome_parser.parser_rosbag2_all()
 