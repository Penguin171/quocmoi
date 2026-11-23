#include "nrf24l01.h"

/* ========= Tap lenh SPI cua nRF24L01+ ========= */
#define NRF_CMD_R_REGISTER      0x00
#define NRF_CMD_W_REGISTER      0x20
#define NRF_CMD_R_RX_PAYLOAD    0x61
#define NRF_CMD_W_TX_PAYLOAD    0xA0
#define NRF_CMD_FLUSH_TX        0xE1
#define NRF_CMD_FLUSH_RX        0xE2
#define NRF_CMD_NOP             0xFF

/* ========= Dia chi thanh ghi nRF24L01+ ========= */
#define NRF_REG_CONFIG          0x00
#define NRF_REG_EN_AA           0x01
#define NRF_REG_EN_RXADDR       0x02
#define NRF_REG_SETUP_AW        0x03
#define NRF_REG_SETUP_RETR      0x04
#define NRF_REG_RF_CH           0x05
#define NRF_REG_RF_SETUP        0x06
#define NRF_REG_STATUS          0x07
#define NRF_REG_RX_ADDR_P0      0x0A
#define NRF_REG_TX_ADDR         0x10
#define NRF_REG_RX_PW_P0        0x11
#define NRF_REG_FIFO_STATUS     0x17

/* ========= Co trong thanh ghi STATUS ========= */
#define NRF_STATUS_RX_DR        (1 << 6)
#define NRF_STATUS_TX_DS        (1 << 5)
#define NRF_STATUS_MAX_RT       (1 << 4)

static SPI_HandleTypeDef *nrf_spi = NULL;

static inline void CSN_Low(void)  { HAL_GPIO_WritePin(NRF_CSN_PORT, NRF_CSN_PIN, GPIO_PIN_RESET); }
static inline void CSN_High(void) { HAL_GPIO_WritePin(NRF_CSN_PORT, NRF_CSN_PIN, GPIO_PIN_SET);   }
static inline void CE_Low(void)   { HAL_GPIO_WritePin(NRF_CE_PORT,  NRF_CE_PIN,  GPIO_PIN_RESET); }
static inline void CE_High(void)  { HAL_GPIO_WritePin(NRF_CE_PORT,  NRF_CE_PIN,  GPIO_PIN_SET);   }

static void NRF24_WriteReg(uint8_t reg, uint8_t value) {
    uint8_t buf[2];
    buf[0] = NRF_CMD_W_REGISTER | (reg & 0x1F);
    buf[1] = value;

    CSN_Low();
    HAL_SPI_Transmit(nrf_spi, buf, 2, 100);
    CSN_High();
}

static void NRF24_WriteRegMulti(uint8_t reg, const uint8_t *data, uint8_t len) {
    uint8_t cmd = NRF_CMD_W_REGISTER | (reg & 0x1F);

    CSN_Low();
    HAL_SPI_Transmit(nrf_spi, &cmd, 1, 100);
    HAL_SPI_Transmit(nrf_spi, (uint8_t *)data, len, 100);
    CSN_High();
}

uint8_t NRF24_ReadReg(uint8_t reg) {
    uint8_t cmd = NRF_CMD_R_REGISTER | (reg & 0x1F);
    uint8_t val = 0;

    CSN_Low();
    HAL_SPI_Transmit(nrf_spi, &cmd, 1, 100);
    HAL_SPI_Receive(nrf_spi, &val, 1, 100);
    CSN_High();

    return val;
}

static void NRF24_SendCmd(uint8_t cmd) {
    CSN_Low();
    HAL_SPI_Transmit(nrf_spi, &cmd, 1, 100);
    CSN_High();
}

void NRF24_Init(SPI_HandleTypeDef *hspi) {
    nrf_spi = hspi;

    CE_Low();
    CSN_High();
    HAL_Delay(100); /* Cho nRF24L01+ on dinh nguon sau Power-On */

    NRF24_WriteReg(NRF_REG_CONFIG, 0x00);      /* Tam tat nguon de cau hinh */
    NRF24_WriteReg(NRF_REG_EN_AA, 0x01);       /* Bat Auto-ACK cho Pipe 0 (doi thanh 0x00 neu mo phong Proteus loi ACK) */
    NRF24_WriteReg(NRF_REG_EN_RXADDR, 0x01);   /* Bat Pipe 0 */
    NRF24_WriteReg(NRF_REG_SETUP_AW, 0x03);    /* Dia chi 5 byte */
    NRF24_WriteReg(NRF_REG_SETUP_RETR, 0x5F);  /* Delay 1500us, thu lai toi da 15 lan (chuan cho 250kbps) */
    NRF24_WriteReg(NRF_REG_RF_CH, NRF_RF_CHANNEL);

    /*
     * RF_SETUP = 0x22:
     *   - Toc do: 250 kbps (RF_DR_LOW = 1, RF_DR_HIGH = 0) -> bat song xa & on dinh nhat
     *   - Cong suat PA: -12 dBm (LOW) -> an it dong, khong sut ap khi dung nguon 3.3V tren Blue Pill
     *   (Khi co nguon 3.3V rieng >= 500mA, co the doi 0x22 thanh 0x26 cho MAX 0 dBm)
     */
    NRF24_WriteReg(NRF_REG_RF_SETUP, 0x22);

    NRF24_WriteReg(NRF_REG_RX_PW_P0, NRF_PAYLOAD_SIZE);
    NRF24_WriteReg(NRF_REG_STATUS, 0x70);      /* Xoa cac co RX_DR, TX_DS, MAX_RT */
    NRF24_SendCmd(NRF_CMD_FLUSH_TX);
    NRF24_SendCmd(NRF_CMD_FLUSH_RX);
}

void NRF24_SetTxMode(const uint8_t *address) {
    CE_Low();

    /* Voi Auto-ACK, RX_ADDR_P0 bat buoc phai giong TX_ADDR */
    NRF24_WriteRegMulti(NRF_REG_TX_ADDR, address, 5);
    NRF24_WriteRegMulti(NRF_REG_RX_ADDR_P0, address, 5);

    /* EN_CRC=1 (bit 3), PWR_UP=1 (bit 1), PRIM_RX=0 (TX mode) -> 0x0A */
    NRF24_WriteReg(NRF_REG_CONFIG, 0x0A);
    NRF24_WriteReg(NRF_REG_STATUS, 0x70);
    NRF24_SendCmd(NRF_CMD_FLUSH_TX);

    HAL_Delay(5); /* Cho PWR_UP >= 1.5ms */
}

void NRF24_SetRxMode(const uint8_t *address) {
    CE_Low();

    NRF24_WriteRegMulti(NRF_REG_RX_ADDR_P0, address, 5);
    NRF24_WriteReg(NRF_REG_RX_PW_P0, NRF_PAYLOAD_SIZE);

    /* EN_CRC=1 (bit 3), PWR_UP=1 (bit 1), PRIM_RX=1 (RX mode) -> 0x0B */
    NRF24_WriteReg(NRF_REG_CONFIG, 0x0B);
    NRF24_WriteReg(NRF_REG_STATUS, 0x70);
    NRF24_SendCmd(NRF_CMD_FLUSH_RX);

    HAL_Delay(5);
    CE_High(); /* Bat dau lang nghe lien tuc */
}

bool NRF24_Transmit(const uint8_t *data, uint8_t len) {
    uint8_t cmd = NRF_CMD_W_TX_PAYLOAD;
    uint32_t start_tick = HAL_GetTick();

    CE_Low();
    NRF24_WriteReg(NRF_REG_STATUS, 0x70);
    NRF24_SendCmd(NRF_CMD_FLUSH_TX);

    /* Nap du lieu vao TX FIFO */
    CSN_Low();
    HAL_SPI_Transmit(nrf_spi, &cmd, 1, 100);
    HAL_SPI_Transmit(nrf_spi, (uint8_t *)data, len, 100);
    CSN_High();

    /* Kich xung CE de phat goi tin */
    CE_High();
    HAL_Delay(1);
    CE_Low();

    /* Cho co TX_DS (thanh cong) hoac MAX_RT (that bai) voi timeout 50ms */
    while ((HAL_GetTick() - start_tick) < 50) {
        uint8_t status = NRF24_ReadReg(NRF_REG_STATUS);
        if (status & NRF_STATUS_TX_DS) {
            NRF24_WriteReg(NRF_REG_STATUS, NRF_STATUS_TX_DS);
            return true;
        }
        if (status & NRF_STATUS_MAX_RT) {
            NRF24_WriteReg(NRF_REG_STATUS, NRF_STATUS_MAX_RT);
            NRF24_SendCmd(NRF_CMD_FLUSH_TX);
            return false;
        }
    }

    NRF24_SendCmd(NRF_CMD_FLUSH_TX);
    return false;
}

bool NRF24_DataAvailable(void) {
    uint8_t status = NRF24_ReadReg(NRF_REG_STATUS);
    uint8_t fifo   = NRF24_ReadReg(NRF_REG_FIFO_STATUS);

    if ((status & NRF_STATUS_RX_DR) || ((fifo & 0x01) == 0)) {
        return true;
    }
    return false;
}

void NRF24_Receive(uint8_t *data, uint8_t len) {
    uint8_t cmd = NRF_CMD_R_RX_PAYLOAD;

    CSN_Low();
    HAL_SPI_Transmit(nrf_spi, &cmd, 1, 100);
    HAL_SPI_Receive(nrf_spi, data, len, 100);
    CSN_High();

    NRF24_WriteReg(NRF_REG_STATUS, NRF_STATUS_RX_DR);
}

uint8_t NRF24_CalcChecksum(const GasPacket_t *pkt) {
    const uint8_t *raw = (const uint8_t *)pkt;
    uint8_t cs = 0;
    for (uint8_t i = 0; i < (sizeof(GasPacket_t) - 1); i++) {
        cs ^= raw[i];
    }
    return cs;
}
