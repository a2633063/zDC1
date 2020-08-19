#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "espconn.h"

#include "user_config.h"
#include "../cJson/cJSON.h"
#include "user_wifi.h"
#include "user_io.h"
#include "user_update.h"
#include "user_json.h"
#include "user_setting.h"
#include "user_function.h"


bool ICACHE_FLASH_ATTR json_plug_analysis(int udp_flag, unsigned char x, cJSON * pJsonRoot, cJSON * pJsonSend);
bool ICACHE_FLASH_ATTR json_plug_task_analysis(unsigned char x, unsigned char y, cJSON * pJsonRoot, cJSON * pJsonSend);

void ICACHE_FLASH_ATTR user_json_analysis(bool udp_flag, u8* jsonRoot) {
	uint8_t i;
	bool update_user_config_flag = false;   //标志位,记录最后是否需要更新储存的数据
	uint8_t plug_retained = 0;
	cJSON *pJsonRoot = cJSON_Parse(jsonRoot);	//首先整体判断是否为一个json格式的数据
	//如果是否json格式数据
	if (pJsonRoot != NULL) {

		//串口打印数据
//		char *s = cJSON_Print(pJsonRoot);
//		os_printf("pJsonRoot: %s\r\n", s);
//		cJSON_free((void *) s);

//解析device report
		os_printf("start json:\r\n");
		cJSON *p_cmd = cJSON_GetObjectItem(pJsonRoot, "cmd");
		if (p_cmd && cJSON_IsString(p_cmd) && os_strcmp(p_cmd->valuestring, "device report") == 0) {

			os_printf("device report\r\n");
			cJSON *pRoot = cJSON_CreateObject();
			cJSON_AddStringToObject(pRoot, "name", user_config.name);
			cJSON_AddStringToObject(pRoot, "ip", strIP);
			cJSON_AddStringToObject(pRoot, "mac", strMac);
			cJSON_AddNumberToObject(pRoot, "type", TYPE);
			cJSON_AddStringToObject(pRoot, "type_name", TYPE_NAME);
			char *s = cJSON_Print(pRoot);
			os_printf("pRoot: %s\r\n", s);

			user_send(udp_flag, s, 0);

			cJSON_free((void *) s);
			cJSON_Delete(pRoot);
		}

		//解析
		cJSON *p_name = cJSON_GetObjectItem(pJsonRoot, "name");
		cJSON *p_mac = cJSON_GetObjectItem(pJsonRoot, "mac");

		//
		if ((p_name && cJSON_IsString(p_name) && os_strcmp(p_name->valuestring, user_config.name) == 0) //name
		|| (p_mac && cJSON_IsString(p_mac) && os_strcmp(p_mac->valuestring, strMac) == 0)	//mac
				) {

			cJSON *json_send = cJSON_CreateObject();
			cJSON_AddStringToObject(json_send, "mac", strMac);

			//解析版本
			cJSON *p_version = cJSON_GetObjectItem(pJsonRoot, "version");
			if (p_version) {
				cJSON_AddStringToObject(json_send, "version", VERSION);
			}

			//返回wifi ssid
			cJSON *p_ssid = cJSON_GetObjectItem(pJsonRoot, "ssid");
			if (p_ssid) {
				struct station_config ssidGet;
				if (wifi_station_get_config_default(&ssidGet)) {
					cJSON_AddStringToObject(json_send, "ssid", ssidGet.ssid);
				} else {
					cJSON_AddStringToObject(json_send, "ssid", "get wifi_ssid fail");
				}
			}

			//设置上报频率
			cJSON *p_interval = cJSON_GetObjectItem(pJsonRoot, "interval");
			if (p_interval) {
				if (cJSON_IsNumber(p_interval) && p_interval->valueint > 0 && p_interval->valueint <= 255) {
					user_config.interval = p_interval->valueint;
					update_user_config_flag = true;
				}
				cJSON_AddNumberToObject(json_send, "interval", user_config.interval);
			}


			cJSON *p_setting = cJSON_GetObjectItem(pJsonRoot, "setting");
			if (p_setting) {

				//解析ota
				uint8_t userBin = system_upgrade_userbin_check();
				cJSON *p_ota1 = cJSON_GetObjectItem(p_setting, "ota1");
				cJSON *p_ota2 = cJSON_GetObjectItem(p_setting, "ota2");
				if (userBin == UPGRADE_FW_BIN2) {
					if (p_ota1 && cJSON_IsString(p_ota1)) {
						if (cJSON_IsString(p_ota1))
							user_ota_start(p_ota1->valuestring);
					}
				} else {
					if (p_ota2 && cJSON_IsString(p_ota2)) {
						if (cJSON_IsString(p_ota2))
							user_ota_start(p_ota2->valuestring);
					}
				}

				//设置设备名称
				cJSON *p_setting_name = cJSON_GetObjectItem(p_setting, "name");
				if (p_setting_name && cJSON_IsString(p_setting_name)) {
					update_user_config_flag = true;
					os_sprintf(user_config.name, p_setting_name->valuestring);
				}

				//设置wifi ssid
				cJSON *p_setting_wifi_ssid = cJSON_GetObjectItem(p_setting, "wifi_ssid");
				cJSON *p_setting_wifi_password = cJSON_GetObjectItem(p_setting, "wifi_password");
				if (p_setting_wifi_ssid && cJSON_IsString(p_setting_wifi_ssid) && p_setting_wifi_password
						&& cJSON_IsString(p_setting_wifi_password)) {

					user_wifi_set(p_setting_wifi_ssid->valuestring, p_setting_wifi_password->valuestring);
//					struct station_config stationConf;
//					stationConf.bssid_set = 0; //need not check MAC address of AP
//					os_sprintf(stationConf.ssid, p_setting_wifi_ssid->valuestring);
//					os_sprintf(stationConf.password, p_setting_wifi_password->valuestring);
//					wifi_station_set_config(&stationConf);
				}

				//设置mqtt ip
				cJSON *p_mqtt_ip = cJSON_GetObjectItem(p_setting, "mqtt_uri");
				if (p_mqtt_ip && cJSON_IsString(p_mqtt_ip)) {
					update_user_config_flag = true;
					os_sprintf(user_config.mqtt_ip, p_mqtt_ip->valuestring);
				}

				//设置mqtt port
				cJSON *p_mqtt_port = cJSON_GetObjectItem(p_setting, "mqtt_port");
				if (p_mqtt_port && cJSON_IsNumber(p_mqtt_port)) {
					update_user_config_flag = true;
					user_config.mqtt_port = p_mqtt_port->valueint;
				}

				//设置mqtt user
				cJSON *p_mqtt_user = cJSON_GetObjectItem(p_setting, "mqtt_user");
				if (p_mqtt_user && cJSON_IsString(p_mqtt_user)) {
					update_user_config_flag = true;
					os_sprintf(user_config.mqtt_user, p_mqtt_user->valuestring);
				}

				//设置mqtt password
				cJSON *p_mqtt_password = cJSON_GetObjectItem(p_setting, "mqtt_password");
				if (p_mqtt_password && cJSON_IsString(p_mqtt_password)) {
					update_user_config_flag = true;
					os_sprintf(user_config.mqtt_password, p_mqtt_password->valuestring);
				}

				//开始返回数据
				cJSON *json_setting_send = cJSON_CreateObject();
				//返回设备ota
				if (p_ota1)
					cJSON_AddStringToObject(json_setting_send, "ota1", p_ota1->valuestring);
				if (p_ota2)
					cJSON_AddStringToObject(json_setting_send, "ota2", p_ota2->valuestring);
				//设置设备名称
				if (p_setting_name)
					cJSON_AddStringToObject(json_setting_send, "name", user_config.name);

				//设置设备wifi
				if (p_setting_wifi_ssid || p_setting_wifi_password) {
					struct station_config configGet;
					if (wifi_station_get_config_default(&configGet)) {
						cJSON_AddStringToObject(json_setting_send, "wifi_ssid", configGet.ssid);
						cJSON_AddStringToObject(json_setting_send, "wifi_password", configGet.password);
					} else {
						cJSON_AddStringToObject(json_setting_send, "wifi_ssid", "get wifi_ssid fail");
						cJSON_AddStringToObject(json_setting_send, "wifi_password", "get wifi_password fail");
					}
				}

				//设置mqtt ip
				if (p_mqtt_ip)
					cJSON_AddStringToObject(json_setting_send, "mqtt_uri", user_config.mqtt_ip);

				//设置mqtt port
				if (p_mqtt_port)
					cJSON_AddNumberToObject(json_setting_send, "mqtt_port", user_config.mqtt_port);

				//设置mqtt user
				if (p_mqtt_user)
					cJSON_AddStringToObject(json_setting_send, "mqtt_user", user_config.mqtt_user);

				//设置mqtt password
				if (p_mqtt_password)
					cJSON_AddStringToObject(json_setting_send, "mqtt_password", user_config.mqtt_password);

				cJSON_AddItemToObject(json_send, "setting", json_setting_send);

				if ((p_mqtt_ip && cJSON_IsString(p_mqtt_ip) && p_mqtt_port && cJSON_IsNumber(p_mqtt_port) && p_mqtt_user
						&& cJSON_IsString(p_mqtt_user) && p_mqtt_password && cJSON_IsString(p_mqtt_password) && !user_mqtt_is_connect())) {
					system_restart();
				}
			}

			//解析plug-----------------------------------------------------------------
			for (i = 0; i < PLUG_NUM; i++) {
				if (json_plug_analysis(udp_flag, i, pJsonRoot, json_send)) {
					update_user_config_flag = true;
					plug_retained = 1;
				}
			}

			cJSON_AddNumberToObject(cJSON_GetObjectItem(json_send, "plug_0"), "on", user_config.plug[0].on);
			cJSON_AddNumberToObject(cJSON_GetObjectItem(json_send, "plug_1"), "on", user_config.plug[1].on);
			cJSON_AddNumberToObject(cJSON_GetObjectItem(json_send, "plug_2"), "on", user_config.plug[2].on);
			cJSON_AddNumberToObject(cJSON_GetObjectItem(json_send, "plug_3"), "on", user_config.plug[3].on);

			if (plug_retained == 1) {
				user_io_set_plug_all(user_config.plug[0].on, user_config.plug[1].on, user_config.plug[2].on, user_config.plug[3].on);
			}

			cJSON_AddStringToObject(json_send, "name", user_config.name);

			char *json_str = cJSON_Print(json_send);
			os_printf("json_send: %s\r\n", json_str);
			user_send(udp_flag, json_str, plug_retained);
			cJSON_free((void *) json_str);

			if (update_user_config_flag) {
				user_setting_set_config();
				update_user_config_flag = false;
			}

			cJSON_Delete(json_send);
		}

	} else {
		os_printf("this is not a json data:\r\n%s\r\n", jsonRoot);
	}

	cJSON_Delete(pJsonRoot);
//	os_printf("get freeHeap2: %d \n\n", system_get_free_heap_size());
}

/*
 *解析处理定时任务json
 *udp_flag:发送udp/mqtt标志位
 *x:插座编号
 */
bool ICACHE_FLASH_ATTR
json_plug_analysis(int udp_flag, unsigned char x, cJSON * pJsonRoot, cJSON * pJsonSend) {
	if (!pJsonRoot)
		return false;
	if (!pJsonSend)
		return false;
	char i;
	bool return_flag = false;
	char plug_str[] = "plug_X";
	plug_str[5] = x + '0';

	cJSON *p_plug = cJSON_GetObjectItem(pJsonRoot, plug_str);
	if (!p_plug)
		return_flag = false;

	cJSON *json_plug_send = cJSON_CreateObject();

	//解析plug on------------------------------------------------------
	if (p_plug) {
		cJSON *p_plug_on = cJSON_GetObjectItem(p_plug, "on");
		if (p_plug_on) {
			if (cJSON_IsNumber(p_plug_on)) {
				user_config.plug[x].on = !!(p_plug_on->valueint);
				//user_io_set_plug(x, p_plug_on->valueint);
				return_flag = true;
				if (x > 0 && user_config.plug[x].on == 1) {	//当打开分开关时,自动打开总开关
					user_config.plug[0].on = 1;
				} else if (x == 0 && user_config.plug[x].on == 0) {	//当关闭总开关时,关闭所有开关
					user_config.plug[1].on = 0;
					user_config.plug[2].on = 0;
					user_config.plug[3].on = 0;
				}
			}
		}

		//解析plug中setting项目----------------------------------------------
		cJSON *p_plug_setting = cJSON_GetObjectItem(p_plug, "setting");
		if (p_plug_setting) {
			cJSON *json_plug_setting_send = cJSON_CreateObject();
			//解析plug中setting中name----------------------------------------
			cJSON *p_plug_setting_name = cJSON_GetObjectItem(p_plug_setting, "name");
			if (p_plug_setting_name) {
				if (cJSON_IsString(p_plug_setting_name)) {
					return_flag = true;
					os_sprintf(user_config.plug[x].name, p_plug_setting_name->valuestring);
				}
				cJSON_AddStringToObject(json_plug_setting_send, "name", user_config.plug[x].name);
			}

			//解析plug中setting中task----------------------------------------
			for (i = 0; i < PLUG_TIME_TASK_NUM; i++) {
				if (json_plug_task_analysis(x, i, p_plug_setting, json_plug_setting_send))
					return_flag = true;
			}

			cJSON_AddItemToObject(json_plug_send, "setting", json_plug_setting_send);
		}
	}

//	cJSON_AddNumberToObject(json_plug_send, "on", user_config.plug[x].on);

	cJSON_AddItemToObject(pJsonSend, plug_str, json_plug_send);
	return return_flag;
}

/*
 *解析处理定时任务json
 *x:插座编号 y:任务编号
 */
bool ICACHE_FLASH_ATTR
json_plug_task_analysis(unsigned char x, unsigned char y, cJSON * pJsonRoot, cJSON * pJsonSend) {
	if (!pJsonRoot)
		return false;
	bool return_flag = false;

	char plug_task_str[] = "task_X";
	plug_task_str[5] = y + '0';

	cJSON *p_plug_task = cJSON_GetObjectItem(pJsonRoot, plug_task_str);
	if (!p_plug_task)
		return false;

	cJSON *json_plug_task_send = cJSON_CreateObject();

	cJSON *p_plug_task_hour = cJSON_GetObjectItem(p_plug_task, "hour");
	cJSON *p_plug_task_minute = cJSON_GetObjectItem(p_plug_task, "minute");
	cJSON *p_plug_task_repeat = cJSON_GetObjectItem(p_plug_task, "repeat");
	cJSON *p_plug_task_action = cJSON_GetObjectItem(p_plug_task, "action");
	cJSON *p_plug_task_on = cJSON_GetObjectItem(p_plug_task, "on");

	if (p_plug_task_hour && p_plug_task_minute && p_plug_task_repeat && p_plug_task_action && p_plug_task_on) {

		if (cJSON_IsNumber(p_plug_task_hour) && cJSON_IsNumber(p_plug_task_minute) && cJSON_IsNumber(p_plug_task_repeat)
				&& cJSON_IsNumber(p_plug_task_action) && cJSON_IsNumber(p_plug_task_on)) {
			return_flag = true;
			user_config.plug[x].task[y].hour = p_plug_task_hour->valueint;
			user_config.plug[x].task[y].minute = p_plug_task_minute->valueint;
			user_config.plug[x].task[y].repeat = p_plug_task_repeat->valueint;
			user_config.plug[x].task[y].action = p_plug_task_action->valueint;
			user_config.plug[x].task[y].on = p_plug_task_on->valueint;
		}

	}
	cJSON_AddNumberToObject(json_plug_task_send, "hour", user_config.plug[x].task[y].hour);
	cJSON_AddNumberToObject(json_plug_task_send, "minute", user_config.plug[x].task[y].minute);
	cJSON_AddNumberToObject(json_plug_task_send, "repeat", user_config.plug[x].task[y].repeat);
	cJSON_AddNumberToObject(json_plug_task_send, "action", user_config.plug[x].task[y].action);
	cJSON_AddNumberToObject(json_plug_task_send, "on", user_config.plug[x].task[y].on);

	cJSON_AddItemToObject(pJsonSend, plug_task_str, json_plug_task_send);
	return return_flag;
}

