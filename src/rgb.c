#include "rgb.h"
#include "usb_cdc.h"

#ifdef LEDTYPE_WS2812
static uint32_i rgbGRBData[2];
uint8_t         isLedDataChanged = 0;

void __ws2812_init()
{
  LED_IO   = 0;
  LED_2_IO = 0;
}
void __ws2812_send(uint32_t value)
{
  uint8_t i, j;
  for (i = 0; i < 24; i++)
  {
    if (value & 0x800000)
    {
      LED_IO = 1;
      for (j = 4; j > 0; j--)
        __asm__("nop");
      LED_IO = 0;
      for (j = 3; j > 0; j--)
        __asm__("nop");
    }
    else
    {
      LED_IO = 1;
      for (j = 1; j > 0; j--)
        __asm__("nop");
      LED_IO = 0;
      for (j = 6; j > 0; j--)
        __asm__("nop");
    }
    value <<= 1;
  }
  LED_IO = 0;
}
void __ws2812_send_p2(uint32_t value)
{
  uint8_t i, j;
  for (i = 0; i < 24; i++)
  {
    if (value & 0x800000)
    {
      LED_2_IO = 1;
      for (j = 4; j > 0; j--)
        __asm__("nop");
      LED_2_IO = 0;
      for (j = 3; j > 0; j--)
        __asm__("nop");
    }
    else
    {
      LED_2_IO = 1;
      for (j = 1; j > 0; j--)
        __asm__("nop");
      LED_2_IO = 0;
      for (j = 6; j > 0; j--)
        __asm__("nop");
    }
    value <<= 1;
  }
  LED_2_IO = 0;
}
#endif

#if LED_COUNT > 0
void rgbInit()
{
#ifdef LEDTYPE_WS2812
  __ws2812_init();
#elif defined(LEDTYPE_RGB)
  return;
#endif
  return;
}

void rgbSet(uint8_t index, uint32_t value)
{
#ifdef LEDTYPE_WS2812
  rgbGRBData[index] = ((value & 0x00FF00) << 8) | ((value & 0xFF0000) >> 8) |
                      (value & 0x0000FF);
#endif
}

void rgbPush()
{
#ifdef LEDTYPE_WS2812
  // 阻断CDC接收
  UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;
  UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;

  E_DIS = 1;
  __ws2812_init();
  __ws2812_send(rgbGRBData[0]);
  __ws2812_send(rgbGRBData[0]);
  __ws2812_send(rgbGRBData[0]);
  __ws2812_send(rgbGRBData[0]);
  __ws2812_send(rgbGRBData[0]);
  __ws2812_send(rgbGRBData[0]);

  __ws2812_send_p2(rgbGRBData[1]);
  __ws2812_send_p2(rgbGRBData[1]);
  __ws2812_send_p2(rgbGRBData[1]);
  __ws2812_send_p2(rgbGRBData[1]);
  __ws2812_send_p2(rgbGRBData[1]);
  __ws2812_send_p2(rgbGRBData[1]);

  E_DIS = 0;

  // 重启CDC接收
  if (CDC1_Rx_Pending == 0)
  {
    UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
  }
  if (CDC2_Rx_Pending == 0)
  {
    UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
  }
#endif
}
#endif