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

#ifdef MOTOR
#include "motor.h"
#endif

#ifdef TOUCH_COUNT
#include "touchkey.h"
#endif

void __usbDeviceInterrupt() __interrupt(INT_NO_USB) __using(1); // USB中断定义
#ifdef TOUCH_COUNT
void __TK_int_ISR() __interrupt(INT_NO_TKEY) __using(1); // TouchKey中断定义
#endif

void __tim2Interrupt() __interrupt(INT_NO_TMR2) __using(2);

extern uint8_t sysMsCounter;

#if KEY_COUNT <= 8
uint8_t prevKey = 0; // 上一次扫描时的按键激活状态记录
uint8_t activeKey;   // 最近一次扫描时的按键激活状态记录
#elif KEY_COUNT <= 16
uint16_t prevKey = 0; // 上一次扫描时的按键激活状态记录
uint16_t activeKey;   // 最近一次扫描时的按键激活状态记录
#endif

/** @brief 游戏手柄报表，16个按键，16比特 */
uint8_t controllerKeyH = 0, controllerKeyL = 0;

void main()
{
  uint8_t  i, j;
  uint16_t timer = 0;
  uint8_t *buffer;

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

  CDC_InitBaud();
  usbDevInit();
  debounceInit();
  rgbInit();

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
        timer = 0x0400;
        Enp1IntIn();
        if ((timer & 0x00FF) == 0x0000)
        {
          rgbPush();
        }
      }
      timer--;
    }

    activeKey = 0;
    for (i = 0; i < KEY_COUNT; i++)
    {
      activeKey <<= 1;
      activeKey |= isKeyActive(i);
    }
    if (prevKey != activeKey)
    {
      prevKey ^= activeKey;
      for (i = 0; i < KEY_COUNT; i++)
      {
        // 如果第i个键是被更改的
        if ((prevKey >> i) & 0x01)
        {
#ifdef MOTOR
          // 如果第i个键是被更改的，并且这个操作是激活
          if ((activeKey >> i) & 0x01)
          {
            activeMotor(5000);
          }
#endif
          // usbReleaseAll();
          switch (cfg->keyConfig[i].mode)
          {
          // 手柄按钮 及 手柄摇杆
          case GamepadButton:
            break;
          case GamepadButton_Indexed:

            buffer    = GetEndpointInBuffer(1);
            buffer[0] = 1;

            dataForUpload = (DataUpload *)(&(buffer[1]));
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

            Enp1IntIn(); // 发送 HID1 数据包
            break;
          default:
            break;
          }
        }
      }
      prevKey = activeKey;
    }

    if (Ep1RequestReplay)
    {
      Ep1RequestReplay = 0;
      Enp1IntIn();
    }
  }
}