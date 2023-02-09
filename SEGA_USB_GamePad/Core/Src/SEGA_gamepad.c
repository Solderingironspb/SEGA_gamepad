/**
 ******************************************************************************
 *  @file SEGA_gamepad.c
 *  @brief Библиотека для работы с 6-кнопочным джойстиком от SEGA Mega Drive 2
 *  @author Волков Олег
 *  @date 09.02.2023
 *
 ******************************************************************************
 * @attention
 *
 *  Порт подключения геймпада DB-9
 *  Назначения контактов:
 *  1 - PIN1
 *  2 - PIN2
 *  3 - PIN3
 *  4 - PIN4
 *  5 - 5V
 *  6 - PIN6
 *  7 - PIN7 SELECT
 *  8 - GND
 *  9 - PIN9
 * 
 *  В схеме используется триггер Шмитта SN74HC14, чтоб согласовать приходящий сигнал с джойстика в 5в, с входом МК 3.3в
 *
 *  В библиотеке задействовано 2 таймера:
 *  TIM2 - Опрашивает контроллер 240 раз в секунду при помощи TIM3 и забирает данные в переменную Buttons
 *  TIM3 - Задает частоту стробирующих импульсов на PIN7(SELECT). Длина импульсов 20 мкс (50 кГц). 
 *  Между фронтами производит считывание данных о кнопках, для этого частота задается в 100 кГц.
 *
 * Таблица истинности. После Триггерра Шмитта сигнал с джойстика: 1 - кнопка нажата, 2 - кнопка не нажата.
 *
 * | COUNTER  |   PULSE    |	SELECT  |	PIN 1   |	PIN 2   |	PIN 3   |	PIN 4   |	PIN 6   |	PIN 9   |
 * | 0        |  0(idle)   |   HIGH	    |     UP    |  	DOWN	|   LEFT    |	RIGHT	|     B	    |    C      |
 * | 1        |  1         |   LOW      |     UP	|   DOWN	|    *      |     *     |     A     |	START   |
 * | 2        |  1	       |   HIGH	    |     UP	|   DOWN    |  	LEFT    | 	RIGHT   |     B     |    C      |
 * | 3        |  2	       |   LOW      |     UP    | 	DOWN	|    *      |     *     |     A     |	START   |
 * | 4        |  2         |   HIGH	    |     UP    |	DOWN	|   LEFT    | 	RIGHT   |     B     |    C      |
 * | 5        |  3	       |   LOW      |     **	|    **     |    *      |     *     |     A     |	START   |
 * | 6        |  3         |   HIGH	    |     Z	    |    Y      |    X	    |    MODE	|     B	    |    C      |
 * | 7        |  4	       |   LOW      |     –	    |    –      |    –      |     –     |     A     |	START   |
 * | 8        |  4/0(idle) |   HIGH	    |     UP	|   DOWN	|   LEFT	|    RIGHT	|     B	    |    C      |
 *  
 * Создана переменная uint16_t Buttons; //Переменная под 12 кнопок
 * Биты:
 *      15-12 Не используются
 *      11 - A
 *      10 - B
 *       9 - C
 *       8 - X
 *       7 - Y
 *       6 - Z
 *       5 - Start
 *       4 - MODE
 *       3 - UP
 *       2 - DOWN
 *       1 - LEFT
 *       0 - RIGHT
 *
 *  YouTube: https://www.youtube.com/channel/UCzZKTNVpcMSALU57G1THoVw
 *  GitHub: https://github.com/Solderingironspb 
 *  Группа ВК: https://vk.com/solderingiron.stm32 
 *
 ******************************************************************************
 */

#include "SEGA_gamepad.h"
#include "usb_device.h"
#include "usbd_customhid.h"

uint16_t Buttons; //Переменная под 12 кнопок
bool flag_SELECT;        //Флаг для переключения ножки SELECT
uint8_t Counter; //Счетчик переключений сигнала SELECT
extern USB_Custom_HID_Gamepad Gamepad_data;
extern PCD_HandleTypeDef hpcd_USB_FS;
extern USBD_HandleTypeDef hUsbDeviceFS;

 /**
 ***************************************************************************************
 *  @breif Функция для инициализации ножек МК, к которым подключен геймпад.
 ***************************************************************************************
 */
void SEGA_GPIO_Init(void){
    /*Настройка ножек*/
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_IOPAEN); //Запуск тактирования порта А
    //PA0 - PIN1(Input floating)
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE0_Msk, 0b00 << GPIO_CRL_MODE0_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF0_Msk, 0b01 << GPIO_CRL_CNF0_Pos);
    //PA1 - PIN2(Input floating)
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE1_Msk, 0b00 << GPIO_CRL_MODE1_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF1_Msk, 0b01 << GPIO_CRL_CNF1_Pos);
    //PA2 - PIN3(Input floating)
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE2_Msk, 0b00 << GPIO_CRL_MODE2_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF2_Msk, 0b01 << GPIO_CRL_CNF2_Pos);
    //PA3 - PIN4(Input floating)
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE3_Msk, 0b00 << GPIO_CRL_MODE3_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF3_Msk, 0b01 << GPIO_CRL_CNF3_Pos);
    //PA4 - PIN6(Input floating)
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE4_Msk, 0b00 << GPIO_CRL_MODE4_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF4_Msk, 0b01 << GPIO_CRL_CNF4_Pos);
    //PA5 - PIN9(Input floating)
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE5_Msk, 0b00 << GPIO_CRL_MODE5_Pos);
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF5_Msk, 0b01 << GPIO_CRL_CNF5_Pos);
    //A6 - PIN7 SELECT (Output push-pull)
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_MODE6, 0b10 << GPIO_CRL_MODE6_Pos); 
    MODIFY_REG(GPIOA->CRL, GPIO_CRL_CNF6, 0b00 << GPIO_CRL_CNF6_Pos); 
}

/**
***************************************************************************************
*  @breif Прерывания от таймера 2
*  @attention Настраиваем таймер, чтоб заходил в прерывание с частотой 240 Гц. 
*  С этой частотой будем опрашивать геймпад.
***************************************************************************************
*/
void TIM2_IRQHandler(void) {
    //Опрос джойстика 240 раз в секунду
    if (READ_BIT(TIM2->SR, TIM_SR_UIF)) {
        flag_SELECT = 1;
        SET_BIT(TIM3->CR1, TIM_CR1_CEN); //Запуск таймера
        CLEAR_BIT(TIM2->SR, TIM_SR_UIF); //Сбросим флаг прерывания
    }
}

/**
***************************************************************************************
*  @breif Прерывания от таймера 3
*  @attention Настраиваем таймер, чтоб заходил в прерывание с частотой 100 кГц. 
*  Счетчик будет выдавать стробирующие импульсы на ножку SELECT.
*  Счетчик будет считать от 0 до 16.
*  На нечетных значениях счетчика будет происходить переключение ножки SELECT
*  На четных значениях будем опрашивать состояние кнопок.
***************************************************************************************
*/
void TIM3_IRQHandler(void) {
    //Делаем стробирующий сигнал на ножке PIN7 SELECT
    if (READ_BIT(TIM3->SR, TIM_SR_UIF)) {
        if (Counter % 2 != 0) {
            //Считывать сигнал будем между фронтами, чтоб не нарваться на переходный процесс
            flag_SELECT = !flag_SELECT;
            if (flag_SELECT) {
               SEGA_SELECT_ON; //0 (idle)
            }
            else {
                SEGA_SELECT_OFF; //1	
            }
        }
	
        switch (Counter) {
        case 0:
            //UP
            if (READ_BIT(GPIOA->IDR, SEGA_PIN1)) {
                SET_BIT(Buttons, SEGA_UP_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_UP_Pos); 
            }
            //DOWN
            if (READ_BIT(GPIOA->IDR, SEGA_PIN2)) {
                SET_BIT(Buttons, SEGA_DOWN_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_DOWN_Pos); 
            }
            //LEFT
            if (READ_BIT(GPIOA->IDR, SEGA_PIN3)) {
                SET_BIT(Buttons, SEGA_LEFT_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_LEFT_Pos); 
            }
            //RIGHT
            if (READ_BIT(GPIOA->IDR, SEGA_PIN4)) {
                SET_BIT(Buttons, SEGA_RIGHT_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_RIGHT_Pos); 
            }
            //B
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_B_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_B_Pos); 
            }
            //C
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_C_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_C_Pos); 
            }
            break;
        case 2:
            //UP
            if (READ_BIT(GPIOA->IDR, SEGA_PIN1)) {
                SET_BIT(Buttons, SEGA_UP_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_UP_Pos); 
            }
            //DOWN
            if (READ_BIT(GPIOA->IDR, SEGA_PIN2)) {
                SET_BIT(Buttons, SEGA_DOWN_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_DOWN_Pos); 
            }
            //A
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_A_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_A_Pos); 
            }
            //START
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_START_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_START_Pos); 
            }
            break;
        case 4:
            //UP
            if (READ_BIT(GPIOA->IDR, SEGA_PIN1)) {
                SET_BIT(Buttons, SEGA_UP_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_UP_Pos); 
            }
            //DOWN
            if (READ_BIT(GPIOA->IDR, SEGA_PIN2)) {
                SET_BIT(Buttons, SEGA_DOWN_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_DOWN_Pos); 
            }
            //LEFT
            if (READ_BIT(GPIOA->IDR, SEGA_PIN3)) {
                SET_BIT(Buttons, SEGA_LEFT_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_LEFT_Pos); 
            }
            //RIGHT
            if (READ_BIT(GPIOA->IDR, SEGA_PIN4)) {
                SET_BIT(Buttons, SEGA_RIGHT_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_RIGHT_Pos); 
            }
            //B
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_B_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_B_Pos); 
            }
            //C
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_C_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_C_Pos); 
            }
            break;
        case 6:
            //UP
            if (READ_BIT(GPIOA->IDR, SEGA_PIN1)) {
                SET_BIT(Buttons, SEGA_UP_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_UP_Pos); 
            }
            //DOWN
            if (READ_BIT(GPIOA->IDR, SEGA_PIN2)) {
                SET_BIT(Buttons, SEGA_DOWN_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_DOWN_Pos); 
            }
            //A
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_A_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_A_Pos); 
            }
            //START
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_START_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_START_Pos); 
            }
            break;
        case 8:
            //UP
            if (READ_BIT(GPIOA->IDR, SEGA_PIN1)) {
                SET_BIT(Buttons, SEGA_UP_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_UP_Pos); 
            }
            //DOWN
            if (READ_BIT(GPIOA->IDR, SEGA_PIN2)) {
                SET_BIT(Buttons, SEGA_DOWN_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_DOWN_Pos); 
            }
            //LEFT
            if (READ_BIT(GPIOA->IDR, SEGA_PIN3)) {
                SET_BIT(Buttons, SEGA_LEFT_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_LEFT_Pos); 
            }
            //RIGHT
            if (READ_BIT(GPIOA->IDR, SEGA_PIN4)) {
                SET_BIT(Buttons, SEGA_RIGHT_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_RIGHT_Pos); 
            }
            //B
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_B_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_B_Pos); 
            }
            //C
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_C_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_C_Pos); 
            }
            break;
        case 10:
            //A
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_A_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_A_Pos); 
            }
            //START
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_START_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_START_Pos); 
            }
            break;
        case 12:
            //Z
            if (READ_BIT(GPIOA->IDR, SEGA_PIN1)) {
                SET_BIT(Buttons, SEGA_Z_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_Z_Pos); 
            }
            //Y
            if (READ_BIT(GPIOA->IDR, SEGA_PIN2)) {
                SET_BIT(Buttons, SEGA_Y_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_Y_Pos); 
            }
            //X
            if (READ_BIT(GPIOA->IDR, SEGA_PIN3)) {
                SET_BIT(Buttons, SEGA_X_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_X_Pos); 
            }
            //MODE
            if (READ_BIT(GPIOA->IDR, SEGA_PIN4)) {
                SET_BIT(Buttons, SEGA_MODE_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_MODE_Pos); 
            }
            //B
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_B_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_B_Pos); 
            }
            //C
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_C_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_C_Pos); 
            }
            break;
        case 14:
            //A
            if (READ_BIT(GPIOA->IDR, SEGA_PIN6)) {
                SET_BIT(Buttons, SEGA_A_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_A_Pos); 
            }
            //START
            if (READ_BIT(GPIOA->IDR, SEGA_PIN9)) {
                SET_BIT(Buttons, SEGA_START_Pos);
            }
            else {
                CLEAR_BIT(Buttons, SEGA_START_Pos); 
            }
            break;
        }
		
        Counter++;
        if (Counter > 16) {
            Counter = 0; //Сбросим счетчик импульсов
            CLEAR_BIT(TIM3->CR1, TIM_CR1_CEN); //Остановим таймер
            //Если какая-то ножка нажата - мигнем светодиодом
            if (Buttons) {
                SEGA_LED_ON;
            }
            else {
                SEGA_LED_OFF;
            }
	        
	        if (READ_BIT(Buttons, SEGA_UP_Pos) && !READ_BIT(Buttons, SEGA_DOWN_Pos))
	        {
		        Gamepad_data.y = -128;
	        }
	        else if (READ_BIT(Buttons, SEGA_DOWN_Pos) && !READ_BIT(Buttons, SEGA_UP_Pos)) {
		        Gamepad_data.y = 127;
	        }
	        else if (READ_BIT(Buttons, SEGA_UP_Pos) && READ_BIT(Buttons, SEGA_DOWN_Pos)) {
		        Gamepad_data.y = 0;
	        }
	        else {
		        Gamepad_data.y = 0;
	        }
		
	        if (READ_BIT(Buttons, SEGA_LEFT_Pos) && !READ_BIT(Buttons, SEGA_RIGHT_Pos))
	        {
		        Gamepad_data.x = -128;
	        }
	        else if (READ_BIT(Buttons, SEGA_RIGHT_Pos) && !READ_BIT(Buttons, SEGA_LEFT_Pos)) {
		        Gamepad_data.x = 127;
	        }
	        else if (READ_BIT(Buttons, SEGA_LEFT_Pos) && READ_BIT(Buttons, SEGA_RIGHT_Pos)) {
		        Gamepad_data.x = 0;
	        }
	        else {
		        Gamepad_data.x = 0;
	        }
	        Gamepad_data.buttons = Buttons >> 4;
	        USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&Gamepad_data, sizeof(Gamepad_data));
        }
        CLEAR_BIT(TIM3->SR, TIM_SR_UIF); //Сбросим флаг прерывания
    }
}