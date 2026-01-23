#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <mutex>

#include "observer.h"
#include "recorder/data_storage.h"
#include "trigger/trigger_manager.h"
// #include "../uploader/data_reporter.h"

namespace dcp::channel {
class ChannelManager : public Observer {
public:
    ChannelManager() = default;
    ~ChannelManager() override = default;

    // bool Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node,
    //           const dcp::trigger::StrategyConfig& config,
    //           const std::shared_ptr<dcp::trigger::TriggerManager>& trigger_manager,
    //           const std::shared_ptr<dcp::recorder::RsclRecorder>& rscl_recorder);
    bool Init(const std::shared_ptr<rclcpp::Node>& node,
              const dcp::trigger::StrategyConfig& config,
              const std::shared_ptr<dcp::trigger::TriggerManager>& trigger_manager);

    void AddObserver(const std::shared_ptr<Observer>& observer) const;
    void RemoveObserver(const std::shared_ptr<Observer>& observer) const;
    void Notify(const std::string& topic, const rclcpp::SerializedMessage& msg) const;
    // void Notify(const std::string& topic, const TRawMessagePtr& idl);

private:
    bool InitSubscribers();
    bool InitObservers();
    void OnMessageReceived(const std::string& topic, const rclcpp::SerializedMessage& msg) override;
    // void OnMessageReceived(const std::string& topic, const TRawMessagePtr& idl) override;

    std::shared_ptr<rclcpp::Node> node_;
    std::map<std::string, rclcpp::GenericSubscription::SharedPtr> subscribers_;
    trigger::StrategyConfig strategy_config_;
    std::shared_ptr<trigger::TriggerManager> trigger_manager_{nullptr};
    std::unique_ptr<Subject> message_subject_;

    // std::shared_ptr<senseAD::rscl::comm::Node> node_{nullptr};
    // senseAD::rscl::comm::SubscriberBase::Ptr suber_;
    // std::unordered_map<std::string, senseAD::rscl::comm::SubscriberBase::Ptr> subscribers_;
    // std::shared_ptr<dcp::recorder::RsclRecorder> rscl_recorder_{nullptr};

};
}