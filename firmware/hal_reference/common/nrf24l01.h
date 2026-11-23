#ifndef __NRF24L01_H__
#define __NRF24L01_H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* ========= Cau hinh chan GPIO cho nRF24L01+ ========= */
#define NRF_CE_PORT       GPIOB
#define NRF_CE_PIN        GPIO_PIN_0

#define NRF_CSN_PORT      GPIOA
#define NRF_CSN_PIN       GPIO_PIN_4

/* ========= Cau hinh kenh & dia chi RF (TX va RX phai giong nhau) ========= */
#define NRF_RF_CHANNEL    76      /* 2476 MHz (tranh nhieu Wi-Fi) */
#define NRF_PAYLOAD_SIZE  8       /* Kich thuoc goi tin co dinh = 8 byte */

/* ========= Goi du lieu cam bien khi gas MQ5 (8 bytes) ========= */
typedef struct __attribute__((packed)) {
    uint8_t  header;        /* 0xAA: Byte nhan dien goi hop le */
    uint8_t  seq;           /* So thu tu goi tin (0..255) */
    uint16_t adc_raw;       /* Gia tri ADC 12-bit (0..4095) */
    uint16_t mq5_mv;        /* Dien ap thuc tai chan AO cua MQ5 (mV, 0..5000) */
    uint8_t  gas_percent;   /* Phan tram muc khi gas uoc luong (0..100%) */
    uint8_t  checksum;      /* XOR checksum cua 7 byte dau */
} GasPacket_t;

/* ========= Ham giao tiep nRF24L01+ ========= */
void     NRF24_Init(SPI_HandleTypeDef *hspi);
void     NRF24_SetTxMode(const uint8_t *address);
void     NRF24_SetRxMode(const uint8_t *address);
bool     NRF24_Transmit(const uint8_t *data, uint8_t len);
bool     NRF24_DataAvailable(void);
void     NRF24_Receive(uint8_t *data, uint8_t len);
uint8_t  NRF24_ReadReg(uint8_t reg);
uint8_t  NRF24_CalcChecksum(const GasPacket_t *pkt);

#endif /* __NRF24L01_H__ */
