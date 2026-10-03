# cmd_vel_monitor — 速度指令安全监控

对机器人 `/cmd_vel` 速度指令做限幅与异常值过滤，并将处理后的安全速度与机器人状态提供给下游。

ROS 2 Humble，ament_cmake，C++17。

---

## 一、系统结构

两个节点，启动后自动完成分工：

```
                    ┌──────────────┐      ┌──────────────┐
   /cmd_vel ───────▶│  safety_node │─────▶│ monitor_node │──▶ 终端打印
   (geometry_msgs/  │  截断 + 清洗  │      │  状态监视     │
    msg/Twist)      └──────┬───────┘      └──────▲───────┘
                          │                      │
                          │   /cmd_vel_safe      │ /robot_status
                          │   (geometry_msgs/    │ (std_msgs/msg/
                          │    msg/Twist)        │  String)
                          └──────────────────────┘
```

### 1. safety_node（安全处理节点）

订阅 `/cmd_vel`，对每个速度分量做两项处理：

- **截断限幅**：线性速度超过 `max_linear_vel`、角速度超过 `max_angular_vel` 时，截断到 ±上限（保留符号，倒车同样受限）。
- **异常值清洗**：分量为 `NaN`、`+inf`、`-inf` 时置为 `0`。

处理顺序为**先清洗、后截断**。原因是 `NaN` 与任何数值比较恒返回 `false`，若直接调用 `std::min` / `std::max` / `std::clamp`，`NaN` 会原样透传，异常值就拦不住。

### 2. monitor_node（状态监视节点）

订阅两个话题并打印到终端：
- `/robot_status` → 打印当前机器人状态（"正常" 或 "触发限制"）
- `/cmd_vel_safe` → 打印限幅后的实际速度

---

## 二、Topic 输入输出

| 方向 | 话题名 | 类型 | 说明 |
|------|--------|------|------|
| 输入 | `/cmd_vel` | `geometry_msgs/msg/Twist` | 原始速度指令 |
| 输出 | `/cmd_vel_safe` | `geometry_msgs/msg/Twist` | 限幅清洗后的安全速度 |
| 输出 | `/robot_status` | `std_msgs/msg/String` | `"正常"` 或 `"触发限制"` |

QoS：三个话题均使用 `rclcpp::SensorDataQoS()`（best_effort），与常见速度指令发布端兼容。

**注意**：下游节点订阅 `/cmd_vel_safe` 或 `/robot_status` 时必须使用 best_effort QoS，用默认的 reliable QoS 会收不到消息。

处理的速度分量为 `linear.x`、`linear.y`、`angular.z`；其余分量（`linear.z`、`angular.x/y`）原样透传。

---

## 三、关键参数

在 `config/parameters.yaml` 中配置，由 `safety_node` 声明：

| 参数名 | 类型 | 默认值 | 单位 | 说明 |
|--------|------|--------|------|------|
| `max_linear_vel` | double | 0.5 | m/s | 线性速度上限 |
| `max_angular_vel` | double | 1.0 | rad/s | 角速度上限 |

启动时打印实际生效值，便于确认配置已加载：

```
[INFO] [safety_node]: max_linear_vel=0.500, max_angular_vel=1.000
```

---

## 四、编译与运行

```bash
# 1. 编译
cd ~/work/1/ros2_ws
colcon build
source install/setup.bash

# 2. 启动（一条命令拉起两个节点并加载参数）
ros2 launch cmd_vel_monitor bringup.launch.py

# 3. 播放录包验证
ros2 bag play /home/rrobot/cmd_vel

# 按 Ctrl+C 退出，launch 会自动清理两个子进程
```

单独启动某个节点：

```bash
ros2 run cmd_vel_monitor safety_node
ros2 run cmd_vel_monitor monitor_node
```

---

## 五、如何验证

### 方法 1：播放 rosbag（主要验证方式）

```bash
ros2 bag info /home/rrobot/cmd_vel
#   /cmd_vel  geometry_msgs/msg/Twist   320 帧 / 35.9 秒

ros2 launch cmd_vel_monitor bringup.launch.py    # 终端 A
ros2 bag play /home/rrobot/cmd_vel               # 终端 B
```

实测结果（完整播放 320 帧）：

| 统计项 | 数量 |
|--------|------|
| 收到的 `/cmd_vel` 帧数 | 320 |
| 输出的 `/cmd_vel_safe` 帧数 | 320 |
| 输出的 `/robot_status` 帧数 | 320 |
| 其中状态为 "正常" | 263 |
| 其中状态为 "触发限制" | 57 |

输入 320 帧、输出 320 帧，逐条一一对应，**无丢失、无重复**，符合设计预期。

### 方法 2：手动发布测试指令

```bash
# 正常速度 → 状态"正常"
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 0.2}, angular: {z: 0.5}}"

# 超限速度 → 被截断到 ±上限，状态"触发限制"
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: 3.0}, angular: {z: -5.0}}"

# 异常值 → 归零，状态"触发限制"
ros2 topic pub --once /cmd_vel geometry_msgs/msg/Twist \
  "{linear: {x: .nan}, angular: {z: .inf}}"
```

预期终端输出：

```
[INFO] [monitor_node]: 安全速度指令: 线速度 x=0.500 y=0.000, 角速度 z=-1.000
[INFO] [monitor_node]: 机器人当前状态: 触发限制
```

### 方法 3：rqt_graph 查看拓扑

```bash
rqt_graph        # 或 ros2 run rqt_graph rqt_graph
```

点击工具栏刷新按钮，应看到 `/safety_node` 与 `/monitor_node` 两个节点，
以及 `/cmd_vel → safety_node → /cmd_vel_safe / robot_status → monitor_node` 的连线。

---

## 六、目录结构

```
cmd_vel_monitor/
├── CMakeLists.txt
├── package.xml
├── README.md
├── config/
│   └── parameters.yaml       # 速度上限参数
├── launch/
│   └── bringup.launch.py     # 一次启动两个节点
├── include/cmd_vel_monitor/
│   └── cmd_vel_monitor.hpp
└── src/
    ├── safety_node.cpp       # 安全处理节点
    ├── monitor_node.cpp      # 状态监视节点
    ├── cmd_vel_monitor.cpp   # 早期脚手架代码（作业未使用）
    └── main.cpp              # 早期脚手架入口（作业未使用）
```

`src/cmd_vel_monitor.cpp` 与 `src/main.cpp` 是最初建立包结构时生成的脚手架，
已由上述两个节点取代，不参与作业功能。

---

## 七、AI 使用情况

**本项目在 AI 辅助下完成了基础代码架构。**

具体而言：项目初期在 AI 辅助下完成了 ROS 2 包的结构搭建、CMakeLists.txt 与
package.xml 的构建配置，以及两个节点的基础代码框架。后续的逻辑设计
（限幅策略、NaN 清洗顺序、参数方案）、功能验证与文档整理由本人完成。

---

## 八、依赖

| 依赖包 | 用途 |
|--------|------|
| `rclcpp` | ROS 2 C++ 客户端库 |
| `geometry_msgs` | `Twist` 速度消息 |
| `std_msgs` | `String` 状态消息 |

均已在 `package.xml` 中声明，在 `CMakeLists.txt` 中通过 `find_package` +
`ament_target_dependencies` 引入。