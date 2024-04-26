#include "rgb.h"

#ifdef LEDTYPE_WS2812
static uint32_i rgbGRBData[2];
uint8_t  isLedDataChanged = 0;

void __ws2812_init()
{
  LED_IO   = 0;
  LED_2_IO = 0;
}
void __ws2812_send(uint32_t value, __sbit led_io)
{
  uint8_t i, j;
  for (i = 0; i < 24; i++)
  {
    if (value & 0x800000)
    {
      led_io = 1;
      for (j = 4; j > 0; j--)
        __asm__("nop");
      led_io = 0;
      for (j = 1; j > 0; j--)
        __asm__("nop");
    }
    else
    {
      led_io = 1;
      for (j = 1; j > 0; j--)
        __asm__("nop");
      led_io = 0;
      for (j = 4; j > 0; j--)
        __asm__("nop");
    }
    value <<= 1;
  }
  led_io = 0;
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
  E_DIS = 1;
  __ws2812_init();
  __ws2812_send(rgbGRBData[0], LED_IO);
  __ws2812_send(rgbGRBData[1], LED_2_IO);
  E_DIS = 0;
#endif
}
#endif