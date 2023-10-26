#include "pn532_i2c.h"
#include "sys.h"
#include <string.h>

#if defined(HAS_I2C)

uint8_t         command = 0x00;
extern uint16_t sysMsCounter;
uint8_x         buff[6];

uint8_c PN532_NACK[] = {0, 0, 0xFF, 0xFF, 0, 0};

void _begin()
{
  return;
}

void _wakeup()
{
  return;
}

int8_t getResponseLength(uint8_t buf[], uint8_t len, uint16_t timeout)
{
  uint8_t i;
  sysMsCounter = 0;
  do
  {
    __i2c_start();
    __i2c_wr(PN532_I2C_ADDRESS | 0x01);
    __i2c_ack();
    buff[0] = __i2c_rd();
    if (buff[0] & 0x01)
    {
      __i2c_ack();
      sysMsCounter = 0;
      break;
    }
    __i2c_nak();
    __i2c_stop();
    if (sysMsCounter > timeout)
    {
      sysMsCounter = 0;
      return PN532_TIMEOUT;
    }
  } while (1);
  buff[0] = __i2c_rd();
  __i2c_ack();
  buff[1] = __i2c_rd();
  __i2c_ack();
  buff[2] = __i2c_rd();
  __i2c_ack();
  if (buff[0] != PN532_PREAMBLE || buff[1] != PN532_STARTCODE1 || buff[2] != PN532_STARTCODE2)
  {
    return PN532_INVALID_FRAME;
  }
  buff[3] = __i2c_rd();
  __i2c_ack();

  __i2c_start();
  __i2c_wr(PN532_I2C_ADDRESS & 0xFE);
  __i2c_ack();
  for (i = 0; i < 6; i++)
  {
    __i2c_wr(PN532_NACK[i]);
    __i2c_ack();
  }
  __i2c_stop();

  return buff[3];
}

int8_t _writeCommand(uint8_t *header, uint8_t hlen, uint8_t *body, uint8_t blen)
{
  uint8_t length = hlen + blen + 1, i, checksum = PN532_HOSTTOPN532;
  sysMsCounter = 0;

  command = header[0];

  __i2c_start();
  __i2c_wr(PN532_I2C_ADDRESS & 0xFE);
  __i2c_ack();
  __i2c_wr(PN532_PREAMBLE);
  __i2c_ack();
  __i2c_wr(PN532_STARTCODE1);
  __i2c_ack();
  __i2c_wr(PN532_STARTCODE2);
  __i2c_ack();

  __i2c_wr(length);
  __i2c_ack();
  __i2c_wr(~length + 1);
  __i2c_ack();

  __i2c_wr(PN532_HOSTTOPN532);
  __i2c_ack();
  for (i = 0; i < hlen; i++)
  {
    __i2c_wr(header[i]);
    __i2c_ack();
    checksum += header[i];
  }
  for (i = 0; i < blen; i++)
  {
    __i2c_wr(body[i]);
    __i2c_ack();
    checksum += body[i];
  }
  checksum = ~checksum + 1;
  __i2c_wr(checksum);
  __i2c_ack();
  __i2c_wr(PN532_POSTAMBLE);
  __i2c_ack();
  __i2c_stop();

  // WaitForACK
  do
  {
    __i2c_start();
    __i2c_wr(PN532_I2C_ADDRESS | 0x01);
    __i2c_ack();
    buff[0] = __i2c_rd();
    if (buff[0] & 0x01)
    {
      __i2c_ack();
      sysMsCounter = 0;
      break;
    }
    __i2c_nak();
    __i2c_stop();
    if (sysMsCounter > 1000)
    {
      sysMsCounter = 0;
      return PN532_TIMEOUT;
    }
  } while (1);
  buff[0] = __i2c_rd();
  __i2c_ack();
  buff[1] = __i2c_rd();
  __i2c_ack();
  buff[2] = __i2c_rd();
  __i2c_ack();
  buff[3] = __i2c_rd();
  __i2c_ack();
  buff[4] = __i2c_rd();
  __i2c_ack();
  buff[5] = __i2c_rd();
  __i2c_nak();
  __i2c_stop();
  if (memcmp(buff, "\x0\x0\xFF\x0\xFF\x0", 6))
  {
    return PN532_INVALID_ACK;
  }
  return PN532_OK;
}

int8_t _readResponse(uint8_t buf[], uint8_t len, uint16_t timeout)
{
  uint8_t length;
  uint8_t cmd = command + 1;
  uint8_t i, checksum;
  sysMsCounter = 0;

  length = getResponseLength(buf, len, timeout);

  sysMsCounter = 0;
  do
  {
    __i2c_start();
    __i2c_wr(PN532_I2C_ADDRESS | 0x01);
    __i2c_ack();
    buff[0] = __i2c_rd();
    if (buff[0] & 0x01)
    {
      __i2c_ack();
      sysMsCounter = 0;
      break;
    }
    __i2c_nak();
    __i2c_stop();
    if (sysMsCounter > timeout)
    {
      sysMsCounter = 0;
      return PN532_TIMEOUT;
    }
  } while (1);
  buff[0] = __i2c_rd();
  __i2c_ack();
  buff[1] = __i2c_rd();
  __i2c_ack();
  buff[2] = __i2c_rd();
  __i2c_ack();
  if (buff[0] != PN532_PREAMBLE || buff[1] != PN532_STARTCODE1 || buff[2] != PN532_STARTCODE2)
  {
    return PN532_INVALID_FRAME;
  }
  length = __i2c_rd();
  __i2c_ack();

  if (0 != (length + __i2c_rd()))
  { // checksum of length
    __i2c_nak();
    __i2c_stop();
    return PN532_INVALID_FRAME;
  }
  __i2c_ack();

  buff[0] = __i2c_rd();
  __i2c_ack();
  buff[1] = __i2c_rd();
  if (PN532_PN532TOHOST != buff[0] || (cmd) != buff[1])
  {
    __i2c_nak();
    __i2c_stop();
    return PN532_INVALID_FRAME;
  }
  __i2c_ack();

  length -= 2;
  if (length > len)
  {
    __i2c_stop();
    return PN532_NO_SPACE; // not enough space
  }

  checksum = PN532_PN532TOHOST + cmd;
  for (i = 0; i < length; i++)
  {
    buf[i] = __i2c_rd();
    __i2c_ack();
    checksum += buf[i];
  }

  buff[0] = __i2c_rd();
  if (0 != (uint8_t)(buff[0] + checksum))
  {
    __i2c_nak();
    __i2c_stop();
    return PN532_INVALID_FRAME;
  }
  __i2c_ack();

  __i2c_rd();
  __i2c_nak();

  __i2c_stop();

  return length;
}

#endif