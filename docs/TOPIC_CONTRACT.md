# Topic Contract

Mission code should depend on these stable interfaces rather than Gazebo-specific internals.

| Function | Stable ROS topic | Notes |
|---|---|---|
| Front RGB | `/camera/front/rgb` | Real and simulation should match |
| Front depth | `/camera/front/depth` | Enabled in migrated simulation |
| Bottom RGB | `/camera/bottom/rgb` | Real and simulation should match |
| Bottom depth | `/camera/bottom/depth` | Real and simulation should match |
| MLX mission temperature | `/MLX90614/obj_temp` | Mission logic should use this scalar-like measurement |
| Raw simulated thermal image | `/MLX90614/thermal_image` | Simulation backend only; not for spatial mission decisions |
| Cone release command | `/cone/detach` | Simulation backend; later wrap behind payload interface |

Future additions: red-LED command/state, PX4 state/odometry contract, ARK Flow diagnostics, and mission-manager state.

## PX4 flight-control contract

Primary autonomous flight-control path:

IMAV autonomy nodes -> imav_flight_interface -> px4_msgs -> Micro XRCE-DDS -> PX4

Baseline: PX4 v1.17 with matching px4_msgs release/1.17.

### PX4 to ROS 2

- Vehicle state: /fmu/out/vehicle_status -> px4_msgs/msg/VehicleStatus
- Vehicle odometry: /fmu/out/vehicle_odometry -> px4_msgs/msg/VehicleOdometry
- Local position: /fmu/out/vehicle_local_position -> px4_msgs/msg/VehicleLocalPosition
- Command acknowledgement: /fmu/out/vehicle_command_ack -> px4_msgs/msg/VehicleCommandAck

PX4 message versioning may append a suffix such as _v1. The current runtime therefore may expose topics such as /fmu/out/vehicle_status_v1 or /fmu/out/vehicle_local_position_v1.

### ROS 2 to PX4

- Offboard control heartbeat: /fmu/in/offboard_control_mode -> px4_msgs/msg/OffboardControlMode
- Position/velocity/yaw target: /fmu/in/trajectory_setpoint -> px4_msgs/msg/TrajectorySetpoint
- Arm/disarm/mode commands: /fmu/in/vehicle_command -> px4_msgs/msg/VehicleCommand

### Ownership rule

Only imav_flight_interface may directly publish normal autonomous PX4 flight-control messages. Mission nodes must request movement through the flight-interface layer.

### Safety rule

Topic availability alone does not mean the vehicle is safe to arm. Estimator health, PX4 state, Offboard heartbeat, failsafes and command acknowledgements must be checked before flight commands are permitted.

