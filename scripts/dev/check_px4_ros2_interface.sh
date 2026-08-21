#!/usr/bin/env bash

set -eo pipefail

echo
echo "============================================================"
echo " IMAV - PX4 <-> ROS 2 INTERFACE CONTRACT CHECK"
echo "============================================================"

# ------------------------------------------------------------
# Environment
# ------------------------------------------------------------

if [ -f /opt/ros/jazzy/setup.bash ]; then
    source /opt/ros/jazzy/setup.bash
fi

if [ -f /workspace/install/setup.bash ]; then
    source /workspace/install/setup.bash
fi

echo
echo "ROS_DISTRO=${ROS_DISTRO:-NOT_SET}"
echo "ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-NOT_SET}"

# ------------------------------------------------------------
# Helper:
# PX4 may expose a message either as:
#
#   /fmu/out/vehicle_status
#
# or, for versioned messages:
#
#   /fmu/out/vehicle_status_v1
#
# This function resolves the actual topic currently provided.
# ------------------------------------------------------------

resolve_topic()
{
    local base="$1"
    local matches
    local count

    if ros2 topic list | grep -qx "$base"; then
        echo "$base"
        return 0
    fi

    matches="$(
        ros2 topic list |
        grep -E "^${base}_v[0-9]+$" ||
        true
    )"

    count="$(
        printf '%s\n' "$matches" |
        sed '/^$/d' |
        wc -l
    )"

    if [ "$count" -eq 1 ]; then
        printf '%s\n' "$matches"
        return 0
    fi

    return 1
}

PASS=0
FAIL=0

# ------------------------------------------------------------
# Helper:
# Verify that a logical interface exists AND uses the expected
# px4_msgs type.
# ------------------------------------------------------------

check_topic()
{
    local label="$1"
    local base="$2"
    local expected_type="$3"

    local topic
    local actual_type

    topic="$(resolve_topic "$base" || true)"

    if [ -z "$topic" ]; then
        echo "FAIL  $label"
        echo "      Missing: $base or versioned equivalent"
        FAIL=$((FAIL + 1))
        return
    fi

    actual_type="$(
        ros2 topic type "$topic" 2>/dev/null |
        head -n 1
    )"

    if [ "$actual_type" != "$expected_type" ]; then
        echo "FAIL  $label"
        echo "      Topic:    $topic"
        echo "      Expected: $expected_type"
        echo "      Actual:   $actual_type"
        FAIL=$((FAIL + 1))
        return
    fi

    echo "PASS  $label"
    echo "      Topic: $topic"
    echo "      Type:  $actual_type"

    PASS=$((PASS + 1))
}

echo
echo "------------------------------------------------------------"
echo " PX4 -> ROS 2 STATE INTERFACES"
echo "------------------------------------------------------------"

check_topic \
    "VehicleStatus" \
    "/fmu/out/vehicle_status" \
    "px4_msgs/msg/VehicleStatus"

check_topic \
    "VehicleOdometry" \
    "/fmu/out/vehicle_odometry" \
    "px4_msgs/msg/VehicleOdometry"

check_topic \
    "VehicleLocalPosition" \
    "/fmu/out/vehicle_local_position" \
    "px4_msgs/msg/VehicleLocalPosition"

check_topic \
    "VehicleCommandAck" \
    "/fmu/out/vehicle_command_ack" \
    "px4_msgs/msg/VehicleCommandAck"


echo
echo "------------------------------------------------------------"
echo " ROS 2 -> PX4 CONTROL INTERFACES"
echo "------------------------------------------------------------"

check_topic \
    "OffboardControlMode" \
    "/fmu/in/offboard_control_mode" \
    "px4_msgs/msg/OffboardControlMode"

check_topic \
    "TrajectorySetpoint" \
    "/fmu/in/trajectory_setpoint" \
    "px4_msgs/msg/TrajectorySetpoint"

check_topic \
    "VehicleCommand" \
    "/fmu/in/vehicle_command" \
    "px4_msgs/msg/VehicleCommand"


echo
echo "------------------------------------------------------------"
echo " CONTRACT RESULT"
echo "------------------------------------------------------------"

echo "PASS=${PASS}"
echo "FAIL=${FAIL}"

if [ "$FAIL" -eq 0 ] && [ "$PASS" -eq 7 ]; then
    echo
    echo "PX4_ROS2_INTERFACE_CONTRACT=PASS"
    echo
    echo "IMPORTANT:"
    echo "This proves the required DDS interfaces exist."
    echo "It does NOT arm, fly, or command the vehicle."
    exit 0
else
    echo
    echo "PX4_ROS2_INTERFACE_CONTRACT=FAIL"
    exit 1
fi
