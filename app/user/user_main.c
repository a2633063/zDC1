/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/1/1, v1.0 create this file.
 *******************************************************************************/
#include "../include/user_udp.h"
#include "ets_sys.h"
#include "osapi.h"

#include "user_wifi.h"
#include "user_interface.h"
#include "uart.h"
#include "user_config.h"
#include "user_udp.h"
#include "user_setting.h"
#include "user_os_timer.h"
#include "user_sntp.h"
#include "user_mqtt.h"
#include "user_io.h"

user_config_t user_config;
uint16_t power;
uint16_t voltage;
uint16_t current;
void user_rf_pre_init(void) {
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void) {
	int i, j;
	uint32 x;
	system_uart_swap();
	uart_init(4800, 115200);
	os_printf(" \n \nStart user%d.bin\n", system_upgrade_userbin_check() + 1);
	os_printf("SDK version:%s\n", system_get_sdk_version());
	os_printf("FW version:%s\n", VERSION);

	user_setting_init();
	user_key_init();
	user_led_init();
	user_io_init();
	user_wifi_init();
	user_sntp_init();
	user_os_timer_init();
	//UDP初始化,监听端口10182,当接收到特定字符串时,返回本设备IP及MAC地址
	user_devicefind_init(10182);
//	TCP初始化,监听端口10191
//	user_tcp_init(10191);
	//TCP初始化 80端口 webserver
	//user_webserver_init(80);

	user_io_set_plug_all(2,2,2,2);

}


