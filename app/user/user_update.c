#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"

#include "../include/upgrade.h"
#include "../include/espconn.h"
#include "user_update.h"

#include "user_led.h"
uint8_t *domain = NULL;
uint8_t * ota_path = NULL;
void ICACHE_FLASH_ATTR ota_finished_callback(void *arg) {
	struct upgrade_server_info *update = arg;
	if (update->upgrade_flag == 1) {
		os_printf("[OTA]success; rebooting!\n");
		system_upgrade_reboot();
	} else {
		os_printf("[OTA]failed!\n");
	}

	os_free(update->pespconn);
	os_free(update->url);
	os_free(update);
}

void ICACHE_FLASH_ATTR ota_start_Upgrade(const char *name,const char *server_ip) {

//	uint8_t userBin = system_upgrade_userbin_check();
//	switch (userBin) {
//	case UPGRADE_FW_BIN1:
////		file = "user1.1024.new.2.bin";
//		*(ota_path + path_length - 16) = '2';
//		break; //  user2.bin
//	case UPGRADE_FW_BIN2:
////		file = "user2.1024.new.2.bin";
//		*(ota_path + path_length - 16) = '1';
//		break; // user1.bin
//	default:
//		os_printf("[OTA]Invalid userbin number!\n");
//		user_set_led_logo(1);
//		return;
//	}

	struct upgrade_server_info* update =
			(struct upgrade_server_info *) os_zalloc(
					sizeof(struct upgrade_server_info));
	update->pespconn = (struct espconn *) os_zalloc(sizeof(struct espconn));

	os_memcpy(update->ip, server_ip, 4);
//		update->ip[0] = 192;
//		update->ip[1] = 168;
//		update->ip[2] = 2;
//		update->ip[3] = 105;
	update->port = 80;
	os_printf("[OTA]Server "IPSTR":%d. Path: [%s]\n", IP2STR(update->ip),
			update->port, ota_path);
	update->check_cb = ota_finished_callback;
	update->check_times = 10000;
	update->url = (uint8 *) os_zalloc(512);

	os_sprintf((char*) update->url, "GET %s HTTP/1.1\r\n"
			"Host: %s:%d\r\n"
			"Connection: keep-alive\r\n"
			"\r\n", ota_path, name, update->port);

	os_printf("\r\n\r\n\r\n%s\r\n\r\n",update->url);
	if (system_upgrade_start(update) == false) {
		os_printf("[OTA]Could not start upgrade\n");
		os_free(update->pespconn);
		os_free(update->url);
		os_free(update);
		user_set_led_logo(1);
	} else {
		os_printf("[OTA]Upgrading...\n");
		user_set_led_logo(0);
	}
}

LOCAL void ICACHE_FLASH_ATTR
user_ota_dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;

	if (ipaddr != NULL) {
		os_printf("user_esp_platform_dns_found %d.%d.%d.%d\n",
				*((uint8 *) &ipaddr->addr), *((uint8 *) &ipaddr->addr + 1),
				*((uint8 *) &ipaddr->addr + 2), *((uint8 *) &ipaddr->addr + 3));

		if (ota_path == NULL) {
			os_printf("OTA fail:path is null!\n");
			return;
		}

		ota_start_Upgrade(name, (char *) &ipaddr->addr);
		user_set_led_logo(0);
	} else {
		os_printf("user_esp_platform_dns_found fail\n");
	}
}

void ICACHE_FLASH_ATTR user_ota_start(char *s) {

	uint16_t pos = 0;
	int16_t pos1 = 0;
	uint8_t i, j;
	uint16_t max_length = os_strlen(s);

	if (domain != NULL)
		os_free(domain);
	domain = (uint8_t *) os_malloc(32);
	os_memset(domain, 0, 32);
	if (os_strncmp(s, "https://", 8) == 0) {
		pos = 8;
	} else if (os_strncmp(s, "http://", 7) == 0) {
		pos = 7;
	} else
		return;

	for (i = pos; i < max_length && i < 32; i++) {
		if (*(s + i) == '/') {
			pos1 = i;
			break;
		}
	}
	if (pos1 <= 0 || pos1 > 32)
		return;

	os_strncpy(domain, s + pos, pos1 - pos);

	if (ota_path != NULL)
		os_free(ota_path);
	ota_path = (uint8_t *) os_malloc(max_length);
	os_memset(ota_path, 0, max_length);
	os_strncpy(ota_path, s + pos1, max_length);
	os_printf("domain:%s\n", domain);
	os_printf("path:%s\n", ota_path);

	struct espconn socket;
	ip_addr_t ip_addr;
	espconn_gethostbyname(&socket, domain, &ip_addr, user_ota_dns_found);
//	char serverip[] = { 192, 168, 2,105 };
//	char path[] = "ESP/";
//	ota_start_Upgrade( serverip, 81, path);
//	user_set_led_logo(0);

	os_free(domain);
}
