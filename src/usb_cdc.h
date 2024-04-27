#ifndef __USB_CDC_H
#define __USB_CDC_H

#include "ch552.h"
#include "comio.h"

// CDC bRequests:
// bmRequestType = 0xA1
#define SERIAL_STATE 0x20
#define GET_LINE_CODING 0X21 // This request allows the host to find out the currently configured line coding.
// bmRequestType = 21
#define SET_LINE_CODING 0X20        // Configures DTE rate, stop-bits, parity, and number-of-character
#define SET_CONTROL_LINE_STATE 0X22 // This request generates RS-232/V.24 style control signals.
#define SEND_BREAK 0x23

#define CDC_FLAG_NOSTOP 0x80

// #define LINECODING_ADDR 0x01C0
#define LINECODING_SIZE 7

// 初始化波特率为57600，1停止位，无校验，8数据位。
// extern __xdata  __at(LINECODING_ADDR)
extern uint8_x LineCoding[];

// CDC configuration
extern uint32_t CDC_Baud;

#define CDC_PUTCHARBUF_LEN 32

void CDC_InitBaud(void);
void CDC_SetBaud(void);
void CDC_USB_Poll(void);
void CDC_UART_Poll(void);

void USB_EP2_IN_cb(void);
void USB_EP2_OUT_cb(void);

void USB_EP3_IN_cb(void);
void USB_EP3_OUT_cb(void);

#endif
