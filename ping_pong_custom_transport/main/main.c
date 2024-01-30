#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "driver/uart.h"

#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <std_msgs/msg/header.h>
#include <std_msgs/msg/int32.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

#include <rmw_microxrcedds_c/config.h>
#include <rmw_microros/rmw_microros.h>
#include "esp32_serial_transport.h"

#define STRING_BUFFER_LEN 50

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Aborting.\n",__LINE__,(int)temp_rc);vTaskDelete(NULL);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){printf("Failed status on line %d: %d. Continuing.\n",__LINE__,(int)temp_rc);}}

rcl_publisher_t publisher;
rcl_subscription_t subscriber;
std_msgs__msg__Header send_msg;
std_msgs__msg__Header recv_msg;
uint data=0;

/*void timer_callback(rcl_timer_t * timer, int64_t last_call_time)
{
	send_msg.stamp.sec=data++;
	send_msg.frame_id.data="Hallo";
	(void) last_call_time;
	if (timer != NULL) {
		RCSOFTCHECK(rcl_publish(&publisher, &send_msg, NULL));
	printf("Sent: %s\n", send_msg.frame_id.data);
	}
}*/

void subscription_callback(const void * msgin)
{
	const std_msgs__msg__Header * msg = (const std_msgs__msg__Header *)msgin;
	printf("Ping received with seq %s. Answering.\n", msg->frame_id.data);
	rcl_publish(&publisher, (const void*)msg, NULL);
}

void micro_ros_task(void * arg)
{
	rcl_allocator_t allocator = rcl_get_default_allocator();
	rclc_support_t support;

	// Create init_options.
	rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
	RCCHECK(rcl_init_options_init(&init_options, allocator));

	// Setup support structure.
	RCCHECK(rclc_support_init_with_options(&support, 0, NULL, &init_options, &allocator));

	// Create node.
	rcl_node_t node = rcl_get_zero_initialized_node();
	RCCHECK(rclc_node_init_default(&node, "pingpong", "", &support));

	// Create publisher.
	RCCHECK(rclc_publisher_init_best_effort(
		&publisher,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Header),
		"/uros_pong"));

	// Create subscriber.
	RCCHECK(rclc_subscription_init_best_effort(
		&subscriber,
		&node,
		ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Header),
		"/uros_ping"));
	
	// Create timer.
	/*rcl_timer_t timer = rcl_get_zero_initialized_timer();
	const unsigned int timer_timeout = 1000;
	RCCHECK(rclc_timer_init_default(
		&timer,
		&support,
		RCL_MS_TO_NS(timer_timeout),
		timer_callback));*/

	// Create executor.
	rclc_executor_t executor = rclc_executor_get_zero_initialized_executor();
	RCCHECK(rclc_executor_init(&executor, &support.context, 2, &allocator));
	unsigned int rcl_wait_timeout = 1000;   // in ms
	RCCHECK(rclc_executor_set_timeout(&executor, RCL_MS_TO_NS(rcl_wait_timeout)));

	// Add timer and subscriber to executor.
	//RCCHECK(rclc_executor_add_timer(&executor, &timer));
	RCCHECK(rclc_executor_add_subscription(&executor, &subscriber, &recv_msg, 
	&subscription_callback, ON_NEW_DATA));

	// Create and allocate the incoming messages
	char recv_buffer[STRING_BUFFER_LEN];
	recv_msg.frame_id.data = recv_buffer;
	recv_msg.frame_id.capacity = STRING_BUFFER_LEN;

	// Spin forever.
	rclc_executor_spin(&executor);

	/*while(1){
		rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100));
		usleep(100000);
	}*/

	// Free resources.
	RCCHECK(rcl_subscription_fini(&subscriber, &node));
	RCCHECK(rcl_publisher_fini(&publisher, &node));
	RCCHECK(rcl_node_fini(&node));

  	vTaskDelete(NULL);
}

static size_t uart_port = UART_NUM_0;

void app_main(void)
{
#if defined(RMW_UXRCE_TRANSPORT_CUSTOM)
	rmw_uros_set_custom_transport(
		true,
		(void *) &uart_port,
		esp32_serial_open,
		esp32_serial_close,
		esp32_serial_write,
		esp32_serial_read
	);
#else
#error micro-ROS transports misconfigured
#endif  // RMW_UXRCE_TRANSPORT_CUSTOM

    xTaskCreate(micro_ros_task,
            "uros_task",
            CONFIG_MICRO_ROS_APP_STACK,
            NULL,
            CONFIG_MICRO_ROS_APP_TASK_PRIO,
            NULL);
}
