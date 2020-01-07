#ifndef __USER_IO_H__
#define __USER_IO_H__

#include "gpio.h"

void user_io_init(void);
uint8 user_io_read_key(uint8_t * val);
bool user_io_set_plug(int8_t plug_id, int8_t on);
bool user_io_set_plug_all(int8_t on0, int8_t on1, int8_t on2, int8_t on3);
#endif
