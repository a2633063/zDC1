#ifndef __USER_LED_H__
#define __USER_LED_H__

#include "gpio.h"

#define GPIO_LED_LOGO_IO_MUX     PERIPHS_IO_MUX_MTMS_U
#define GPIO_LED_LOGO_IO_NUM     14
#define GPIO_LED_LOGO_IO_FUNC    FUNC_GPIO14

#define GPIO_LED_WIFI_IO_MUX     PERIPHS_IO_MUX_GPIO0_U
#define GPIO_LED_WIFI_IO_NUM     0
#define GPIO_LED_WIFI_IO_FUNC    FUNC_GPIO0

void user_led_init(void);
void user_set_led_logo(int8_t level);
void user_set_led_wifi(int8_t level);


#endif
