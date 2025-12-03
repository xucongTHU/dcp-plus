#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

#include "channel/observer.h"
#include "recorder/data_storage.h"
#include "trigger_engine/trigger_manager.h"
// #include "uploader/data_reporter.h"


namespace dcl {
namespace channel {

class ChannelManager : public Observer {
  public:
    ChannelManager() = default;
    virtual ~ChannelManager() = default;
    
    bool Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node, 
              const dcl::trigger::StrategyConfig& config,
              const std::shared_ptr<dcl::trigger::TriggerManager>& trigger_manager,
              const std::shared_ptr<dcl::recorder::RsclRecorder>& rscl_recorder);
    
    void AddObserver(std::shared_ptr<Observer> observer);
    void RemoveObserver(std::shared_ptr<Observer> observer);
    void Notify(const std::string& topic, const TRawMessagePtr& msg);

  private:
    bool InitSubscribers(); 
    bool InitObservers();
    void onMessageReceived(const std::string& topic, const TRawMessagePtr& msg) override;


    std::shared_ptr<senseAD::rscl::comm::Node> node_{nullptr};
    dcl::trigger::StrategyConfig strategy_config_;
    senseAD::rscl::comm::SubscriberBase::Ptr suber_;
    std::unordered_map<std::string, senseAD::rscl::comm::SubscriberBase::Ptr> subscribers_;
    std::unique_ptr<Subject> message_subject_;
    std::shared_ptr<dcl::recorder::RsclRecorder> rscl_recorder_{nullptr};
    std::shared_ptr<dcl::trigger::TriggerManager> trigger_manager_{nullptr};
};

} 
} 