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
