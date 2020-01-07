#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"


#include "user_led.h"

void ICACHE_FLASH_ATTR
user_led_init(void) {
	//PIN_PULLUP_EN(PERIPHS_IO_MUX_GPIO0_U);
	PIN_FUNC_SELECT(GPIO_LED_LOGO_IO_MUX, GPIO_LED_LOGO_IO_FUNC);
	PIN_FUNC_SELECT(GPIO_LED_WIFI_IO_MUX, GPIO_LED_WIFI_IO_FUNC);
	user_set_led_logo(0);
	user_set_led_wifi(0);
}

void ICACHE_FLASH_ATTR
user_set_led_logo(int8_t level) {
	if (level != -1) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_LED_LOGO_IO_NUM), !level);
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_LED_LOGO_IO_NUM), !GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_LED_LOGO_IO_NUM)));
	}
}

void ICACHE_FLASH_ATTR
user_set_led_wifi(int8_t level) {
	if (level != -1) {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_LED_WIFI_IO_NUM), !level);
	} else {
		GPIO_OUTPUT_SET(GPIO_ID_PIN(GPIO_LED_WIFI_IO_NUM), !GPIO_INPUT_GET(GPIO_ID_PIN(GPIO_LED_WIFI_IO_NUM)));
	}
}
