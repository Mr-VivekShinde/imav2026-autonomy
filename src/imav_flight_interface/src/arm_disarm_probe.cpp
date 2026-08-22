#include <chrono>
#include <cstdint>
#include <memory>

#include "rclcpp/rclcpp.hpp"

#include "px4_msgs/msg/vehicle_command.hpp"
#include "px4_msgs/msg/vehicle_command_ack.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"

using namespace std::chrono_literals;

class ArmDisarmProbe : public rclcpp::Node
{
public:
  ArmDisarmProbe()
  : Node("arm_disarm_probe")
  {
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10));
    qos.best_effort();
    qos.durability_volatile();

    command_pub_ =
      this->create_publisher<px4_msgs::msg::VehicleCommand>(
        "/fmu/in/vehicle_command", qos);

    status_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleStatus>(
        "/fmu/out/vehicle_status_v1",
        qos,
        [this](const px4_msgs::msg::VehicleStatus::SharedPtr msg)
        {
          arming_state_ = msg->arming_state;
          status_received_ = true;
        });

    ack_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleCommandAck>(
        "/fmu/out/vehicle_command_ack",
        qos,
        [this](const px4_msgs::msg::VehicleCommandAck::SharedPtr msg)
        {
          if (msg->command ==
              px4_msgs::msg::VehicleCommand::VEHICLE_CMD_COMPONENT_ARM_DISARM)
          {
            RCLCPP_INFO(
              this->get_logger(),
              "ACK | command=%u result=%u",
              static_cast<unsigned>(msg->command),
              static_cast<unsigned>(msg->result));
          }
        });

    timer_ =
      this->create_wall_timer(
        500ms,
        std::bind(&ArmDisarmProbe::step, this));

    RCLCPP_WARN(
      this->get_logger(),
      "SIMULATION TEST ONLY: ARM briefly, verify status, then DISARM. No takeoff command.");
  }

private:
  enum class State
  {
    WAIT_STATUS,
    SEND_ARM,
    WAIT_ARMED,
    HOLD_ARMED,
    SEND_DISARM,
    WAIT_DISARMED,
    DONE
  };

  void send_arm_disarm(bool arm)
  {
    px4_msgs::msg::VehicleCommand msg{};

    msg.timestamp =
      static_cast<uint64_t>(this->now().nanoseconds() / 1000);

    msg.param1 = arm ? 1.0F : 0.0F;

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
      arm ? "ARM command sent." : "DISARM command sent.");
  }

  void step()
  {
    switch (state_)
    {
      case State::WAIT_STATUS:
        if (status_received_) {
          state_ = State::SEND_ARM;
        }
        break;

      case State::SEND_ARM:
        send_arm_disarm(true);
        state_ = State::WAIT_ARMED;
        wait_counter_ = 0;
        break;

      case State::WAIT_ARMED:
        if (arming_state_ ==
            px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "PX4 STATUS CONFIRMED: ARMED");

          state_ = State::HOLD_ARMED;
          wait_counter_ = 0;
        }
        else if (++wait_counter_ > 10)
        {
          RCLCPP_ERROR(
            this->get_logger(),
            "ARM was not confirmed. Sending DISARM for safety.");

          state_ = State::SEND_DISARM;
        }
        break;

      case State::HOLD_ARMED:
        if (++wait_counter_ >= 4)
        {
          state_ = State::SEND_DISARM;
        }
        break;

      case State::SEND_DISARM:
        send_arm_disarm(false);
        state_ = State::WAIT_DISARMED;
        wait_counter_ = 0;
        break;

      case State::WAIT_DISARMED:
        if (arming_state_ ==
            px4_msgs::msg::VehicleStatus::ARMING_STATE_DISARMED)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "PX4 STATUS CONFIRMED: DISARMED");

          state_ = State::DONE;
        }
        else if (++wait_counter_ > 10)
        {
          RCLCPP_ERROR(
            this->get_logger(),
            "DISARM confirmation timeout.");
        }
        break;

      case State::DONE:
        break;
    }
  }

  rclcpp::Publisher<
    px4_msgs::msg::VehicleCommand>::SharedPtr command_pub_;

  rclcpp::Subscription<
    px4_msgs::msg::VehicleStatus>::SharedPtr status_sub_;

  rclcpp::Subscription<
    px4_msgs::msg::VehicleCommandAck>::SharedPtr ack_sub_;

  rclcpp::TimerBase::SharedPtr timer_;

  State state_{State::WAIT_STATUS};

  uint8_t arming_state_{0};
  bool status_received_{false};

  int wait_counter_{0};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ArmDisarmProbe>());
  rclcpp::shutdown();
  return 0;
}
