/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_devicefind.c
 *
 * Description: Find your hardware's information while working any mode.
 *
 * Modification history:
 *     2014/3/12, v1.0 create this file.
 *******************************************************************************/

#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_udp.h"
#include "user_function.h"

/*---------------------------------------------------------------------------*/
LOCAL struct espconn ptrespconn;

/******************************************************************************
 * FunctionName : user_devicefind_init
 * Description  : the espconn struct parame init
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_devicefind_init(int port) {
	ptrespconn.type = ESPCONN_UDP;
	ptrespconn.proto.udp = (esp_udp *) os_zalloc(sizeof(esp_udp));
	ptrespconn.proto.udp->local_port = port;
	espconn_regist_recvcb(&ptrespconn, user_con_received);
	espconn_create(&ptrespconn);
}

void ICACHE_FLASH_ATTR
user_udp_send(uint8_t *s) {
	if (wifi_station_get_connect_status() == STATION_GOT_IP || wifi_softap_get_station_num() > 0) {
		ptrespconn.proto.udp->remote_port = 10181;	//获取端口
		ptrespconn.proto.udp->remote_ip[0] = 255;	//获取IP地址
		ptrespconn.proto.udp->remote_ip[1] = 255;
		ptrespconn.proto.udp->remote_ip[2] = 255;
		ptrespconn.proto.udp->remote_ip[3] = 255;
		espconn_send(&ptrespconn, s, os_strlen(s));	//发送数据
	}
}

//void ICACHE_FLASH_ATTR
//user_udp_send_debug(int port,uint8_t *s) {
//	if (wifi_station_get_connect_status() == STATION_GOT_IP || wifi_softap_get_station_num() > 0) {
//		ptrespconn.proto.udp->remote_port = port;	//获取端口
//		ptrespconn.proto.udp->remote_ip[0] = 255;	//获取IP地址
//		ptrespconn.proto.udp->remote_ip[1] = 255;
//		ptrespconn.proto.udp->remote_ip[2] = 255;
//		ptrespconn.proto.udp->remote_ip[3] = 255;
//		espconn_send(&ptrespconn, s, os_strlen(s));	//发送数据
//	}
//}
