
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stm32f103xx_CMSIS.h>

	typedef struct __attribute__((packed)) {
		int8_t x;
		int8_t y;
		uint8_t buttons;
	}USB_Custom_HID_Gamepad;

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */


