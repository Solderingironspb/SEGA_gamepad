/**
 ******************************************************************************
 *  @file SEGA_gamepad.h
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

#include <stm32f1xx.h>
#include <stm32f103xx_CMSIS.h>
#include <stdbool.h>

/*Макросы*/
#define SEGA_PIN1 GPIO_IDR_IDR0
#define SEGA_PIN2 GPIO_IDR_IDR1
#define SEGA_PIN3 GPIO_IDR_IDR2
#define SEGA_PIN4 GPIO_IDR_IDR3
#define SEGA_PIN6 GPIO_IDR_IDR4
#define SEGA_PIN9 GPIO_IDR_IDR5

#define SEGA_A_Pos     (1 << 11)
#define SEGA_B_Pos     (1 << 10)
#define SEGA_C_Pos     (1 << 9)
#define SEGA_X_Pos     (1 << 8)
#define SEGA_Y_Pos     (1 << 7)
#define SEGA_Z_Pos     (1 << 6)
#define SEGA_START_Pos (1 << 5)
#define SEGA_MODE_Pos  (1 << 4)
#define SEGA_UP_Pos    (1 << 3)
#define SEGA_DOWN_Pos  (1 << 2)
#define SEGA_LEFT_Pos  (1 << 1)
#define SEGA_RIGHT_Pos (1 << 0)

#define SEGA_SELECT_ON  GPIOA->BSRR = GPIO_BSRR_BS6
#define SEGA_SELECT_OFF GPIOA->BSRR = GPIO_BSRR_BR6
#define SEGA_LED_ON     GPIOC->BSRR = GPIO_BSRR_BR13
#define SEGA_LED_OFF    GPIOC->BSRR = GPIO_BSRR_BS13

void SEGA_GPIO_Init(void); //Настройка ножек для работы с геймпадом