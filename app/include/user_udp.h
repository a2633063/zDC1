#ifndef __USER_DEVICEFIND_H__
#define __USER_DEVICEFIND_H__
#include "ets_sys.h"
void user_devicefind_init(int port);
void user_udp_send(uint8_t *s);
//void user_udp_send_debug(int port,uint8_t *s);
#endif
