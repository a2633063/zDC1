#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"

#include "smartconfig.h"
#include "user_wifi.h"

#include "user_mqtt.h"
#include "../include/espconn.h"

char hwaddr[6];
char strMac[16] = { 0 };
char strIP[16];

/*
 * wifi配置
 * 	wifi连接成功后初始化,配置SmartConfig,wifi指示灯
 */

LOCAL unsigned char user_smarconfig_flag = 0;	//smarconfig标志位,防止smarconfig出错

//wifi event 回调函数
void wifi_handle_event_cb(System_Event_t *evt) {
	switch (evt->event) {
	case EVENT_STAMODE_CONNECTED:
		os_printf("wifi connect to ssid %s, channel %d\n", evt->event_info.connected.ssid, evt->event_info.connected.channel);
		break;
	case EVENT_STAMODE_DISCONNECTED:
		os_printf("wifi disconnect from ssid %s, reason %d\n", evt->event_info.disconnected.ssid, evt->event_info.disconnected.reason);
		wifi_status_led_install(GPIO_WIFI_LED_IO_NUM, GPIO_WIFI_LED_IO_MUX, GPIO_WIFI_LED_IO_FUNC);
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		os_printf("wifi change mode: %d -> %d\n", evt->event_info.auth_change.old_mode, evt->event_info.auth_change.new_mode);
		break;
	case EVENT_STAMODE_GOT_IP:
//		os_printf("wifi got ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR, IP2STR(&evt->event_info.got_ip.ip),
//				IP2STR(&evt->event_info.got_ip.mask), IP2STR(&evt->event_info.got_ip.gw));
//		os_printf("\n");
		os_sprintf(strIP, IPSTR, IP2STR(&evt->event_info.got_ip.ip));
		wifi_status_led_uninstall();
		user_set_led_wifi(1);

		user_mqtt_connect();	//连接MQTT服务器

		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		os_printf("wifi station: " MACSTR "join, AID = %d\n", MAC2STR(evt->event_info.sta_connected.mac), evt->event_info.sta_connected.aid);
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		os_printf("wifi station: " MACSTR "leave, AID = %d\n", MAC2STR(evt->event_info.sta_disconnected.mac), evt->event_info.sta_disconnected.aid);
//		user_mqtt_disconnect();	//连接MQTT服务器
		break;
	default:
		break;
	}
}

//smartconfig过程回调函数
void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata) {
	switch (status) {
	case SC_STATUS_WAIT:
		os_printf("1:SC_STATUS_WAIT\n");
		user_smarconfig_flag = 1;
		break;
	case SC_STATUS_FIND_CHANNEL:
		os_printf("2:SC_STATUS_FIND_CHANNEL\n");
		user_smarconfig_flag = 1;
		break;
	case SC_STATUS_GETTING_SSID_PSWD:
		os_printf("3:SC_STATUS_GETTING_SSID_PSWD\n");
		sc_type *type = pdata;
		if (*type == SC_TYPE_ESPTOUCH) {
			os_printf("SC_TYPE:SC_TYPE_ESPTOUCH\n");
		} else {
			os_printf("SC_TYPE:SC_TYPE_AIRKISS\n");
		}
		break;
	case SC_STATUS_LINK:
		os_printf("4:SC_STATUS_LINK\n");
		struct station_config *sta_conf = pdata;
		wifi_station_set_config(sta_conf);
		wifi_station_disconnect();
		wifi_station_connect();
		break;
	case SC_STATUS_LINK_OVER:
		os_printf("5:SC_STATUS_LINK_OVER\n");
		if (pdata != NULL) {
			uint8 phone_ip[4] = { 0 };
			memcpy(phone_ip, (uint8*) pdata, 4);
			os_printf("Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
		}
		user_smartconfig_stop();
		os_printf("smartconfig complete");
		user_smarconfig_flag = 0;
		wifi_status_led_uninstall();
		user_set_led_wifi(1);
		break;
	}
}

void ICACHE_FLASH_ATTR user_wifi_init(void) {

	//设置为station模式
	if (wifi_get_opmode() != STATION_MODE || wifi_get_opmode_default() != STATION_MODE) {
		wifi_set_opmode(STATION_MODE);
		os_printf("set wifi mode:station");
	}
	//设置自动连接AP
	if (wifi_station_get_auto_connect() == 0) {
		wifi_station_set_auto_connect(1);
		os_printf("set auto connect AP:true");
	}
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	user_set_led_wifi(0);
	wifi_status_led_install(GPIO_WIFI_LED_IO_NUM, GPIO_WIFI_LED_IO_MUX, GPIO_WIFI_LED_IO_FUNC);

	wifi_get_macaddr(STATION_IF, hwaddr);
	os_sprintf(strMac, "%02x%02x%02x%02x%02x%02x", MAC2STR(hwaddr));
	os_printf("strMac : %s \n", strMac);

	char strName[32] = { 0 };
	os_sprintf(strName, DEVICE_NAME, hwaddr[4], hwaddr[5]);
	wifi_station_set_hostname(strName);

	if (gpio16_input_get()) {
		user_mqtt_init();
	} else {	//按住按键开机,为热点模式
		user_set_led_wifi(1);
		user_set_led_logo(1);
		wifi_set_opmode_current(SOFTAP_MODE);

		struct softap_config configAp;
		os_sprintf(configAp.ssid, strName);
		os_printf("softAP SSID : %s \n", strName);
		configAp.ssid_len = os_strlen(strName);
		configAp.channel = 5;
		configAp.authmode = AUTH_OPEN;
		configAp.ssid_hidden = 0;
		configAp.max_connection = 4;
		configAp.beacon_interval = 100;

		wifi_softap_set_config(&configAp);

		struct ip_info info;
		wifi_softap_dhcps_stop();
		IP4_ADDR(&info.ip, 192, 168, 0, 1);
		IP4_ADDR(&info.gw, 192, 168, 0, 1);
		IP4_ADDR(&info.netmask, 255, 255, 255, 0);
		wifi_set_ip_info(SOFTAP_IF, &info);
		wifi_softap_dhcps_start();
		return;
	}

	os_printf("user_wifi_init\n");
}

void ICACHE_FLASH_ATTR user_smartconfig(void) {
	//设置为station模式
	if (wifi_get_opmode() != STATION_MODE || wifi_get_opmode_default() != STATION_MODE) {
		wifi_set_opmode(STATION_MODE);
		os_printf("set wifi mode:station");
	}

	if (user_smarconfig_flag != 0) {
		smartconfig_stop();
		user_smarconfig_flag = 0;
	}
	os_printf("smartconfig start");
	smartconfig_start(smartconfig_done);
	user_set_led_wifi(1);
	user_set_led_logo(1);

	wifi_status_led_install(GPIO_WIFI_LED_IO_NUM, GPIO_WIFI_LED_IO_MUX, GPIO_WIFI_LED_IO_FUNC);
}

void ICACHE_FLASH_ATTR user_smartconfig_stop(void) {
	if (user_smarconfig_flag != 0) {
		smartconfig_stop();
		user_smarconfig_flag = 0;
		os_printf("smartconfig stop");
		wifi_status_led_uninstall();
		user_set_led_wifi(0);
		user_set_led_logo(0);
	}
}

bool ICACHE_FLASH_ATTR user_smartconfig_is_starting() {
	return user_smarconfig_flag;
}
