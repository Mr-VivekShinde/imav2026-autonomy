# Migration Notes

Source reference used for the initial simulator migration:

- Upstream repository: https://github.com/Aachen-Drone-Development-Initiative/IMAV.git
- Uploaded reference commit: 559f105592bd008be7f21911a457e3bb16db4941
- Uploaded branch/status snapshot showed an unfinished rebase; no Git history was imported.

Changes already applied in this clean baseline:

1. New repository layout.
2. Ubuntu 24.04 / ROS 2 Jazzy / Gazebo Harmonic / PX4 v1.17.0 baseline declared.
3. Mission-4 models removed from the baseline world.
4. Front D435i depth sensor enabled.
5. Swapped `cone/model.config` and `obstacles/model.config` metadata corrected.
6. Hard-coded Gazebo `gz-sim-7` plugin path not carried forward.
7. Random Mission-3 hot-box generator rewritten with explicit input/output paths.
8. Existing x500 retained only as a temporary reference vehicle.
