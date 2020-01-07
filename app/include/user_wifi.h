#ifndef __USER_WIFI_H__
#define __USER_WIFI_H__


#define GPIO_WIFI_LED_IO_MUX     PERIPHS_IO_MUX_GPIO0_U
#define GPIO_WIFI_LED_IO_NUM     0
#define GPIO_WIFI_LED_IO_FUNC    FUNC_GPIO0

extern char hwaddr[6];
extern char strMac[16];
extern char strIP[16];


void user_wifi_init(void);
void user_smartconfig(void);
void user_smartconfig_stop(void);
bool user_smartconfig_is_starting();
#endif
