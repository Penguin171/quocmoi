/*
 * ============================================================================
 *  BAI TEST CAU (a) TREN PROTEUS:
 *  1 STM32F103C8 + Cam bien MQ5 (PA0 qua cau phan ap 10k/15k) + LCD 20x4 (LM044L)
 *  Moi truong: STM32CubeMX -> Keil C v5 (HAL) -> Xuat file .HEX nap Proteus
 * ============================================================================
 */

#include "main.h"
#include "lcd20x4.h"
#include <stdio.h>

/* Handle ADC1 do STM32CubeMX sinh ra trong main.c */
extern ADC_HandleTypeDef hadc1;

/* Nguong hien thi trang thai (%) */
#define MQ5_CLEAN_MV        600
#define MQ5_MAX_MV          4500
#define GAS_WARN_PERCENT    30
#define GAS_DANGER_PERCENT  60

/* Doc trung binh 8 mau ADC1_IN0 (PA0) */
static uint16_t MQ5_ReadADC(void) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 8; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 20) == HAL_OK) {
            sum += HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
    }
    return (uint16_t)(sum / 8);
}

/* Quy doi ADC 12-bit sang dien ap thuc cua MQ5 (bu cau phan ap R1=10k, R2=15k) */
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

void App_CauA_Run(void) {
    char line[21];

    /* Khoi tao LCD 20x4 */
    LCD_Init();
    LCD_PrintLine(0, "MQ5 GAS SENSOR TEST");
    LCD_PrintLine(1, "STM32F103 + LCD20x4");
    HAL_Delay(500);

    /* Hieu chuan ADC */
    HAL_ADCEx_Calibration_Start(&hadc1);

    while (1) {
        uint16_t adc_val = MQ5_ReadADC();
        uint16_t mq5_mv  = MQ5_CalcVoltage_mV(adc_val);
        uint8_t  gas_pct = MQ5_CalcPercent(mq5_mv);

        uint16_t v_int = mq5_mv / 1000;
        uint16_t v_dec = (mq5_mv % 1000) / 10;

        /* Dong 1: Tieu de */
        LCD_PrintLine(0, "  MQ5 GAS MONITOR   ");

        /* Dong 2: ADC & Dien ap MQ5 */
        snprintf(line, sizeof(line), "ADC:%4u  V:%u.%02uV", adc_val, v_int, v_dec);
        LCD_PrintLine(1, line);

        /* Dong 3: % Khi gas + Thanh muc */
        uint8_t bars = gas_pct / 10;
        if (bars > 10) bars = 10;
        char bar_str[11];
        for (uint8_t i = 0; i < 10; i++) {
            bar_str[i] = (i < bars) ? '#' : '-';
        }
        bar_str[10] = '\0';
        snprintf(line, sizeof(line), "Gas:%3u%% [%s]", gas_pct, bar_str);
        LCD_PrintLine(2, line);

        /* Dong 4: Trang thai */
        if (gas_pct >= GAS_DANGER_PERCENT) {
            LCD_PrintLine(3, "Status: DANGER! GAS!");
        } else if (gas_pct >= GAS_WARN_PERCENT) {
            LCD_PrintLine(3, "Status: WARNING");
        } else {
            LCD_PrintLine(3, "Status: SAFE");
        }

        /* Nhay LED PC13 (neu co) */
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

        HAL_Delay(250);
    }
}

/*
 * Cach goi trong main.c (do STM32CubeMX tao):
 *   1. Khai bao o dau file main.c:  extern void App_CauA_Run(void);
 *   2. Trong ham main(), ngay sau MX_ADC1_Init(); goi:  App_CauA_Run();
 */
