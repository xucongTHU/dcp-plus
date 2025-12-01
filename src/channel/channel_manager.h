#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

#include "ad_rscl/ad_rscl.h"
#include "channel/observer.h"
// #include "strategy/StrategyConfig.h"
#include "recorder/data_storage.h"
// #include "trigger/base/TriggerFactory.hpp"
// #include "uploader/data_reporter.h"

using namespace dcl::data_storage;
using namespace dcl::trigger;
using namespace dcl::data_report;

namespace dcl {
namespace channel {

class ChannelManager : public Observer {
  public:
    ChannelManager() = default;
    virtual ~ChannelManager() = default;
    
    bool Init(const std::shared_ptr<senseAD::rscl::comm::Node>& node, 
              const strategy::StrategyConfig& config,
              const std::shared_ptr<TriggerFactory>& trigger_factory,
              const std::shared_ptr<RsclRecorder>& rscl_recorder,
              const std::shared_ptr<DataReporter>& data_reporter);
    
    void AddObserver(std::shared_ptr<Observer> observer);
    void RemoveObserver(std::shared_ptr<Observer> observer);
    void Notify(const std::string& topic, const TRawMessagePtr& msg);

  private:
    bool InitSubscribers(); 
    bool InitObservers();
    void OnMessageReceived(const std::string& topic, const TRawMessagePtr& msg) override;


    std::shared_ptr<senseAD::rscl::comm::Node> node_{nullptr};
    strategy::StrategyConfig strategy_config_;
    senseAD::rscl::comm::SubscriberBase::Ptr suber_;
    std::unordered_map<std::string, senseAD::rscl::comm::SubscriberBase::Ptr> subscribers_;
    std::unique_ptr<Subject> message_subject_;
    std::shared_ptr<RsclRecorder> rscl_recorder_{nullptr};
    std::shared_ptr<TriggerFactory> trigger_factory_{nullptr};
    std::shared_ptr<DataReporter> data_reporter_{nullptr};
};

} 
} 