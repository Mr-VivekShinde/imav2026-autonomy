#include <chrono>
#include <cstdint>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "px4_msgs/msg/vehicle_command.hpp"
#include "px4_msgs/msg/vehicle_command_ack.hpp"

using namespace std::chrono_literals;

class VehicleCommandProbe : public rclcpp::Node
{
public:
  VehicleCommandProbe()
  : Node("vehicle_command_probe")
  {
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10));
    qos.best_effort();
    qos.durability_volatile();

    command_pub_ =
      this->create_publisher<px4_msgs::msg::VehicleCommand>(
        "/fmu/in/vehicle_command",
        qos);

    ack_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleCommandAck>(
        "/fmu/out/vehicle_command_ack",
        qos,
        [this](const px4_msgs::msg::VehicleCommandAck::SharedPtr msg)
        {
          if (msg->command !=
              px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM)
          {
            return;
          }

          ack_received_ = true;

          RCLCPP_INFO(
            this->get_logger(),
            "ACK RECEIVED | command=%u result=%u",
            static_cast<unsigned int>(msg->command),
            static_cast<unsigned int>(msg->result));
        });

    send_timer_ =
      this->create_wall_timer(
        2s,
        [this]()
        {
          if (command_sent_) {
            return;
          }

          send_safe_disarm_command();
          command_sent_ = true;
        });

    RCLCPP_WARN(
      this->get_logger(),
      "SAFE COMMAND TEST: sends DISARM only. Never ARM or TAKEOFF.");
  }

private:
  void send_safe_disarm_command()
  {
    px4_msgs::msg::VehicleCommand msg{};

    msg.timestamp =
      static_cast<uint64_t>(this->now().nanoseconds() / 1000);

    // param1 = 0 means DISARM.
    msg.param1 = 0.0F;

    msg.command =
      px4_msgs::msg::VehicleCommand::
        VEHICLE_CMD_COMPONENT_ARM_DISARM;

    msg.target_system = 1;
    msg.target_component = 1;
    msg.source_system = 1;
    msg.source_component = 1;
    msg.from_external = true;

    command_pub_->publish(msg);

    RCLCPP_WARN(
      this->get_logger(),
      "SAFE DISARM VehicleCommand published once.");
  }

  rclcpp::Publisher<
    px4_msgs::msg::VehicleCommand>::SharedPtr command_pub_;

  rclcpp::Subscription<
    px4_msgs::msg::VehicleCommandAck>::SharedPtr ack_sub_;

  rclcpp::TimerBase::SharedPtr send_timer_;

  bool command_sent_{false};
  bool ack_received_{false};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VehicleCommandProbe>());
  rclcpp::shutdown();
  return 0;
}
