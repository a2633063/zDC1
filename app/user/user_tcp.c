/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_webserver.c
 *
 * Description: The web server mode configration.
 *              Check your hardware connection with the host while use this mode.
 * Modification history:
 *     2014/3/12, v1.0 create this file.
 *******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"

#include "espconn.h"
#include "user_tcp.h"

#include "../include/user_udp.h"
#include "user_update.h"
#include "user_function.h"

LOCAL struct espconn ptrespconn;


/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL ICACHE_FLASH_ATTR
void tcp_recon(void *arg, sint8 err) {
	struct espconn *pesp_conn = arg;

	os_printf("tcp reconnect: %d.%d.%d.%d:%d err %d \n", pesp_conn->proto.tcp->remote_ip[0],
			pesp_conn->proto.tcp->remote_ip[1], pesp_conn->proto.tcp->remote_ip[2], pesp_conn->proto.tcp->remote_ip[3],
			pesp_conn->proto.tcp->remote_port, err);
}

/******************************************************************************
 * FunctionName : webserver_recon
 * Description  : the connection has been err, reconnection
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL ICACHE_FLASH_ATTR
void tcp_discon(void *arg) {
	struct espconn *pesp_conn = arg;

	os_printf("tcp disconnect: %d.%d.%d.%d:%d disconnect\n", pesp_conn->proto.tcp->remote_ip[0],
			pesp_conn->proto.tcp->remote_ip[1], pesp_conn->proto.tcp->remote_ip[2], pesp_conn->proto.tcp->remote_ip[3],
			pesp_conn->proto.tcp->remote_port);
}

LOCAL ICACHE_FLASH_ATTR
void tcp_con(void *arg) {
	struct espconn *pesp_conn = arg;

	os_printf("tcp connected: %d.%d.%d.%d:%d \n", pesp_conn->proto.tcp->remote_ip[0],
			pesp_conn->proto.tcp->remote_ip[1], pesp_conn->proto.tcp->remote_ip[2], pesp_conn->proto.tcp->remote_ip[3],
			pesp_conn->proto.tcp->remote_port);
}

/******************************************************************************
 * FunctionName : user_accept_listen
 * Description  : server listened a connection successfully
 * Parameters   : arg -- Additional argument to pass to the callback function
 * Returns      : none
 *******************************************************************************/
LOCAL void ICACHE_FLASH_ATTR
tcp_listen(void *arg) {
	struct espconn *pesp_conn = arg;

	espconn_regist_connectcb(pesp_conn, tcp_con);
	espconn_regist_recvcb(pesp_conn, user_con_received);
//	espconn_regist_recvcb(pesp_conn, tcp_recv);
	espconn_regist_reconcb(pesp_conn, tcp_recon);
	espconn_regist_disconcb(pesp_conn, tcp_discon);
}

/******************************************************************************
 * FunctionName : user_webserver_init
 * Description  : parameter initialize as a server
 * Parameters   : port -- server port
 * Returns      : none
 *******************************************************************************/
void ICACHE_FLASH_ATTR
user_tcp_init(uint32 port) {
	LOCAL struct espconn esp_conn;
	LOCAL esp_tcp esptcp;

	esp_conn.type = ESPCONN_TCP;
	esp_conn.state = ESPCONN_NONE;
	esp_conn.proto.tcp = &esptcp;
	esp_conn.proto.tcp->local_port = port;
	espconn_regist_connectcb(&esp_conn, tcp_listen);

	espconn_accept(&esp_conn);

}
