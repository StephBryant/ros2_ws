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

#include <memory>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

// 监视节点：订阅安全节点处理过的速度指令和状态，把状态打印到终端
class MonitorNode : public rclcpp::Node
{
public:
  MonitorNode()
  : rclcpp::Node("monitor_node")
  {
    // 注意：QoS 必须和发布端一致。
    // safety_node 用的是 SensorDataQoS（best_effort）发布，
    // 如果这里用默认的 reliable QoS 订阅，就一条都收不到。
    cmd_vel_safe_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
      "/cmd_vel_safe", rclcpp::SensorDataQoS(),
      std::bind(&MonitorNode::cmd_vel_safe_callback, this, std::placeholders::_1));

    robot_status_sub_ = this->create_subscription<std_msgs::msg::String>(
      "/robot_status", rclcpp::SensorDataQoS(),
      std::bind(&MonitorNode::robot_status_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "monitor_node 已启动，正在监听 /cmd_vel_safe 和 /robot_status ...");
  }

private:
  // 收到安全节点处理过的速度，打印出来
  void cmd_vel_safe_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
  {
    RCLCPP_INFO(
      this->get_logger(), "安全速度指令: 线速度 x=%.3f y=%.3f, 角速度 z=%.3f",
      msg->linear.x, msg->linear.y, msg->angular.z);
  }

  // 收到状态字符串，直接打印出来（内容是 safety_node 发过来的“正常”或“触发限制”）
  void robot_status_callback(const std_msgs::msg::String::SharedPtr msg)
  {
    RCLCPP_INFO(this->get_logger(), "机器人当前状态: %s", msg->data.c_str());
  }

  rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_safe_sub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr robot_status_sub_;
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MonitorNode>());
  rclcpp::shutdown();
  return 0;
}