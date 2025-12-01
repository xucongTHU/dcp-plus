#include "channel/message_provider.h"
#include "common/log/logger.h"
#include "common/base.h"

namespace dcl {
namespace channel {

void MessageProvider::onMessageReceived(const std::string& topic,
            const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg)
{
    ///TODO
    if(topic == "/canbus/vehicle_report")
    {
        updateVehicleInfo(topic, msg);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    } else if (topic == "/decision_planning/planning_state") {
        updatePlanningState(topic, msg);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    }
    else if (topic == "/mcu/vehicle_processing") {
        updateAebDecelReq(msg);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    }
    else if (topic == "/mcu/state_machine") {
        updateMcuDrvOverride(msg);
        AD_INFO(MessageProvider, "Observed topic: %s", topic.c_str());
    }
}

void MessageProvider::updateVehicleInfo(const std::string& topic,
            const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg)
{
    kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
            (msg->Bytes()), msg->ByteSize());
    ::capnp::FlatArrayMessageReader reader(vi_kjarr);

    // auto schema = capnp::Schema::from<senseAD::msg::vehicle::VehicleReport>();
    // auto vehicleReportReader = reader.getRoot<capnp::DynamicStruct>(schema);
    auto vehicleReportReader = reader.getRoot<senseAD::msg::vehicle::VehicleReport>();

    // senseAD::rscl::common::dynamicPrintValue(vehicleReportReader);
    updateGear(vehicleReportReader);
    updateAutoModeEnable(vehicleReportReader);
    updateChassisVehicleMps(vehicleReportReader);
}

void MessageProvider::updateGear(const senseAD::msg::vehicle::VehicleReport::Reader& report)
{
    gear_.store(report.getGear().getActual());
    AD_INFO(MessageProvider, "gear : %d", static_cast<int32_t>(gear_.load()));
}

void MessageProvider::updateAutoModeEnable(const senseAD::msg::vehicle::VehicleReport::Reader& report) 
{
    autoModeEnable_.store(report.getMode().getEnable());
    AD_INFO(MessageProvider, "autoModeEnable_ : %d", autoModeEnable_.load());
}

void MessageProvider::updateChassisVehicleMps(const senseAD::msg::vehicle::VehicleReport::Reader& report)
{
    chassisVehicleMps_.store(report.getChassis().getVehicleMps());
    AD_INFO(MessageProvider, "chassisVehicleMps_ : %f", chassisVehicleMps_.load());
}

void MessageProvider::updatePlanningState(const std::string& topic,
    const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg)
{
    kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
            (msg->Bytes()), msg->ByteSize());
    ::capnp::FlatArrayMessageReader reader(vi_kjarr);

    auto planning_state = reader.getRoot<senseAD::msg::planning::PlanningState>();

    // senseAD::rscl::common::dynamicPrintValue(planning_state);
    vehicle_state_.store(planning_state.getVehicleState());
    AD_INFO(MessageProvider, "vehicle_state : %d", static_cast<int32_t>(vehicle_state_.load()));
}

void MessageProvider::updateAebDecelReq(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg)
{
    kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
        (msg->Bytes()), msg->ByteSize());
    ::capnp::FlatArrayMessageReader reader(vi_kjarr);

    auto vehicleProcessing = reader.getRoot<senseAD::msg::planning::Vehicleprocessing>();

    aebDecelReq_.store(vehicleProcessing.getAebDecelReq());
    AD_INFO(MessageProvider, "aebDecelReq_ : %f", aebDecelReq_.load());
}

void MessageProvider::updateMcuDrvOverride(const std::shared_ptr<ReceivedMsg<senseAD::rscl::comm::RawMessage>>& msg) {
    kj::ArrayPtr<const capnp::word> vi_kjarr(reinterpret_cast<const capnp::word*>
        (msg->Bytes()), msg->ByteSize());
    ::capnp::FlatArrayMessageReader reader(vi_kjarr);

    auto mcuStateMachineInfo = reader.getRoot<senseAD::msg::planning::MCUStateMachineInfo>();

    mcuDrvOverride_.store(mcuStateMachineInfo.getMcuDrvOverride());
    AD_INFO(MessageProvider, "mcuDrvOverride_ : %d", static_cast<int32_t>(mcuDrvOverride_.load()));
}

}
}
