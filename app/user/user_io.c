#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "driver\i2c_master.h"
#include "user_mqtt.h"
#include "gpio.h"

#include "user_io.h"
#include "user_json.h"
#include "user_wifi.h"
#include "user_function.h"

#define GPIO_IO_INT_IO_MUX     PERIPHS_IO_MUX_GPIO4_U
#define GPIO_IO_INT_IO_NUM     4
#define GPIO_IO_INT_IO_FUNC    FUNC_GPIO4
//此数组决定对应继电器的关系
const uint8_t val_plug[] = { 0x80, 0x40, 0x20, 0x10 };

#define IIC_ADDR 0x40

bool ICACHE_FLASH_ATTR io_iic_write(uint8_t addr, uint8_t val) {
	uint8 ack;

	i2c_master_start();
	i2c_master_writeByte(IIC_ADDR);
	ack = i2c_master_checkAck();
	if (!ack) {
		os_printf("iic addr not ack\n");
		i2c_master_stop();
		return false;
	}

	i2c_master_writeByte(addr);
	ack = i2c_master_checkAck();
	if (!ack) {
		os_printf("icc addr 0x00 not ack\n");
		i2c_master_stop();
		return false;
	}

	i2c_master_writeByte(val);
	ack = i2c_master_checkAck();
	if (!ack) {
		os_printf("icc addr 0x00 not ack\n");
		i2c_master_stop();
		return false;
	}
	i2c_master_stop();
	return true;
}

bool ICACHE_FLASH_ATTR io_iic_read(uint8_t addr, uint8_t * val) {
	uint8 ack, i;

	i2c_master_start();
	i2c_master_writeByte(IIC_ADDR);
	ack = i2c_master_checkAck();
	if (!ack) {
		os_printf("iic addr not ack\n");
		i2c_master_stop();
		return false;
	}

	i2c_master_writeByte(addr);
	ack = i2c_master_checkAck();
	if (!ack) {
		os_printf("icc addr 0x00 not ack\n");
		i2c_master_stop();
		return false;
	}

	i2c_master_stop();
	i2c_master_wait(4000);

	i2c_master_start();
	i2c_master_writeByte(IIC_ADDR + 1);
	ack = i2c_master_checkAck();
	if (!ack) {
		os_printf("iic read addr not ack\n");
		i2c_master_stop();
		return false;
	}

	*val = i2c_master_readByte();
	i2c_master_send_nack();
//	os_printf("iic read:%x\n", *val);

	i2c_master_stop();
	return true;

}

LOCAL void io_intr_handler(void *arg) {

	// 读取GPIO中断状态
	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

//	//关闭GPIO中断
//	ETS_GPIO_INTR_DISABLE();

//清除GPIO中断标志
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);

	if (gpio_status & BIT(GPIO_IO_INT_IO_NUM)) {
		uint8_t val, plug_id;
		int8_t res = 0;
		char strJson[50];

		do {
			if (user_io_read_key(&val)) {
				uint8_t state = ~(0xf0 | val);
				switch (state) {
				case 0x1:
					plug_id = 1;
					break;
				case 0x2:
					plug_id = 2;
					break;
				case 0x4:
					plug_id = 3;
					break;
				default:
					return;
					//break;
				}
				os_sprintf(strJson, "{\"mac\":\"%s\",\"plug_%d\":{\"on\":%d}}", strMac, plug_id, !user_config.plug[plug_id].on);
				user_json_analysis(false, strJson);
				break;
			} else
				res++;
		} while (res < 5);

	}

//	//开启GPIO中断
//	ETS_GPIO_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR
user_io_init(void) {
	i2c_master_gpio_init();

	PIN_FUNC_SELECT(GPIO_IO_INT_IO_MUX, GPIO_IO_INT_IO_FUNC);

	GPIO_DIS_OUTPUT(GPIO_IO_INT_IO_NUM);
	ETS_GPIO_INTR_DISABLE();
	//设置io扩展ic外部中断 gpio4
	ETS_GPIO_INTR_ATTACH(io_intr_handler, NULL);

	//clear gpio14 status
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, BIT(GPIO_IO_INT_IO_NUM));
	//enable interrupt
	gpio_pin_intr_state_set(GPIO_ID_PIN(GPIO_IO_INT_IO_NUM), 2);			//GPIO_PIN_INTR_NEGEDGE
	ETS_GPIO_INTR_ENABLE();
}

bool ICACHE_FLASH_ATTR
user_io_read_key(uint8_t * val) {
	return io_iic_read(0x00, val);
}

bool ICACHE_FLASH_ATTR
user_io_set_plug(int8_t plug_id, int8_t on) {
	uint8_t val;

	if (!user_io_read_key(&val))
		return false;

	user_config.plug[plug_id].on = (on == -1) ? !user_config.plug[plug_id].on : (!!on);

	if (plug_id > 0 && user_config.plug[plug_id].on != 0) {
		user_config.plug[0].on = 1;
	} else if (user_config.plug[0].on == 0) {
		user_config.plug[1].on = 0;
		user_config.plug[2].on = 0;
		user_config.plug[3].on = 0;
	}

	val &= 0x0f;
	val = val | (user_config.plug[0].on << 7) | (user_config.plug[1].on << 6) | (user_config.plug[2].on << 5) | (user_config.plug[3].on << 4);

	user_set_led_logo(user_config.plug[0].on);
	os_printf("set plug %d : %d\n", plug_id, on);
	return io_iic_write(0x01, val);
}

bool ICACHE_FLASH_ATTR
user_io_set_plug_all(int8_t on0, int8_t on1, int8_t on2, int8_t on3) {
	uint8_t i, val;
	uint8_t on[4] = { on0, on1, on2, on3 };

	for (i = 0; i < 4; i++) {
		if (on[i] == -1) {
			user_config.plug[i].on = !user_config.plug[i].on;
		} else if (on[i] == 0 || on[i] == 1) {
			user_config.plug[i].on = on[i];
		}
	}


	val = 0x0f;
	val = val | (user_config.plug[0].on << 7) | (user_config.plug[1].on << 6) | (user_config.plug[2].on << 5) | (user_config.plug[3].on << 4);

	user_set_led_logo(user_config.plug[0].on);
	os_printf("set plug %d,%d,%d,%d\n", user_config.plug[0].on, user_config.plug[1].on, user_config.plug[2].on, user_config.plug[3].on);
	if (io_iic_write(0x01, val))
		return true;
	else {
		os_delay_us(300);
		return io_iic_write(0x01, val);
	}
}
