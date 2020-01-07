#ifndef __USER_FUNCTION_H__
#define __USER_FUNCTION_H__


void user_send(bool udp_flag,uint8_t *s, char retained );
void user_con_received(void *arg, char *pusrdata, unsigned short length) ;
void user_send_power(void) ;
#endif
