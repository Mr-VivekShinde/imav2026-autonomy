FROM ubuntu:24.04

ARG ROS_DISTRO=jazzy
ARG PX4_TAG=v1.17.0
ARG MAVSDK_PYTHON=3.17.2

ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    ROS_DISTRO=${ROS_DISTRO} \
    ROS_DOMAIN_ID=26 \
    PX4_DIR=/opt/PX4-Autopilot

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl wget gnupg lsb-release locales sudo \
    git git-lfs build-essential cmake ninja-build pkg-config \
    python3 python3-pip python3-venv python3-dev \
    tmux nano less iproute2 net-tools \
    && locale-gen en_US.UTF-8 \
    && rm -rf /var/lib/apt/lists/*

# ROS 2 repository for Ubuntu 24.04 (Noble).
RUN curl -fsSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
      -o /usr/share/keyrings/ros-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu noble main" \
      > /etc/apt/sources.list.d/ros2.list

# Gazebo stable repository. ROS 2 Jazzy is paired with Gazebo Harmonic.
RUN curl -fsSL https://packages.osrfoundation.org/gazebo.gpg \
      -o /usr/share/keyrings/pkgs-osrf-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/pkgs-osrf-archive-keyring.gpg] http://packages.osrfoundation.org/gazebo/ubuntu-stable noble main" \
      > /etc/apt/sources.list.d/gazebo-stable.list

RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-${ROS_DISTRO}-desktop \
    ros-${ROS_DISTRO}-ros-gz \
    ros-${ROS_DISTRO}-cv-bridge \
    ros-${ROS_DISTRO}-rqt-image-view \
    ros-${ROS_DISTRO}-rosbag2-storage-mcap \
    python3-colcon-common-extensions \
    python3-rosdep \
    python3-vcstool \
    python3-opencv \
    libopencv-contrib-dev \
    gz-harmonic \
    && rm -rf /var/lib/apt/lists/*

RUN rosdep init 2>/dev/null || true && rosdep update

# Pin PX4 to the same stable release used on the competition flight controller.
WORKDIR /opt
RUN git clone --branch ${PX4_TAG} --depth 1 --recursive \
      https://github.com/PX4/PX4-Autopilot.git PX4-Autopilot \
    && cd PX4-Autopilot \
    && bash Tools/setup/ubuntu.sh --no-nuttx \
    && make px4_sitl_default -j2

# PX4 v1.17 uses Micro XRCE-DDS v2.x.
# ROS 2 Jazzy is compatible with Micro XRCE-DDS Agent v2.4.3.
ARG MICRO_XRCE_DDS_AGENT_TAG=v2.4.3
RUN git clone --branch ${MICRO_XRCE_DDS_AGENT_TAG} --depth 1 \
      https://github.com/eProsima/Micro-XRCE-DDS-Agent.git /opt/Micro-XRCE-DDS-Agent \
    && cd /opt/Micro-XRCE-DDS-Agent \
    && mkdir build \
    && cd build \
    && cmake .. \
    && make -j2 \
    && make install \
    && ldconfig \
    && rm -rf /opt/Micro-XRCE-DDS-Agent

# Keep Python application dependencies separate from system OpenCV/cv_bridge.
# --break-system-packages is required on Ubuntu 24.04's externally-managed Python.
RUN python3 -m pip install --no-cache-dir --break-system-packages \
    mavsdk==${MAVSDK_PYTHON} \
    simple-pid==2.0.1

WORKDIR /workspace
RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> /root/.bashrc \
    && echo '[[ -f /workspace/install/setup.bash ]] && source /workspace/install/setup.bash' >> /root/.bashrc

CMD ["bash"]
