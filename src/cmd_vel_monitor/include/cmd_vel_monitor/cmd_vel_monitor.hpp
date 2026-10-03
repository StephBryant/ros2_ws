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

#ifndef CMD_VEL_MONITOR__CMD_VEL_MONITOR_HPP_
#define CMD_VEL_MONITOR__CMD_VEL_MONITOR_HPP_

#include <memory>

#include "rclcpp/rclcpp.hpp"

namespace cmd_vel_monitor
{

/// Minimal skeleton node; subscription logic goes here.
class CmdVelMonitor : public rclcpp::Node
{
public:
  explicit CmdVelMonitor(const rclcpp::NodeOptions & options);
};

}  // namespace cmd_vel_monitor

#endif  // CMD_VEL_MONITOR__CMD_VEL_MONITOR_HPP_