#include "channel_manager.h"
#include "common/log/logger.h"

namespace dcp::channel{

bool ChannelManager::Init(const std::shared_ptr<rclcpp::Node>& node,
                          const dcp::trigger::StrategyConfig& config,
                          const std::shared_ptr<dcp::trigger::TriggerManager>& trigger_manager)
{

    node_ = node;
    strategy_config_ = config;
    trigger_manager_ = trigger_manager;
    // rscl_recorder_ = rscl_recorder;

    message_subject_ = std::make_unique<Subject>();

    bool ret = InitSubscribers();
    CHECK_AND_RETURN(ret, ChannelManager, "InitSubscribers failed", false);

    ret = InitObservers();
    CHECK_AND_RETURN(ret, ChannelManager, "InitObservers failed", false);

    return ret;
}

bool ChannelManager::InitSubscribers() {
    for (const auto& strategy : strategy_config_.strategies) {
        if (!strategy.trigger.enabled)  continue;
        for (const auto& channel : strategy.dds.channels) {
            std::string topic = channel.topic;
            if (subscribers_.find(topic) != subscribers_.end()) {
                continue;
            }

            std::string message_type;
            auto callback = [this, topic](const std::shared_ptr<rclcpp::SerializedMessage>& msg) {
                this->Notify(topic, *msg);
            };

            auto subscriber = node_->create_generic_subscription(
                topic,
                message_type,
                rclcpp::QoS(10),
                callback
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
    // if (rscl_recorder_) {
    //     AddObserver(rscl_recorder_);
    //     AD_INFO(ChannelManager, "Added RsclRecorder as observer");
    // }

    if (trigger_manager_) {
        for (const auto& strategy : strategy_config_.strategies) {
            if (auto trigger = trigger_manager_->getTrigger(strategy.trigger.triggerId)) {
                AddObserver(trigger);
                AD_INFO(ChannelManager, "Added %s as observer", strategy.trigger.triggerId.c_str());
            }

        }
    }

    AD_INFO(ChannelManager, "InitObservers ok");
    return true;
}

void ChannelManager::OnMessageReceived(const std::string& topic, const rclcpp::SerializedMessage& msg) {
    // 实现消息处理逻辑
    AD_WARN(ChannelManager, "Received message on topic: %s", topic.c_str());
}

void ChannelManager::AddObserver(const std::shared_ptr<Observer>& observer) const
{
    if (message_subject_) {
        message_subject_->addObserver(observer);
    }
}

void ChannelManager::RemoveObserver(const std::shared_ptr<Observer>& observer) const
{
    if (message_subject_) {
        message_subject_->removeObserver(observer);
    }
}

void ChannelManager::Notify(const std::string& topic, const rclcpp::SerializedMessage& msg) const
{
    if (message_subject_) {
        message_subject_->notifyAll(topic, msg);
    }
}


}