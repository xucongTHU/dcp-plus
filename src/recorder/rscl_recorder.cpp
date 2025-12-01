//
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#include "rscl_recorder.h"
#include "ad_rscl/comm/hetero_channel/channel_factory.h"
#include "common/utils/utils.h"
#include "common/log/logger.h"
// #include "common/time/Timer.h"
#include "common/config/app_config.h"
#include <future>

namespace dcl {
namespace recorder {

RsclRecorder::RsclRecorder(const std::shared_ptr<rscl::Node>& node, const std::shared_ptr<dcl::trigger::Strategy>& s)
    : node_(node), strategy_(s) {
    if (node_) {
        service_discovery_ = node_->GetServiceDiscovery();
    }
}

bool RsclRecorder::Init() {
    if (!strategy_) {
        AD_ERROR(RsclRecorder, "No enabled strategy found");
        return false;
    }
    cache_mode_ = strategy_->mode.cacheMode;
    AD_INFO(RsclRecorder, "Cache config - Forward duration: %ds, Backward duration: %ds",
              cache_mode_.forwardCaptureDurationSec, cache_mode_.backwardCaptureDurationSec);

    if (!InitRingBuffers()) {
        AD_ERROR(RsclRecorder, "Init buffers failed");
        return false;
    }

    AD_INFO(RsclRecorder, "RsclRecorder Init ok");
    return true;
}

bool RsclRecorder::InitRingBuffers() {
    for (const auto& channel : strategy_->dds.channels) {
        const std::string& topic = channel.topic;
        if (channel.originalFrameRate <=0 || channel.capturedFrameRate <=0)
        {
            return false;
        }

        int32_t forward_size = cache_mode_.forwardCaptureDurationSec * channel.capturedFrameRate;
        int32_t backward_size = cache_mode_.backwardCaptureDurationSec * channel.capturedFrameRate;

        auto forward_buf = std::make_unique<BufferType>(forward_size);
        if (!forward_buf) {
            AD_ERROR(RsclRecorder, "Create forward buffer failed for topic: %s", topic.c_str());
            return false;
        }

        auto backward_buf = std::make_unique<BufferType>(backward_size);
        if (!backward_buf) {
            AD_ERROR(RsclRecorder, "Create backward buffer failed for topic: %s", topic.c_str());
            return false;
        }

        forward_ringbuffers_[topic] = std::move(forward_buf);
        backward_ringbuffers_[topic] = std::move(backward_buf);
        AD_INFO(RsclRecorder, "Init buffer for topic: %s, forward size: %d, backward size: %d", topic.c_str(), forward_size, backward_size);
    }
    return true;
}

void RsclRecorder::onMessageReceived(const std::string& topic, const TRawMessagePtr& msg) {
    if (!msg) return;
    uint64_t message_timestamp = common::GetCurrentTimestampUs();
    // LOG_INFO("Received message on topic: %s, timestamp: %llu", topic.c_str(), message_timestamp);

    std::lock_guard<std::mutex> lock(buffer_mutex_);
    if (forward_ringbuffers_.count(topic)) {
        uint64_t forward_duration_us = cache_mode_.forwardCaptureDurationSec * 1000000ULL;
        while (!forward_ringbuffers_[topic]->empty() &&
               (message_timestamp - forward_ringbuffers_[topic]->front().timestamp) > forward_duration_us) {
            forward_ringbuffers_[topic]->pop_front();
        }
        forward_ringbuffers_[topic]->push_back(TimestampedData{msg, static_cast<uint64_t>(message_timestamp)});
    }

    if (is_triggered_ && backward_ringbuffers_.count(topic)) {
        uint64_t backward_duration_us = cache_mode_.backwardCaptureDurationSec * 1000000ULL;
        if ((message_timestamp - trigger_timestamp_) <= backward_duration_us) {
            backward_ringbuffers_[topic]->push_back(TimestampedData{msg, static_cast<uint64_t>(message_timestamp)});
        }
    }
}

void RsclRecorder::TriggerRecord(uint64_t triggerTimestamp, const std::string& outputfilePath) {
    if (is_triggered_) {
        AD_WARN(RsclRecorder, "Already triggered, ignore");
        // return;
    }

    is_triggered_ = true;
    trigger_timestamp_ = triggerTimestamp;
    AD_INFO(RsclRecorder, "Triggered at %llu, backward duration: %ds", trigger_timestamp_, cache_mode_.backwardCaptureDurationSec);

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
    
    // ==ASYNC==
    // future_ = std::async(std::launch::async, [this, outputfilePath]() {
    //     std::this_thread::sleep_for(std::chrono::seconds(cache_mode_.backwardCaptureDurationSec));
    //     {
    //         std::lock_guard<std::mutex> lock(buffer_mutex_);
    //         WriteBuffersToFile(outputfilePath);
    //         triggered_forward_buffers_.clear();
    //     }
    //     is_triggered_ = false;
    //     forward_capture_duration_us_ = 0;
    // });

    std::this_thread::sleep_for(std::chrono::seconds(cache_mode_.backwardCaptureDurationSec));
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        WriteBuffersToFile(outputfilePath);
        triggered_forward_buffers_.clear();
    }
    is_triggered_ = false;
    forward_capture_duration_us_ = 0;
}

bool RsclRecorder::WriteBuffersToFile(const std::string& outputfilePath) {
    uint64_t min_timestamp = UINT64_MAX;
    uint64_t max_timestamp = 0;

    if (!Open(OptMode::OPT_WRITE, outputfilePath)) {
        AD_ERROR(RsclRecorder, "Open file failed: %s", outputfilePath.c_str());
        return false;
    }

    uint64_t start_time = trigger_timestamp_ - (cache_mode_.forwardCaptureDurationSec * 1000000ULL); 
    uint64_t end_time = trigger_timestamp_ + (cache_mode_.backwardCaptureDurationSec * 1000000ULL); 

    for (const auto& channel : strategy_->dds.channels) {
        const std::string& topic = channel.topic;
        auto forward_it = triggered_forward_buffers_.find(topic);
        auto backward_it = backward_ringbuffers_.find(topic);
        auto current_forward_it = forward_ringbuffers_.find(topic);

        if (forward_it == triggered_forward_buffers_.end() && backward_it == backward_ringbuffers_.end()) {
            AD_WARN(RsclRecorder, "No buffer found for topic: %s", topic.c_str());
            continue;
        }

        size_t forward_count = 0;
        size_t backward_count = 0;

        // 写入前向数据
        std::unordered_set<uint64_t> written_timestamps;
        if (forward_it != triggered_forward_buffers_.end()) {
            for (const auto& data : forward_it->second) {
                if (data.timestamp <= trigger_timestamp_ && data.timestamp >= start_time) {
                    uint64_t new_timestamp = data.timestamp * 1000ULL;
                    min_timestamp = std::min(min_timestamp, data.timestamp);
                    max_timestamp = std::max(max_timestamp, data.timestamp);
                    Write(topic, new_timestamp, data.msg->Bytes(), data.msg->ByteSize());
                    written_timestamps.insert(data.timestamp);
                    forward_count++;
                }
            }
        }

        // 处理当前前向缓冲区中的数据
        if (current_forward_it != forward_ringbuffers_.end() && forward_it != triggered_forward_buffers_.end()) {
            const auto& current_buffer = current_forward_it->second;
            for (const auto& data : *current_buffer) {
                if (data.timestamp <= trigger_timestamp_ && data.timestamp >= start_time) {
                    if (written_timestamps.find(data.timestamp) == written_timestamps.end()) {
                        uint64_t new_timestamp = data.timestamp * 1000ULL;
                        min_timestamp = std::min(min_timestamp, data.timestamp);
                        max_timestamp = std::max(max_timestamp, data.timestamp);
                        Write(topic, new_timestamp, data.msg->Bytes(), data.msg->ByteSize());
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
                if (data.timestamp > trigger_timestamp_ && data.timestamp <= end_time) {
                    uint64_t new_timestamp = data.timestamp * 1000ULL;
                    min_timestamp = std::min(min_timestamp, data.timestamp);
                    max_timestamp = std::max(max_timestamp, data.timestamp);
                    Write(topic, new_timestamp, data.msg->Bytes(), data.msg->ByteSize());
                    backward_count++;
                }
            }
        }

        AD_INFO(RsclRecorder, "Topic %s: wrote %zu forward messages, %zu backward messages",
                 topic.c_str(), forward_count, backward_count);
    }

    double duration_seconds = (max_timestamp - min_timestamp) / 1e6;
    AD_INFO(RsclRecorder, "Total recording duration: %.3f seconds", duration_seconds);

    Close();
    AD_INFO(RsclRecorder, "Wrote all topics to file: %s", outputfilePath.c_str());
    return true;
}

bool RsclRecorder::Open(OptMode opt_mode, const std::string& full_path) {
    has_data_writen_ = false;
    if (IsOpened() || service_discovery_ == nullptr) {
        return false;
    }

    if (opt_mode == OptMode::OPT_READ) {
        reader_ = std::make_unique<senseAD::bag::BagReader>(full_path);
    } else if (opt_mode == OptMode::OPT_WRITE) {
        senseAD::bag::BagWriterAttribute writer_attr;
        writer_attr.flags.compress_mode = senseAD::bag::CompressionMode::kLz4;
        writer_ = std::make_unique<senseAD::bag::BagWriter>(full_path, writer_attr);
    } else {
        return false;
    }
    full_path_ = full_path;
    return true;
}

bool RsclRecorder::IsOpened() const {
    return reader_ != nullptr || writer_ != nullptr;
}

bool RsclRecorder::Write(const std::string& topic_name, uint64_t timestamp, const void* buf, size_t buf_len) {
    if (writer_ == nullptr) {
        return false;
    }

    if (service_discovery_ == nullptr) {
        return false;
    }

    if (!channel_infos_.count(topic_name)) {
        channel_infos_[topic_name] = senseAD::bag::ChannelInfo();
        channel_infos_[topic_name].channel_name = topic_name;
        channel_infos_[topic_name].write_channel_name = topic_name;
        senseAD::serde::MsgMeta meta;
        bool ok = senseAD::rscl::comm::hetero::ChannelFactory::Instance()->GetMsgMeta(topic_name, &meta);
        if (!ok && !service_discovery_->GetMsgMetabyTopic(topic_name, &meta)) {
            return false;
        } else {
            channel_infos_[topic_name].type = std::move(meta.msg_type);
            channel_infos_[topic_name].descriptor = std::move(meta.msg_descriptor);
        }
    }
    auto ret = writer_->AddSerializedMessage(timestamp, buf, buf_len, channel_infos_[topic_name]);
    if (ret) {
        has_data_writen_ = true;
    }
    return ret;
}

bool RsclRecorder::HasDataWriten() const {
    return has_data_writen_;
}

TBagInfo RsclRecorder::GetBagInfo() {
    if (reader_ == nullptr || !reader_->IsValid()) {
        return {};
    }
    TBagInfo info;
    info.bag_path = full_path_;
    info.bag_version = static_cast<int>(reader_->GetBagVersion());

    auto bag_flags = reader_->GetBagFlags();
    info.compress_method = bag_flags.CompressedMethod();
    info.is_header_time_mode = bag_flags.IsHeaderTimeMode();

    auto bag_header = reader_->GetBagHeader();
    info.start_time_ns = bag_header.begin_time;
    info.end_time_ns = bag_header.end_time;
    info.dropped_count = bag_header.dropped_count;
    info.channel_count = bag_header.channel_count;
    info.chunk_count = bag_header.chunk_count;

    auto channels = reader_->GetChannelList();
    for (const auto& channel : channels) {
        RsclChannelInfo channel_info;
        channel_info.channel_name = channel;
        channel_info.message_count = reader_->GetMessageNumber(channel);
        channel_info.message_type = reader_->GetMessageType(channel);
        channel_info.start_time_ns = reader_->GetRawChannelInfo(channel)->first_message_time;
        channel_info.end_time_ns = reader_->GetRawChannelInfo(channel)->last_message_time;
        info.channel_infos.emplace_back(channel_info);
    }
    return info;
}

bool RsclRecorder::ReadNextFrame(senseAD::bag::ReadedMessage* message) {
    if (reader_ == nullptr || !reader_->IsValid()) {
        return false;
    }
    return reader_->GetIterator().ReadNextMessage(message);
}

bool RsclRecorder::Close() {
    if (reader_ != nullptr) {
        reader_->Reset();
        reader_.reset(nullptr);
    }
    if (writer_ != nullptr) {
        writer_->Close();
        writer_.reset(nullptr);
    }
    return reader_ == nullptr && writer_ == nullptr;
}

} 
} 
