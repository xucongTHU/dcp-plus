//
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "ros2bag_recorder.h"

#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>

#include "rosbag2_cpp/writers/sequential_writer.hpp"
#include "rosbag2_storage/serialized_bag_message.hpp"
#include "rcutils/error_handling.h"
#include "common/utils/utils.h"

namespace dcp::recorder {

Ros2BagRecorder::Ros2BagRecorder(std::shared_ptr<rclcpp:: Node> node)
    : node_(node),
      current_mode_(OptMode::WRITE),
      is_initialized_(false),
      is_opened_(false),
      has_data_written_(false),
      max_bag_size_mb_(0),
      messages_since_last_log_(0) {
  bag_info_. start_time = std::chrono::system_clock::now();
  last_log_time_ = std::chrono::steady_clock::now();
}

Ros2BagRecorder::~Ros2BagRecorder() {
  if (is_opened_) {
    Close();
  }
}

bool Ros2BagRecorder::Init() {
  if (is_initialized_) {
    RCLCPP_WARN(node_->get_logger(), "Recorder already initialized");
    return true;
  }

  is_initialized_ = true;
  RCLCPP_INFO(node_->get_logger(),
              "Ros2BagRecorder initialized successfully");
  return true;
}


bool Ros2BagRecorder::InitRingBuffers() {
  for (const auto& channel : strategy_->dds.channels) {
    const std::string& topic = channel.topic;
    if (channel.originalFrameRate <=0 || channel.capturedFrameRate <=0)
    {
      return false;
    }
    trigger::CacheMode cache_mode_ = strategy_->mode.cacheMode;

    int32_t forward_size = cache_mode_.forwardCaptureDurationSec * channel.capturedFrameRate;
    int32_t backward_size = cache_mode_.backwardCaptureDurationSec * channel.capturedFrameRate;

    auto forward_buf = std::make_unique<BufferType>(forward_size);
    if (!forward_buf) {
      RCLCPP_ERROR(node_->get_logger(), "Create forward buffer failed for topic: %s", topic.c_str());
      return false;
    }

    auto backward_buf = std::make_unique<BufferType>(backward_size);
    if (!backward_buf) {
      RCLCPP_ERROR(node_->get_logger(), "Create backward buffer failed for topic: %s", topic.c_str());
      return false;
    }

    forward_ringbuffers_[topic] = std::move(forward_buf);
    backward_ringbuffers_[topic] = std::move(backward_buf);
    RCLCPP_INFO(node_->get_logger(), "Init buffer for topic: %s, forward size: %d, backward size: %d", topic.c_str(), forward_size, backward_size);
  }
  return true;
}

bool Ros2BagRecorder::Open(OptMode opt_mode, const std::string& full_path) {
  if (!is_initialized_) {
    RCLCPP_ERROR(node_->get_logger(),
                 "Recorder must be initialized before opening");
    return false;
  }

  if (is_opened_) {
    RCLCPP_WARN(node_->get_logger(), "Bag already open, closing first");
    if (! Close()) {
      RCLCPP_ERROR(node_->get_logger(), "Failed to close previous bag");
      return false;
    }
  }

  try {
    current_mode_ = opt_mode;
    current_bag_path_ = full_path;

    if (opt_mode == OptMode::WRITE) {
      rosbag2_storage::StorageOptions storage_options;
      storage_options.uri = full_path;
      storage_options.storage_id = "sqlite3";
      if (max_bag_size_mb_ > 0) {
        storage_options.max_bagfile_size = max_bag_size_mb_ * 1024 * 1024;
      }

      rosbag2_cpp::ConverterOptions converter_options;
      converter_options.input_serialization_format =
          rmw_get_serialization_format();
      converter_options.output_serialization_format =
          rmw_get_serialization_format();

      writer_ = std::make_unique<rosbag2_cpp::Writer>(std::make_unique<rosbag2_cpp::writers::SequentialWriter>());
      writer_->open(storage_options, converter_options);

      bag_info_.bag_path = full_path;
      bag_info_.is_opened = true;
      bag_info_.mode = OptMode::WRITE;
      bag_info_.start_time = std::chrono::system_clock::now();
      bag_info_.serialization_format = rmw_get_serialization_format();

      RCLCPP_INFO(node_->get_logger(), "Opened bag for writing at:   %s",
                  full_path.c_str());
    } else if (opt_mode == OptMode:: READ) {
      // TODO: Implement READ mode with rosbag2_cpp:: SequentialReader
      RCLCPP_ERROR(node_->get_logger(),
                   "READ mode not yet implemented");
      return false;
    }

    is_opened_ = true;
    has_data_written_ = false;
    return true;

  } catch (const std::exception& e) {
    RCLCPP_ERROR(node_->get_logger(), "Failed to open bag:   %s", e.what());
    is_opened_ = false;
    return false;
  }
}

bool Ros2BagRecorder::IsOpened() const { return is_opened_; }

bool Ros2BagRecorder:: Write(const std::string& topic_name,
                               uint64_t timestamp, const void* buf,
                               size_t buf_len) {
  if (!is_opened_ || !writer_) {
    RCLCPP_ERROR(node_->get_logger(),
                 "Cannot write:  bag not open or writer not initialized");
    return false;
  }

  if (! buf || buf_len == 0) {
    RCLCPP_WARN(node_->get_logger(), "Invalid message data for topic %s",
                topic_name.c_str());
    return false;
  }

  try {
    // Create serialized message
    auto serialized_msg = std::make_shared<rclcpp::SerializedMessage>(buf_len);
    auto& rcl_msg = serialized_msg->get_rcl_serialized_message();

    // Copy data
    std::memcpy(rcl_msg.buffer, buf, buf_len);

    // Create bag message
    auto bag_msg =
        std::make_shared<rosbag2_storage::SerializedBagMessage>();

    bag_msg->topic_name = topic_name;

    // Transfer ownership of serialized data
    bag_msg->serialized_data = std::shared_ptr<rcutils_uint8_array_t>(
        new rcutils_uint8_array_t,
        [this](rcutils_uint8_array_t* data) {
          if (rcutils_uint8_array_fini(data) != RCUTILS_RET_OK) {
            RCLCPP_WARN(node_->get_logger(),
                        "Failed to finalize serialized message");
          }
          delete data;
        });

    *bag_msg->serialized_data = rcl_msg;

    // Set timestamp
    bag_msg->time_stamp = timestamp;

    // Write to bag
    writer_->write(bag_msg);

    has_data_written_ = true;
    update_statistics(topic_name, timestamp, buf_len);

    return true;

  } catch (const std:: exception& e) {
    RCLCPP_ERROR(node_->get_logger(),
                 "Error writing message from %s:   %s", topic_name. c_str(),
                 e.what());
    return false;
  }
}

TBagInfo Ros2BagRecorder::GetBagInfo() const {
  TBagInfo info = bag_info_;
  info.end_time = std::chrono:: system_clock::now();
  return info;
}

bool Ros2BagRecorder::ReadNextFrame(ReadedMessage* message) {
  if (!is_opened_ || current_mode_ != OptMode::READ) {
    RCLCPP_ERROR(node_->get_logger(),
                 "Cannot read: bag not open in READ mode");
    return false;
  }

  if (!message) {
    return false;
  }

  // TODO: Implement reading logic
  RCLCPP_ERROR(node_->get_logger(), "READ mode not yet implemented");
  return false;
}

bool Ros2BagRecorder::Close() {
  if (! is_opened_) {
    return true;
  }

  try {
    if (writer_) {
      writer_. reset();
    }

    is_opened_ = false;
    bag_info_.is_opened = false;
    bag_info_.end_time = std::chrono:: system_clock::now();

    RCLCPP_INFO(node_->get_logger(),
                "Bag closed.  Recorded %zu messages in %. 2f seconds",
                bag_info_.total_messages,
                std::chrono::duration<double>(bag_info_.end_time -
                                             bag_info_. start_time)
                    .count());

    return true;

  } catch (const std::exception& e) {
    RCLCPP_ERROR(node_->get_logger(), "Error closing bag:   %s", e.what());
    return false;
  }
}

bool Ros2BagRecorder::HasDataWritten() const { return has_data_written_; }

bool Ros2BagRecorder::TriggerRecord(uint64_t trigger_timestamp,
                                    const std::string& output_file_path) {
  // if (!is_opened_) {
  //   RCLCPP_ERROR(node_->get_logger(), "Cannot trigger: bag not open");
  //   return false;
  // }

  if (is_triggered_) {
    RCLCPP_WARN(node_->get_logger(), "Trigger ignored: bag already triggered");
    return false; //TODO
  }

  is_triggered_ = true;
  trigger_timestamp_ = trigger_timestamp;
  RCLCPP_INFO(node_->get_logger(), "Triggered at %llu, backward duration: %ds", trigger_timestamp_, cache_mode_.backwardCaptureDurationSec);

  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    uint64_t forward_duration_us = cache_mode_.forwardCaptureDurationSec * 1000000ULL;
    forward_capture_duration_us_ = forward_duration_us;

    for (const auto& pair : forward_ringbuffers_) {
      const std::string& topic = pair.first;
      const auto& buffer = pair.second;

      std::vector<TimestampedData> saved_data;
      for (const auto& data : *buffer) {
        if (data.timestamp <= trigger_timestamp_ &&
            (trigger_timestamp_ - data.timestamp) <= forward_duration_us) {
          saved_data.push_back(data);
            }
      }
      triggered_forward_buffers_[topic] = std::move(saved_data);
    }
  }

  std::this_thread::sleep_for(std::chrono::seconds(cache_mode_.backwardCaptureDurationSec));
  {
    std::lock_guard<std::mutex> lock(buffer_mutex_);
    write_ringbuffer(output_file_path);
    triggered_forward_buffers_.clear();
  }
  is_triggered_ = false;
  forward_capture_duration_us_ = 0;

}

bool Ros2BagRecorder::write_ringbuffer(const std::string& outputfilePath) {
    uint64_t min_timestamp = UINT64_MAX;
    uint64_t max_timestamp = 0;

    if (!Open(OptMode::WRITE, outputfilePath)) {
      RCLCPP_ERROR(node_->get_logger(), "Failed to open bag:   %s", outputfilePath.c_str());
      return false;
    }

    uint64_t start_time = trigger_timestamp_ - (cache_mode_.forwardCaptureDurationSec * 1e9);
    uint64_t end_time = trigger_timestamp_ + (cache_mode_.backwardCaptureDurationSec * 1e9);

    for (const auto& channel : strategy_->dds.channels) {
        const std::string& topic = channel.topic;
        auto forward_it = triggered_forward_buffers_.find(topic);
        auto backward_it = backward_ringbuffers_.find(topic);
        auto current_forward_it = forward_ringbuffers_.find(topic);

        if (forward_it == triggered_forward_buffers_.end() && backward_it == backward_ringbuffers_.end()) {
          RCLCPP_WARN(node_->get_logger(), "No buffer found for topic: %s", topic.c_str());
          continue;
        }

        size_t forward_count = 0;
        size_t backward_count = 0;

        std::unordered_set<uint64_t> written_timestamps;
        if (forward_it != triggered_forward_buffers_.end()) {
          for (const auto& data : forward_it->second) {
            auto& rcl_msg = data.msg.get_rcl_serialized_message();
            if (data.timestamp <= trigger_timestamp_ && data.timestamp >= start_time) {
              min_timestamp = std::min(min_timestamp, data.timestamp);
              max_timestamp = std::max(max_timestamp, data.timestamp);
              Write(topic, data.timestamp, rcl_msg.buffer, rcl_msg.buffer_length);
              written_timestamps.insert(data.timestamp);
              forward_count++;
            }
          }
        }

        // 处理当前前向缓冲区中的数据
        if (current_forward_it != forward_ringbuffers_.end() && forward_it != triggered_forward_buffers_.end()) {
          const auto& current_buffer = current_forward_it->second;
          for (const auto& data : *current_buffer) {
            auto& rcl_msg = data.msg.get_rcl_serialized_message();
            if (data.timestamp <= trigger_timestamp_ && data.timestamp >= start_time) {
              if (written_timestamps.find(data.timestamp) == written_timestamps.end()) {
                min_timestamp = std::min(min_timestamp, data.timestamp);
                max_timestamp = std::max(max_timestamp, data.timestamp);
                Write(topic, data.timestamp, rcl_msg.buffer, rcl_msg.buffer_length);
                written_timestamps.insert(data.timestamp);
                forward_count++;
              }
            }
          }
        }

        // 写入后向数据
        if (backward_it != backward_ringbuffers_.end()) {
          auto& backward_buf = backward_it->second;
          for (const auto& data : *backward_buf) {
            auto& rcl_msg = data.msg.get_rcl_serialized_message();
            if (data.timestamp > trigger_timestamp_ && data.timestamp <= end_time) {
              min_timestamp = std::min(min_timestamp, data.timestamp);
              max_timestamp = std::max(max_timestamp, data.timestamp);
              Write(topic, data.timestamp, rcl_msg.buffer, rcl_msg.buffer_length);
              backward_count++;
            }
          }
        }

      RCLCPP_INFO(node_->get_logger(), "Topic %s: wrote %zu forward messages, %zu backward messages",
                 topic.c_str(), forward_count, backward_count);
    }

    double duration_seconds = (max_timestamp - min_timestamp) / 1e6;
    RCLCPP_INFO(node_->get_logger(), "Total recording duration: %.3f seconds", duration_seconds);

    Close();
    RCLCPP_INFO(node_->get_logger(), "Wrote all topics to file: %s", outputfilePath.c_str());
    return true;
}

bool Ros2BagRecorder::SetMaxBagSize(size_t max_size_mb) {
  max_bag_size_mb_ = max_size_mb;

  RCLCPP_INFO(node_->get_logger(), "Max bag size set to:   %zu MB",
              max_size_mb);

  return true;
}

TBagInfo Ros2BagRecorder::GetStatistics() const {
  return GetBagInfo();
}

void Ros2BagRecorder::OnMessageReceived(const std::string& topic, const rclcpp::SerializedMessage& msg) {
  // if (msg) return;
  uint64_t message_timestamp = common::GetCurrentTimestamp();
  // LOG_INFO("Received message on topic: %s, timestamp: %llu", topic.c_str(), message_timestamp);

  std::lock_guard<std::mutex> lock(buffer_mutex_);
  if (forward_ringbuffers_.count(topic)) {
    uint64_t forward_duration_us = cache_mode_.forwardCaptureDurationSec * 1000000ULL;
    while (!forward_ringbuffers_[topic]->empty() &&
           (message_timestamp - forward_ringbuffers_[topic]->front().timestamp) > forward_duration_us) {
      forward_ringbuffers_[topic]->pop_front();
           }
    forward_ringbuffers_[topic]->push_back(TimestampedData{msg, message_timestamp});
  }

  if (is_triggered_ && backward_ringbuffers_.count(topic)) {
    uint64_t backward_duration_us = cache_mode_.backwardCaptureDurationSec * 1000000ULL;
    if ((message_timestamp - trigger_timestamp_) <= backward_duration_us) {
      backward_ringbuffers_[topic]->push_back(TimestampedData{msg, static_cast<uint64_t>(message_timestamp)});
    }
  }

}


void Ros2BagRecorder::update_statistics(const std::string& topic_name,
                                           uint64_t timestamp,
                                           size_t data_size) {
  bag_info_.total_messages++;
  bag_info_.total_data_size += data_size;
  messages_since_last_log_++;

  if (bag_info_.start_timestamp == 0) {
    bag_info_.start_timestamp = timestamp;
  }
  bag_info_.end_timestamp = timestamp;

  // Update per-topic statistics
  if (topics_metadata_.find(topic_name) != topics_metadata_.end()) {
    topics_metadata_[topic_name].message_count++;
    topics_metadata_[topic_name].last_timestamp = timestamp;
    topics_metadata_[topic_name].data_size += data_size;
    bag_info_.topics[topic_name] = topics_metadata_[topic_name];
  }

  // Log statistics periodically
  auto now = std::chrono::steady_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
      now - last_log_time_);

  if (elapsed.count() >= 10) {  // Log every 10 seconds
    log_statistics();
    last_log_time_ = now;
    messages_since_last_log_ = 0;
  }
}

void Ros2BagRecorder::log_statistics() {
  RCLCPP_INFO(
      node_->get_logger(),
      "[Recorder Stats] Total:  %zu msgs, %. 2f MB, Topics: %zu, Duration: %.1f s",
      bag_info_.total_messages,
      static_cast<double>(bag_info_.total_data_size) / (1024 * 1024),
      bag_info_.num_topics,
      std::chrono::duration<double>(std::chrono::system_clock::now() -
                                    bag_info_. start_time)
          .count());

  for (const auto& [topic, metadata] : bag_info_.topics) {
    RCLCPP_DEBUG(node_->get_logger(),
                 "  %s: %zu msgs, %.2f MB", topic.c_str(),
                 metadata.message_count,
                 static_cast<double>(metadata.data_size) / (1024 * 1024));
  }
}

}