#include "main.h"
#include "SEGA_gamepad.h"

extern uint16_t Buttons; //Переменная под 12 кнопок

int main(void) {
    CMSIS_Debug_init();
    CMSIS_RCC_SystemClock_72MHz();
    CMSIS_SysTick_Timer_init();
    CMSIS_PC13_OUTPUT_Push_Pull_init();//Ножка, которая будет мигать при нажатии кнопок геймпада
    SEGA_LED_OFF;
    CMSIS_TIM2_init(); //Таймер на 240 Гц
    CMSIS_TIM3_init(); //Таймер на 100кГц, для ножки PA7(SELECT). Длина импульса 20 мкс. Забираем данные между фронтами.
    SEGA_GPIO_Init(); //Настройка ножек для работы с геймпадом
    
    while (1) {
	
    }
}