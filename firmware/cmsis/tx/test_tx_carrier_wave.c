/*
 * ============================================================================
 *  BAI TEST SONG MANG LIEN TUC (CONT_WAVE) CHO MACH PHAT (TX)
 *  Muc dich: Kiem tra 100% tang RF / PA / LNA cua 2 module nRF24L01+ con song khong
 *
 *  Cach dung:
 *    - Mach Thu (RX): GIU NGUYEN code hien tai (KHONG can nap lai!)
 *    - Mach Phat (TX): Thay file nay vao Source Group 1 -> Build (F7) -> Nap (F8)
 *
 *  Cach doc ket qua:
 *    - TX se luan phien: 2 giay PHAT SONG KENH 76 (LED PC13 SANG)
 *                        2 giay NGUNG PHAT       (LED PC13 TAT)
 *    - Nhin chu "RPD:" tren dong 3 cua LCD 20x4 ben RX:
 *        + Khi LED TX SANG -> LCD hien "RPD:1", LED TX TAT -> "RPD:0":
 *            => 100% CA 2 MODULE DEU CON SONG TANG RF!
 *        + Khi LED TX SANG ma LCD van dung im "RPD:0" (du da thu dao cheo 2 module):
 *            => It nhat 1 module bi hong tang RF / chip PA (RFX2401C) hoac mat anten.
 * ============================================================================
 */

#include "stm32f10x.h"
#include <stdint.h>

#define NRF_RF_CHANNEL      76      /* Khop voi kenh 76 ben RX */

static volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void) {
    ms_ticks++;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = ms_ticks;
    while ((ms_ticks - start) < ms) {}
}

static void delay_short(void) {
    for (volatile uint32_t i = 0; i < 400; i++) {
        __NOP();
    }
}

static inline void pin_write(GPIO_TypeDef *port, uint8_t pin, uint8_t val) {
    if (val) port->BSRR = (1U << pin);
    else     port->BSRR = (1U << (pin + 16U));
}

static inline void CSN_Low(void)  { pin_write(GPIOA, 4, 0); delay_short(); }
static inline void CSN_High(void) { pin_write(GPIOA, 4, 1); delay_short(); }
static inline void CE_Low(void)   { pin_write(GPIOB, 0, 0); }
static inline void CE_High(void)  { pin_write(GPIOB, 0, 1); }

static void GPIO_SPI1_Init(void) {
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN |
                    RCC_APB2ENR_IOPCEN | RCC_APB2ENR_SPI1EN;

    GPIOA->CRL &= ~(0xFU << 16);
    GPIOA->CRL |=  (0x3U << 16);
    CSN_High();

    GPIOA->CRL &= ~((0xFU << 20) | (0xFU << 24) | (0xFU << 28));
    GPIOA->CRL |=  ((0xBU << 20) | (0x4U << 24) | (0xBU << 28));

    GPIOB->CRL &= ~(0xFU << 0);
    GPIOB->CRL |=  (0x3U << 0);
    CE_Low();

    GPIOC->CRH &= ~(0xFU << 20);
    GPIOC->CRH |=  (0x2U << 20);
    pin_write(GPIOC, 13, 1);

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t SPI1_Transfer(uint8_t data) {
    while (!(SPI1->SR & SPI_SR_TXE));
    SPI1->DR = data;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return (uint8_t)(SPI1->DR);
}

static void SPI1_WaitIdle(void) {
    while (SPI1->SR & SPI_SR_BSY);
}

static void NRF_WriteReg(uint8_t reg, uint8_t val) {
    CSN_Low();
    SPI1_Transfer(0x20U | (reg & 0x1FU));
    SPI1_Transfer(val);
    SPI1_WaitIdle();
    CSN_High();
}

static uint8_t NRF_ReadReg(uint8_t reg) {
    uint8_t val;
    CSN_Low();
    SPI1_Transfer(reg & 0x1FU);
    val = SPI1_Transfer(0xFFU);
    SPI1_WaitIdle();
    CSN_High();
    return val;
}

int main(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000U);

    GPIO_SPI1_Init();
    delay_ms(120);

    /* Cau hinh che do phat song mang lien tuc (Datasheet nRF24L01+ Appendix C) */
    CE_Low();
    NRF_WriteReg(0x00, 0x02);           /* CONFIG: PWR_UP=1, PRIM_RX=0 */
    delay_ms(5);
    NRF_WriteReg(0x05, NRF_RF_CHANNEL); /* Kenh 76 (2476 MHz) */
    NRF_WriteReg(0x06, 0x96);           /* CONT_WAVE=1 (bit7), PLL_LOCK=1 (bit4), PA MAX (0dBm) */

    /* Kiem tra SPI: neu loi SPI thi nhay LED sieu nhanh */
    if (NRF_ReadReg(0x05) != NRF_RF_CHANNEL) {
        while (1) {
            pin_write(GPIOC, 13, 0); delay_ms(100);
            pin_write(GPIOC, 13, 1); delay_ms(100);
        }
    }

    /* Vong lap test: 2 giay PHAT (LED SANG) -> 2 giay TAT (LED TAT) */
    while (1) {
        CE_High();                  /* Bat dau phat song mang lien tuc tren kenh 76 */
        pin_write(GPIOC, 13, 0);    /* LED PC13 SANG */
        delay_ms(2000);

        CE_Low();                   /* Ngung phat song */
        pin_write(GPIOC, 13, 1);    /* LED PC13 TAT */
        delay_ms(2000);
    }
}
