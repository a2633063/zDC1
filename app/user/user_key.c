#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"

#include "../cJson/cJSON.h"
#include "user_key.h"
#include "user_led.h"
#include "user_wifi.h"
#include "user_json.h"
#include "user_setting.h"

#define KEY_LONG_PRESS_TIME 10
LOCAL os_timer_t timer_key;

uint8_t user_smarconfig_flag = 0;

LOCAL uint8_t key_time = 0;

LOCAL void ICACHE_FLASH_ATTR
user_key_short_press(void) {
	uint8_t i,result = 1;
	char strJson[128];
	os_printf("user_key_short_press\n");
	if (user_smartconfig_is_starting()) {
		user_smartconfig_stop();
		return;
	}

	for (i = 0; i < PLUG_NUM; i++) {
		if (user_config.plug[i].on != 0) {
			result = 0;
			break;
		}
	}

	os_sprintf(strJson, "{\"mac\":\"%s\",\"plug_0\":{\"on\":%d},\"plug_1\":{\"on\":%d},\"plug_2\":{\"on\":%d},\"plug_3\":{\"on\":%d}}", strMac,
			result, result, result, result);
	user_json_analysis(false, strJson);
}

LOCAL void ICACHE_FLASH_ATTR
user_key_long_5s_press(void) {
	os_printf("user_key_long_5s_press\n");
	user_smartconfig();
}

LOCAL void ICACHE_FLASH_ATTR
user_key_long_10s_press(void) {
	os_printf("user_key_long_10s_press\n");
	//恢复出厂设置
	user_config.name[0] = 0xff;
	user_config.name[1] = 0xff;
	user_config.name[2] = 0xff;
	user_config.name[3] = 0;
	user_setting_set_config();
	system_restore();
}

void ICACHE_FLASH_ATTR user_key_timer_func(void *arg) {

	static uint8_t key_trigger, key_continue;
	static uint8_t key_last;
	static uint8_t key_timer_num = 0;

	uint8_t state = ~(0xfe | gpio16_input_get());
	key_trigger = state & (state ^ key_continue);
	key_continue = state;
//	os_printf("pressed:%x	%x\n", key_trigger, key_continue);

	if (key_trigger != 0)
		key_time = 0; //新按键按下时,重新开始按键计时
	if (key_continue != 0) {
		key_timer_num++;
		if (key_timer_num > 4) {
			key_timer_num = 0;
			//any button pressed
			key_time++;
			if (key_time < KEY_LONG_PRESS_TIME)
				key_last = key_continue;
			else {
				os_printf("button long pressed:%d\n", key_time);

				if (key_time == 50) {
					user_key_long_5s_press();
				} else if (key_time == 100) {
					user_key_long_10s_press();
				} else if (key_time == 102) {
					user_set_led_wifi(1);
					user_set_led_logo(1);
				} else if (key_time == 103) {
					user_set_led_wifi(0);
					user_set_led_logo(0);
					key_time = 101;
				}
			}
		}
	} else if (key_time > 0) {
		//button released
		if (key_time < KEY_LONG_PRESS_TIME) {   //100ms*10=1s 大于1s为长按

//	            os_printf("button short pressed:%d",key_time);
			user_key_short_press();
		} else if (key_time > 100) {
			system_restart();
		}
		key_time = 0;
		key_last = 0;

	}

}

void ICACHE_FLASH_ATTR
user_key_init(void) {

	//GPIO16不支持外部中断,所以无法直接使用key driver
	gpio16_input_conf();	//配置按键GPIO16为输入

	os_timer_disarm(&timer_key);
	os_timer_setfn(&timer_key, (os_timer_func_t *) user_key_timer_func, NULL);
	os_timer_arm(&timer_key, 20, 1);	//100ms

}
