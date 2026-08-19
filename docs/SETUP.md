# Local Setup

## Supported baseline

- Host: Ubuntu 24.04 LTS preferred. Ubuntu 22.04 can also host Docker, but the project container itself is Ubuntu 24.04.
- ROS 2: Jazzy
- Gazebo: Harmonic
- PX4 SITL: v1.17.0
- Container runtime: Docker Engine + Docker Compose plugin

## Host packages

Install only host-development tools directly on the laptop:

- Git
- Git LFS
- Docker Engine
- Docker Compose plugin
- An X11-compatible desktop session for Gazebo GUI

Do not install ROS 2, Gazebo, PX4, MAVSDK, or project Python packages on the laptop merely to run the project. They live in the Docker image.

## Python virtual environment policy

The main ROS/Gazebo project does **not** use a host virtual environment. Docker is the dependency boundary.

If a future non-ROS helper tool needs a Python venv, put it at repository root as `.venv/`. It is already ignored by Git. Do not place virtual environments inside `src/`, `simulation/`, or individual ROS packages.

## First checks

Run:

    ./scripts/host/check_host.sh

Then build the simulation image:

    ./scripts/sim/build_image.sh

Then start/open the persistent development container:

    ./scripts/sim/shell.sh

Open additional terminals with the same command; each one uses `docker compose exec` into the same running container.

Inside the container, validate the workspace:

    ./scripts/dev/build_workspace.sh

## First simulation smoke test

Use four terminal shells attached to the same container (or use tmux inside it):

1. `./scripts/sim/start_world.sh`
2. `./scripts/sim/start_gui.sh`
3. `./scripts/sim/spawn_x500.sh`
4. `./scripts/sim/start_px4.sh`
5. Optional sensor bridge: `./scripts/sim/start_bridge.sh`

The x500 is temporary. Do not tune mission algorithms around its final dynamics.

## Stop the environment

    ./scripts/sim/stop.sh
