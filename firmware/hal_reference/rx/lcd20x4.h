#ifndef __LCD20X4_H__
#define __LCD20X4_H__

#include "main.h"
#include <stdint.h>

/* ========= Cau hinh chan GPIO cho LCD 20x4 (che do 4-bit) ========= */
#define LCD_RS_PORT     GPIOB
#define LCD_RS_PIN      GPIO_PIN_12

#define LCD_EN_PORT     GPIOB
#define LCD_EN_PIN      GPIO_PIN_13

#define LCD_D4_PORT     GPIOB
#define LCD_D4_PIN      GPIO_PIN_14

#define LCD_D5_PORT     GPIOB
#define LCD_D5_PIN      GPIO_PIN_15

#define LCD_D6_PORT     GPIOA
#define LCD_D6_PIN      GPIO_PIN_8

#define LCD_D7_PORT     GPIOA
#define LCD_D7_PIN      GPIO_PIN_9

/* ========= Ham dieu khien LCD 20x4 ========= */
void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col); /* row: 0..3, col: 0..19 */
void LCD_Print(const char *str);
void LCD_PrintLine(uint8_t row, const char *str); /* In du 20 ky tu, tu dong xoa ky tu thua */

#endif /* __LCD20X4_H__ */
