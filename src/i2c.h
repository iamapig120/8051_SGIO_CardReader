#ifndef __I2C_H
#define __I2C_H

#include "bsp.h"
#include "ch552.h"

#if defined(HAS_ROM) || defined(HAS_I2C)
void    __i2c_reset();
void    __i2c_start();
void    __i2c_stop();
void    __i2c_ack();
void    __i2c_nak();
void    __i2c_wr(uint8_t data);
uint8_t __i2c_rd();
#endif

#endif