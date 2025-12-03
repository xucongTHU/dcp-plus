#include "channel_manager.h"
#include "common/log/logger.h"

namespace dcl {
namespace channel {

bool ChannelManager::Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node, 
                          const dcl::trigger::StrategyConfig& config,
                          const std::shared_ptr<dcl::trigger::TriggerManager>& trigger_manager,
                          const std::shared_ptr<dcl::recorder::RsclRecorder>& rscl_recorder) {
    node_ = node;
    strategy_config_ = config;
    trigger_manager_ = trigger_manager;
    rscl_recorder_ = rscl_recorder;

    message_subject_ = std::make_unique<Subject>();

    bool ret = InitSubscribers();
    CHECK_AND_RETURN(ret, ChannelManager, "InitSubscribers failed", false);

    ret = InitObservers();
    CHECK_AND_RETURN(ret, ChannelManager, "InitObservers failed", false);

    return ret;
}

bool ChannelManager::InitSubscribers() {
    senseAD::rscl::idl::SubscriberConf conf;
    conf.mutable_qos_profile()->set_depth(20); 
    conf.mutable_qos_profile()->set_reliability(
        senseAD::rscl::idl::QosReliabilityPolicy::RELIABILITY_BEST_EFFORT);

    for (const auto& strategy : strategy_config_.strategies) {
        if (!strategy.trigger.enabled)  continue;
        for (const auto& channel : strategy.dds.channels) {
            std::string topic = channel.topic;
            if (subscribers_.find(topic) != subscribers_.end()) {
                continue;
            }

            auto subscriber = node_->CreateSubscriber<senseAD::rscl::comm::RawMessage>(
                topic,
                [this, topic](const TRawMessagePtr& raw_message) {
                    this->Notify(topic, raw_message);
                },
                conf
            );

            if (!subscriber) {
                AD_ERROR(ChannelManager, "Create subscriber failed for topic: %s", topic.c_str());
                return false;
            }
            AD_INFO(ChannelManager, "Init subscriber for topic: %s, node: %p, subscriber: %p", topic.c_str(), node_.get(), subscriber.get());
            subscribers_[topic] = subscriber;
        }
    }
    return true;

}

bool ChannelManager::InitObservers() {
    if (rscl_recorder_) {
        AddObserver(rscl_recorder_);
        AD_INFO(ChannelManager, "Added RsclRecorder as observer");
    }


    if (trigger_manager_) {
        for (const auto& strategy : strategy_config_.strategies) {
            auto trigger = trigger_manager_->getTrigger(strategy.trigger.triggerId);
            if (trigger) {
                AddObserver(trigger);
                AD_INFO(ChannelManager, "Added %s as observer", strategy.trigger.triggerId.c_str());
            }

        }
    }

    AD_INFO(ChannelManager, "InitObservers ok");
    return true;
}

void ChannelManager::onMessageReceived(const std::string& topic, const TRawMessagePtr& msg) {
    auto cur_clock_mode = senseAD::base::time::ClockMode::SYSTEM_TIME;
    uint64_t message_time = senseAD::base::time::Time::Now(&cur_clock_mode).ToMicrosecond();
    auto header = msg->Header();
    if (header.is_enabled)
    {
        // message_time = msg->Header().stamp;
    }
    else
    {
        AD_ERROR(ChannelManager, "OnMessageReceived, topic: %s, header parse error", topic.c_str());
        return;
    }

    // in start of replay mode rscl timestamp is zero
    if (message_time == 0) {
        AD_ERROR(ChannelManager, "OnMessageReceived, topic: %s, message_time is zero", topic.c_str());
        return;
    }
}

void ChannelManager::AddObserver(std::shared_ptr<Observer> observer) {
    message_subject_->addObserver(observer);
}

void ChannelManager::RemoveObserver(std::shared_ptr<Observer> observer) {
    message_subject_->removeObserver(observer);
}

void ChannelManager::Notify(const std::string& topic, const TRawMessagePtr& msg) {
    message_subject_->notifyAll(topic, msg);
}


} 
} 