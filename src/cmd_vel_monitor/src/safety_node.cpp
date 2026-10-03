// Copyright 2026 rrobot
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class SafetyNode : public rclcpp::Node
{
public:
  SafetyNode()
  : rclcpp::Node("safety_node")
  {
    // 参数：速度上限，可在启动时用 --ros-args -p 覆盖
    max_linear_vel_ = this->declare_parameter<double>("max_linear_vel", 0.5);
    max_angular_vel_ = this->declare_parameter<double>("max_angular_vel", 1.0);

    RCLCPP_INFO(
      this->get_logger(), "max_linear_vel=%.3f, max_angular_vel=%.3f",
      max_linear_vel_, max_angular_vel_);

    // 用 SensorDataQoS（best_effort），和常见的速度指令发布端兼容
    cmd_vel_safe_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(
      "/cmd_vel_safe", rclcpp::SensorDataQoS());
    robot_status_pub_ = this->create_publisher<std_msgs::msg::String>(
      "/robot_status", rclcpp::SensorDataQoS());

    cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel", rclcpp::SensorDataQoS(),
      std::bind(&SafetyNode::cmd_vel_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "safety_node started.");
  }

private:
  // 不是有效数字（NaN / ±inf）就置 0
  static double sanitize(double v)
  {
    return std::isfinite(v) ? v : 0.0;
  }

  // 超出上限就截断到 ±上限，返回是否发生了截断
  bool clamp(double & v, double limit)
  {
    if (v > limit) {
      v = limit;
      return true;
    }
    if (v < -limit) {
      v = -limit;
      return true;
    }
    return false;
  }

  void cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    geometry_msgs::msg::Twist out;
    bool limited = false;

    // 先判 NaN/inf 再截断：NaN 和任何数比较都是 false，
    // 直接 clamp 的话 NaN 会原样漏过去。
    double linear_x = sanitize(msg->linear.x);
    double linear_y = sanitize(msg->linear.y);
    double angular_z = sanitize(msg->angular.z);
    limited = (linear_x != msg->linear.x) || (linear_y != msg->linear.y) ||
      (angular_z != msg->angular.z);

    limited |= clamp(linear_x, max_linear_vel_);
    limited |= clamp(linear_y, max_linear_vel_);
    limited |= clamp(angular_z, max_angular_vel_);

    out.linear.x = linear_x;
    out.linear.y = linear_y;
    out.angular.z = angular_z;
    cmd_vel_safe_pub_->publish(out);

    std_msgs::msg::String status;
    status.data = limited ? "触发限制" : "正常";
    robot_status_pub_->publish(status);

    if (limited) {
      RCLCPP_WARN(
        this->get_logger(), "cmd_vel 超限，已截断: linear=(%.3f, %.3f) angular.z=%.3f",
        out.linear.x, out.linear.y, out.angular.z);
    }
  }

  double max_linear_vel_;
  double max_angular_vel_;

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_sub_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_safe_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr robot_status_pub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<SafetyNode>());
  rclcpp::shutdown();
  return 0;
}