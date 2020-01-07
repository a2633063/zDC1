#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"

#include "espconn.h"

#include "user_wifi.h"
#include "user_udp.h"
#include "user_mqtt.h"
#include "user_function.h"
#include "user_setting.h"

void ICACHE_FLASH_ATTR
user_send(bool udp_flag, uint8_t *s, char retained) {
	if (udp_flag || !user_mqtt_is_connect()) {
		user_udp_send(s);
	} else {
		user_mqtt_send(s, 1, retained);
	}
}

void ICACHE_FLASH_ATTR
user_con_received(void *arg, char *pusrdata, unsigned short length) {
	if (length == 1 && *pusrdata == 127)
		return;

	struct espconn *pesp_conn = arg;

	int i, j;
	uint32_t k;

	user_json_analysis(true, pusrdata);

}

void ICACHE_FLASH_ATTR user_send_power(void) {
	uint8_t i;
	uint8_t *send_buf = NULL;
	send_buf = (uint8_t *) os_malloc(256); //
	if (send_buf != NULL) {
		os_sprintf(send_buf, "{\"mac\":\"%s\",\"power\":\"%d.%d\",\"voltage\":\"%d\",\"current\":\"%d.%02d\"}", strMac, power / 10, power % 10,
				voltage, current / 100, current % 100);

		if (!user_mqtt_is_connect()) {
			user_udp_send(send_buf);
		} else {
			user_mqtt_send_senser(send_buf, 0, 0);
		}

	}
	if (send_buf)
		os_free(send_buf);

}
