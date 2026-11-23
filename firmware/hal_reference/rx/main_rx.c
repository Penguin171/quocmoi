/*
 * ============================================================================
 *  MACH THU (RX): STM32F103C8T6 + nRF24L01+ + LCD 20x4 (4-bit)
 *  Moi truong: Keil C v5 (STM32CubeMX - HAL)
 * ============================================================================
 *  Huong dan su dung:
 *  - Tao project bang STM32CubeMX voi SPI1 (PA5/PA6/PA7),
 *    GPIO Output cho PA4 (CSN), PB0 (CE), PC13 (LED),
 *    va PB12, PB13, PB14, PB15, PA8, PA9 (LCD 20x4).
 *  - Chen phan code nay va goi App_RX_Run() trong main() sau MX_SPI1_Init().
 */

#include "main.h"
#include "nrf24l01.h"
#include "lcd20x4.h"
#include <stdio.h>

/* Handle SPI1 do STM32CubeMX sinh ra trong main.c */
extern SPI_HandleTypeDef hspi1;

/* Dia chi RF 5 byte (phai giong het ben TX) */
static const uint8_t RF_ADDRESS[5] = { 'G', 'A', 'S', '0', '1' };

/* Nguong canh bao khi gas (%) */
#define GAS_WARN_PERCENT    30
#define GAS_DANGER_PERCENT  60

/* Timeout mat tin hieu RF (ms) */
#define RF_TIMEOUT_MS       3000

static void RX_DisplayPacket(const GasPacket_t *pkt) {
    char line[21];
    uint16_t v_int = pkt->mq5_mv / 1000;
    uint16_t v_dec = (pkt->mq5_mv % 1000) / 10; /* 2 chu so thap phan */

    /* Dong 1: Tieu de + so thu tu goi tin */
    snprintf(line, sizeof(line), "MQ5 GAS MONITOR #%03u", pkt->seq);
    LCD_PrintLine(0, line);

    /* Dong 2: Gia tri ADC va dien ap thuc MQ5 */
    snprintf(line, sizeof(line), "ADC:%4u  V:%u.%02uV", pkt->adc_raw, v_int, v_dec);
    LCD_PrintLine(1, line);

    /* Dong 3: Phan tram khi gas + thanh bieu do don gian */
    uint8_t bars = pkt->gas_percent / 10; /* 0..10 o */
    if (bars > 10) bars = 10;
    char bar_str[11];
    for (uint8_t i = 0; i < 10; i++) {
        bar_str[i] = (i < bars) ? '#' : '-';
    }
    bar_str[10] = '\0';
    snprintf(line, sizeof(line), "Gas:%3u%% [%s]", pkt->gas_percent, bar_str);
    LCD_PrintLine(2, line);

    /* Dong 4: Trang thai an toan */
    if (pkt->gas_percent >= GAS_DANGER_PERCENT) {
        LCD_PrintLine(3, "Status: DANGER! GAS!");
    } else if (pkt->gas_percent >= GAS_WARN_PERCENT) {
        LCD_PrintLine(3, "Status: WARNING");
    } else {
        LCD_PrintLine(3, "Status: SAFE");
    }
}

/* ======================== MAIN LOOP (RX) ======================== */
void App_RX_Run(void) {
    GasPacket_t pkt;
    uint32_t last_rx_tick = 0;
    uint8_t  has_signal   = 0;

    /* Khoi tao LCD 20x4 */
    LCD_Init();
    LCD_PrintLine(0, "MQ5 GAS MONITOR");
    LCD_PrintLine(1, "RF: nRF24L01+ 2.4GHz");
    LCD_PrintLine(2, "Waiting for TX...");
    LCD_PrintLine(3, "Status: INIT");

    /* Khoi tao nRF24L01+ o che do thu (RX) */
    NRF24_Init(&hspi1);
    NRF24_SetRxMode(RF_ADDRESS);

    while (1) {
        if (NRF24_DataAvailable()) {
            NRF24_Receive((uint8_t *)&pkt, sizeof(GasPacket_t));

            /* Kiem tra header va checksum de tranh hien thi du lieu nhieu */
            if (pkt.header == 0xAA && pkt.checksum == NRF24_CalcChecksum(&pkt)) {
                last_rx_tick = HAL_GetTick();
                has_signal   = 1;

                /* Nhay LED PC13 bao nhan thanh cong */
                HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);

                /* Cap nhat LCD 20x4 */
                RX_DisplayPacket(&pkt);
            }
        }

        /* Kiem tra mat ket noi qua 3 giay */
        if (has_signal && ((HAL_GetTick() - last_rx_tick) > RF_TIMEOUT_MS)) {
            has_signal = 0;
            LCD_PrintLine(2, "RF Link Lost!       ");
            LCD_PrintLine(3, "Status: NO SIGNAL   ");
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* Tat LED */
        }

        HAL_Delay(20);
    }
}

/*
 * Trong ham main() do STM32CubeMX tao ra, sau khi goi MX_SPI1_Init(),
 * chi can goi:
 *     App_RX_Run();
 */
