//
// Copyright (c) 2025 T3CAIC. All rights reserved.
// Tsung Xu<xucong@t3caic.com>
//

#ifndef RSCL_RECORDER_H
#define RSCL_RECORDER_H

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <vector>
#include <future>
#include <unordered_set>
#include "common/ringBuffer.h"
#include "trigger_engine/strategy_config.h"
#include "channel/observer.h"

#include "ad_rscl/ad_rscl.h"
#include "ad_bag/bag_writer.h"
#include "ad_bag/bag_reader.h"
#include "ad_bag/bag_flags.h"

namespace dcl {
namespace recorder {

struct RsclChannelInfo {
    uint64_t message_count;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    std::string message_type;
    std::string channel_name;
};

struct TBagInfo {
    bool is_header_time_mode;
    int bag_version;
    uint32_t channel_count;
    uint32_t chunk_count;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    uint64_t dropped_count;
    std::string bag_path;
    std::string compress_method;
    std::vector<RsclChannelInfo> channel_infos;
};

enum class OptMode {
    OPT_READ,
    OPT_WRITE,
};

using TRawMessagePtr = std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>;

class RsclRecorder : public Observer {
  public:
    RsclRecorder(const std::shared_ptr<rscl::Node>& node, const std::shared_ptr<dcl::trigger::Strategy>& s);
    ~RsclRecorder() = default;

    bool Open(OptMode opt_mode, const std::string& full_path);
    bool IsOpened() const;
    bool Write(const std::string& topic_name, uint64_t timestamp, const void* buf, size_t buf_len);
    TBagInfo GetBagInfo();
    bool ReadNextFrame(senseAD::bag::ReadedMessage* message);
    bool Close();
    bool HasDataWriten() const;

    bool Init();
    void TriggerRecord(uint64_t triggerTimestamp, const std::string& outputfilePath);

  private:
    bool InitRingBuffers();
    void onMessageReceived(const std::string& topic, const TRawMessagePtr& msg) override;
    bool WriteBuffersToFile(const std::string& outputfilePath);

  private: 
    std::shared_ptr<rscl::Node> node_;
    std::unique_ptr<senseAD::bag::BagReader> reader_;
    std::unique_ptr<senseAD::bag::BagWriter> writer_; 
    senseAD::service_discovery::ServiceDiscovery* service_discovery_;

    bool has_data_writen_{false};
    std::string full_path_;
    std::unordered_map<std::string, senseAD::bag::ChannelInfo> channel_infos_;
    std::shared_ptr<dcl::trigger::Strategy> strategy_{nullptr};
    dcl::trigger::CacheMode cache_mode_;

    struct TimestampedData {
        TRawMessagePtr msg;
        uint64_t timestamp;
    };
    using BufferType = common::RingBuffer<TimestampedData>;
    std::unordered_map<std::string, std::unique_ptr<BufferType>> forward_ringbuffers_;
    std::unordered_map<std::string, std::unique_ptr<BufferType>> backward_ringbuffers_;
    std::unordered_map<std::string, std::vector<TimestampedData>> triggered_forward_buffers_;
    uint64_t forward_capture_duration_us_{0};

    std::atomic<bool> is_triggered_{false};
    uint64_t trigger_timestamp_{0};
    std::mutex buffer_mutex_;

    // std::future<void> future_;
};

}
}

#endif //RSCL_RECORDER_H
