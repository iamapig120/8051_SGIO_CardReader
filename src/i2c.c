#include "i2c.h"
#include "sys.h"

/* -------- I2C驱动 -------- */
#if defined(HAS_ROM) || defined(HAS_I2C)
/*
 * EEPROM时序复位
 */
void __i2c_reset() {
    I2C_SDA = 1; delay_us(5);
    for (uint8_t i = 0; i < 9; i++) {
        I2C_SCL = 1; delay_us(5);
        if (I2C_SDA == 1) break;
        I2C_SCL = 0; delay_us(5);
    }
    I2C_SCL = 0; delay_us(5);
    I2C_SDA = 1;
    I2C_SCL = 1; delay_us(5);
    I2C_SDA = 0; delay_us(5);
}

/*
 * 开始时序
 */
void __i2c_start() {
    I2C_SDA = 1;
    I2C_SCL = 1; delay_us(5);
    I2C_SDA = 0; delay_us(5);
}

/*
 * 停止时序
 */
void __i2c_stop() {
    I2C_SDA = 0;
    I2C_SCL = 1; delay_us(5);
    I2C_SDA = 1; delay_us(5);
}

/*
 * 应答时序
 */
void __i2c_ack() {
    I2C_SCL = 0;
    I2C_SDA = 1;
    I2C_SCL = 1; delay_us(5);
    for (uint8_t i = 0; i < 0xFF; i++)
        if (I2C_SDA == 0) break;
    I2C_SCL = 0; delay_us(5);
}

/*
 * 非应答时序
 */
void __i2c_nak() {
    I2C_SCL = 0;
    I2C_SDA = 1;
    I2C_SCL = 1; delay_us(5);
    I2C_SCL = 0; delay_us(5);
}

/*
 * 发送一个字节
 */
void __i2c_wr(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        I2C_SCL = 0; delay_us(5);
        I2C_SDA = (data & 0x80); delay_us(5);
        I2C_SCL = 1; delay_us(5);
        data <<= 1;
    }
    I2C_SCL = 0;
    I2C_SDA = 1;
    delay_us(5);
}

/*
 * 读取一个字节
 */
uint8_t __i2c_rd() {
    uint8_t data = 0;
    __bit tmp;
    I2C_SCL = 0; delay_us(5);
    for (uint8_t i = 0; i < 8; i++) {
        I2C_SCL = 1; delay_us(5);
        tmp = I2C_SDA;
        data <<= 1;
        data |= tmp;
        delay_us(5);
        I2C_SCL = 0; delay_us(5);
    }
    return data;
}

#endif
