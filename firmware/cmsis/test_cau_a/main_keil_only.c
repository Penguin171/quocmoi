/*
 * ============================================================================
 *  BAI TEST CAU (a) - TAO PROJECT TRUC TIEP TRONG KEIL C v5 (KHONG CAN CUBEMX)
 *  Chip: STM32F103C8T6 | MQ5 (PA0 qua cau phan ap 10k/15k) | LCD 20x4 (LM044L)
 *
 *  Cach tao project trong Keil C v5:
 *    1. Project -> New uVision Project -> chon STM32F103C8
 *    2. Cua so Manage Run-Time Environment: chi can tich 2 o:
 *         - CMSIS  -> CORE
 *         - Device -> Startup
 *    3. Them file main.c nay vao Source Group 1
 *    4. Alt+F7 -> tab Output -> tich "Create HEX File" -> bam F7 (Build)
 * ============================================================================
 */

#include "stm32f10x.h"
#include <stdio.h>
#include <stdint.h>

/* ========= Nguong cam bien MQ5 ========= */
#define MQ5_CLEAN_MV        600
#define MQ5_MAX_MV          4500
#define GAS_WARN_PERCENT    30
#define GAS_DANGER_PERCENT  60

/* ========= Bo dem thoi gian SysTick (1ms) ========= */
static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms) {
        /* Cho du thoi gian */
    }
}

static void delay_short(void) {
    for (volatile uint32_t i = 0; i < 400; i++) {
        __NOP();
    }
}

/* ========= Macro dieu khien chan GPIO ========= */
static inline void pin_write(GPIO_TypeDef *port, uint8_t pin, uint8_t val) {
    if (val) {
        port->BSRR = (1U << pin);
    } else {
        port->BSRR = (1U << (pin + 16U));
    }
}

/* ========= Khoi tao GPIO =========
 *  PA0       : Analog Input (MQ5 qua cau phan ap 10k/15k)
 *  PB12      : LCD RS
 *  PB13      : LCD E
 *  PB14,PB15 : LCD D4, D5
 *  PA8, PA9  : LCD D6, D7
 *  PC13      : LED bao trang thai
 */
static void GPIO_Init_All(void) {
    /* Bat clock cho GPIOA, GPIOB, GPIOC */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_IOPCEN;

    /* PA0: Analog Input (MODE=00, CNF=00) */
    GPIOA->CRL &= ~(0xFU << 0);

    /* PA8, PA9: Output Push-Pull 2MHz (MODE=10, CNF=00 -> 0x2) */
    GPIOA->CRH &= ~((0xFU << 0) | (0xFU << 4));
    GPIOA->CRH |=  ((0x2U << 0) | (0x2U << 4));

    /* PB12, PB13, PB14, PB15: Output Push-Pull 2MHz */
    GPIOB->CRH &= ~((0xFU << 16) | (0xFU << 20) | (0xFU << 24) | (0xFU << 28));
    GPIOB->CRH |=  ((0x2U << 16) | (0x2U << 20) | (0x2U << 24) | (0x2U << 28));

    /* PC13: Output Push-Pull 2MHz */
    GPIOC->CRH &= ~(0xFU << 20);
    GPIOC->CRH |=  (0x2U << 20);
    pin_write(GPIOC, 13, 1); /* Tat LED ban dau */
}

/* ========= Khoi tao ADC1 kenh 0 (PA0) ========= */
static void ADC1_Init_PA0(void) {
    /* Chia clock ADC /6 (dam bao <= 14MHz) va bat clock ADC1 */
    RCC->CFGR &= ~RCC_CFGR_ADCPRE;
    RCC->CFGR |=  RCC_CFGR_ADCPRE_DIV6;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* Thoi gian lay mau kenh 0 = 239.5 chu ky (chinh xac voi cau phan ap 10k/15k) */
    ADC1->SMPR2 |= (0x7U << 0);

    /* 1 kenh chuyen doi: kenh 0 (PA0) */
    ADC1->SQR1 = 0;
    ADC1->SQR3 = 0;

    /* Bat ADC1 */
    ADC1->CR2 |= ADC_CR2_ADON;
    delay_ms(2);

    /* Hieu chuan (Calibration) ADC */
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL);

    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL);
}

static uint16_t ADC1_Read_Single(void) {
    /* Ghi ADON=1 lan 2 khi ADC dang bat se bat dau chuyen doi */
    ADC1->CR2 |= ADC_CR2_ADON;
    while (!(ADC1->SR & ADC_SR_EOC));
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

static uint16_t MQ5_ReadADC_Avg(void) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 8; i++) {
        sum += ADC1_Read_Single();
    }
    return (uint16_t)(sum / 8U);
}

/* Quy doi ADC -> dien ap thuc chan AO cua MQ5 (bu cau phan ap R1=10k, R2=15k) */
static uint16_t MQ5_CalcVoltage_mV(uint16_t adc_raw) {
    uint32_t v_pa0_mv = ((uint32_t)adc_raw * 3300UL) / 4095UL;
    uint32_t v_ao_mv  = (v_pa0_mv * 25UL) / 15UL;
    if (v_ao_mv > 5000UL) v_ao_mv = 5000UL;
    return (uint16_t)v_ao_mv;
}

static uint8_t MQ5_CalcPercent(uint16_t mq5_mv) {
    if (mq5_mv <= MQ5_CLEAN_MV) return 0;
    if (mq5_mv >= MQ5_MAX_MV)   return 100;
    return (uint8_t)(((uint32_t)(mq5_mv - MQ5_CLEAN_MV) * 100UL) / (MQ5_MAX_MV - MQ5_CLEAN_MV));
}

/* ========= Thu vien LCD 20x4 (4-bit) ========= */
static void LCD_PulseEnable(void) {
    pin_write(GPIOB, 13, 1); /* E = 1 */
    delay_short();
    pin_write(GPIOB, 13, 0); /* E = 0 */
    delay_short();
}

static void LCD_Write4Bits(uint8_t nibble) {
    pin_write(GPIOB, 14, (nibble & 0x01U) ? 1 : 0); /* D4 */
    pin_write(GPIOB, 15, (nibble & 0x02U) ? 1 : 0); /* D5 */
    pin_write(GPIOA,  8, (nibble & 0x04U) ? 1 : 0); /* D6 */
    pin_write(GPIOA,  9, (nibble & 0x08U) ? 1 : 0); /* D7 */
    LCD_PulseEnable();
}

static void LCD_SendByte(uint8_t val, uint8_t is_data) {
    pin_write(GPIOB, 12, is_data); /* RS */
    LCD_Write4Bits((val >> 4) & 0x0FU);
    LCD_Write4Bits(val & 0x0FU);
}

static void LCD_SendCmd(uint8_t cmd) {
    LCD_SendByte(cmd, 0);
    if (cmd == 0x01U || cmd == 0x02U) {
        delay_ms(2);
    }
}

static void LCD_SendData(uint8_t data) {
    LCD_SendByte(data, 1);
}

static void LCD_Init(void) {
    delay_ms(50);
    pin_write(GPIOB, 12, 0); /* RS = 0 */
    pin_write(GPIOB, 13, 0); /* E  = 0 */

    LCD_Write4Bits(0x03);
    delay_ms(5);
    LCD_Write4Bits(0x03);
    delay_ms(1);
    LCD_Write4Bits(0x03);
    delay_ms(1);
    LCD_Write4Bits(0x02); /* Che do 4-bit */

    LCD_SendCmd(0x28); /* 4-bit, 2/4 dong, font 5x8 */
    LCD_SendCmd(0x0C); /* Bat hien thi, tat con tro */
    LCD_SendCmd(0x06); /* Tu dong dich con tro sang phai */
    LCD_SendCmd(0x01); /* Xoa man hinh */
    delay_ms(2);
}

static void LCD_SetCursor(uint8_t row, uint8_t col) {
    static const uint8_t row_offsets[4] = { 0x00, 0x40, 0x14, 0x54 };
    if (row > 3) row = 3;
    if (col > 19) col = 19;
    LCD_SendCmd(0x80U | (row_offsets[row] + col));
}

static void LCD_PrintLine(uint8_t row, const char *str) {
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

/* ======================== CHUONG TRINH CHINH ======================== */
int main(void) {
    char line[21];
    uint8_t led_state = 0;

    /* Cap nhat xung nhip he thong & cau hinh SysTick ngat moi 1ms */
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);

    GPIO_Init_All();
    ADC1_Init_PA0();
    LCD_Init();

    LCD_PrintLine(0, "MQ5 GAS SENSOR TEST");
    LCD_PrintLine(1, "STM32F103 + LCD20x4");
    delay_ms(500);

    while (1) {
        uint16_t adc_val = MQ5_ReadADC_Avg();
        uint16_t mq5_mv  = MQ5_CalcVoltage_mV(adc_val);
        uint8_t  gas_pct = MQ5_CalcPercent(mq5_mv);

        uint16_t v_int = mq5_mv / 1000U;
        uint16_t v_dec = (mq5_mv % 1000U) / 10U;

        /* Dong 1 */
        LCD_PrintLine(0, "  MQ5 GAS MONITOR   ");

        /* Dong 2 */
        snprintf(line, sizeof(line), "ADC:%4u  V:%u.%02uV", adc_val, v_int, v_dec);
        LCD_PrintLine(1, line);

        /* Dong 3 */
        uint8_t bars = gas_pct / 10U;
        if (bars > 10) bars = 10;
        char bar_str[11];
        for (uint8_t i = 0; i < 10; i++) {
            bar_str[i] = (i < bars) ? '#' : '-';
        }
        bar_str[10] = '\0';
        snprintf(line, sizeof(line), "Gas:%3u%% [%s]", gas_pct, bar_str);
        LCD_PrintLine(2, line);

        /* Dong 4 */
        if (gas_pct >= GAS_DANGER_PERCENT) {
            LCD_PrintLine(3, "Status: DANGER! GAS!");
        } else if (gas_pct >= GAS_WARN_PERCENT) {
            LCD_PrintLine(3, "Status: WARNING");
        } else {
            LCD_PrintLine(3, "Status: SAFE");
        }

        /* Dao trang thai LED PC13 */
        led_state = !led_state;
        pin_write(GPIOC, 13, led_state);

        delay_ms(250);
    }
}
