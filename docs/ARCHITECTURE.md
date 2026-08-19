# Phase-0 Architecture

The project deliberately separates the competition code from the temporary x500 model.

```text
Host laptop
└── Docker
    └── Ubuntu 24.04 simulation container
        ├── ROS 2 Jazzy
        ├── Gazebo Harmonic
        ├── PX4 SITL v1.17.0
        └── /workspace  <- bind mount of this repository
            ├── simulation/
            ├── src/
            ├── config/
            └── scripts/
```

Simulation data flow:

```text
Gazebo world
   ├── temporary x500 + IMAV sensors
   ├── Mission 1 obstacles
   ├── Mission 2 room
   └── Mission 3 boxes
          │
          ├── Gazebo transport ──> ros_gz_bridge ──> ROS 2 sensor topics
          │
          └── PX4 Gazebo plugins <──> PX4 SITL v1.17.0
```

The future CAD migration should replace only the vehicle geometry/dynamics layer while preserving stable topic and frame interfaces.
