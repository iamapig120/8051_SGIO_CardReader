#include "rom.h"
#include "sys.h"

#include "i2c.h"
 
#if defined(HAS_ROM)

/*
 * 向指定地址写入一个字节
 */
void __eeprom_write(uint16_t addr, uint8_t data) {
    ROM_WP = 0; delay_us(5);
    addr &= (ROM_SIZE - 1);
    uint8_t devAddr = 0xA0 | (((addr >> 8) & 0x07) << 1);
    uint8_t wordAddr = addr & 0xFF;
    __i2c_start();
    __i2c_wr(devAddr & 0xFE);
    __i2c_ack();
    __i2c_wr(wordAddr);
    __i2c_ack();
    __i2c_wr(data);
    __i2c_ack();
    __i2c_stop();
    delay_ms(2);
    ROM_WP = 1; delay_us(5);
}

/*
 * 读取指定地址的字节
 */
uint8_t __eeprom_read(uint16_t addr) {
    addr &= (ROM_SIZE - 1);
    uint8_t devAddr = 0xA0 | (((addr >> 8) & 0x07) << 1);
    uint8_t wordAddr = addr & 0xFF;
    uint8_t data = 0;
    __i2c_start();
    __i2c_wr(devAddr & 0xFE);
    __i2c_ack();
    __i2c_wr(wordAddr);
    __i2c_ack();
    __i2c_start();
    __i2c_wr(devAddr | 0x01);
    __i2c_ack();
    data = __i2c_rd();
    __i2c_nak();
    __i2c_stop();
    return data;
}
#else
// volatile __bit __rom_dummy;
#endif
/* -------- 外部存储器驱动 -------- */

/*
 * 内部存储器写入
 */
void __flash_write(uint8_t addr, uint8_t data) {
    // addr &= (FLASH_SIZE - 1);
    // uint8_t offset = addr << 1;
    addr <<= 1;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG |= bDATA_WE;
    ROM_ADDR = DATA_FLASH_ADDR + addr;
    ROM_DATA_L = data;
    ROM_CTRL = ROM_CMD_WRITE;
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    GLOBAL_CFG &= ~bDATA_WE;
}

/*
 * 内部存储器读取
 */
uint8_t __flash_read(uint8_t addr) {
    // addr &= (FLASH_SIZE - 1);
    // uint8_t offset = addr << 1;
    addr <<= 1;
    ROM_ADDR = DATA_FLASH_ADDR + addr;
    ROM_CTRL = ROM_CMD_READ;
    return ROM_DATA_L;
}

/*
 * ROM初始化
 */
#if defined(HAS_ROM)
void romInit() {
    I2C_SDA = 1;
    I2C_SCL = 1;
    ROM_WP = 0; delay_us(5);
    __i2c_reset();
    ROM_WP = 1; delay_us(5);
}
#endif

/*
 * 从内部存储器读取一个字节
 */
uint8_t romRead8i(uint8_t addr) {
    return __flash_read(addr);
}

/*
 * 向内部存储器写入一个字节
 */
void romWrite8i(uint8_t addr, uint8_t data) {
    __flash_write(addr, data);
}

/*
 * 从内部存储器读取一个字
 */
uint16_t romRead16i(uint8_t addr) {
    return ((uint16_t)__flash_read(addr)) << 8 | ((uint16_t)__flash_read(addr + 1));
}

/*
 * 向内部存储器写入一个字
 */
void romWrite16i(uint8_t addr, uint16_t data) {
    __flash_write(addr, data >> 8);
    __flash_write(addr + 1, data & 0xFF);
}

/*
 * 从外部存储器读取一个字节
 */
uint8_t romRead8e(uint16_t addr) {
#if defined(HAS_ROM)
    return __eeprom_read(addr);
#else
    return addr == 0; // Dummy thing
#endif
}

/*
 * 向外部存储器写入一个字节
 */
void romWrite8e(uint16_t addr, uint8_t data) {
#if defined(HAS_ROM)
    __eeprom_write(addr, data);
#else
    // __rom_dummy = addr == 0 && data == 0; // Dummy thing
#endif
}

/*
 * 从外部存储器读取一个字
 */
uint16_t romRead16e(uint16_t addr) {
#if defined(HAS_ROM)
    return ((uint16_t) __eeprom_read(addr)) << 8 | ((uint16_t) __eeprom_read(addr + 1));
#else
    return addr == 0; // Dummy thing
#endif
}

/*
 * 向外部存储器写入一个字
 */
void romWrite16e(uint16_t addr, uint16_t data) {
#if defined(HAS_ROM)
    __eeprom_write(addr, (data >> 8) & 0xFF);
    __eeprom_write(addr + 1, data & 0xFF);
#else
    // __rom_dummy = addr == 0 && data == 0; // Dummy thing
#endif
}
