#include "../include/user_udp.h"
#include "ets_sys.h"
#include "osapi.h"

#include "user_wifi.h"
#include "user_interface.h"
#include "uart.h"
#include "user_config.h"
#include "user_udp.h"
#include "user_setting.h"
#include "user_os_timer.h"
#include "user_sntp.h"
#include "user_mqtt.h"
#include "user_io.h"
#include "user_webserver.h"

user_config_t user_config;
uint16_t power;
uint16_t voltage;
uint16_t current;

#if ((SPI_FLASH_SIZE_MAP == 0) || (SPI_FLASH_SIZE_MAP == 1))
#error "The flash map is not supported"
#elif (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#elif (SPI_FLASH_SIZE_MAP == 3)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 4)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#elif (SPI_FLASH_SIZE_MAP == 5)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x1fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x1fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x1fd000
#elif (SPI_FLASH_SIZE_MAP == 6)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x101000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0x3fb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0x3fc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0x3fd000
#else
#error "The flash map is not supported"
#endif


static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 						0x0, 												0x1000},
    { SYSTEM_PARTITION_OTA_1,   						0x1000, 											SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   						SYSTEM_PARTITION_OTA_2_ADDR, 						SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  						SYSTEM_PARTITION_RF_CAL_ADDR, 						0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 						SYSTEM_PARTITION_PHY_DATA_ADDR, 					0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 				SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 			0x3000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, sizeof(at_partition_table)/sizeof(at_partition_table[0]),SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void) {
	int i, j;
	uint32 x;
	system_uart_swap();
	uart_init(4800, 115200);
	os_printf(" \n \nStart user%d.bin\n", system_upgrade_userbin_check() + 1);
	os_printf("SDK version:%s\n", system_get_sdk_version());
	os_printf("FW version:%s\n", VERSION);

	user_setting_init();
	user_key_init();
	user_led_init();
	user_io_init();
	user_wifi_init();
	user_sntp_init();
	user_os_timer_init();
	//UDP初始化,监听端口10182,当接收到特定字符串时,返回本设备IP及MAC地址
	user_devicefind_init(10182);
//	TCP初始化,监听端口10191
//	user_tcp_init(10191);
	//TCP初始化 80端口 webserver
	user_webserver_init(80);

	user_io_set_plug_all(2,2,2,2);

}


