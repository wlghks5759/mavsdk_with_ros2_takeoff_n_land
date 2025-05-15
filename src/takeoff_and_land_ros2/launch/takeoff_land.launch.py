from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            'connection_url',
            default_value='udpin://0.0.0.0:14540', # Default for SITL
            description='MAVSDK connection URL (e.g., serial:///dev/ttyUSB0:57600, udpin://0.0.0.0:14540, tcpin://:5790)'
        ),

        Node(
            package='takeoff_and_land_ros2',
            executable='takeoff_and_land_node',
            name='takeoff_and_land_node',
            output='screen',
            emulate_tty=True, # For MAVSDK log output if any, and RCLCPP_INFO
            parameters=[{
                'connection_url': LaunchConfiguration('connection_url')
            }]
        )
    ])