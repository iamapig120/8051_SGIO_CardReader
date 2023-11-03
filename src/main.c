#include "adc.h"
#include "bsp.h"
#include "ch552.h"
#include "debounce.h"
#include "rgb.h"
#include "rom.h"
#include "sys.h"
#include "usb.h"
#include "usb_cdc.h"
#include "usb_sega_io.h"

#include <string.h>

#ifdef TOUCH_COUNT
#include "touchkey.h"
#endif

void __usbDeviceInterrupt() __interrupt(INT_NO_USB) __using(1); // USB中断定义
void __tim2Interrupt() __interrupt(INT_NO_TMR2) __using(2);
void __ADCInterrupt() __interrupt(INT_NO_ADC) __using(3);

#ifdef TOUCH_COUNT
void __TK_int_ISR() __interrupt(INT_NO_TKEY) __using(1); // TouchKey中断定义
#endif

extern uint8_t sysMsCounter;

#if KEY_COUNT <= 8
uint8_t prevKey = 0; // 上一次扫描时的按键激活状态记录

uint8_t activeKey; // 最近一次扫描时的按键激活状态记录
#elif KEY_COUNT <= 16
uint16_t prevKey = 0; // 上一次扫描时的按键激活状态记录
uint16_t activeKey;   // 最近一次扫描时的按键激活状态记录
#endif

uint8_t prevAdc   = 0x80;
uint8_t adcUpdate = 0;

void main()
{
  uint8_t         i, j;
  static uint16_t timer = 0;

  sysClockConfig();
  delay_ms(20);

#if defined(HAS_ROM)
  romInit();
  delay_ms(300);
#endif

#ifdef TOUCH_COUNT
  TK_Init(BIT4 | BIT5 | BIT6 | BIT7, 0, 1); /* Choose TIN2, TIN3, TIN4, TIN5 for touchkey input. enable interrupt. */
  TK_SelectChannel(0);                      /* NOTICE: ch is not compatible with the IO actually. */
#endif

  // sysLoadConfig();
  // SysConfig *cfg = sysGetConfig();
  SysConfig *cfg = (&sysConfig);

  CDC_InitBaud();
  usbDevInit();
  debounceInit();
  rgbInit();

  ADCInit(1); // 慢采样
  ADC_ChannelSelect(1);

  EA = 1; // 启用中断

  delay_ms(100);

  usbReleaseAll();
  io4Init();
  // requestHIDData();

  sysTickConfig();

  sysMsCounter = 0;
  while (1)
  {
    while (sysMsCounter--)
    {
#ifdef MOTOR
      motorUpdate();
#endif
      debounceUpdate();
      if (timer == 0)
      {
        timer = 0x0FFF; // 大于4s一个HID数据包
        Enp1IntIn();

        if ((timer & 0x007F) == 0x00) // 大约每秒更新8次灯光，单次更新约需要0.5ms
        {
          rgbPush();
        }
      }
      timer--;
    }

    activeKey = 0;
    if (prevAdc > adc_data)
    {
      if (prevAdc - adc_data > 0x04)
      {
        prevAdc   = adc_data;
        adcUpdate = 1;
      }
    }
    else
    {
      if (adc_data - prevAdc > 0x04)
      {
        prevAdc   = adc_data;
        adcUpdate = 1;
      }
    }

    for (i = 0; i < KEY_COUNT; i++)
    {
      activeKey <<= 1;
      activeKey |= isKeyActive(i);
    }
    if (prevKey != activeKey || adcUpdate)
    {
      prevKey ^= activeKey;
      for (i = 0; i < KEY_COUNT; i++)
      {
        // 如果第i个键是被更改的
        if ((prevKey >> i) & 0x01)
        {
          // usbReleaseAll();
          switch (cfg->keyConfig[i].mode)
          {
          // 手柄按钮 及 手柄摇杆
          case GamepadButton:
            break;
          case GamepadButton_Indexed:

            EP1_IN_BUF[0] = 1;

            dataForUpload = (DataUpload *)(&(EP1_IN_BUF[1]));
            memset(dataForUpload->buttons, 0x00, 4);

            for (j = 0; j < KEY_COUNT; j++)
            {
              if (cfg->keyConfig[j].mode == GamepadButton_Indexed)
              {
                if ((activeKey >> j) & 0x01)
                {
                  (dataForUpload->buttons[cfg->keyConfig[j].codeLH]) |= (cfg->keyConfig[j].codeLL);
                }
              }
            }

            dataForUpload->buttons[3] ^= 0x80; // Lside取反
            dataForUpload->buttons[1] ^= 0x40; // Rside取反

            break;
          default:
            break;
          }
        }
      }

      dataForUpload->analog[0] = (((uint16_t)prevAdc) << 8);

      prevKey   = activeKey;
      adcUpdate = 0;
      Enp1IntIn(); // 发送 HID1 数据包
    }

    CDC_USB_Poll();
    CDC_UART_Poll();
  }
}