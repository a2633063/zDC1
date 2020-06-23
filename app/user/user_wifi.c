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

wifi_state_t wifi_states = STATE_WIFI_STAMODE_IDE;

LOCAL os_timer_t mdns_restart_timer; //timer,获取ip后过一段时间才能启动mdns/设置wif后过一段时间才能重启设备
void user_wifi_mdns_restart_timer_fun(uint8 flag);
/*
 * wifi配置
 * 	wifi连接成功后初始化,配置SmartConfig,wifi指示灯
 */

struct mdns_info info;
char mdns_strName[32] = { 0 };
char mdns_data_mac[32] = { 0 };
char mdns_data_type[16] = { 0 };
void user_mdns_config() {

	struct ip_info ipconfig;
	wifi_get_ip_info(STATION_IF, &ipconfig);

	os_memset(mdns_strName, 0, 32);
	os_memset(mdns_data_mac, 0, 32);
	os_memset(mdns_data_type, 0, 16);
	os_sprintf(mdns_strName, MDNS_DEVICE_NAME, strMac + 8);
	info.host_name = mdns_strName;
	info.ipAddr = ipconfig.ip.addr; //ESP8266 station IP; //ESP8266 station IP
	info.server_name = "zcontrol";
	info.server_port = 10182;
	//info.txt_data[0] = "version = "VERSION;
	os_sprintf(mdns_data_mac, "mac = %s", strMac);
	info.txt_data[0] = mdns_data_mac;
	os_sprintf(mdns_data_type, "type = %d", TYPE);
	info.txt_data[1] = mdns_data_type;
	espconn_mdns_init(&info);

}
//wifi event 回调函数
void wifi_handle_event_cb(System_Event_t *evt) {
	switch (evt->event) {
	case EVENT_STAMODE_CONNECTED:
		wifi_states = STATE_WIFI_STAMODE_CONNECTED;
		os_printf("wifi connect to ssid %s, channel %d\n", evt->event_info.connected.ssid, evt->event_info.connected.channel);
		break;
	case EVENT_STAMODE_DISCONNECTED:
		wifi_states = STATE_WIFI_STAMODE_DISCONNECTED;
		os_printf("wifi disconnect from ssid %s, reason %d\n", evt->event_info.disconnected.ssid, evt->event_info.disconnected.reason);
		wifi_status_led_install(GPIO_WIFI_LED_IO_NUM, GPIO_WIFI_LED_IO_MUX, GPIO_WIFI_LED_IO_FUNC);
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		os_printf("wifi change mode: %d -> %d\n", evt->event_info.auth_change.old_mode, evt->event_info.auth_change.new_mode);
		break;
	case EVENT_STAMODE_GOT_IP:
		wifi_states = STATE_WIFI_STAMODE_GOT_IP;
		os_printf("wifi got ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR, IP2STR(&evt->event_info.got_ip.ip), IP2STR(&evt->event_info.got_ip.mask),
				IP2STR(&evt->event_info.got_ip.gw));
		os_printf("\n");
		os_sprintf(strIP, IPSTR, IP2STR(&evt->event_info.got_ip.ip));
		wifi_status_led_uninstall();
		user_set_led_wifi(1);
		user_mqtt_connect();	//连接MQTT服务器
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		wifi_states = STATE_WIFI_SOFTAPMODE_CONNECTED;
		os_printf("wifi station: " MACSTR "join, AID = %d\n", MAC2STR(evt->event_info.sta_connected.mac), evt->event_info.sta_connected.aid);
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		wifi_states = STATE_WIFI_SOFTAPMODE;
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
		wifi_states = STATE_WIFI_SMARTCONFIG;
		break;
	case SC_STATUS_FIND_CHANNEL:
		os_printf("2:SC_STATUS_FIND_CHANNEL\n");
		wifi_states = STATE_WIFI_SMARTCONFIG;
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
		wifi_states = STATE_WIFI_STAMODE_CONNECTING;
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
		wifi_status_led_uninstall();
		user_set_led_wifi(1);
		break;
	}
}
void ICACHE_FLASH_ATTR user_wifi_AP() {
	if (wifi_get_opmode() == STATIONAP_MODE)
		return;
	wifi_set_opmode_current(SOFTAP_MODE);
	char strName[32] = { 0 };
	os_sprintf(strName, MDNS_DEVICE_NAME, strMac + 8);
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
	wifi_states = STATE_WIFI_SOFTAPMODE;
}

struct station_config wifi_set_stationConf;
void ICACHE_FLASH_ATTR user_wifi_set(char *ssid, char *pass) {

	os_printf("user_wifi_set ssid:%s\n", ssid);
	os_printf("user_wifi_set password:%s\n", pass);
	wifi_set_stationConf.bssid_set = 0; //need not check MAC address of AP
	os_memcpy(&wifi_set_stationConf.ssid, ssid, 32);
	os_memcpy(&wifi_set_stationConf.password, pass, 64);

//	//设置为station模式
//	if (wifi_get_opmode() != STATION_MODE || wifi_get_opmode_default() != STATION_MODE) {
//		wifi_set_opmode(STATION_MODE);
//		os_printf("set wifi mode:station\n");
//	}

//	wifi_station_disconnect();
//	wifi_station_connect();
	os_timer_disarm(&mdns_restart_timer);
	os_timer_setfn(&mdns_restart_timer, (os_timer_func_t *) user_wifi_mdns_restart_timer_fun, (void *) 2);
	os_timer_arm(&mdns_restart_timer, 1000, 0); //1000ms后重启
}
void ICACHE_FLASH_ATTR user_wifi_init(void) {
	int i;
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

	struct station_config config[5];
	i = wifi_station_get_ap_info(config);
	os_printf("wifi info : %d \n", i);

	if (gpio16_input_get() && i > 0) {
		user_mqtt_init();
	} else {	//按住按键开机,为热点模式
		wifi_status_led_uninstall();
		user_set_led_wifi(1);
		user_set_led_logo(1);

		uint32 io_info[][3] = { { GPIO_WIFI_LED_IO_MUX, GPIO_WIFI_LED_IO_FUNC, GPIO_WIFI_LED_IO_NUM } };
		uint32 pwm_duty_init[1] = { 11111111 };
		pwm_init(1000000, pwm_duty_init, 1, io_info);
		os_printf("pwm_init\n");
		pwm_start();
		user_wifi_AP();
		return;
	}
	os_printf("user_wifi_init\n");
}

void user_wifi_mdns_restart_timer_fun(uint8 flag) {
	if (flag == 1) {
		//启动mdns
		user_mdns_config();
	} else if (flag == 2) {
		//重启设备
		wifi_set_opmode(STATION_MODE);
		wifi_station_set_config(&wifi_set_stationConf);
		os_timer_disarm(&mdns_restart_timer);
		os_timer_setfn(&mdns_restart_timer, (os_timer_func_t *) user_wifi_mdns_restart_timer_fun, (void *) 0);
		os_timer_arm(&mdns_restart_timer, 1000, 0); //1000ms后重启
		//system_restart();
	} else if (flag == 0) {
		system_restart();
	}
}
void ICACHE_FLASH_ATTR user_smartconfig(void) {

	if (wifi_get_opmode() == SOFTAP_MODE) {
		os_printf("SoftAP,no smartconfig");
		return;
	}

	//设置为station模式
	if (wifi_get_opmode() != STATION_MODE || wifi_get_opmode_default() != STATION_MODE) {
		wifi_set_opmode(STATION_MODE);
		os_printf("set wifi mode:station");
	}

	if (wifi_states == STATE_WIFI_SMARTCONFIG) {
		smartconfig_stop();
		wifi_states = STATE_WIFI_STAMODE_IDE;
	}
	os_printf("smartconfig start");
	smartconfig_start(smartconfig_done);
	wifi_states = STATE_WIFI_SMARTCONFIG;
	user_set_led_wifi(1);
	user_set_led_logo(1);

	wifi_status_led_install(GPIO_WIFI_LED_IO_NUM, GPIO_WIFI_LED_IO_MUX, GPIO_WIFI_LED_IO_FUNC);
}

void ICACHE_FLASH_ATTR user_smartconfig_stop(void) {
	if (wifi_states == STATE_WIFI_SMARTCONFIG) {
		smartconfig_stop();
		if (wifi_states == STATE_WIFI_SMARTCONFIG)
			wifi_states = STATE_WIFI_STAMODE_IDE;
		os_printf("smartconfig stop");
		wifi_status_led_uninstall();
		user_set_led_wifi(0);
		user_set_led_logo(0);
	}
}

bool ICACHE_FLASH_ATTR user_smartconfig_is_starting() {
	return wifi_states == STATE_WIFI_SMARTCONFIG;
}
