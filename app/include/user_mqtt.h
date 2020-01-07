#ifndef __USER_MQTT_H__
#define __USER_MQTT_H__

BOOL user_mqtt_send(const uint8_t * data, char qos, char retained);
BOOL user_mqtt_send_topic(const uint8_t *topic, const uint8_t * data, char qos, uint8_t retained);
BOOL user_mqtt_send_senser(char *arg, char qos, char retained);
void ICACHE_FLASH_ATTR user_mqtt_connect(void);
void ICACHE_FLASH_ATTR user_mqtt_disconnect(void);
void ICACHE_FLASH_ATTR user_mqtt_init(void);
bool user_mqtt_is_connect();

#endif

