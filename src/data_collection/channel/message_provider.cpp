#include "channel/message_provider.h"
#include "common/log/logger.h"
#include "common/base.h"

namespace dcp::channel{

void MessageProvider::OnMessageReceived(const std::string& topic, const rclcpp::SerializedMessage& msg)
{
    ///TODO
    if(topic == "/canbus/vehicle_report")
    {
        updateVehicleInfo(topic, msg);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    } else if (topic == "/decision_planning/planning_state") {
        // updatePlanningState(topic, idl);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    }
    else if (topic == "/mcu/vehicle_processing") {
        // updateAebDecelReq(idl);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    }
    else if (topic == "/mcu/state_machine") {
        // updateMcuDrvOverride(idl);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    }
}

void MessageProvider::updateVehicleInfo(const std::string& topic, const rclcpp::SerializedMessage& msg)
{
    data_collection::msg::JointCommand joint_cmd;
    rclcpp::Serialization<data_collection::msg::JointCommand> serialization;
    serialization.deserialize_message(&msg, &joint_cmd);
    updateJointCmd(joint_cmd);
}

void MessageProvider::updateJointCmd(const data_collection::msg::JointCommand& joint_cmd)
{
    AD_INFO(MessageProvider, "joint_cmd : %d", joint_cmd.position[0]);
}
//
// void MessageProvider::updateGear(const senseAD::idl::vehicle::VehicleReport::Reader& report)
// {
//     gear_.store(report.getGear().getActual());
//     AD_INFO(MessageProvider, "gear : %d", static_cast<int32_t>(gear_.load()));
// }
//
// void MessageProvider::updateAutoModeEnable(const senseAD::idl::vehicle::VehicleReport::Reader& report)
// {
//     autoModeEnable_.store(report.getMode().getEnable());
//     AD_INFO(MessageProvider, "autoModeEnable_ : %d", autoModeEnable_.load());
// }
//
// void MessageProvider::updateChassisVehicleMps(const senseAD::idl::vehicle::VehicleReport::Reader& report)
// {
//     chassisVehicleMps_.store(report.getChassis().getVehicleMps());
//     AD_INFO(MessageProvider, "chassisVehicleMps_ : %f", chassisVehicleMps_.load());
// }
//
// void MessageProvider::updatePlanningState(const std::string& topic,
//                                           const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& idl)
// {
//     kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
//                                              (idl->Bytes()), idl->ByteSize());
//     ::capnp::FlatArrayMessageReader reader(vi_kjarr);
//
//     auto planning_state = reader.getRoot<senseAD::idl::planning::PlanningState>();
//
//     // senseAD::rscl::common::dynamicPrintValue(planning_state);
//     vehicle_state_.store(planning_state.getVehicleState());
//     AD_INFO(MessageProvider, "vehicle_state : %d", static_cast<int32_t>(vehicle_state_.load()));
// }
//
// void MessageProvider::updateAebDecelReq(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& idl)
// {
//     kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
//                                              (idl->Bytes()), idl->ByteSize());
//     ::capnp::FlatArrayMessageReader reader(vi_kjarr);
//
//     auto vehicleProcessing = reader.getRoot<senseAD::idl::planning::Vehicleprocessing>();
//
//     aebDecelReq_.store(vehicleProcessing.getAebDecelReq());
//     AD_INFO(MessageProvider, "aebDecelReq_ : %f", aebDecelReq_.load());
// }
//
// void MessageProvider::updateMcuDrvOverride(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& idl) {
//     kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
//                                              (idl->Bytes()), idl->ByteSize());
//     ::capnp::FlatArrayMessageReader reader(vi_kjarr);
//
//     auto mcuStateMachineInfo = reader.getRoot<senseAD::idl::planning::MCUStateMachineInfo>();
//
//     mcuDrvOverride_.store(mcuStateMachineInfo.getMcuDrvOverride());
//     AD_INFO(MessageProvider, "mcuDrvOverride_ : %d", static_cast<int32_t>(mcuDrvOverride_.load()));
// }

}
