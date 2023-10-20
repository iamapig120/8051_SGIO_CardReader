#ifndef __COMIO_H
#define __COMIO_H

#include "ch552.h"

typedef enum
{
  ACK_OK                   = 0x01,
  ACK_SUM_ERROR            = 0x02,
  ACK_PARITY_ERROR         = 0x03,
  ACK_FARMING_ERROR        = 0x04,
  ACK_OVER_RUN_ERROR       = 0x05,
  ACK_RECV_BUFFER_OVERFLOW = 0x06,
  ACK_INVALID              = 0xFF,
} IO_ACK_STATUS;

typedef enum
{
  REPORT_OK = 1,
  REPORT_BUSY,
  REPORT_UNKNOWN_COMMAND,
  REPORT_PARAM_ERROR,
  REPORT_INVALID = 255,
} IO_REPORT_STATUS;

typedef enum
{
  // Basic
  CMD_RESET       = 0x10,
  CMD_SET_TIMEOUT = 0x11,
  CMD_SET_DISABLE = 0x14,
  // Versions
  CMD_GET_FW_VERSION = 0x30,
  CMD_GET_HW_VERSION = 0x32,
  // Card read
  CMD_START_POLLING = 0x40,
  CMD_STOP_POLLING  = 0x41,
  CMD_CARD_DETECT   = 0x42,
  CMD_CARD_SELECT   = 0x43,
  CMD_CARD_HALT     = 0x44,
  // MIFARE
  CMD_MIFARE_KEY_SET_A   = 0x50,
  CMD_MIFARE_AUTHORIZE_A = 0x51,
  CMD_MIFARE_READ        = 0x52,
  CMD_MIFARE_WRITE       = 0x53,
  CMD_MIFARE_KEY_SET_B   = 0x54,
  CMD_MIFARE_AUTHORIZE_B = 0x55,
  // Boot,update
  CMD_TO_UPDATER_MODE   = 0x60,
  CMD_SEND_HEX_DATA     = 0x61,
  CMD_TO_NORMAL_MODE    = 0x62,
  CMD_SEND_BINDATA_INIT = 0x63,
  CMD_SEND_BINDATA_EXEC = 0x64,
  // FeliCa
  CMD_FELICA_PUSH                    = 0x70,
  CMD_FELICA_THROUGH                 = 0x71,
  CMD_FELICA_THROUGH_POLL            = 0x00,
  CMD_FELICA_THROUGH_READ            = 0x06,
  CMD_FELICA_THROUGH_WRITE           = 0x08,
  CMD_FELICA_THROUGH_GET_SYSTEM_CODE = 0x0C,
  CMD_FELICA_THROUGH_NDA_A4          = 0xA4,
  // LED board
  CMD_EXT_BOARD_SET_LED            = 0x80,
  CMD_EXT_BOARD_SET_LED_RGB        = 0x81,
  CMD_EXT_BOARD_SET_LED_RGB_DIRECT = 0x82,
  // Extra
  CMD_EXT_BOARD_INFO       = 0xf0,
  CMD_EXT_BOARD_STATUS     = 0xf1,
  CMD_EXT_FIRM_SUM         = 0xf2,
  CMD_EXT_PROTOCOL_VERSION = 0xf3,
} IO_COMMAND;

typedef struct
{
  IO_COMMAND command;
  uint8_t    data[];
  // last byte: uint8_t checksum;
} IO_Request;

typedef struct
{
  IO_ACK_STATUS    status;
  IO_COMMAND       command;
  IO_REPORT_STATUS report;
  uint8_t          data[];
  // last byte: uint8_t checksum;
} IO_Response;

typedef union
{
  uint8_t buffer[];
  struct
  {
    uint8_t sync;
    uint8_t dstNodeId;
    uint8_t srcNodeId;
    uint8_t length;
    union
    {
      IO_Request  request;
      IO_Response response;
    };
  };
} IO_Packet;

#endif