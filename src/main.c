#include "adc.h"
#include "bsp.h"
#include "ch422.h"
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

extern uint8_t sysMsCounter;

#if KEY_COUNT <= 8
uint8_t prevKey = 0; // 上一次扫描时的按键激活状态记录
uint8_t activeKey;   // 最近一次扫描时的按键激活状态记录
#elif KEY_COUNT <= 16
uint16_t prevKey = 0; // 上一次扫描时的按键激活状态记录
uint16_t activeKey;   // 最近一次扫描时的按键激活状态记录
#endif

uint16_t prevAdc = 0x8000;

uint16_t timer = 0;

extern uint8_t UsbConfig;

void main()
{
  uint8_t i, j;

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

  sysLoadConfig();
  SysConfig *cfg = sysGetConfig();
  // SysConfig *cfg = (&sysConfig);

  // CDC_InitBaud();

  debounceInit();
  rgbInit();
  ch422Active();

  // ADCInit(1); // 慢采样
  // ADC_ChannelSelect(1);

  EA = 1; // 启用中断

  delay_ms(10);

  usbDevInit();
  usbReleaseAll();
  io4Init();

  sysTickConfig();

  // rgbSet(1, 0x00FFFFFF);
  // rgbSet(0, 0x00FFFFFF);
  // isLedDataChanged |= 0x02;

  // ADC_START = 1;

  sysMsCounter = 0;

  while (1)
  {
    while (sysMsCounter--)
    {
      debounceUpdate(); // 按键和去抖检测

      // 灯光检测
      // IIC 通讯灯光，主按键灯光
      if (isLedDataChanged & 0x01)
      {
        ch422Fresh();
        isLedDataChanged &= ~0x01;
      }

      if (timer == 0)
      {
        timer = 0x0600; // 大约1.5s一个HID数据包
        // isLedDataChanged |= 0x03; // 定期更新LED灯
        Enp1IntIn();

        // 强制刷新灯光
        ch422Fresh();
        rgbPush();
      }
      // 每秒更新8次灯光
      if (timer & 0x7F)
      {
        // 2812 灯光，侧键灯光
        if (isLedDataChanged & 0x02)
        {
          rgbPush();
          isLedDataChanged &= ~0x02;
        }
      }
      timer--;
    }

    activeKey = 0;

    if (prevAdc > adc_var)
    {
      if (prevAdc - adc_var > 0x40)
      {
        prevAdc                  = adc_var;
        dataForUpload->analog[0] = prevAdc;
        dataForUpload->analog[1] = prevAdc;
        dataForUpload->analog[2] = prevAdc;
        dataForUpload->analog[3] = prevAdc;
        dataForUpload->analog[4] = prevAdc;
        dataForUpload->analog[5] = prevAdc;
        dataForUpload->analog[6] = prevAdc;
        dataForUpload->analog[7] = prevAdc;
        Enp1IntIn();
      }
    }
    else
    {
      if (adc_var - prevAdc > 0x40)
      {
        prevAdc                  = adc_var;
        dataForUpload->analog[0] = prevAdc;
        dataForUpload->analog[1] = prevAdc;
        dataForUpload->analog[2] = prevAdc;
        dataForUpload->analog[3] = prevAdc;
        dataForUpload->analog[4] = prevAdc;
        dataForUpload->analog[5] = prevAdc;
        dataForUpload->analog[6] = prevAdc;
        dataForUpload->analog[7] = prevAdc;
        Enp1IntIn();
      }
    }

    for (i = 0; i < KEY_COUNT; i++)
    {
      // activeKey <<= 1;
      activeKey |= (uint16_t)isKeyActive(i) << i;
    }
    if (prevKey != activeKey)
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

            // dataForUpload->buttons[3] ^= 0x80; // Lside取反
            // dataForUpload->buttons[1] ^= 0x40; // Rside取反

            dataForUpload->buttons[3] |= 0x80; // Lside block
            dataForUpload->buttons[1] |= 0x40; // Rside block

            break;
          default:
            break;
          }
        }
      }

      prevKey = activeKey;

      Enp1IntIn(); // 发送 HID1 数据包
    }

    CDC_USB_Poll();
    CDC_UART_Poll();
    CDC_data_check();
    // ADC 重采样
    if (ADC_START == 0)
    {
      ADC_update();
      ADC_START = 1;
    }
  }
}