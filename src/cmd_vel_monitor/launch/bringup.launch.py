"""启动 cmd_vel_monitor 的全部节点，并加载速度限制参数。"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    # 参数文件在安装目录（install/.../share/cmd_vel_monitor/）下的位置
    pkg_share = get_package_share_directory('cmd_vel_monitor')
    params_file = os.path.join(pkg_share, 'config', 'parameters.yaml')

    return LaunchDescription([
        # 安全节点：订阅 /cmd_vel，截断后发布 /cmd_vel_safe 和 /robot_status
        Node(
            package='cmd_vel_monitor',
            executable='safety_node',
            name='safety_node',
            output='screen',
            parameters=[params_file],
        ),
        # 监视节点：打印状态到终端
        Node(
            package='cmd_vel_monitor',
            executable='monitor_node',
            name='monitor_node',
            output='screen',
        ),
    ])