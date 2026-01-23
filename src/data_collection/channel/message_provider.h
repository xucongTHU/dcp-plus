#pragma once

#include <atomic>
#include "common/base.h"
#include "channel/observer.h"
// #include "ad_rscl/common/message_print.h"
// #include "ad_rscl/ad_rscl.h"
//
// #include "ad_msg_idl/ad_vehicle/vehicle.capnp.h"
// #include "ad_msg_idl/ad_planning/planning.capnp.h"
#include "data_collection/msg/joint_command.hpp"

namespace dcp::channel{

class MessageProvider : public Observer {
public:
    explicit MessageProvider(const std::shared_ptr<rclcpp::Node>& node ):node_(node)
    {}
    virtual ~MessageProvider() = default;

    void OnMessageReceived(const std::string& topic, const rclcpp::SerializedMessage& msg) override;
    // dcp::any getGear(){return static_cast<int32_t>(gear_.load());}
    // dcp::any getVehicleState(){return static_cast<int32_t>(vehicle_state_.load());}
    // dcp::any getAutoModeEnable() {return autoModeEnable_.load();}
    // dcp::any getChassisVehicleMps() {return chassisVehicleMps_.load();}
    // dcp::any getAebDecelReq() {return aebDecelReq_.load();}
    // dcp::any getMcuDrvOverride() {return static_cast<int32_t>(mcuDrvOverride_.load());}


private:
    ///update every signals of rawmessage

    //1,vehicle.capnp
    // fetch  gear, brake,
    void updateVehicleInfo(const std::string& topic,
                           const rclcpp::SerializedMessage& msg);

    void updateJointCmd(const data_collection::msg::JointCommand& msg);
    //
    // void updateGear(const senseAD::idl::vehicle::VehicleReport::Reader& report);
    // void updateAutoModeEnable(const senseAD::idl::vehicle::VehicleReport::Reader& report);
    // void updateChassisVehicleMps(const & report);
    //
    // void updatePlanningState(const std::string& topic,
    //                          const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& idl);
    //
    // void updateAebDecelReq(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& idl);
    //
    // void updateMcuDrvOverride(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& idl);

    ///update every signals of data structure

private:
    std::shared_ptr<rclcpp::Node> node_{nullptr};
    /// signals
    // std::atomic<senseAD::idl::vehicle::GearCommand> gear_{senseAD::idl::vehicle::GearCommand::GEAR_NONE};
    // std::atomic<senseAD::idl::planning::PlanningState::VehicleState> vehicle_state_{senseAD::idl::planning::PlanningState::VehicleState::DISACTIVE};
    // std::atomic<bool> autoModeEnable_{false};
    // std::atomic<double> chassisVehicleMps_{0.0};
    // std::atomic<double> aebDecelReq_{0.0};
    // std::atomic<senseAD::idl::planning::MCUStateMachineInfo::Override> mcuDrvOverride_{senseAD::idl::planning::MCUStateMachineInfo::Override::UNKOWN};
};

}
