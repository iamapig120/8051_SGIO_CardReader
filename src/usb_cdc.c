#include "usb_cdc.h"
#include "ch552.h"
#include "rgb.h"
#include "usb.h"
#include <string.h>

// 初始化波特率为57600，1停止位，无校验，8数据位。
uint8_x __at(LINECODING_ADDR) LineCoding[LINECODING_SIZE];

// CDC Tx
uint8_x                  CDC_ResponseStringBuf[64];          // The buffer for CDC_PutChar
__idata uint8_t          CDC_PutCharBuf[CDC_PUTCHARBUF_LEN]; // The buffer for CDC_PutChar
__idata volatile uint8_t CDC_PutCharBuf_Last  = 0;           // Point to the last char in the buffer
__idata volatile uint8_t CDC_PutCharBuf_First = 0;           // Point to the first char in the buffer
__idata volatile uint8_t CDC_Tx_Busy          = 0;
__idata volatile uint8_t CDC_Tx_Full          = 0;

// CDC Rx
uint8_x                  CDC_RequestPacketBuf[256];
__idata volatile uint8_t CDC_RequestPacketPos = 0;
__idata volatile uint8_t CDC_Rx_Pending       = 0; // Number of bytes need to be processed before accepting more USB packets
__idata volatile uint8_t CDC_Rx_CurPos        = 0;

// CDC configuration
extern uint8_t UsbConfig;
uint32_t       CDC_Baud = 0; // The baud rate

void CDC_InitBaud(void)
{
  LineCoding[0] = 0x00;
  LineCoding[1] = 0xE1;
  LineCoding[2] = 0x00;
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
    CDC_Baud = 57600;
}

void USB_EP2_IN_cb(void)
{
  UEP2_T_LEN = 0;
  if (CDC_Tx_Full)
  {
    // Send a zero-length-packet(ZLP) to end this transfer
    UEP2_CTRL   = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK next IN transfer
    CDC_Tx_Full = 0;
    // CDC_Tx_Busy remains set until the next ZLP sent to the host
  }
  else
  {
    UEP2_CTRL   = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
    CDC_Tx_Busy = 0;
  }
}

void USB_EP2_OUT_cb(void)
{
  CDC_Rx_Pending = USB_RX_LEN;
  CDC_Rx_CurPos  = 0; // Reset Rx pointer
  // Reject packets by replying NAK, until uart_poll() finishes its job, then it informs the USB peripheral to accept incoming packets
  UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;
}

void USB_EP4_IN_cb(void)
{
  UEP4_T_LEN = 0; // 预使用发送长度一定要清空
  if (CDC_Tx_Full)
  {
    // Send a zero-length-packet(ZLP) to end this transfer
    UEP4_CTRL   = UEP4_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK next IN transfer
    CDC_Tx_Full = 0;
    // CDC_Tx_Busy remains set until the next ZLP sent to the host
  }
  else
  {
    UEP4_CTRL   = UEP4_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
    CDC_Tx_Busy = 0;
  }
}

void CDC_PutChar(uint8_t tdata)
{
  // Add new data to CDC_PutCharBuf
  CDC_PutCharBuf[CDC_PutCharBuf_Last++] = tdata;
  if (CDC_PutCharBuf_Last >= CDC_PUTCHARBUF_LEN)
  {
    // Rotate the tail to the beginning of the buffer
    CDC_PutCharBuf_Last = 0;
  }

  if (CDC_PutCharBuf_Last == CDC_PutCharBuf_First)
  {
    // Buffer is full
    CDC_Tx_Full = 1;

    while (CDC_Tx_Full) // Wait until the buffer has vacancy
      CDC_USB_Poll();
  }
}

void CDC_PutString(char *str)
{
  while (*str)
    CDC_PutChar(*(str++));
}

// Handles CDC_PutCharBuf and IN transfer
void CDC_USB_Poll()
{
  static uint8_t usb_frame_count = 0;
  uint8_t        usb_tx_len;

  if (UsbConfig)
  {
    if (usb_frame_count++ > 100)
    {
      usb_frame_count = 0;

      if (!CDC_Tx_Busy)
      {
        if (CDC_PutCharBuf_First == CDC_PutCharBuf_Last)
        {
          if (CDC_Tx_Full)
          { // Buffer is full
            CDC_Tx_Busy = 1;

            // length (the first byte to send, the end of the buffer)
            usb_tx_len = CDC_PUTCHARBUF_LEN - CDC_PutCharBuf_First;
            memcpy(EP2_IN_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

            // length (the first byte in the buffer, the last byte to send), if any
            if (CDC_PutCharBuf_Last > 0)
              memcpy(&EP2_IN_BUF[usb_tx_len], CDC_PutCharBuf, CDC_PutCharBuf_Last);

            // Send the entire buffer
            UEP2_T_LEN = CDC_PUTCHARBUF_LEN;
            UEP2_CTRL  = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // ACK next IN transfer

            // A 64-byte packet is going to be sent, according to USB specification, USB uses a less-than-max-length packet to demarcate an end-of-transfer
            // As a result, we need to send a zero-length-packet.
            return;
          }

          // Otherwise buffer is empty, nothing to send
          return;
        }
        else
        {
          CDC_Tx_Busy = 1;

          // CDC_PutChar() is the only way to insert into CDC_PutCharBuf, it detects buffer overflow and notify the CDC_USB_Poll().
          // So in this condition the buffer can not be full, so we don't have to send a zero-length-packet after this.

          if (CDC_PutCharBuf_First > CDC_PutCharBuf_Last)
          { // Rollback
            // length (the first byte to send, the end of the buffer)
            usb_tx_len = CDC_PUTCHARBUF_LEN - CDC_PutCharBuf_First;
            memcpy(EP2_IN_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

            // length (the first byte in the buffer, the last byte to send), if any
            if (CDC_PutCharBuf_Last > 0)
            {
              memcpy(&EP2_IN_BUF[usb_tx_len], CDC_PutCharBuf, CDC_PutCharBuf_Last);
              usb_tx_len += CDC_PutCharBuf_Last;
            }

            UEP2_T_LEN = usb_tx_len;
          }
          else
          {
            usb_tx_len = CDC_PutCharBuf_Last - CDC_PutCharBuf_First;
            memcpy(EP2_IN_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

            UEP2_T_LEN = usb_tx_len;
          }

          CDC_PutCharBuf_First += usb_tx_len;
          if (CDC_PutCharBuf_First >= CDC_PUTCHARBUF_LEN)
            CDC_PutCharBuf_First -= CDC_PUTCHARBUF_LEN;

          // ACK next IN transfer
          UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
        }
      }
    }
  }
}

void ledBoardOnPackect(IO_Packet *reqPacket)
{
  uint8_t    checkSum, i; // Response flag, also use for checksum & i
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
    // rgbSet(3, 0x00000000);
    rgbPush();
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
    rgbSet(3, ((uint32_t)(reqPacket->response.data[178]) << 16) | ((uint32_t)(reqPacket->response.data[179]) << 8) | (uint32_t)(reqPacket->response.data[180])); // Right
    // rgbSet(3, (uint32_t)(reqPacket->response.data[1]) << 16 | (uint32_t)(reqPacket->response.data[2] << 8) | (reqPacket->response.data[3]));       // Left
    rgbPush();
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

  checkSum = 0;

  resPackect->length += 3;

  checkSum += resPackect->dstNodeId;
  checkSum += resPackect->srcNodeId;
  checkSum += resPackect->length;
  checkSum += resPackect->response.status;
  checkSum += resPackect->response.command;
  checkSum += resPackect->response.report;

  for (i = 0; i < resPackect->length - 3; i++)
  {
    checkSum += resPackect->response.data[i];
  }
  resPackect->response.data[resPackect->length - 3] = checkSum;

  CDC_PutChar(resPackect->sync);
  for (i = 1; i < resPackect->length + 5; i++)
  {
    if (resPackect->buffer[i] == 0xE0 || resPackect->buffer[i] == 0xD0)
    {
      CDC_PutChar(0xD0);
      CDC_PutChar(resPackect->buffer[i] - 1);
    }
    else
    {
      CDC_PutChar(resPackect->buffer[i]);
    }
  }
}

void CDC_UART_Poll()
{
  uint8_t        cur_byte;
  static uint8_t checksum = 0, prev_byte = 0x00;
  IO_Packet     *reqPackect = (IO_Packet *)CDC_RequestPacketBuf;

  // If there are data pending
  while (CDC_Rx_Pending)
  {
    cur_byte = EP2_OUT_BUF[CDC_Rx_CurPos];
    if (cur_byte == 0xE0 & prev_byte != 0xD0)
    {
      checksum             = 0x20;
      CDC_RequestPacketPos = 0;
    }
    else if (prev_byte == 0xD0)
    {
      cur_byte += 1;
    }
    else if (cur_byte == 0xD0)
    {

      CDC_Rx_Pending--;
      CDC_Rx_CurPos++;
      if (CDC_Rx_Pending == 0)
      {
        UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
      }
      return;
    }

    CDC_RequestPacketBuf[CDC_RequestPacketPos] = cur_byte;
    CDC_RequestPacketPos++;
    if (CDC_RequestPacketPos > 5 && CDC_RequestPacketPos - 5 == reqPackect->length && cur_byte == checksum)
    {
      ledBoardOnPackect(reqPackect);
    }
    checksum += cur_byte;

    CDC_Rx_Pending--;
    CDC_Rx_CurPos++;
    if (CDC_Rx_Pending == 0)
    {
      UEP2_CTRL = UEP2_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
    }

    prev_byte = cur_byte;
  }
}