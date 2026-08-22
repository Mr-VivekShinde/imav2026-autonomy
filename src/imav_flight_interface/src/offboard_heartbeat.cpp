#include <chrono>
#include <cstdint>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "px4_msgs/msg/offboard_control_mode.hpp"

using namespace std::chrono_literals;

class OffboardHeartbeat : public rclcpp::Node
{
public:
  OffboardHeartbeat()
  : Node("offboard_heartbeat")
  {
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10));
    qos.best_effort();
    qos.durability_volatile();

    publisher_ =
      this->create_publisher<px4_msgs::msg::OffboardControlMode>(
        "/fmu/in/offboard_control_mode",
        qos);

    timer_ =
      this->create_wall_timer(
        100ms,
        std::bind(&OffboardHeartbeat::publish_heartbeat, this));

    RCLCPP_WARN(
      this->get_logger(),
      "HEARTBEAT ONLY: this node does NOT arm, switch modes, or publish trajectory setpoints.");
  }

private:
  void publish_heartbeat()
  {
    px4_msgs::msg::OffboardControlMode msg{};

    msg.timestamp =
      static_cast<uint64_t>(this->now().nanoseconds() / 1000);

    // Declare position as the future offboard control level.
    // No TrajectorySetpoint is published in this test.
    msg.position = true;

    publisher_->publish(msg);

    heartbeat_count_++;

    if ((heartbeat_count_ % 10) == 0) {
      RCLCPP_INFO(
        this->get_logger(),
        "OffboardControlMode heartbeat alive: %lu messages",
        static_cast<unsigned long>(heartbeat_count_));
    }
  }

  rclcpp::Publisher<
    px4_msgs::msg::OffboardControlMode>::SharedPtr publisher_;

  rclcpp::TimerBase::SharedPtr timer_;

  uint64_t heartbeat_count_{0};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<OffboardHeartbeat>());
  rclcpp::shutdown();
  return 0;
}
