/*
 * ============================================================================
 *  MACH PHAT (TX): STM32F103C8T6 + MQ5 (PA0 qua cau phan ap 10k/15k) + nRF24L01+
 *  Moi truong: Keil C v5 (STM32CubeMX - HAL)
 * ============================================================================
 *  Huong dan su dung:
 *  - Tao project bang STM32CubeMX voi ADC1 (IN0 - PA0), SPI1 (PA5/PA6/PA7),
 *    GPIO Output cho PA4 (CSN), PB0 (CE), PC13 (LED).
 *  - Chen phan code USER CODE ben duoi vao file Core/Src/main.c.
 */

#include "main.h"
#include "nrf24l01.h"

/* Cac handle do STM32CubeMX sinh ra trong main.c */
extern ADC_HandleTypeDef hadc1;
extern SPI_HandleTypeDef hspi1;

/* Dia chi RF 5 byte (TX va RX bat buoc giong nhau) */
static const uint8_t RF_ADDRESS[5] = { 'G', 'A', 'S', '0', '1' };

/* Ham doc gia tri trung binh 16 mau ADC1_IN0 (PA0) de chong nhieu */
static uint16_t MQ5_ReadADC_Avg(void) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 16; i++) {
        HAL_ADC_Start(&hadc1);
        if (HAL_ADC_PollForConversion(&hadc1, 20) == HAL_OK) {
            sum += HAL_ADC_GetValue(&hadc1);
        }
        HAL_ADC_Stop(&hadc1);
        HAL_Delay(2);
    }
    return (uint16_t)(sum / 16);
}

/*
 * Quy doi ADC 12-bit (0..4095) sang dien ap thuc tai chan AO cua MQ5 (mV):
 *   V_PA0 (mV) = (adc * 3300) / 4095
 *   Do cau phan ap R1 = 10k (tren), R2 = 15k (duoi):
 *   V_PA0 = V_AO * (15 / 25) = V_AO * 3 / 5  =>  V_AO = V_PA0 * 5 / 3
 */
static uint16_t MQ5_CalcVoltage_mV(uint16_t adc_raw) {
    uint32_t v_pa0_mv = ((uint32_t)adc_raw * 3300UL) / 4095UL;
    uint32_t v_ao_mv  = (v_pa0_mv * 25UL) / 15UL;
    if (v_ao_mv > 5000UL) v_ao_mv = 5000UL;
    return (uint16_t)v_ao_mv;
}

/*
 * Uoc luong muc khi gas 0..100% tu dien ap AO cua MQ5:
 *   - Khong khi sach: MQ5 thuong ra ~400mV - 800mV
 *   - Khi co khi gas: dien ap tang len toi ~4500mV
 * (Ban co the chinh MQ5_CLEAN_MV theo gia tri thuc te do duoc trong phong)
 */
#define MQ5_CLEAN_MV    600
#define MQ5_MAX_MV      4500

static uint8_t MQ5_CalcPercent(uint16_t mq5_mv) {
    if (mq5_mv <= MQ5_CLEAN_MV) return 0;
    if (mq5_mv >= MQ5_MAX_MV)   return 100;
    return (uint8_t)(((uint32_t)(mq5_mv - MQ5_CLEAN_MV) * 100UL) / (MQ5_MAX_MV - MQ5_CLEAN_MV));
}

/* ======================== MAIN LOOP (TX) ======================== */
void App_TX_Run(void) {
    GasPacket_t pkt;
    uint8_t seq_num = 0;

    /* Hieu chuan ADC noi bo STM32F1 de tang do chinh xac */
    HAL_ADCEx_Calibration_Start(&hadc1);

    /* Khoi tao nRF24L01+ o che do phat (TX) */
    NRF24_Init(&hspi1);
    NRF24_SetTxMode(RF_ADDRESS);

    while (1) {
        uint16_t adc_val = MQ5_ReadADC_Avg();
        uint16_t mq5_mv  = MQ5_CalcVoltage_mV(adc_val);
        uint8_t  gas_pct = MQ5_CalcPercent(mq5_mv);

        pkt.header      = 0xAA;
        pkt.seq         = seq_num++;
        pkt.adc_raw     = adc_val;
        pkt.mq5_mv      = mq5_mv;
        pkt.gas_percent = gas_pct;
        pkt.checksum    = NRF24_CalcChecksum(&pkt);

        if (NRF24_Transmit((const uint8_t *)&pkt, sizeof(GasPacket_t))) {
            /* Gui thanh cong (co ACK tu RX): nhay LED PC13 1 lan ngan */
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); /* LED sang */
            HAL_Delay(50);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   /* LED tat */
        } else {
            /* Gui that bai: nhay nhanh 2 lan de bao loi RF */
            for (uint8_t i = 0; i < 2; i++) {
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
                HAL_Delay(30);
                HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
                HAL_Delay(30);
            }
        }

        HAL_Delay(450); /* Chu ky gui ~0.5 giay/lan */
    }
}

/*
 * Trong ham main() do STM32CubeMX tao ra, sau khi goi MX_ADC1_Init() va MX_SPI1_Init(),
 * chi can goi:
 *     App_TX_Run();
 */
