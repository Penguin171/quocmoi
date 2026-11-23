#include "lcd20x4.h"

/* Delay ngan (~50us) cho xung Enable cua LCD, khong phu thuoc Timer */
static void LCD_DelayShort(void) {
    for (volatile uint32_t i = 0; i < 600; i++) {
        __NOP();
    }
}

static void LCD_PulseEnable(void) {
    HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_SET);
    LCD_DelayShort();
    HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_RESET);
    LCD_DelayShort();
}

static void LCD_Write4Bits(uint8_t nibble) {
    HAL_GPIO_WritePin(LCD_D4_PORT, LCD_D4_PIN, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D5_PORT, LCD_D5_PIN, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D6_PORT, LCD_D6_PIN, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D7_PORT, LCD_D7_PIN, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    LCD_PulseEnable();
}

static void LCD_SendByte(uint8_t val, uint8_t is_data) {
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, is_data ? GPIO_PIN_SET : GPIO_PIN_RESET);
    LCD_Write4Bits((val >> 4) & 0x0F);
    LCD_Write4Bits(val & 0x0F);
}

static void LCD_SendCmd(uint8_t cmd) {
    LCD_SendByte(cmd, 0);
    if (cmd == 0x01 || cmd == 0x02) {
        HAL_Delay(2);
    }
}

static void LCD_SendData(uint8_t data) {
    LCD_SendByte(data, 1);
}

void LCD_Init(void) {
    HAL_Delay(50); /* Cho LCD khoi dong sau cap nguon 5V */

    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_RESET);

    /* Chuoi khoi tao chuan HD44780 sang che do 4-bit */
    LCD_Write4Bits(0x03);
    HAL_Delay(5);
    LCD_Write4Bits(0x03);
    HAL_Delay(1);
    LCD_Write4Bits(0x03);
    HAL_Delay(1);
    LCD_Write4Bits(0x02); /* Chuyen sang 4-bit */

    LCD_SendCmd(0x28); /* 4-bit, 2/4 dong, font 5x8 */
    LCD_SendCmd(0x0C); /* Bat hien thi, tat con tro */
    LCD_SendCmd(0x06); /* Tu dong tang con tro sang phai */
    LCD_Clear();
}

void LCD_Clear(void) {
    LCD_SendCmd(0x01);
    HAL_Delay(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    /* Dia chi DDRAM chuan cho LCD 20x4 (LM044L) */
    static const uint8_t row_offsets[4] = { 0x00, 0x40, 0x14, 0x54 };
    if (row > 3) row = 3;
    if (col > 19) col = 19;
    LCD_SendCmd(0x80 | (row_offsets[row] + col));
}

void LCD_Print(const char *str) {
    while (*str) {
        LCD_SendData((uint8_t)(*str++));
    }
}

void LCD_PrintLine(uint8_t row, const char *str) {
    uint8_t i = 0;
    LCD_SetCursor(row, 0);
    while (i < 20 && str[i] != '\0') {
        LCD_SendData((uint8_t)str[i]);
        i++;
    }
    while (i < 20) {
        LCD_SendData(' ');
        i++;
    }
}
