/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/1/23, v1.0 create this file.
 *******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_interface.h"

#include "user_config.h"
#include "../cJson/cJSON.h"
#include "user_key.h"
#include "user_wifi.h"
#include "user_sntp.h"

LOCAL os_timer_t timer_rtc;

uint32 utc_time = 0;
void ICACHE_FLASH_ATTR user_os_timer_func(void *arg) {
	static uint8_t timer_count = 0;
	uint8_t DeviceBuffer[28] = { 0 };
	int8_t task_flag[PLUG_NUM] = { -1, -1, -1, -1 };   //记录每个插座哪个任务需要返回数据
	uint8_t i, j;

	if (utc_time == 0 || (time.second == 59 && time.minute == 59)) { //每小时校准一次
		if (wifi_station_get_connect_status() == STATION_GOT_IP) {
			utc_time = sntp_get_current_timestamp();
		}
	}

	if (utc_time > 0) {
		utc_time++;
		os_sprintf(DeviceBuffer, "%s", sntp_get_real_time(utc_time));
		time_strtohex(DeviceBuffer);

		if (time.second == 0)
			os_printf("20%02d/%02d/%02d 周%d %02d:%02d:%02d\n", time.year, time.month, time.day, time.week, time.hour, time.minute, time.second);

		bool update_user_config_flag = false;
		for (i = 0; i < PLUG_NUM; i++) {
			for (j = 0; j < PLUG_TIME_TASK_NUM; j++) {
				if (user_config.plug[i].task[j].on != 0) {

					uint8_t repeat = user_config.plug[i].task[j].repeat;
					if (    //符合条件则改变继电器状态: 秒为0 时分符合设定值, 重复符合设定值
					time.second == 0 && time.minute == user_config.plug[i].task[j].minute && time.hour == user_config.plug[i].task[j].hour
							&& ((repeat == 0x00) || repeat & (1 << (time.week - 1)))) {
						if (user_config.plug[i].on != user_config.plug[i].task[j].action) {
							user_config.plug[i].on = user_config.plug[i].task[j].action;
							//user_io_set_plug(i, user_config.plug[i].task[j].action);
							update_user_config_flag = true;
						}
						if (repeat == 0x00) {
							task_flag[i] = j;
							user_config.plug[i].task[j].on = 0;
							update_user_config_flag = true;
						}
						if (i > 0 && user_config.plug[i].on == 1)
							user_config.plug[0].on = 1;

					}
				}
			}
		}

		//更新储存数据 更新定时任务数据
		if (update_user_config_flag == true) {
			os_printf("update_user_config_flag");
			user_io_set_plug_all(2, 2, 2, 2);
			user_setting_set_config();
			update_user_config_flag = false;

			cJSON *json_send = cJSON_CreateObject();
			cJSON_AddStringToObject(json_send, "mac", strMac);

			for (i = 0; i < PLUG_NUM; i++) {
				char strTemp1[] = "plug_X";
				strTemp1[5] = i + '0';
				cJSON *json_send_plug = cJSON_CreateObject();
				cJSON_AddNumberToObject(json_send_plug, "on", user_config.plug[i].on);

				if (task_flag[i] >= 0) {
					cJSON *json_send_plug_setting = cJSON_CreateObject();

					j = task_flag[i];
					char strTemp2[] = "task_X";
					strTemp2[5] = j + '0';
					cJSON *json_send_plug_task = cJSON_CreateObject();
					cJSON_AddNumberToObject(json_send_plug_task, "hour", user_config.plug[i].task[j].hour);
					cJSON_AddNumberToObject(json_send_plug_task, "minute", user_config.plug[i].task[j].minute);
					cJSON_AddNumberToObject(json_send_plug_task, "repeat", user_config.plug[i].task[j].repeat);
					cJSON_AddNumberToObject(json_send_plug_task, "action", user_config.plug[i].task[j].action);
					cJSON_AddNumberToObject(json_send_plug_task, "on", user_config.plug[i].task[j].on);
					cJSON_AddItemToObject(json_send_plug_setting, strTemp2, json_send_plug_task);

					cJSON_AddItemToObject(json_send_plug, "setting", json_send_plug_setting);

					task_flag[i] = -1;
				}
				cJSON_AddItemToObject(json_send, strTemp1, json_send_plug);
			}

			char *json_str = cJSON_Print(json_send);
			user_send( false, json_str);    //发送数据

			os_free(json_str);
			cJSON_Delete(json_send);
			//            os_printf("cJSON_Delete");
		}

	}

	timer_count++;
	if (timer_count >= user_config.interval) {
		timer_count = 0;
		user_send_power();
	}
}

void ICACHE_FLASH_ATTR
user_os_timer_init(void) {
	os_timer_disarm(&timer_rtc);
	os_timer_setfn(&timer_rtc, (os_timer_func_t *) user_os_timer_func, NULL);
	os_timer_arm(&timer_rtc, 1000, 1);
}
