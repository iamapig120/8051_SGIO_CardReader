#include "i2c.h"

extern uint32_t ledDataFor422;

void ch422Write(uint8_t byte1, uint8_t byte2)
{
  __i2c_start();
  __i2c_wr(byte1);
  __i2c_ack();
  __i2c_wr(byte2);
  __i2c_ack();
  __i2c_stop();
}

// 启用数码管输出
void ch422Active()
{
  ch422Write(0x70, 0xFF);
  ch422Write(0x72, 0xFF);
  ch422Write(0x74, 0xFF);
  ch422Write(0x76, 0xFF);

  ch422Write(0x48, 0x05);
}

void ch422Fresh()
{
  // R1 L1
  ch422Write(0x70, ~(uint8_t)((ledDataFor422 >> 12 & 0x3F) | 0xC0));
  // R2 L2
  ch422Write(0x74, ~(uint8_t)((ledDataFor422 >> 6 & 0x3F) | 0xC0));
  // R3 L3
  ch422Write(0x72, ~(uint8_t)((ledDataFor422 & 0x3F) | 0xC0));
  // ch422Write(0x76, 0x00);
  // ch422Write(0x74, 0x00);
  // ch422Write(0x72, 0x00);
  // ch422Write(0x70, 0x00);
}