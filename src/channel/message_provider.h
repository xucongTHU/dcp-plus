#pragma once

#include <atomic>
#include "common/base.h"
#include "channel/observer.h"
// #include "ad_rscl/common/message_print.h"
#include "ad_rscl/ad_rscl.h"

#include "ad_msg_idl/ad_vehicle/vehicle.capnp.h"
#include "ad_msg_idl/ad_planning/planning.capnp.h"

namespace dcl {
namespace channel {


class MessageProvider : public Observer {
public:
    explicit MessageProvider(const std::shared_ptr<senseAD::rscl::comm::Node>& node ):node_(node)
    {}
    virtual ~MessageProvider() = default;

    void OnMessageReceived(const std::string& topic, const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg) override;
    dcl::any getGear(){return static_cast<int32_t>(gear_.load());}
    dcl::any getVehicleState(){return static_cast<int32_t>(vehicle_state_.load());}
    dcl::any getAutoModeEnable() {return autoModeEnable_.load();}
    dcl::any getChassisVehicleMps() {return chassisVehicleMps_.load();}
    dcl::any getAebDecelReq() {return aebDecelReq_.load();}
    dcl::any getMcuDrvOverride() {return static_cast<int32_t>(mcuDrvOverride_.load());}


private:
    ///update every signals of rawmessage

    //1,vehicle.capnp
    //fetch  gear, brake,
    void updateVehicleInfo(const std::string& topic,
                           const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg);

    void updateGear(const senseAD::msg::vehicle::VehicleReport::Reader& report);    
    void updateAutoModeEnable(const senseAD::msg::vehicle::VehicleReport::Reader& report);
    void updateChassisVehicleMps(const senseAD::msg::vehicle::VehicleReport::Reader& report);

    void updatePlanningState(const std::string& topic,
                             const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg);

    void updateAebDecelReq(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg);

    void updateMcuDrvOverride(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg);

    ///update every signals of data structure

private:
    std::shared_ptr<senseAD::rscl::comm::Node> node_{nullptr};
    /// signals
    std::atomic<senseAD::msg::vehicle::GearCommand> gear_{senseAD::msg::vehicle::GearCommand::GEAR_NONE};
    std::atomic<senseAD::msg::planning::PlanningState::VehicleState> vehicle_state_{senseAD::msg::planning::PlanningState::VehicleState::DISACTIVE};
    std::atomic<bool> autoModeEnable_{false};
    std::atomic<double> chassisVehicleMps_{0.0};
    std::atomic<double> aebDecelReq_{0.0};
    std::atomic<senseAD::msg::planning::MCUStateMachineInfo::Override> mcuDrvOverride_{senseAD::msg::planning::MCUStateMachineInfo::Override::UNKOWN};
};

}
}
