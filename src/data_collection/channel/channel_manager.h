#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

#include "observer.h"
#include "recorder/data_storage.h"
#include "trigger/trigger_manager.h"
// #include "../uploader/data_reporter.h"


namespace dcp {
namespace channel {

class ChannelManager : public Observer {
  public:
    ChannelManager() = default;
    virtual ~ChannelManager() = default;
    
    // bool Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node,
    //           const dcp::trigger::StrategyConfig& config,
    //           const std::shared_ptr<dcp::trigger::TriggerManager>& trigger_manager,
    //           const std::shared_ptr<dcp::recorder::RsclRecorder>& rscl_recorder);
    bool Init(const std::shared_ptr<rclcpp::Node>& node,
              const dcp::trigger::StrategyConfig& config,
              const std::shared_ptr<dcp::trigger::TriggerManager>& trigger_manager);
    
    void AddObserver(std::shared_ptr<Observer> observer);
    void RemoveObserver(std::shared_ptr<Observer> observer);
    void Notify(const std::string& topic, const rclcpp::SerializedMessage& msg);
    // void Notify(const std::string& topic, const TRawMessagePtr& msg);

  private:
    bool InitSubscribers(); 
    bool InitObservers();
    void OnMessageReceived(const std::string& topic, const rclcpp::SerializedMessage& msg) override;
    // void OnMessageReceived(const std::string& topic, const TRawMessagePtr& msg) override;

    std::shared_ptr<rclcpp::Node> node_;
    rclcpp::SubscriptionBase::SharedPtr suber_;
    std::unordered_map<std::string, rclcpp::SubscriptionBase::SharedPtr> subscribers_;
    trigger::StrategyConfig strategy_config_;
    std::shared_ptr<trigger::TriggerManager> trigger_manager_{nullptr};
    std::unique_ptr<Subject> message_subject_;

    // std::shared_ptr<senseAD::rscl::comm::Node> node_{nullptr};
    // senseAD::rscl::comm::SubscriberBase::Ptr suber_;
    // std::unordered_map<std::string, senseAD::rscl::comm::SubscriberBase::Ptr> subscribers_;
    // std::shared_ptr<dcp::recorder::RsclRecorder> rscl_recorder_{nullptr};

};

} 
} 