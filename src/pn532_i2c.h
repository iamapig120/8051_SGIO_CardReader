#ifndef __PN532_I2C_H
#define __PN532_I2C_H

#include "bsp.h"
#include "i2c.h"
#include "pn532.h"

#define PN532_I2C_ADDRESS 0x48

void _begin();
void _wakeup();

int8_t _writeCommand(uint8_t *header, uint8_t hlen, uint8_t *body, uint8_t blen);
int8_t _readResponse(uint8_t buf[], uint8_t len, uint16_t timeout);

#endif