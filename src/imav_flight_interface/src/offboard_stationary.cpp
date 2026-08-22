#include <chrono>
#include <cstdint>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "px4_msgs/msg/offboard_control_mode.hpp"
#include "px4_msgs/msg/trajectory_setpoint.hpp"

using namespace std::chrono_literals;

class OffboardStationary : public rclcpp::Node
{
public:
  OffboardStationary()
  : Node("offboard_stationary")
  {
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10));
    qos.best_effort();
    qos.durability_volatile();

    offboard_pub_ =
      this->create_publisher<px4_msgs::msg::OffboardControlMode>(
        "/fmu/in/offboard_control_mode", qos);

    trajectory_pub_ =
      this->create_publisher<px4_msgs::msg::TrajectorySetpoint>(
        "/fmu/in/trajectory_setpoint", qos);

    timer_ =
      this->create_wall_timer(
        100ms,
        std::bind(&OffboardStationary::publish_stream, this));

    RCLCPP_WARN(
      this->get_logger(),
      "DISARMED TEST ONLY: publishing heartbeat + stationary setpoint. No VehicleCommand is sent.");
  }

private:
  void publish_stream()
  {
    const uint64_t timestamp =
      static_cast<uint64_t>(this->now().nanoseconds() / 1000);

    px4_msgs::msg::OffboardControlMode control{};
    control.timestamp = timestamp;
    control.position = true;

    offboard_pub_->publish(control);

    px4_msgs::msg::TrajectorySetpoint setpoint{};
    setpoint.timestamp = timestamp;

    // PX4 local coordinates use NED:
    // X = North
    // Y = East
    // Z = Down
    //
    // This stationary test requests the local origin:
    // X = 0 m
    // Y = 0 m
    // Z = 0 m
    //
    // The vehicle remains DISARMED, so this does not make it fly.
    setpoint.position[0] = 0.0F;
    setpoint.position[1] = 0.0F;
    setpoint.position[2] = 0.0F;
    setpoint.yaw = 0.0F;

    trajectory_pub_->publish(setpoint);

    stream_count_++;

    if ((stream_count_ % 10) == 0) {
      RCLCPP_INFO(
        this->get_logger(),
        "Stationary Offboard stream alive: %lu messages",
        static_cast<unsigned long>(stream_count_));
    }
  }

  rclcpp::Publisher<
    px4_msgs::msg::OffboardControlMode>::SharedPtr offboard_pub_;

  rclcpp::Publisher<
    px4_msgs::msg::TrajectorySetpoint>::SharedPtr trajectory_pub_;

  rclcpp::TimerBase::SharedPtr timer_;

  uint64_t stream_count_{0};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<OffboardStationary>());
  rclcpp::shutdown();
  return 0;
}
