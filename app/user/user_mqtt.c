#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"

#include "../mqtt/include/debug.h"
#include "../mqtt/include/mqtt.h"
#include "../mqtt/include/mqtt_config.h"

#include "user_config.h"
#include "user_function.h"
#include "user_setting.h"
#include "user_wifi.h"
#include "user_mqtt.h"
#include "user_json.h"
MQTT_Client mqttClient;
LOCAL bool is_connect = false;

#define MAX_MQTT_TOPIC_SIZE         (40)
#define MQTT_CLIENT_SUB_TOPIC   "device/zdc1/%s/set"
#define MQTT_CLIENT_PUB_TOPIC   "device/zdc1/%s/state"
#define MQTT_CLIENT_SENSER_TOPIC   "device/zdc1/%s/sensor"
#define MQTT_CLIENT_WILL_TOPIC   "device/zdc1/%s/availability"

char topic_state[MAX_MQTT_TOPIC_SIZE];
char topic_set[MAX_MQTT_TOPIC_SIZE];
char topic_senser[MAX_MQTT_TOPIC_SIZE];
char willtopic[MAX_MQTT_TOPIC_SIZE];

LOCAL os_timer_t timer_mqtt;
LOCAL uint8_t status = 0;
void ICACHE_FLASH_ATTR user_mqtt_timer_func(void *arg) {

	status++;
	switch (status) {
	case 1:
		user_mqtt_send_topic(willtopic,"1\0",1,1);
		break;

	default:
		os_timer_disarm(&timer_mqtt);
		status = 0;
		break;
	}
}

void mqttConnectedCb(uint32_t *args) {
	uint8_t i;
	is_connect = true;
	MQTT_Client* client = (MQTT_Client*) args;
	os_printf("MQTT: Connected\r\n");
	MQTT_Subscribe(client, topic_set, 0);

	os_timer_disarm(&timer_mqtt);
	os_timer_setfn(&timer_mqtt, (os_timer_func_t *) user_mqtt_timer_func, NULL);
	os_timer_arm(&timer_mqtt, 80, 1);

}

BOOL ICACHE_FLASH_ATTR
user_mqtt_send_topic(const uint8_t *topic, const uint8_t * data ,char qos, uint8_t retained) {
	return is_connect ? MQTT_Publish(&mqttClient, topic, data, os_strlen(data), qos, retained) : false;
}

BOOL ICACHE_FLASH_ATTR
user_mqtt_send(const uint8_t * data, char qos, char retained) {
	return user_mqtt_send_topic(topic_state, data, qos, retained);
}
BOOL ICACHE_FLASH_ATTR
user_mqtt_send_senser(char *arg, char qos, char retained) {
	return user_mqtt_send_topic(topic_senser, arg, qos, retained);
}
void mqttDisconnectedCb(uint32_t *args) {
	os_timer_disarm(&timer_mqtt);
	MQTT_Client* client = (MQTT_Client*) args;
	os_printf("MQTT: 断开连接\r\n");
	is_connect = false;
}

void mqttPublishedCb(uint32_t *args) {
	MQTT_Client* client = (MQTT_Client*) args;
	os_printf("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len) {
	char *topicBuf = (char*) os_zalloc(topic_len + 1), *dataBuf = (char*) os_zalloc(data_len + 1);

	MQTT_Client* client = (MQTT_Client*) args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

//os_printf("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	user_json_analysis(false, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

void ICACHE_FLASH_ATTR user_mqtt_connect(void) {
	MQTT_Connect(&mqttClient);
}

void ICACHE_FLASH_ATTR user_mqtt_disconnect(void) {
	MQTT_Disconnect(&mqttClient);
}

void ICACHE_FLASH_ATTR user_mqtt_init(void) {

	os_sprintf(topic_set, MQTT_CLIENT_SUB_TOPIC, strMac);
	os_sprintf(topic_state, MQTT_CLIENT_PUB_TOPIC, strMac);
	os_sprintf(topic_senser, MQTT_CLIENT_SENSER_TOPIC, strMac);
	os_sprintf(willtopic, MQTT_CLIENT_WILL_TOPIC, strMac);

//MQTT初始化
	MQTT_InitConnection(&mqttClient, user_config.mqtt_ip, user_config.mqtt_port, NO_TLS);

	MQTT_InitClient(&mqttClient, user_config.name, user_config.mqtt_user, user_config.mqtt_password, MQTT_KEEPALIVE, 1);

	MQTT_InitLWT(&mqttClient, willtopic, "0", 1, 1);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);
	os_printf("user_mqtt_init\n");
}

bool ICACHE_FLASH_ATTR user_mqtt_is_connect() {
	return is_connect;
}

