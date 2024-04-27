#include "usb_cdc.h"
#include "ch552.h"
#include "rgb.h"
#include "usb.h"
#include <string.h>

#include "pn532.h"
#include "pn532_i2c.h"

// 初始化波特率为115200，1停止位，无校验，8数据位。
// uint8_x __at(LINECODING_ADDR) LineCoding[LINECODING_SIZE];
uint8_x LineCoding[LINECODING_SIZE];

// CDC Tx
uint8_t          CDC1_PutCharBuf[CDC_PUTCHARBUF_LEN]; // The buffer for CDC1_PutChar
volatile uint8_t CDC1_PutCharBuf_Last  = 0;           // Point to the last char in the buffer
volatile uint8_t CDC1_PutCharBuf_First = 0;           // Point to the first char in the buffer
volatile uint8_t CDC1_Tx_Busy          = 0;
volatile uint8_t CDC1_Tx_Full          = 0;

uint8_t          CDC2_PutCharBuf[CDC_PUTCHARBUF_LEN]; // The buffer for CDC2_PutChar
volatile uint8_t CDC2_PutCharBuf_Last  = 0;           // Point to the last char in the buffer
volatile uint8_t CDC2_PutCharBuf_First = 0;           // Point to the first char in the buffer
volatile uint8_t CDC2_Tx_Busy          = 0;
volatile uint8_t CDC2_Tx_Full          = 0;

// CDC Rx
volatile uint8_t CDC1_Rx_Pending = 0; // Number of bytes need to be processed before accepting more USB packets
volatile uint8_t CDC1_Rx_CurPos  = 0;

volatile uint8_t CDC2_Rx_Pending = 0; // Number of bytes need to be processed before accepting more USB packets
volatile uint8_t CDC2_Rx_CurPos  = 0;

// SG COM Response
uint8_x CDC1_RequestPacketPos = 0;
uint8_x CDC1_RequestPacketBuf[256];

uint8_x CDC2_RequestPacketPos = 0;
uint8_x CDC2_RequestPacketBuf[64];

uint8_x CDC_ResponseStringBuf[64]; // The buffer for Response

// CDC configuration
extern uint8_t UsbConfig;

uint8_t  CDC_Data_Callback_State = 0;
uint32_t CDC_Baud       = 0; // The baud rate

void CDC_InitBaud(void)
{
  LineCoding[0] = 0x00;
  LineCoding[1] = 0xC2;
  LineCoding[2] = 0x01;
  LineCoding[3] = 0x00;
  LineCoding[4] = 0x00;
  LineCoding[5] = 0x00;
  LineCoding[6] = 0x08;
}

void CDC_SetBaud(void)
{
  *((uint8_t *)&CDC_Baud)     = LineCoding[0];
  *((uint8_t *)&CDC_Baud + 1) = LineCoding[1];
  *((uint8_t *)&CDC_Baud + 2) = LineCoding[2];
  *((uint8_t *)&CDC_Baud + 3) = LineCoding[3];

  if (CDC_Baud > 999999)
    CDC_Baud = 115200;
}

void USB_EP2_IN_cb(void)
{
  UEP2_T_LEN = 0;
  if (CDC1_Tx_Full)
  {
    // Send a zero-length-packet(ZLP) to end this transfer
    UEP2_CTRL    = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK next IN transfer
    CDC1_Tx_Full = 0;
    // CDC1_Tx_Busy remains set until the next ZLP sent to the host
  }
  else
  {
    UEP2_CTRL    = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
    CDC1_Tx_Busy = 0;
  }
}

void USB_EP2_OUT_cb(void)
{
  CDC1_Rx_Pending = USB_RX_LEN;
  CDC1_Rx_CurPos  = 0; // Reset Rx pointer
  // Reject packets by replying NAK, until uart_poll() finishes its job, then it informs the USB peripheral to accept incoming packets
  UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;
}

void USB_EP3_OUT_cb(void)
{
  CDC2_Rx_Pending = USB_RX_LEN;
  CDC2_Rx_CurPos  = 0; // Reset Rx pointer
  // Reject packets by replying NAK, until uart_poll() finishes its job, then it informs the USB peripheral to accept incoming packets
  UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;
}

void USB_EP3_IN_cb(void)
{
  UEP3_T_LEN = 0; // 预使用发送长度一定要清空
  if (CDC2_Tx_Full)
  {
    // Send a zero-length-packet(ZLP) to end this transfer
    UEP3_CTRL    = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK next IN transfer
    CDC2_Tx_Full = 0;
    // CDC1_Tx_Busy remains set until the next ZLP sent to the host
  }
  else
  {
    UEP3_CTRL    = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
    CDC2_Tx_Busy = 0;
  }
}

void CDC1_PutChar(uint8_t tdata)
{
  // Add new data to CDC1_PutCharBuf
  CDC1_PutCharBuf[CDC1_PutCharBuf_Last++] = tdata;
  if (CDC1_PutCharBuf_Last >= CDC_PUTCHARBUF_LEN)
  {
    // Rotate the tail to the beginning of the buffer
    CDC1_PutCharBuf_Last = 0;
  }

  if (CDC1_PutCharBuf_Last == CDC1_PutCharBuf_First)
  {
    // Buffer is full
    CDC1_Tx_Full = 1;

    while (CDC1_Tx_Full) // Wait until the buffer has vacancy
      CDC_USB_Poll();
  }
}

void CDC1_PutString(char *str)
{
  while (*str)
    CDC1_PutChar(*(str++));
}

void CDC2_PutChar(uint8_t tdata)
{
  // Add new data to CDC2_PutCharBuf
  CDC2_PutCharBuf[CDC2_PutCharBuf_Last++] = tdata;
  if (CDC2_PutCharBuf_Last >= CDC_PUTCHARBUF_LEN)
  {
    // Rotate the tail to the beginning of the buffer
    CDC2_PutCharBuf_Last = 0;
  }

  if (CDC2_PutCharBuf_Last == CDC2_PutCharBuf_First)
  {
    // Buffer is full
    CDC2_Tx_Full = 1;

    while (CDC2_Tx_Full) // Wait until the buffer has vacancy
      CDC_USB_Poll();
  }
}

void CDC2_PutString(char *str)
{
  while (*str)
    CDC2_PutChar(*(str++));
}

// Handles CDC1_PutCharBuf and IN transfer
void CDC_USB_Poll()
{
  static uint8_t usb_frame_count = 0;
  uint8_t        usb_tx_len;

  if (UsbConfig)
  {
    if (usb_frame_count++ > 100)
    {
      usb_frame_count = 0;

      if (!CDC1_Tx_Busy)
      {
        if (CDC1_PutCharBuf_First == CDC1_PutCharBuf_Last)
        {
          if (CDC1_Tx_Full)
          { // Buffer is full
            CDC1_Tx_Busy = 1;

            // length (the first byte to send, the end of the buffer)
            usb_tx_len = CDC_PUTCHARBUF_LEN - CDC1_PutCharBuf_First;
            memcpy(EP2_IN_BUF, &CDC1_PutCharBuf[CDC1_PutCharBuf_First], usb_tx_len);

            // length (the first byte in the buffer, the last byte to send), if any
            if (CDC1_PutCharBuf_Last > 0)
              memcpy(&EP2_IN_BUF[usb_tx_len], CDC1_PutCharBuf, CDC1_PutCharBuf_Last);

            // Send the entire buffer
            UEP2_T_LEN = CDC_PUTCHARBUF_LEN;
            UEP2_CTRL  = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK next IN transfer

            // A 64-byte packet is going to be sent, according to USB specification, USB uses a less-than-max-length packet to demarcate an end-of-transfer
            // As a result, we need to send a zero-length-packet.
            // return;
          }

          // Otherwise buffer is empty, nothing to send
          // return;
        }
        else
        {
          CDC1_Tx_Busy = 1;

          // CDC1_PutChar() is the only way to insert into CDC1_PutCharBuf, it detects buffer overflow and notify the CDC_USB_Poll().
          // So in this condition the buffer can not be full, so we don't have to send a zero-length-packet after this.

          if (CDC1_PutCharBuf_First > CDC1_PutCharBuf_Last)
          { // Rollback
            // length (the first byte to send, the end of the buffer)
            usb_tx_len = CDC_PUTCHARBUF_LEN - CDC1_PutCharBuf_First;
            memcpy(EP2_IN_BUF, &CDC1_PutCharBuf[CDC1_PutCharBuf_First], usb_tx_len);

            // length (the first byte in the buffer, the last byte to send), if any
            if (CDC1_PutCharBuf_Last > 0)
            {
              memcpy(&EP2_IN_BUF[usb_tx_len], CDC1_PutCharBuf, CDC1_PutCharBuf_Last);
              usb_tx_len += CDC1_PutCharBuf_Last;
            }

            UEP2_T_LEN = usb_tx_len;
          }
          else
          {
            usb_tx_len = CDC1_PutCharBuf_Last - CDC1_PutCharBuf_First;
            memcpy(EP2_IN_BUF, &CDC1_PutCharBuf[CDC1_PutCharBuf_First], usb_tx_len);

            UEP2_T_LEN = usb_tx_len;
          }

          CDC1_PutCharBuf_First += usb_tx_len;
          if (CDC1_PutCharBuf_First >= CDC_PUTCHARBUF_LEN)
            CDC1_PutCharBuf_First -= CDC_PUTCHARBUF_LEN;

          // ACK next IN transfer
          UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
        }
      }
      if (!CDC2_Tx_Busy)
      {
        if (CDC2_PutCharBuf_First == CDC2_PutCharBuf_Last)
        {
          if (CDC2_Tx_Full)
          { // Buffer is full
            CDC2_Tx_Busy = 1;

            // length (the first byte to send, the end of the buffer)
            usb_tx_len = CDC_PUTCHARBUF_LEN - CDC2_PutCharBuf_First;
            memcpy(EP3_IN_BUF, &CDC2_PutCharBuf[CDC2_PutCharBuf_First], usb_tx_len);

            // length (the first byte in the buffer, the last byte to send), if any
            if (CDC2_PutCharBuf_Last > 0)
              memcpy(&EP3_IN_BUF[usb_tx_len], CDC2_PutCharBuf, CDC2_PutCharBuf_Last);

            // Send the entire buffer
            UEP3_T_LEN = CDC_PUTCHARBUF_LEN;
            UEP3_CTRL  = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK next IN transfer

            // A 64-byte packet is going to be sent, according to USB specification, USB uses a less-than-max-length packet to demarcate an end-of-transfer
            // As a result, we need to send a zero-length-packet.
            // return;
          }

          // Otherwise buffer is empty, nothing to send
          // return;
        }
        else
        {
          CDC2_Tx_Busy = 1;

          // CDC2_PutChar() is the only way to insert into CDC2_PutCharBuf, it detects buffer overflow and notify the CDC_USB_Poll().
          // So in this condition the buffer can not be full, so we don't have to send a zero-length-packet after this.

          if (CDC2_PutCharBuf_First > CDC2_PutCharBuf_Last)
          { // Rollback
            // length (the first byte to send, the end of the buffer)
            usb_tx_len = CDC_PUTCHARBUF_LEN - CDC2_PutCharBuf_First;
            memcpy(EP3_IN_BUF, &CDC2_PutCharBuf[CDC2_PutCharBuf_First], usb_tx_len);

            // length (the first byte in the buffer, the last byte to send), if any
            if (CDC2_PutCharBuf_Last > 0)
            {
              memcpy(&EP3_IN_BUF[usb_tx_len], CDC2_PutCharBuf, CDC2_PutCharBuf_Last);
              usb_tx_len += CDC2_PutCharBuf_Last;
            }

            UEP3_T_LEN = usb_tx_len;
          }
          else
          {
            usb_tx_len = CDC2_PutCharBuf_Last - CDC2_PutCharBuf_First;
            memcpy(EP3_IN_BUF, &CDC2_PutCharBuf[CDC2_PutCharBuf_First], usb_tx_len);

            UEP3_T_LEN = usb_tx_len;
          }

          CDC2_PutCharBuf_First += usb_tx_len;
          if (CDC2_PutCharBuf_First >= CDC_PUTCHARBUF_LEN)
            CDC2_PutCharBuf_First -= CDC_PUTCHARBUF_LEN;

          // ACK next IN transfer
          UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
        }
      }
    }
  }
}

void ledBoardOnPackect(IO_Packet *reqPacket)
{
  uint8_t    checksum, i; // Response flag, also use for checksum & i
  IO_Packet *resPackect = (IO_Packet *)CDC_ResponseStringBuf;

  memset(CDC_ResponseStringBuf, 0x00, 64); // Clear resPackect

  resPackect->sync      = 0xE0;
  resPackect->srcNodeId = reqPacket->dstNodeId;
  resPackect->dstNodeId = reqPacket->srcNodeId;

  resPackect->response.status  = ACK_OK;
  resPackect->response.report  = REPORT_OK;
  resPackect->response.command = reqPacket->request.command;

  switch (reqPacket->request.command)
  {
  case CMD_RESET:
    // TODO
    // rgbSet(3, 0x00000000);
    // rgbPush();
    resPackect->length = 0;
    break;
  case CMD_SET_TIMEOUT:
    resPackect->response.data[0] = reqPacket->request.data[0];
    resPackect->response.data[1] = reqPacket->request.data[1];
    resPackect->length           = 2;
    break;
  case CMD_SET_DISABLE:
    resPackect->response.data[0] = reqPacket->request.data[0];
    resPackect->length           = 1;
    break;
  case CMD_EXT_BOARD_SET_LED_RGB_DIRECT:
    rgbSet(1, ((uint32_t)(reqPacket->response.data[178]) << 16) | ((uint32_t)(reqPacket->response.data[179]) << 8) | (uint32_t)(reqPacket->response.data[180])); // Right
    rgbSet(0, (uint32_t)(reqPacket->response.data[1]) << 16 | (uint32_t)(reqPacket->response.data[2] << 8) | (uint32_t)(reqPacket->response.data[3]));           // Left
    // rgbPush();
    isLedDataChanged |= 0x02;
    return;
    break;
  case CMD_EXT_BOARD_INFO:
    memcpy(resPackect->response.data, "15093-06", 8);
    resPackect->response.data[8] = 0x0A;
    memcpy(resPackect->response.data + 9, "6710A", 5);
    resPackect->response.data[14] = 0xFF;
    resPackect->response.data[15] = 0xA0; // revision
    resPackect->length            = 0x10;
    break;
  case CMD_EXT_BOARD_STATUS:
    resPackect->response.data[0] = 0x00; // boardFlag
    resPackect->response.data[1] = 0x00; // uartFlag
    resPackect->response.data[2] = 0x00; // cmdFlag
    resPackect->length           = 0x03;
    break;
  case CMD_EXT_FIRM_SUM:
    resPackect->response.data[0] = 0xAA;
    resPackect->response.data[1] = 0x53;
    resPackect->length           = 0x02;
    break;
  case CMD_EXT_PROTOCOL_VERSION:
    resPackect->response.data[0] = 0x01;
    resPackect->response.data[1] = 0x01; // major
    resPackect->response.data[2] = 0x00; // minor
    resPackect->length           = 0x03;
    break;
  default:
    resPackect->response.status = ACK_INVALID;
    resPackect->length          = 0x00;
    break;
  }

  checksum = 0;

  resPackect->length += 3;

  checksum += resPackect->dstNodeId;
  checksum += resPackect->srcNodeId;
  checksum += resPackect->length;
  checksum += resPackect->response.status;
  checksum += resPackect->response.command;
  checksum += resPackect->response.report;

  for (i = 0; i < resPackect->length - 3; i++)
  {
    checksum += resPackect->response.data[i];
  }
  resPackect->response.data[resPackect->length - 3] = checksum;

  CDC1_PutChar(resPackect->sync);
  for (i = 1; i < resPackect->length + 5; i++)
  {
    if (resPackect->buffer[i] == 0xE0 || resPackect->buffer[i] == 0xD0)
    {
      CDC1_PutChar(0xD0);
      CDC1_PutChar(resPackect->buffer[i] - 1);
    }
    else
    {
      CDC1_PutChar(resPackect->buffer[i]);
    }
  }
}

uint8_c HIRATE_FW_VERSION[]  = {'\x94'};
uint8_c LOWRATE_FW_VERSION[] = {'T', 'N', '3', '2', 'M', 'S', 'E', 'C', '0', '0', '3', 'S', ' ', 'F', '/', 'W', ' ', 'V', 'e', 'r', '1', '.', '2'};

void cardReaderOnPackect(AIME_Request *req)
{
  uint8_t        checksum, i; // Response flag, also use for checksum & i
  AIME_Response *res = (AIME_Response *)CDC_ResponseStringBuf;
  static uint8_t AimeKey[6], BanaKey[6];
  uint16_t       SystemCode;

  memset(CDC_ResponseStringBuf, 0x00, 64); // Clear resPackect

  res->addr   = req->addr;
  res->seq_no = req->seq_no;
  res->cmd    = req->cmd;
  res->status = 0;

  switch (req->cmd)
  {
  case CMD_TO_NORMAL_MODE:
    _writeCommand(CDC2_RequestPacketBuf, 0, CDC_ResponseStringBuf, 0);
    if (getFirmwareVersion())
    {
      res->status = 0x03;
    }
    else
    {
      res->status = 0x01;
    }
    res->payload_len = 0;
    break;
  case CMD_GET_FW_VERSION:
    if (CDC_Baud == 115200)
    {
      memcpy(res->version, HIRATE_FW_VERSION, sizeof(HIRATE_FW_VERSION));
      res->payload_len = sizeof(HIRATE_FW_VERSION);
    }
    else
    {
      memcpy(res->version, LOWRATE_FW_VERSION, sizeof(LOWRATE_FW_VERSION));
      res->payload_len = sizeof(LOWRATE_FW_VERSION);
    }
    break;
  case CMD_GET_HW_VERSION:
    if (CDC_Baud == 115200)
    {
      memcpy(res->version, "837-15396", 9);
      res->payload_len = 9;
    }
    else
    {
      memcpy(res->version, "TN32MSEC003S H/W Ver3.0", 23);
      res->payload_len = 23;
    }

    break;
  case CMD_CARD_DETECT:
    // 卡号发送

    if (readPassiveTargetID(PN532_MIFARE_ISO14443A, res->mifare_uid, &res->id_len, 1000, 0))
    {

      res->payload_len = 7;
      res->type        = 0x10;
      res->count       = 1;
      break;
    }
    else if (felica_Polling(0xFFFF, 0x00, res->IDm, res->PMm, &SystemCode, 0x0F) == 1)
    { //< 0: error
      res->payload_len = 0x13;
      res->count       = 1;
      res->type        = 0x20;
      res->id_len      = 0x10;
      break;
    }
    else
    {

      res->payload_len = 1;
      res->count       = 0;
      break;
    }

  case CMD_MIFARE_READ:
    // TODO
    res->status = 0x01;
    break;
  case CMD_FELICA_THROUGH:
    // TODO
    res->status = 0x01;
    break;
  case CMD_MIFARE_AUTHORIZE_B:
    // TODO
    res->status = 0x01;
    break;
  case CMD_MIFARE_AUTHORIZE_A:
    // TODO
    res->status = 0x01;
    break;
  case CMD_CARD_SELECT:
    // TODO
    break;
  case CMD_MIFARE_KEY_SET_B:
    memcpy(AimeKey, req->key, 6);
    break;
  case CMD_MIFARE_KEY_SET_A:
    memcpy(BanaKey, req->key, 6);
    break;
  case CMD_START_POLLING:
    // TODO
    break;
  case CMD_STOP_POLLING:
    // TODO
    break;
  case CMD_EXT_TO_NORMAL_MODE:
    // TODO
    break;
  case CMD_EXT_BOARD_INFO:
    if (CDC_Baud == 115200)
    {
      memcpy(res->info_payload, "000-00000\xFF\x11\x40", 12);
      res->payload_len = 12;
    }
    else
    {
      memcpy(res->info_payload, "15084\xFF\x10\x00\x12", 9);
      res->payload_len = 9;
    }
    break;
  case CMD_EXT_BOARD_SET_LED_RGB:
    // TODO
    // rgbSet(3, ((uint32_t)(req->color_payload[0]) << 16) | ((uint32_t)(req->color_payload[1]) << 8) | ((uint32_t)(req->color_payload[2])));
    return;
    break;
  default:
    break;
  }

  res->frame_len = 6 + res->payload_len;
  checksum       = 0;
  for (i = 0; i < res->frame_len; i++)
  {
    checksum += res->buffer[i];
  }
  res->buffer[res->frame_len] = checksum;

  CDC2_PutChar(0xE0);
  for (i = 0; i <= res->frame_len; i++)
  {
    if (res->buffer[i] == 0xE0 || res->buffer[i] == 0xD0)
    {
      CDC2_PutChar(0xD0);
      CDC2_PutChar(res->buffer[i] - 1);
    }
    else
    {
      CDC2_PutChar(res->buffer[i]);
    }
  }
}

void CDC_UART_Poll()
{
  uint8_t        cur_byte;
  static uint8_t checksum = 0, cdc1_prev_byte = 0x00;
  static uint8_t cdc2_prev_byte = 0x00;
  IO_Packet     *reqPackect     = (IO_Packet *)CDC1_RequestPacketBuf;
  AIME_Request  *reqPackect2    = (AIME_Request *)CDC2_RequestPacketBuf;

  // If there are data pending
  while (CDC1_Rx_Pending)
  {
    cur_byte = EP2_OUT_BUF[CDC1_Rx_CurPos];
    if (cur_byte == 0xE0 & cdc1_prev_byte != 0xD0)
    {
      checksum              = 0x20;
      CDC1_RequestPacketPos = 0;
      reqPackect->length    = 0xFF;
    }
    else if (cdc1_prev_byte == 0xD0)
    {
      cur_byte += 1;
    }
    else if (cur_byte == 0xD0)
    {

      CDC1_Rx_Pending--;
      CDC1_Rx_CurPos++;
      if (CDC1_Rx_Pending == 0)
      {
        UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
      }
      continue;
      ;
    }

    CDC1_RequestPacketBuf[CDC1_RequestPacketPos] = cur_byte;
    CDC1_RequestPacketPos++;
    if (CDC1_RequestPacketPos > 5 && CDC1_RequestPacketPos - 5 == reqPackect->length)
    {
      if (cur_byte == checksum)
      {
        CDC_Data_Callback_State &= 0x01;
      }
      else
      {
        // Checksum Failed
        CDC1_RequestPacketPos = 0x00;
        checksum              = 0x00;
      }
    }
    else
    {
    }
    checksum += cur_byte;

    CDC1_Rx_Pending--;
    CDC1_Rx_CurPos++;
    if (CDC1_Rx_Pending == 0)
    {
      UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
    }
    cdc1_prev_byte = cur_byte;
  }

  // If there are data pending
  while (CDC2_Rx_Pending)
  {
    cur_byte = EP3_OUT_BUF[CDC2_Rx_CurPos];
    if (cur_byte == 0xE0 & cdc2_prev_byte != 0xD0)
    {
      checksum               = 0x20;
      CDC2_RequestPacketPos  = 0;
      reqPackect2->frame_len = 0xFF;
    }
    else if (cdc2_prev_byte == 0xD0)
    {
      cur_byte += 1;
    }
    else if (cur_byte == 0xD0)
    {

      CDC2_Rx_Pending--;
      CDC2_Rx_CurPos++;
      if (CDC2_Rx_Pending == 0)
      {
        UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
      }
      continue;
      ;
    }

    CDC2_RequestPacketBuf[CDC2_RequestPacketPos] = cur_byte;
    CDC2_RequestPacketPos++;
    if (CDC2_RequestPacketPos > 6 && CDC2_RequestPacketPos - 2 == reqPackect2->frame_len)
    {
      if (cur_byte == checksum)
      {
        CDC_Data_Callback_State &= 0x02;
      }
      else
      {
        // Checksum Failed
        CDC2_RequestPacketPos = 0x00;
        checksum              = 0x00;
      }
    }
    else
    {
    }
    checksum += cur_byte;

    CDC2_Rx_Pending--;
    CDC2_Rx_CurPos++;
    if (CDC2_Rx_Pending == 0)
    {
      UEP3_CTRL = UEP3_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
    }
    cdc2_prev_byte = cur_byte;
  }
}

void CDC_data_check(){
  if(CDC_Data_Callback_State & 0x01){
     ledBoardOnPackect((IO_Packet *)CDC1_RequestPacketBuf);
    CDC_Data_Callback_State &= ~0x01;
  }
  if(CDC_Data_Callback_State & 0x02){
    cardReaderOnPackect((AIME_Request *)CDC2_RequestPacketBuf);
    CDC_Data_Callback_State &= ~0x02;
  }
}