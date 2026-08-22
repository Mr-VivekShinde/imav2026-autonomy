#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>

#include "rclcpp/rclcpp.hpp"

#include "px4_msgs/msg/offboard_control_mode.hpp"
#include "px4_msgs/msg/trajectory_setpoint.hpp"
#include "px4_msgs/msg/vehicle_command.hpp"
#include "px4_msgs/msg/vehicle_command_ack.hpp"
#include "px4_msgs/msg/vehicle_local_position.hpp"
#include "px4_msgs/msg/vehicle_status.hpp"

using namespace std::chrono_literals;


/*
 * TakeoffHoverLand
 * =================
 *
 * Purpose
 * -------
 * Perform our first controlled autonomous flight test:
 *
 *     current position
 *          |
 *          v
 *     take off 1.0 m
 *          |
 *          v
 *     hover 5 seconds
 *          |
 *          v
 *         land
 *
 *
 * IMPORTANT
 * ---------
 * This node is intended for the IMAV Gazebo + PX4 SITL environment
 * at this stage of development.
 *
 * It deliberately uses position control instead of directly controlling
 * attitude, thrust, or motors. PX4 therefore retains its normal internal
 * position, velocity, attitude, and rate controllers.
 */
class TakeoffHoverLand : public rclcpp::Node
{
public:
  TakeoffHoverLand()
  : Node("takeoff_hover_land")
  {
    /*
     * Keep the QoS policy consistent with our already-tested PX4
     * interface probes.
     */
    auto qos = rclcpp::QoS(rclcpp::KeepLast(10));
    qos.best_effort();
    qos.durability_volatile();


    // -------------------------------------------------------------
    // Publishers: ROS 2 -> PX4
    // -------------------------------------------------------------

    offboard_pub_ =
      this->create_publisher<px4_msgs::msg::OffboardControlMode>(
        "/fmu/in/offboard_control_mode",
        qos);

    trajectory_pub_ =
      this->create_publisher<px4_msgs::msg::TrajectorySetpoint>(
        "/fmu/in/trajectory_setpoint",
        qos);

    command_pub_ =
      this->create_publisher<px4_msgs::msg::VehicleCommand>(
        "/fmu/in/vehicle_command",
        qos);


    // -------------------------------------------------------------
    // Subscribers: PX4 -> ROS 2
    // -------------------------------------------------------------

    status_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleStatus>(
        "/fmu/out/vehicle_status_v1",
        qos,
        [this](const px4_msgs::msg::VehicleStatus::SharedPtr msg)
        {
          status_received_ = true;
          status_time_ = this->now();

          arming_state_ = msg->arming_state;
          nav_state_ = msg->nav_state;
          failsafe_ = msg->failsafe;
          preflight_ok_ = msg->pre_flight_checks_pass;
        });


    local_position_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleLocalPosition>(
        "/fmu/out/vehicle_local_position_v1",
        qos,
        [this](const px4_msgs::msg::VehicleLocalPosition::SharedPtr msg)
        {
          position_received_ = true;
          position_time_ = this->now();

          xy_valid_ = msg->xy_valid;
          z_valid_ = msg->z_valid;

          x_ = msg->x;
          y_ = msg->y;
          z_ = msg->z;
          vz_ = msg->vz;
          heading_ = msg->heading;
        });


    /*
     * VehicleCommandAck allows us to see whether PX4 accepted,
     * rejected, or temporarily rejected commands.
     */
    ack_sub_ =
      this->create_subscription<px4_msgs::msg::VehicleCommandAck>(
        "/fmu/out/vehicle_command_ack",
        qos,
        [this](const px4_msgs::msg::VehicleCommandAck::SharedPtr msg)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "PX4 ACK | command=%u result=%u",
            static_cast<unsigned>(msg->command),
            static_cast<unsigned>(msg->result));
        });


    /*
     * Run the state machine at 10 Hz.
     *
     * 100 ms period = 10 messages/second.
     */
    timer_ =
      this->create_wall_timer(
        100ms,
        std::bind(&TakeoffHoverLand::step, this));


    RCLCPP_WARN(
      this->get_logger(),
      "SIMULATION FLIGHT TEST: autonomous 1.0 m takeoff -> "
      "5 s hover -> PX4 LAND.");

    RCLCPP_INFO(
      this->get_logger(),
      "Waiting for PX4 status and valid local position...");
  }


private:

  // =============================================================
  // FLIGHT STATE MACHINE
  // =============================================================

  enum class State
  {
    WAIT_FOR_DATA,

    PRESTREAM,

    REQUEST_OFFBOARD,
    WAIT_OFFBOARD,

    REQUEST_ARM,
    WAIT_ARMED,

    TAKEOFF,
    HOVER,

    REQUEST_LAND,
    WAIT_LANDED,

    DONE,
    ABORT
  };


  // =============================================================
  // CONSTANTS FOR THIS FIRST TEST
  // =============================================================

  /*
   * First flight is deliberately conservative.
   */
  static constexpr float TAKEOFF_HEIGHT_M = 1.0F;

  /*
   * Vehicle must be close to target altitude before hover timing starts.
   */
  static constexpr float ALTITUDE_TOLERANCE_M = 0.20F;

  /*
   * Require low vertical speed when deciding that altitude is reached.
   */
  static constexpr float VERTICAL_SPEED_TOLERANCE_MPS = 0.30F;


  // =============================================================
  // HELPER: current ROS timestamp in microseconds
  // =============================================================

  uint64_t timestamp_us() const
  {
    return static_cast<uint64_t>(
      this->now().nanoseconds() / 1000);
  }


  // =============================================================
  // HELPER: verify fresh PX4 feedback
  // =============================================================

  bool data_fresh() const
  {
    if (!status_received_ || !position_received_) {
      return false;
    }

    const double status_age =
      (this->now() - status_time_).seconds();

    const double position_age =
      (this->now() - position_time_).seconds();

    /*
     * VehicleStatus is lower-rate than local position.
     *
     * We therefore allow VehicleStatus to be up to 3 seconds old,
     * while local position must still be newer than 1 second.
     *
     * This prevents a normal low-rate status update from
     * interrupting our 10 Hz Offboard control stream.
     */
    return status_age < 3.0 &&
           position_age < 1.0;
  }


  // =============================================================
  // SEND A PX4 VEHICLE COMMAND
  // =============================================================

  void send_vehicle_command(
    uint16_t command,
    float param1 = 0.0F,
    float param2 = 0.0F)
  {
    px4_msgs::msg::VehicleCommand msg{};

    msg.timestamp = timestamp_us();

    msg.param1 = param1;
    msg.param2 = param2;

    msg.command = command;

    msg.target_system = 1;
    msg.target_component = 1;

    msg.source_system = 1;
    msg.source_component = 1;

    msg.from_external = true;

    command_pub_->publish(msg);
  }


  // =============================================================
  // CONTINUOUS OFFBOARD HEARTBEAT + POSITION SETPOINT
  // =============================================================

  void publish_offboard_stream()
  {
    const uint64_t timestamp = timestamp_us();


    // -----------------------------------------------------------
    // Tell PX4 we are using POSITION control.
    // -----------------------------------------------------------

    px4_msgs::msg::OffboardControlMode control{};

    control.timestamp = timestamp;

    control.position = true;
    control.velocity = false;
    control.acceleration = false;
    control.attitude = false;
    control.body_rate = false;

    offboard_pub_->publish(control);


    // -----------------------------------------------------------
    // Tell PX4 where the drone should hold.
    // -----------------------------------------------------------

    px4_msgs::msg::TrajectorySetpoint setpoint{};

    setpoint.timestamp = timestamp;

    /*
     * Keep horizontal position at the place where the test began.
     */
    setpoint.position[0] = home_x_;
    setpoint.position[1] = home_y_;

    /*
     * PX4 uses NED:
     *
     *     negative Z = higher altitude
     *
     * Example:
     *
     *     initial Z = 0
     *     desired Z = -1
     *
     * means approximately one metre upward.
     */
    setpoint.position[2] = target_z_;


    /*
     * We are controlling POSITION.
     *
     * Unused velocity/acceleration/jerk values are explicitly NaN,
     * meaning "do not control this field independently".
     */
    setpoint.velocity = {NAN, NAN, NAN};
    setpoint.acceleration = {NAN, NAN, NAN};
    setpoint.jerk = {NAN, NAN, NAN};

    setpoint.yaw = home_yaw_;
    setpoint.yawspeed = NAN;

    trajectory_pub_->publish(setpoint);
  }


  // =============================================================
  // SHOULD THE OFFBOARD STREAM CONTINUE?
  // =============================================================

  bool should_publish_offboard() const
  {
    switch (state_)
    {
      case State::PRESTREAM:
      case State::REQUEST_OFFBOARD:
      case State::WAIT_OFFBOARD:
      case State::REQUEST_ARM:
      case State::WAIT_ARMED:
      case State::TAKEOFF:
      case State::HOVER:
        return true;

      /*
       * After requesting LAND, keep the stream alive only while PX4
       * still reports OFFBOARD. Once PX4 enters AUTO_LAND, the
       * autonomous landing controller owns the vehicle.
       */
      case State::WAIT_LANDED:
        return
          nav_state_ ==
          px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD;

      default:
        return false;
    }
  }


  // =============================================================
  // REQUEST A SAFE LANDING
  // =============================================================

  void request_land(const char * reason)
  {
    RCLCPP_WARN(
      this->get_logger(),
      "LAND requested: %s",
      reason);

    send_vehicle_command(
      px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);

    state_ = State::WAIT_LANDED;
    state_ticks_ = 0;
    land_retries_ = 0;
  }


  // =============================================================
  // MAIN STATE MACHINE — runs every 100 ms
  // =============================================================

  void step()
  {
    // -----------------------------------------------------------
    // STATE 1: Wait for trustworthy PX4 information.
    // -----------------------------------------------------------

    if (state_ == State::WAIT_FOR_DATA)
    {
      if (!data_fresh()) {
        return;
      }

      if (!xy_valid_ || !z_valid_) {
        if (!invalid_position_reported_) {
          RCLCPP_WARN(
            this->get_logger(),
            "PX4 data received, but local XY/Z is not valid yet.");

          invalid_position_reported_ = true;
        }

        return;
      }

      /*
       * Do not duplicate PX4's complete arming-check system here.
       *
       * VehicleStatus::pre_flight_checks_pass is still observed and
       * reported, but PX4 itself remains the authority that accepts
       * or rejects the later ARM command.
       *
       * For this simulation test we require:
       *
       *   1. fresh PX4 data
       *   2. valid local XY/Z position
       *   3. no active failsafe
       *
       * If PX4 considers the vehicle unsafe to arm, the ARM command
       * will be rejected and this node will abort before takeoff.
       */
      if (failsafe_) {
        RCLCPP_WARN(
          this->get_logger(),
          "Waiting: PX4 currently reports FAILSAFE.");

        return;
      }

      if (!preflight_ok_ && !preflight_reported_) {
        RCLCPP_WARN(
          this->get_logger(),
          "PX4 overall pre-flight flag is not fully clear. "
          "Continuing simulation preparation; PX4 will still decide "
          "whether ARM is permitted.");

        preflight_reported_ = true;
      }


      /*
       * Save the starting position.
       *
       * We do NOT assume that the Gazebo vehicle starts exactly
       * at local coordinate [0,0,0].
       */
      home_x_ = x_;
      home_y_ = y_;
      home_z_ = z_;

      home_yaw_ =
        std::isfinite(heading_) ? heading_ : 0.0F;


      /*
       * NED Z increases downward.
       *
       * Therefore one metre upward is:
       *
       *     target_z = home_z - 1.0
       */
      target_z_ =
        home_z_ - TAKEOFF_HEIGHT_M;


      RCLCPP_INFO(
        this->get_logger(),
        "PX4 READY | home NED=[%.2f %.2f %.2f] m",
        static_cast<double>(home_x_),
        static_cast<double>(home_y_),
        static_cast<double>(home_z_));

      RCLCPP_INFO(
        this->get_logger(),
        "Takeoff target NED=[%.2f %.2f %.2f] m",
        static_cast<double>(home_x_),
        static_cast<double>(home_y_),
        static_cast<double>(target_z_));

      RCLCPP_INFO(
        this->get_logger(),
        "Starting 2-second Offboard pre-stream.");

      state_ = State::PRESTREAM;
      state_ticks_ = 0;

      return;
    }


    // -----------------------------------------------------------
    // Once the flight sequence has started, loss of fresh state
    // information is considered abnormal.
    // -----------------------------------------------------------

    if (!data_fresh())
    {
      if (!stale_data_reported_) {
        RCLCPP_ERROR(
          this->get_logger(),
          "PX4 feedback became missing/stale.");

        stale_data_reported_ = true;
      }

      /*
       * Do not send a DISARM command here.
       *
       * Disarming an airborne vehicle would be unsafe.
       *
       * We simply stop progressing the state machine. PX4's own
       * Offboard-loss/failsafe logic remains the final safety layer.
       */
      return;
    }


    // -----------------------------------------------------------
    // Publish heartbeat/setpoint whenever required.
    // -----------------------------------------------------------

    if (should_publish_offboard()) {
      publish_offboard_stream();
    }


    // -----------------------------------------------------------
    // STATE MACHINE
    // -----------------------------------------------------------

    switch (state_)
    {
      // ---------------------------------------------------------
      // Give PX4 two full seconds of Offboard heartbeat/setpoints.
      // 20 ticks at 10 Hz = 2 seconds.
      // ---------------------------------------------------------

      case State::PRESTREAM:

        if (++state_ticks_ >= 20)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "Offboard pre-stream complete.");

          state_ = State::REQUEST_OFFBOARD;
          state_ticks_ = 0;
        }

        break;


      // ---------------------------------------------------------
      // Ask PX4 to enter OFFBOARD mode.
      // ---------------------------------------------------------

      case State::REQUEST_OFFBOARD:

        RCLCPP_INFO(
          this->get_logger(),
          "Requesting PX4 OFFBOARD mode.");

        send_vehicle_command(
          px4_msgs::msg::VehicleCommand::VEHICLE_CMD_DO_SET_MODE,
          1.0F,
          6.0F);

        state_ = State::WAIT_OFFBOARD;
        state_ticks_ = 0;

        break;


      // ---------------------------------------------------------
      // Do not ARM until PX4 itself confirms OFFBOARD.
      // ---------------------------------------------------------

      case State::WAIT_OFFBOARD:

        if (
          nav_state_ ==
          px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "PX4 STATUS CONFIRMED: OFFBOARD");

          state_ = State::REQUEST_ARM;
          state_ticks_ = 0;
        }
        else if (++state_ticks_ > 30)
        {
          RCLCPP_ERROR(
            this->get_logger(),
            "OFFBOARD mode was not confirmed within 3 seconds.");

          state_ = State::ABORT;
        }

        break;


      // ---------------------------------------------------------
      // ARM only after OFFBOARD has been confirmed.
      // ---------------------------------------------------------

      case State::REQUEST_ARM:

        RCLCPP_WARN(
          this->get_logger(),
          "Sending ARM command.");

        send_vehicle_command(
          px4_msgs::msg::VehicleCommand::
            VEHICLE_CMD_COMPONENT_ARM_DISARM,
          1.0F);

        state_ = State::WAIT_ARMED;
        state_ticks_ = 0;

        break;


      // ---------------------------------------------------------
      // Verify that PX4 actually armed.
      // ---------------------------------------------------------

      case State::WAIT_ARMED:

        if (
          arming_state_ ==
          px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "PX4 STATUS CONFIRMED: ARMED");

          RCLCPP_INFO(
            this->get_logger(),
            "Takeoff started.");

          state_ = State::TAKEOFF;
          state_ticks_ = 0;
        }
        else if (++state_ticks_ > 30)
        {
          RCLCPP_ERROR(
            this->get_logger(),
            "ARM was not confirmed within 3 seconds.");

          state_ = State::ABORT;
        }

        break;


      // ---------------------------------------------------------
      // PX4 now climbs toward target_z_.
      // ---------------------------------------------------------

      case State::TAKEOFF:
      {
        if (failsafe_)
        {
          request_land("PX4 entered FAILSAFE during takeoff");
          break;
        }


        const float altitude_error =
          std::fabs(z_ - target_z_);

        const bool altitude_reached =
          altitude_error < ALTITUDE_TOLERANCE_M &&
          std::fabs(vz_) < VERTICAL_SPEED_TOLERANCE_MPS;


        if (altitude_reached)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "TAKEOFF COMPLETE | current Z=%.2f target Z=%.2f",
            static_cast<double>(z_),
            static_cast<double>(target_z_));

          RCLCPP_INFO(
            this->get_logger(),
            "Starting 5-second hover.");

          state_ = State::HOVER;
          state_ticks_ = 0;
        }
        else if (++state_ticks_ > 150)
        {
          /*
           * 150 ticks at 10 Hz = 15 seconds.
           *
           * If one metre altitude cannot be reached in that time,
           * something is wrong, so request a landing rather than
           * continuing indefinitely.
           */
          request_land(
            "takeoff altitude was not reached within 15 seconds");
        }

        break;
      }


      // ---------------------------------------------------------
      // Hold the same XYZ position for five seconds.
      // ---------------------------------------------------------

      case State::HOVER:

        if (failsafe_)
        {
          request_land("PX4 entered FAILSAFE during hover");
          break;
        }


        if (++state_ticks_ >= 50)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "5-second hover complete.");

          state_ = State::REQUEST_LAND;
          state_ticks_ = 0;
        }

        break;


      // ---------------------------------------------------------
      // Ask PX4's normal landing controller to land.
      //
      // We intentionally do NOT command a rapid Z descent ourselves.
      // ---------------------------------------------------------

      case State::REQUEST_LAND:

        request_land(
          "normal test sequence completed");

        break;


      // ---------------------------------------------------------
      // Wait until PX4 reports DISARMED after landing.
      // ---------------------------------------------------------

      case State::WAIT_LANDED:

        if (
          arming_state_ ==
          px4_msgs::msg::VehicleStatus::ARMING_STATE_DISARMED)
        {
          RCLCPP_INFO(
            this->get_logger(),
            "PX4 STATUS CONFIRMED: DISARMED");

          RCLCPP_INFO(
            this->get_logger(),
            "STEP 10 FLIGHT SEQUENCE COMPLETE.");

          state_ = State::DONE;
          state_ticks_ = 0;

          break;
        }


        /*
         * If PX4 somehow remains OFFBOARD rather than changing to
         * AUTO_LAND, re-send LAND at most three times.
         */
        if (
          nav_state_ ==
          px4_msgs::msg::VehicleStatus::NAVIGATION_STATE_OFFBOARD &&
          (++state_ticks_ % 20) == 0 &&
          land_retries_ < 3)
        {
          land_retries_++;

          RCLCPP_WARN(
            this->get_logger(),
            "PX4 still reports OFFBOARD. Re-sending LAND (%d/3).",
            land_retries_);

          send_vehicle_command(
            px4_msgs::msg::VehicleCommand::VEHICLE_CMD_NAV_LAND);
        }

        break;


      // ---------------------------------------------------------
      // Successful completion.
      // ---------------------------------------------------------

      case State::DONE:

        RCLCPP_INFO(
          this->get_logger(),
          "Node finished successfully.");

        rclcpp::shutdown();

        break;


      // ---------------------------------------------------------
      // Abort before flight if OFFBOARD/ARM could not be established.
      // ---------------------------------------------------------

      case State::ABORT:

        if (
          arming_state_ ==
          px4_msgs::msg::VehicleStatus::ARMING_STATE_ARMED)
        {
          request_land(
            "flight sequence aborted while vehicle was armed");
        }
        else
        {
          RCLCPP_ERROR(
            this->get_logger(),
            "Sequence aborted before takeoff.");

          rclcpp::shutdown();
        }

        break;


      case State::WAIT_FOR_DATA:
        break;
    }
  }


  // =============================================================
  // ROS interfaces
  // =============================================================

  rclcpp::Publisher<
    px4_msgs::msg::OffboardControlMode>::SharedPtr
    offboard_pub_;

  rclcpp::Publisher<
    px4_msgs::msg::TrajectorySetpoint>::SharedPtr
    trajectory_pub_;

  rclcpp::Publisher<
    px4_msgs::msg::VehicleCommand>::SharedPtr
    command_pub_;


  rclcpp::Subscription<
    px4_msgs::msg::VehicleStatus>::SharedPtr
    status_sub_;

  rclcpp::Subscription<
    px4_msgs::msg::VehicleLocalPosition>::SharedPtr
    local_position_sub_;

  rclcpp::Subscription<
    px4_msgs::msg::VehicleCommandAck>::SharedPtr
    ack_sub_;


  rclcpp::TimerBase::SharedPtr timer_;


  // =============================================================
  // State-machine data
  // =============================================================

  State state_{State::WAIT_FOR_DATA};

  int state_ticks_{0};
  int land_retries_{0};


  // =============================================================
  // PX4 status
  // =============================================================

  bool status_received_{false};
  bool position_received_{false};

  bool xy_valid_{false};
  bool z_valid_{false};

  bool preflight_ok_{false};
  bool failsafe_{false};

  uint8_t arming_state_{0};
  uint8_t nav_state_{0};


  // =============================================================
  // Current local NED position
  // =============================================================

  float x_{NAN};
  float y_{NAN};
  float z_{NAN};
  float vz_{NAN};
  float heading_{NAN};


  // =============================================================
  // Saved starting position and takeoff target
  // =============================================================

  float home_x_{0.0F};
  float home_y_{0.0F};
  float home_z_{0.0F};
  float home_yaw_{0.0F};

  float target_z_{0.0F};


  // =============================================================
  // Freshness timestamps
  // =============================================================

  rclcpp::Time status_time_{0, 0, RCL_ROS_TIME};
  rclcpp::Time position_time_{0, 0, RCL_ROS_TIME};


  // =============================================================
  // One-shot warning flags
  // =============================================================

  bool invalid_position_reported_{false};
  bool preflight_reported_{false};
  bool stale_data_reported_{false};
};


int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);

  rclcpp::spin(
    std::make_shared<TakeoffHoverLand>());

  rclcpp::shutdown();

  return 0;
}
