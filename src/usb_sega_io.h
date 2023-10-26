#ifndef __USB_SEGA_IO_H
#define __USB_SEGA_IO_H

#include "ch552.h"

typedef enum
{
  NORMAL     = 0x00,
  JAM        = 0x01,
  DISCONNECT = 0x02,
  BUSY       = 0x03,
} CoinCondition;

typedef enum
{
  SET_COMM_TIMEOUT   = 0x01,
  SET_SAMPLING_COUNT = 0x02,
  CLEAR_BOARD_STATUS = 0x03,
  SET_GENERAL_OUTPUT = 0x04,
  SET_PWM_OUTPUT     = 0x05,
  UPDATE_FIRMWARE    = 0x85,
} IO4_COMMAND;

typedef struct
{
  CoinCondition condition;
  uint8_t       count;
} CoinData;

typedef struct
{
  uint16_t  analog[8];
  uint16_t  rotary[4];
  CoinData coin[2];
  uint8_t  buttons[4];
  uint8_t  systemStatus;
  uint8_t  usbStatus;
  uint8_t  _unused[29];
} DataUpload;

typedef struct
{
  IO4_COMMAND command;
  uint8_t     payload[62];
} DataReceive;

void io4Init(void);

extern uint8_c bitPosMap[0x12];

extern DataReceive *dataReceive;
extern DataUpload  *dataForUpload;

extern uint8_t Ep1RequestReplay;

void USB_EP1_OUT_cb(void);

#endif