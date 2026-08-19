# Contributing

## Branch policy

- `main`: must stay runnable.
- Create work branches such as `feature/mission1-window-detection`, `fix/gz-bridge`, or `sim/custom-drone`.
- Do not develop directly on `main` after collaborators join.
- Open a pull request and have at least one teammate review changes before merge.

## Dependency policy

Do not silently upgrade ROS, Gazebo, PX4, MAVSDK, or Python packages. Change `config/versions.env`, rebuild, run smoke tests, and document the result in the pull request.

## Generated data

Do not commit rosbags, PX4 `.ulg` logs, Docker build outputs, colcon `build/install/log`, or local `.env` files.

## CAD

Future STEP/Fusion files are tracked by Git LFS. Run `git lfs install` once on each development machine before working with CAD assets.
