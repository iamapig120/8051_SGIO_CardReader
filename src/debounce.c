#include "debounce.h"

#include "sys.h"
#include <string.h>

#ifdef TOUCH_COUNT
#include "touchkey.h"
#endif

static __data debounce_type debBuffer[DEBOUNCE_SIZE]; // 去抖缓冲
static __data uint16_t      debResult = 0;            // 按键状态
#define __set_pin(k) BT##k = 1;
#define __ch(k) CH##k;
#define __debounce_pin(k)                                        \
  debBuffer[k - 1] <<= 1;                                        \
  debBuffer[k - 1] |= BT##k;                                     \
  if (debBuffer[k - 1] == 0xFE || debBuffer[k - 1] == 0x00)      \
  {                                                              \
    debResult |= 1 << (k - 1);                                   \
  }                                                              \
  else if (debBuffer[k - 1] == 0x01 || debBuffer[k - 1] == 0xFF) \
  {                                                              \
    debResult &= ~(1 << (k - 1));                                \
  }
#define __debounce_key_without_check(k)                  \
  if (debBuffer[k] == 0xFE || debBuffer[k] == 0x00)      \
  {                                                      \
    debResult |= 1 << (k);                               \
  }                                                      \
  else if (debBuffer[k] == 0x01 || debBuffer[k] == 0xFF) \
  {                                                      \
    debResult &= ~(1 << (k));                            \
  }
#define __debounce_touchkey(k)                                   \
  debBuffer[k - 1] <<= 1;                                        \
  debBuffer[k - 1] |= (Touch_State >> (k + 1)) & 1;              \
  if (debBuffer[k - 1] == 0xFE || debBuffer[k - 1] == 0x00)      \
  {                                                              \
    debResult |= 1 << (k - 1);                                   \
  }                                                              \
  else if (debBuffer[k - 1] == 0x01 || debBuffer[k - 1] == 0xFF) \
  {                                                              \
    debResult &= ~(1 << (k - 1));                                \
  }

// uint8_t i = 0;

/*
 * 更新计算去抖
 */
void debounceUpdate()
{
#ifndef TOUCH_COUNT
#if defined(SIM_KEY)
  __debounce_pin(1);
#else
#if (defined(SIMPAD_V2_AE) || defined(SIMPAD_V2))
  __debounce_pin(1);
  __debounce_pin(2);
  __debounce_pin(3);
  __debounce_pin(4);
  __debounce_pin(5);
#elif (defined(SIMPAD_NANO_AE) || defined(SIMPAD_NANO))
  __debounce_pin(1);
  __debounce_pin(2);
  __debounce_pin(3);
#elif defined(SEGA_IO_BOARD)
  // ROW1 = 1;
  // delay_us(40);
  // debBuffer[0] <<= 1;
  // debBuffer[0] |= (COL1 == 0)?1:0;
  // __debounce_key_without_check(0);
  // debBuffer[1] <<= 1;
  // debBuffer[1] |= (COL2 == 0)?1:0;
  // __debounce_key_without_check(1);
  // debBuffer[2] <<= 1;
  // debBuffer[2] |= (COL3 == 0)?1:0;
  // __debounce_key_without_check(2);
  // debBuffer[3] <<= 1;
  // debBuffer[3] |= (COL4 == 0)?1:0;
  // __debounce_key_without_check(3);
  // ROW1 = 0;

  // ROW2 = 1;
  // delay_us(40);
  // debBuffer[4] <<= 1;
  // debBuffer[4] |= (COL1 == 0)?1:0;
  // __debounce_key_without_check(4);
  // debBuffer[5] <<= 1;
  // debBuffer[5] |= (COL2 == 0)?1:0;
  // __debounce_key_without_check(5);
  // debBuffer[6] <<= 1;
  // debBuffer[6] |= (COL3 == 0)?1:0;
  // __debounce_key_without_check(6);
  // debBuffer[7] <<= 1;
  // debBuffer[7] |= (COL4 == 0)?1:0;
  // __debounce_key_without_check(7);
  // ROW2 = 0;

  // ROW3 = 1;
  // delay_us(40);
  // debBuffer[8] <<= 1;
  // debBuffer[8] |= (COL1 == 0)?1:0;
  // __debounce_key_without_check(8);
  // debBuffer[9] <<= 1;
  // debBuffer[9] |= (COL2 == 0)?1:0;
  // __debounce_key_without_check(9);
  // debBuffer[10] <<= 1;
  // debBuffer[10] |= (COL3 == 0)?1:0;
  // __debounce_key_without_check(10);
  // debBuffer[11] <<= 1;
  // debBuffer[11] |= (COL4 == 0)?1:0;
  // __debounce_key_without_check(11);
  // ROW3 = 0;
  if (COL2 == 1 && COL3 == 1 && COL4 == 1)
  {
    COL1 = 0;
    debBuffer[0] <<= 1;
    debBuffer[0] |= COL2;
    __debounce_key_without_check(0);
    debBuffer[4] <<= 1;
    debBuffer[4] |= COL3;
    __debounce_key_without_check(4);
    debBuffer[8] <<= 1;
    debBuffer[8] |= COL4;
    __debounce_key_without_check(8);
    COL1 = 1;
  }
  if (COL2 == 1 && COL3 == 1 && COL4 == 1)
  {
    COL2 = 0;
    debBuffer[5] <<= 1;
    debBuffer[5] |= COL3;
    __debounce_key_without_check(5);
    debBuffer[9] <<= 1;
    debBuffer[9] |= COL4;
    __debounce_key_without_check(9);
    COL2 = 1;
  }
  if (COL2 == 1 && COL3 == 1 && COL4 == 1)
  {
    COL3 = 0;
    debBuffer[2] <<= 1;
    debBuffer[2] |= COL2;
    __debounce_key_without_check(2);
    debBuffer[10] <<= 1;
    debBuffer[10] |= COL4;
    __debounce_key_without_check(10);
    COL3 = 1;
  }
  if (COL2 == 1 && COL3 == 1 && COL4 == 1)
  {
    COL4 = 0;
    debBuffer[3] <<= 1;
    debBuffer[3] |= COL2;
    __debounce_key_without_check(3);
    debBuffer[7] <<= 1;
    debBuffer[7] |= COL3;
    __debounce_key_without_check(7);
    COL4 = 1;
  }
  debBuffer[1] <<= 1;
  debBuffer[1] |= COL2;
  __debounce_key_without_check(1);
  debBuffer[6] <<= 1;
  debBuffer[6] |= COL3;
  __debounce_key_without_check(6);
  debBuffer[11] <<= 1;
  debBuffer[11] |= COL4;
  __debounce_key_without_check(11);

#endif
#endif
#elif defined SIMPAD_TOUCH
  __debounce_touchkey(1);
  __debounce_touchkey(2);
  __debounce_touchkey(3);
  __debounce_touchkey(4);
#endif
}

__bit isKeyActive(uint8_t i)
{
  return (debResult >> i) & 0x01;
}

void debounceInit()
{
#ifndef SIMPAD_TOUCH
#if defined(SIM_KEY)
  __set_pin(1);
#else
#if (defined(SIMPAD_V2_AE) || defined(SIMPAD_V2))
  __set_pin(1);
  __set_pin(2);
  __set_pin(3);
  __set_pin(4);
  __set_pin(5);
#elif (defined(SIMPAD_NANO_AE) || defined(SIMPAD_NANO))
  __set_pin(1);
  __set_pin(2);
  __set_pin(3);
#elif (defined(SIMPAD_TOUCH))
  __set_pin(1);
  __set_pin(2);
  __set_pin(3);
  __set_pin(4);
#elif defined(SEGA_IO_BOARD)
  // ROW1 = 0;
  // ROW2 = 0;
  // ROW3 = 0;

  // P3_MOD_OC &= 0xF4;
  // P3_DIR_PU &= 0xF4;

  // P1_MOD_OC &= 0xCC;
  // P1_DIR_PU &= 0xCC;

  // P3_MOD_OC &= 0x2F;
  // P3_DIR_PU &= 0x2F;

  // P1_MOD_OC &= 0x10;

  // COL1 = 0;
  // COL2 = 0;
  // COL3 = 0;
  // COL4 = 0;

  COL1 = 1;
  COL2 = 1;
  COL3 = 1;
  COL4 = 1;
  // Init CMD
#endif
#endif
#endif
  memset(debBuffer, 0xFF, DEBOUNCE_SIZE);
  // memset(debResult, 0x00, DEBOUNCE_SIZE);
}
