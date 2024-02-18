# micro-ROS-rtt-test-serial

This is an adaption of the ping-pong_client with the QoS set to best-effort and the example of int32_publisher_custom_transport adapted with the code of the ping_pong example, named then "ping_pong_custom_transport".

micro-ROS-rtt repo for the ping_pong_client: https://github.com/micro-ROS/micro-ROS-rtt

## For testing: 

ESP32 Side:
1. build the micro-ROS-rtt on a ROS2-System
2. configure UART Pins in the menuconfig (normally TX:1, RX:3) and set the transport to serial (also in colcon.meta)
3. build and flash "ping_pong_custom_transport" on an esp32
4. Start the micro-ROS-agent for serial with: 
`` ros2 run microros-agent serial --dev /dev/<USB-Port> -v6 ``

ROS2 Device:

1. Build and source the "micro-ROS-rtt"-folder. 
2. Start the pingpong_client with: 
`ros2 run micro_ros_rtt pingpong_client --ros-args -p max_messages:=<n msgs> > <path_to_data_file> `
