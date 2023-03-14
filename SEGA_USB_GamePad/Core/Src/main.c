#include "main.h"
#include "usb_device.h"
#include "usbd_customhid.h"
#include "SEGA_gamepad.h"

extern uint16_t Buttons; //Переменная под 12 кнопок
USB_Custom_HID_Gamepad Gamepad_data;

extern PCD_HandleTypeDef hpcd_USB_FS;
extern USBD_HandleTypeDef hUsbDeviceFS;

void USB_LP_CAN1_RX0_IRQHandler(void){
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

void USB_HP_CAN1_TX_IRQHandler(void){
    
}





int main(void){
    CMSIS_Debug_init();
    CMSIS_RCC_SystemClock_72MHz();
    CMSIS_SysTick_Timer_init();
	CMSIS_PC13_OUTPUT_Push_Pull_init(); //Ножка, которая будет мигать при нажатии кнопок геймпада
	SEGA_LED_OFF;
	CMSIS_TIM2_init(); //Таймер на 240 Гц
	CMSIS_TIM3_init(); //Таймер на 100кГц, для ножки PA7(SELECT). Длина импульса 20 мкс. Забираем данные между фронтами.
	SEGA_GPIO_Init(); //Настройка ножек для работы с геймпадом
    MX_USB_DEVICE_Init();
    
    while (1){
 
    }
  
}


