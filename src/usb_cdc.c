#include "usb_cdc.h"
#include "ch552.h"

//初始化波特率为57600，1停止位，无校验，8数据位。
uint8_x __at(LINECODING_ADDR) LineCoding[LINECODING_SIZE];

// CDC Tx
__idata uint8_t          CDC_PutCharBuf[CDC_PUTCHARBUF_LEN]; // The buffer for CDC_PutChar
__idata volatile uint8_t CDC_PutCharBuf_Last  = 0;           // Point to the last char in the buffer
__idata volatile uint8_t CDC_PutCharBuf_First = 0;           // Point to the first char in the buffer
__idata volatile uint8_t CDC_Tx_Busy          = 0;
__idata volatile uint8_t CDC_Tx_Full          = 0;

// CDC Rx
__idata volatile uint8_t CDC_Rx_Pending = 0; // Number of bytes need to be processed before accepting more USB packets
__idata volatile uint8_t CDC_Rx_CurPos  = 0;

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