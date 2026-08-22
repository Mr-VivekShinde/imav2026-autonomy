#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"

#include "px4_msgs/msg/vehicle_local_position.hpp"
#include "px4_msgs/msg/vehicle_odometry.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"

using namespace std::chrono_literals;

class Px4InterfaceMonitor : public rclcpp::Node
{
public:
  Px4InterfaceMonitor()
  : Node("px4_interface_monitor")
  {
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10));
    qos.best_effort();
    qos.durability_volatile();

    status_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleStatus>(
        "/fmu/out/vehicle_status_v1",
        qos,
        std::bind(
          &Px4InterfaceMonitor::status_callback,
          this,
          std::placeholders::_1));

    odometry_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleOdometry>(
        "/fmu/out/vehicle_odometry",
        qos,
        std::bind(
          &Px4InterfaceMonitor::odometry_callback,
          this,
          std::placeholders::_1));

    local_position_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
        "/fmu/out/vehicle_local_position_v1",
        qos,
        std::bind(
          &Px4InterfaceMonitor::local_position_callback,
          this,
          std::placeholders::_1));

    report_timer_ =
      this->create_wall_timer(
        1s,
        std::bind(
          &Px4InterfaceMonitor::report,
          this));

    RCLCPP_INFO(this->get_logger(), "PX4 interface monitor started.");
    RCLCPP_INFO(this->get_logger(), "READ ONLY: no PX4 commands are published.");
  }

private:
  void status_callback(const px4_msgs::msg::VehicleStatus::SharedPtr msg)
  {
    status_received_ = true;
    status_time_ = this->now();
    arming_state_ = msg->arming_state;
    nav_state_ = msg->nav_state;
    failsafe_ = msg->failsafe;
  }

  void odometry_callback(const px4_msgs::msg::VehicleOdometry::SharedPtr msg)
  {
    odometry_received_ = true;
    odometry_time_ = this->now();
    x_ = msg->position[0];
    y_ = msg->position[1];
    z_ = msg->position[2];
  }

  void local_position_callback(
    const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg)
  {
    local_position_received_ = true;
    local_position_time_ = this->now();
    xy_valid_ = msg->xy_valid;
    z_valid_ = msg->z_valid;
  }

  bool fresh(const rclcpp::Time & last_time, bool received) const
  {
    if (!received) {
      return false;
    }
    return (this->now() - last_time).seconds() < 2.0;
  }

  void report()
  {
    const bool status_ok = fresh(status_time_, status_received_);
    const bool odometry_ok = fresh(odometry_time_, odometry_received_);
    const bool local_ok =
      fresh(local_position_time_, local_position_received_);

    if (!status_ok || !odometry_ok || !local_ok) {
      RCLCPP_WARN(
        this->get_logger(),
        "PX4 DATA NOT READY | status=%s odometry=%s local_position=%s",
        status_ok ? "OK" : "MISSING/STALE",
        odometry_ok ? "OK" : "MISSING/STALE",
        local_ok ? "OK" : "MISSING/STALE");
      return;
    }

    RCLCPP_INFO(
      this->get_logger(),
      "PX4 LINK OK | arming_state=%u nav_state=%u failsafe=%s | "
      "local_xy=%s local_z=%s | NED=[%.2f %.2f %.2f] m",
      static_cast<unsigned int>(arming_state_),
      static_cast<unsigned int>(nav_state_),
      failsafe_ ? "YES" : "NO",
      xy_valid_ ? "VALID" : "INVALID",
      z_valid_ ? "VALID" : "INVALID",
      static_cast<double>(x_),
      static_cast<double>(y_),
      static_cast<double>(z_));
  }

  rclcpp::Subscription<px4_msgs::msg::VehicleStatus>::SharedPtr status_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleOdometry>::SharedPtr odometry_sub_;
  rclcpp::Subscription<px4_msgs::msg::VehicleLocalPosition>::SharedPtr
    local_position_sub_;

  rclcpp::TimerBase::SharedPtr report_timer_;

  rclcpp::Time status_time_{0, 0, RCL_ROS_TIME};
  rclcpp::Time odometry_time_{0, 0, RCL_ROS_TIME};
  rclcpp::Time local_position_time_{0, 0, RCL_ROS_TIME};

  bool status_received_{false};
  bool odometry_received_{false};
  bool local_position_received_{false};

  uint8_t arming_state_{0};
  uint8_t nav_state_{0};
  bool failsafe_{false};

  bool xy_valid_{false};
  bool z_valid_{false};

  float x_{NAN};
  float y_{NAN};
  float z_{NAN};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<Px4InterfaceMonitor>());
  rclcpp::shutdown();
  return 0;
}
