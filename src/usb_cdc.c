#include "usb.h"
#include "usb_cdc.h"
#include "ch552.h"
#include <string.h>

// 初始化波特率为57600，1停止位，无校验，8数据位。
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

void CDC_PutChar(uint8_t tdata) {
	// Add new data to CDC_PutCharBuf
	CDC_PutCharBuf[CDC_PutCharBuf_Last++] = tdata;
	if(CDC_PutCharBuf_Last >= CDC_PUTCHARBUF_LEN) {
		// Rotate the tail to the beginning of the buffer
		CDC_PutCharBuf_Last = 0;
	}

	if (CDC_PutCharBuf_Last == CDC_PutCharBuf_First) {
		// Buffer is full
		CDC_Tx_Full = 1;

		while(CDC_Tx_Full)	// Wait until the buffer has vacancy
			CDC_USB_Poll();
	}
}

void CDC_PutString(char *str) {
	while(*str)
		CDC_PutChar(*(str++));
}

// Handles CDC_PutCharBuf and IN transfer
void CDC_USB_Poll() {
	static uint8_t usb_frame_count = 0;
	uint8_t usb_tx_len;

	if(UsbConfig) {
		if(usb_frame_count++ > 100) {
			usb_frame_count = 0;

			if(!CDC_Tx_Busy) {
				if(CDC_PutCharBuf_First == CDC_PutCharBuf_Last) {
					if (CDC_Tx_Full) { // Buffer is full
						CDC_Tx_Busy = 1;

						// length (the first byte to send, the end of the buffer)
						usb_tx_len = CDC_PUTCHARBUF_LEN - CDC_PutCharBuf_First;
						memcpy(EP2_IN_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

						// length (the first byte in the buffer, the last byte to send), if any
						if (CDC_PutCharBuf_Last > 0)
							memcpy(&EP2_IN_BUF[usb_tx_len], CDC_PutCharBuf, CDC_PutCharBuf_Last);

						// Send the entire buffer
						UEP2_T_LEN = CDC_PUTCHARBUF_LEN;
						UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;	// ACK next IN transfer

						// A 64-byte packet is going to be sent, according to USB specification, USB uses a less-than-max-length packet to demarcate an end-of-transfer
						// As a result, we need to send a zero-length-packet.
						return;
					}

					// Otherwise buffer is empty, nothing to send
					return;
				} else {
					CDC_Tx_Busy = 1;

					// CDC_PutChar() is the only way to insert into CDC_PutCharBuf, it detects buffer overflow and notify the CDC_USB_Poll().
					// So in this condition the buffer can not be full, so we don't have to send a zero-length-packet after this.

					if(CDC_PutCharBuf_First > CDC_PutCharBuf_Last) { // Rollback
						// length (the first byte to send, the end of the buffer)
						usb_tx_len = CDC_PUTCHARBUF_LEN - CDC_PutCharBuf_First;
						memcpy(EP2_IN_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

						// length (the first byte in the buffer, the last byte to send), if any
						if (CDC_PutCharBuf_Last > 0) {
							memcpy(&EP2_IN_BUF[usb_tx_len], CDC_PutCharBuf, CDC_PutCharBuf_Last);
							usb_tx_len += CDC_PutCharBuf_Last;
						}

						UEP2_T_LEN = usb_tx_len;
					} else {
						usb_tx_len = CDC_PutCharBuf_Last - CDC_PutCharBuf_First;
						memcpy(EP2_IN_BUF, &CDC_PutCharBuf[CDC_PutCharBuf_First], usb_tx_len);

						UEP2_T_LEN = usb_tx_len;
					}

					CDC_PutCharBuf_First += usb_tx_len;
					if(CDC_PutCharBuf_First>=CDC_PUTCHARBUF_LEN)
						CDC_PutCharBuf_First -= CDC_PUTCHARBUF_LEN;

					// ACK next IN transfer
					UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;
				}
			}
		}

	}
}
