#include "usb.h"
#include "usb_cdc.h"
#include "usb_sega_io.h"

#include "rgb.h"

#include "ch552.h"
#include "sys.h"
#include <stdlib.h>
#include <string.h>

// Used in SET_FEATURE and CLEAR_FEATURE
#define USB_FEATURE_ENDPOINT_HALT 0        // Endpoint only
#define USB_FEATURE_DEVICE_REMOTE_WAKEUP 1 // Device only

// DMA缓冲区必须对齐到偶地址，xRAM自动分配地址往后移动
uint8_x __at(0x0000) Ep0Buffer[ENDP0_BUFFER_SIZE];                                                                                        // 端点0 OUT&IN缓冲区
uint8_x __at(ENDP0_BUFFER_SIZE) Ep4BufferOut[ENDP4_BUFFER_SIZE];                                                                          // 端点4 OUT缓冲区
uint8_x __at(ENDP0_BUFFER_SIZE + ENDP4_BUFFER_SIZE) Ep4BufferIn[ENDP4_BUFFER_SIZE];                                                       // 端点4 IN缓冲区
uint8_x __at(ENDP0_BUFFER_SIZE + 2 * ENDP4_BUFFER_SIZE) Ep1Buffer[2 * ENDP1_BUFFER_SIZE];                                                 // 端点1 OUT&IN缓冲区
uint8_x __at(ENDP0_BUFFER_SIZE + 2 * ENDP4_BUFFER_SIZE + 2 * ENDP1_BUFFER_SIZE) Ep2Buffer[2 * ENDP2_BUFFER_SIZE];                         // 端点2 OUT&IN缓冲区
uint8_x __at(ENDP0_BUFFER_SIZE + 2 * ENDP4_BUFFER_SIZE + 2 * ENDP1_BUFFER_SIZE + 2 * ENDP2_BUFFER_SIZE) Ep3Buffer[2 * ENDP3_BUFFER_SIZE]; // 端点3 OUT&IN缓冲区
// 自动分配地址从0x0240开始，需修改make文件

const uint8_c usbDevDesc[] = {
    0x12,       // 描述符长度(18字节)
    0x01,       // 描述符类型
    0x10, 0x01, // 本设备所用USB版本(1.1)
    // 0x00, 0x02,              // 本设备所用USB版本(2.0)
    0xEF,                         // 类代码
    0x02,                         // 子类代码
    0x01,                         // 设备所用协议
    ENDP0_BUFFER_SIZE,            // 端点0最大包长
    VENDOR_ID_L, VENDOR_ID_H,     // 厂商ID
    PRODUCT_ID_L, PRODUCT_ID_H,   // 产品ID
    PRODUCT_BCD_L, PRODUCT_BCD_H, // 设备版本号 (2.04)
    0x01,                         // 描述厂商信息的字符串描述符的索引值
    0x02,                         // 描述产品信息的字串描述符的索引值
    0x03,                         // 描述设备序列号信息的字串描述符的索引值
    0x01                          // 可能的配置数
};

const uint8_c SegaRepDesc[] = {
    0x05, 0x01,                   // Usage Page (Generic Desktop Ctrls)
    0x09, 0x04,                   // Usage (Joystick)
    0xA1, 0x01,                   // Collection (Application)
    0x85, 0x01,                   //   Report ID (1)
    0x09, 0x01,                   //   Usage (Pointer)
    0xA1, 0x00,                   //   Collection (Physical)
    0x09, 0x30,                   //     Usage (X)
    0x09, 0x31,                   //     Usage (Y)
    0x09, 0x30,                   //     Usage (X)
    0x09, 0x31,                   //     Usage (Y)
    0x09, 0x30,                   //     Usage (X)
    0x09, 0x31,                   //     Usage (Y)
    0x09, 0x30,                   //     Usage (X)
    0x09, 0x31,                   //     Usage (Y)
    0x09, 0x33,                   //     Usage (Rx)
    0x09, 0x34,                   //     Usage (Ry)
    0x09, 0x33,                   //     Usage (Rx)
    0x09, 0x34,                   //     Usage (Ry)
    0x09, 0x36,                   //     Usage (Slider)
    0x09, 0x36,                   //     Usage (Slider)
    0x15, 0x00,                   //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //     Logical Maximum (65534)
    0x35, 0x00,                   //     Physical Minimum (0)
    0x47, 0xFF, 0xFF, 0x00, 0x00, //     Physical Maximum (65534)
    0x95, 0x0E,                   //     Report Count (14)
    0x75, 0x10,                   //     Report Size (16)
    0x81, 0x02,                   //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,                         //   End Collection
    0x05, 0x02,                   //   Usage Page (Sim Ctrls)
    0x05, 0x09,                   //   Usage Page (Button)
    0x19, 0x01,                   //   Usage Minimum (0x01)
    0x29, 0x30,                   //   Usage Maximum (0x30)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x01,                   //   Logical Maximum (1)
    0x45, 0x01,                   //   Physical Maximum (1)
    0x75, 0x01,                   //   Report Size (1)
    0x95, 0x30,                   //   Report Count (48)
    0x81, 0x02,                   //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x00,                   //   Usage (0x00)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x1D,                   //   Report Count (29)
    0x81, 0x01,                   //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0xA0, 0xFF,             //   Usage Page (Vendor Defined 0xFFA0)
    0x09, 0x00,                   //   Usage (0x00)
    0x85, 0x10,                   //   Report ID (16)
    0xA1, 0x01,                   //   Collection (Application)
    0x09, 0x00,                   //     Usage (0x00)
    0x15, 0x00,                   //     Logical Minimum (0)
    0x26, 0xFF, 0x00,             //     Logical Maximum (255)
    0x75, 0x08,                   //     Report Size (8)
    0x95, 0x3F,                   //     Report Count (63)
    0x91, 0x02,                   //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                         //   End Collection
    0xC0,                         // End Collection
};

const uint8_c CustomRepDesc[] = {
    0x06, 0x00, 0xFF,            // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,                  // Usage (0x01)
    0xA1, 0x01,                  // Collection (Application)
    0x85, 0xAA,                  //   Report ID (170)
    0x95, ENDP3_BUFFER_SIZE - 1, //   Report Count (64)
    0x75, 0x08,                  //   Report Size (8)
    0x25, 0x01,                  //   Logical Maximum (1)
    0x15, 0x00,                  //   Logical Minimum (0)
    0x09, 0x01,                  //   Usage (0x01)
    0x81, 0x02,                  //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x55,                  //   Report ID (85)
    0x95, ENDP3_BUFFER_SIZE - 1, //   Report Count (64)
    0x75, 0x08,                  //   Report Size (8)
    0x25, 0x01,                  //   Logical Maximum (1)
    0x15, 0x00,                  //   Logical Minimum (0)
    0x09, 0x01,                  //   Usage (0x01)
    0x91, 0x02,                  //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,                        // End Collection
};

const uint8_c usbCfgDesc[] = {
    0x09,                 //   bLength
    USB_DESCR_TYP_CONFIG, //   bDescriptorType (Configuration)
    0xCD, 0x00,           //   wTotalLength 198
    0x06,                 //   bNumInterfaces 6 (6接口 1_SGIO 2_HID通信 34_CDC 56_CDC)
    0x01,                 //   bConfigurationValue
    0x04,                 //   iConfiguration (String Index)
    0x80,                 //   Attributes, D7 must be 1, D6 Self-powered, D5 Remote Wakeup, D4-D0=0
    0x32,                 //   bMaxPower 100mA

    /* -------------------------------- */
    //  SEGA_IO

    0x09,                 //   bLength
    USB_DESCR_TYP_INTERF, //   bDescriptorType (Interface)
    0x00,                 //   bInterfaceNumber 0
    0x00,                 //   bAlternateSetting
    0x02,                 //   bNumEndpoints 1
    0x03,                 //   bInterfaceClass
    0x00,                 //   bInterfaceSubClass
    0x00,                 //   bInterfaceProtocol
    0x06,                 //   iInterface (String Index)

    0x09,                      //   bLength
    USB_DESCR_TYP_HID,         //   bDescriptorType (HID)
    0x11, 0x01,                //   bcdHID 1.11
    0x00,                      //   bCountryCode
    0x01,                      //   bNumDescriptors
    USB_DESCR_TYP_REPORT,      //   bDescriptorType[0] (HID)
    sizeof(SegaRepDesc), 0x00, //   wDescriptorLength[0] 113

    0x07,                    //   bLength
    USB_DESCR_TYP_ENDP,      //   bDescriptorType (Endpoint)
    0x81,                    //   bEndpointAddress (IN/D2H)
    0x03,                    //   bmAttributes (Interrupt)
    ENDP1_BUFFER_SIZE, 0x00, //   wMaxPacketSize 64
    0x01,                    //   bInterval 5 (unit depends on device speed)

    0x07,                    // bLength
    USB_DESCR_TYP_ENDP,      // bDescriptorType (Endpoint)
    0x01,                    // bEndpointAddress (OUT/H2D)
    0x03,                    // bmAttributes (Interrupt)
    ENDP1_BUFFER_SIZE, 0x00, // wMaxPacketSize 64
    0x05,                    // bInterval 5 (unit depends on device speed)

    /* -------------------------------- */
    //  Custom HID
    // Interface Association Descriptor 接口关联描述符
    // 8,                 // Length of the descriptor 描述符长度
    // USB_DESCR_TYP_IAD, // Type: Interface Association Descriptor (IAD) 描述符类型：接口关联描述符
    // 0x02,              // First interface: 2 in this case, see following 第一个要关联的接口ID
    // 0x01,              // Total number of grouped interfaces 总共要关联的接口数量
    // 0x03,              // bFunctionClass
    // 0x00,              // bFunctionSubClass
    // 0x00,              // bFunctionProtocol
    // 0x00,              // Index of string descriptor describing this function 字符串描述符索引

    0x09,                 //   bLength
    USB_DESCR_TYP_INTERF, //   bDescriptorType (Interface)
    0x01,                 //   bInterfaceNumber 1
    0x00,                 //   bAlternateSetting
    0x02,                 //   bNumEndpoints 2
    0x03,                 //   bInterfaceClass
    0x00,                 //   bInterfaceSubClass
    0x00,                 //   bInterfaceProtocol
    0x00,                 //   iInterface (String Index)

    0x09,                        //   bLength
    USB_DESCR_TYP_HID,           //   bDescriptorType (HID)
    0x11, 0x01,                  //   bcdHID 1.10
    0x00,                        //   bCountryCode
    0x01,                        //   bNumDescriptors
    USB_DESCR_TYP_REPORT,        //   bDescriptorType[0] (HID)
    sizeof(CustomRepDesc), 0x00, //   wDescriptorLength[0] 36

    0x07,                    // bLength
    USB_DESCR_TYP_ENDP,      // bDescriptorType (Endpoint)
    0x83,                    // bEndpointAddress (IN/D2H)
    0x03,                    // bmAttributes (Interrupt)
    ENDP2_BUFFER_SIZE, 0x00, // wMaxPacketSize 64
    0x32,                    // bInterval 0 (unit depends on device speed)

    0x07,                    // bLength
    USB_DESCR_TYP_ENDP,      // bDescriptorType (Endpoint)
    0x03,                    // bEndpointAddress (OUT/H2D)
    0x03,                    // bmAttributes (Interrupt)
    ENDP2_BUFFER_SIZE, 0x00, // wMaxPacketSize 64
    0x32,                    // bInterval 0 (unit depends on device speed)

    /* -------------------------------- */
    //  Custom CDC
    // Interface Association Descriptor (CDC) 接口关联描述符
    8,                 // Length of the descriptor 描述符长度
    USB_DESCR_TYP_IAD, // Type: Interface Association Descriptor (IAD) 描述符类型：接口关联描述符
    0x02,              // First interface: 2 in this case, see following 第一个要关联的接口ID
    0x02,              // Total number of grouped interfaces 总共要关联的接口数量
    0x02,              // bFunctionClass
    0x02,              // bFunctionSubClass
    0x00,              // bFunctionProtocol
    0x07,              // Index of string descriptor describing this function 字符串描述符索引

    // Interface descriptor (CDC) 接口描述符
    9,                    // Length of the descriptor 描述符长度
    USB_DESCR_TYP_INTERF, // Type: Interface Descriptor 描述符类型：接口描述符
    0x02,                 // Interface ID 接口ID
    0x00,                 // Alternate setting 备用设置
    0x01,                 // Number of Endpoints 使用端点数量
    0x02,                 // Interface class code - Communication Interface Class 接口类别码：Communications and CDC Control
    0x02,                 // Subclass code - Abstract Control Model 抽象控制模型
    0x01,                 // Protocol code - AT Command V.250 protocol 通用AT命令协议
    0x00,                 // Index of corresponding string descriptor (On Windows, it is called "Bus reported device description") 字符串描述符索引

    // Header Functional descriptor (CDC)
    5,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE 描述符类性：类特殊接口，CS_INTERFACE
    0x00,                  // bDescriptorsubtype, HEADER
    0x20, 0x01,            // bcdCDC

    // Call Management Functional Descriptor (CDC)
    5,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE
    0x01,                  // bDescriptorsubtype, Call Management Functional Descriptor
    0x00,                  // bmCapabilities 设备不自己处理调用管理
    // Bit0: Does not handle call management
    // Bit1: Call Management over Comm Class interface
    0x03, // Data Interfaces 数据所用接口

    // Abstract Control Management (CDC)
    4,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE
    0x02,                  // bDescriptorsubtype, Abstract Control Management
    0x02,                  // bmCapabilities
    // Bit0: CommFeature
    // Bit1: LineStateCoding  Device supports the request combination of
    // Set_Line_Coding,	Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State.
    // Bit2: SendBreak
    // Bit3: NetworkConnection
    // 支持 Set_Line_Coding、Set_Control_Line_State、Get_Line_Coding、Serial_State

    // Union Functional Descriptor (CDC)
    5,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE
    0x06,                  // bDescriptorSubtype: Union func desc
    0x02,                  // bMasterInterface: Communication class interface
    0x03,                  // bSlaveInterface: Data Class Interface

    // EndPoint descriptor (CDC Upload, Interrupt)
    7,                  // Length of the descriptor
    USB_DESCR_TYP_ENDP, // Type: Endpoint Descriptor
    0x85,               // Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number（不存在的端点）
    0x03,               // Attributes:
                        // D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
                        // 			The following only apply to isochronous endpoints. Else set to 0.
                        // D3:2 Synchronization Type: 00 = No Synchronization 01 = Asynchronous 10 = Adaptive 11 = Synchronous
                        // D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
                        // D7:6 = 0
    0x08, 0x00,         // Maximum packet size can be handled
    0x40,               // Interval for polling, in units of 1 ms for low/full speed

    // Interface descriptor (CDC)
    9,                    // Length of the descriptor
    USB_DESCR_TYP_INTERF, // Type: Interface Descriptor
    0x03,                 // Interface ID
    0x00,                 // Alternate setting
    0x02,                 // Number of Endpoints
    0x0A,                 // Interface class code - CDC Data
    0x00,                 // Subclass code
    0x00,                 // Protocol code
    0x00,                 // Index of corresponding string descriptor (On Windows, it is called "Bus reported device description")

    // EndPoint descriptor (CDC Upload, Bulk)
    7,                       // Length of the descriptor
    USB_DESCR_TYP_ENDP,      // Type: Endpoint Descriptor
    0x02,                    // Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number
    0x02,                    // Attributes:
                             // D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
                             // 			The following only apply to isochronous endpoints. Else set to 0.
                             // D3:2 Synchronization Type: 00 = No Synchronization 01 = Asynchronous 10 = Adaptive 11 = Synchronous
                             // D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
                             // D7:6 = 0
    ENDP3_BUFFER_SIZE, 0x00, // Maximum packet size can be handled
    0x00,                    // Interval for polling, in units of 1 ms for low/full speed

    // EndPoint descriptor (CDC Upload, Bulk)
    7,                       // Length of the descriptor
    USB_DESCR_TYP_ENDP,      // Type: Endpoint Descriptor
    0x82,                    // Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number
    0x02,                    // Attributes:
                             // D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
                             // 			The following only apply to isochronous endpoints. Else set to 0.
                             // D3:2 Synchronization Type: 00 = No Synchronization 01 = Asynchronous 10 = Adaptive 11 = Synchronous
                             // D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
                             // D7:6 = 0
    ENDP3_BUFFER_SIZE, 0x00, // Maximum packet size can be handled
    0x00,                    // Interval for polling, in units of 1 ms for low/full speed

    /* -------------------------------- */
    //  Custom CDC
    // Interface Association Descriptor (CDC) 接口关联描述符
    8,                 // Length of the descriptor 描述符长度
    USB_DESCR_TYP_IAD, // Type: Interface Association Descriptor (IAD) 描述符类型：接口关联描述符
    0x04,              // First interface: 4 in this case, see following 第一个要关联的接口ID
    0x02,              // Total number of grouped interfaces 总共要关联的接口数量
    0x02,              // bFunctionClass
    0x02,              // bFunctionSubClass
    0x00,              // bFunctionProtocol
    0x08,              // Index of string descriptor describing this function 字符串描述符索引

    // Interface descriptor (CDC) 接口描述符
    9,                    // Length of the descriptor 描述符长度
    USB_DESCR_TYP_INTERF, // Type: Interface Descriptor 描述符类型：接口描述符
    0x04,                 // Interface ID 接口ID
    0x00,                 // Alternate setting 备用设置
    0x01,                 // Number of Endpoints 使用端点数量
    0x02,                 // Interface class code - Communication Interface Class 接口类别码：Communications and CDC Control
    0x02,                 // Subclass code - Abstract Control Model 抽象控制模型
    0x01,                 // Protocol code - AT Command V.250 protocol 通用AT命令协议
    0x00,                 // Index of corresponding string descriptor (On Windows, it is called "Bus reported device description") 字符串描述符索引

    // Header Functional descriptor (CDC)
    5,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE 描述符类性：类特殊接口，CS_INTERFACE
    0x00,                  // bDescriptorsubtype, HEADER
    0x20, 0x01,            // bcdCDC

    // Call Management Functional Descriptor (CDC)
    5,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE
    0x01,                  // bDescriptorsubtype, Call Management Functional Descriptor
    0x00,                  // bmCapabilities 设备不自己处理调用管理
    // Bit0: Does not handle call management
    // Bit1: Call Management over Comm Class interface
    0x05, // Data Interfaces 数据所用接口

    // Abstract Control Management (CDC)
    4,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE
    0x02,                  // bDescriptorsubtype, Abstract Control Management
    0x02,                  // bmCapabilities
    // Bit0: CommFeature
    // Bit1: LineStateCoding  Device supports the request combination of
    // Set_Line_Coding,	Set_Control_Line_State, Get_Line_Coding, and the notification Serial_State.
    // Bit2: SendBreak
    // Bit3: NetworkConnection
    // 支持 Set_Line_Coding、Set_Control_Line_State、Get_Line_Coding、Serial_State

    // Union Functional Descriptor (CDC)
    5,                     // Length of the descriptor
    USB_DESCR_TYP_CS_INTF, // bDescriptortype, CS_INTERFACE
    0x06,                  // bDescriptorSubtype: Union func desc
    0x04,                  // bMasterInterface: Communication class interface
    0x05,                  // bSlaveInterface0: Data Class Interface

    // EndPoint descriptor (CDC Upload, Interrupt)
    7,                  // Length of the descriptor
    USB_DESCR_TYP_ENDP, // Type: Endpoint Descriptor
    0x86,               // Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number（不存在的端点）
    0x03,               // Attributes:
                        // D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
                        // 			The following only apply to isochronous endpoints. Else set to 0.
                        // D3:2 Synchronization Type: 00 = No Synchronization 01 = Asynchronous 10 = Adaptive 11 = Synchronous
                        // D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
                        // D7:6 = 0
    0x08, 0x00,         // Maximum packet size can be handled
    0x40,               // Interval for polling, in units of 1 ms for low/full speed

    // Interface descriptor (CDC)
    9,                    // Length of the descriptor
    USB_DESCR_TYP_INTERF, // Type: Interface Descriptor
    0x05,                 // Interface ID
    0x00,                 // Alternate setting
    0x02,                 // Number of Endpoints
    0x0A,                 // Interface class code - CDC Data
    0x00,                 // Subclass code
    0x00,                 // Protocol code
    0x00,                 // Index of corresponding string descriptor (On Windows, it is called "Bus reported device description")

    // EndPoint descriptor (CDC Upload, Bulk)
    7,                       // Length of the descriptor
    USB_DESCR_TYP_ENDP,      // Type: Endpoint Descriptor
    0x04,                    // Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number
    0x02,                    // Attributes:
                             // D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
                             // 			The following only apply to isochronous endpoints. Else set to 0.
                             // D3:2 Synchronization Type: 00 = No Synchronization 01 = Asynchronous 10 = Adaptive 11 = Synchronous
                             // D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
                             // D7:6 = 0
    ENDP4_BUFFER_SIZE, 0x00, // Maximum packet size can be handled
    0x00,                    // Interval for polling, in units of 1 ms for low/full speed

    // EndPoint descriptor (CDC Upload, Bulk)
    7,                       // Length of the descriptor
    USB_DESCR_TYP_ENDP,      // Type: Endpoint Descriptor
    0x84,                    // Endpoint: D7: 0-Out 1-In, D6-D4=0, D3-D0 Endpoint number
    0x02,                    // Attributes:
                             // D1:0 Transfer type: 00 = Control 01 = Isochronous 10 = Bulk 11 = Interrupt
                             // 			The following only apply to isochronous endpoints. Else set to 0.
                             // D3:2 Synchronization Type: 00 = No Synchronization 01 = Asynchronous 10 = Adaptive 11 = Synchronous
                             // D5:4	Usage Type: 00 = Data endpoint 01 = Feedback endpoint 10 = Implicit feedback Data endpoint 11 = Reserved
                             // D7:6 = 0
    ENDP4_BUFFER_SIZE, 0x00, // Maximum packet size can be handled
    0x00                     // Interval for polling, in units of 1 ms for low/full speed
};

const uint8_c usbLangDesc[] = {0x04, USB_DESCR_TYP_STRING, 0x09, 0x04};
// const uint8_c usbManuDesc[] = {0x14, USB_DESCR_TYP_STRING, 'S', 0, 'i', 0, 'm', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0};
const uint8_c usbManuDesc[] = {0x0A, USB_DESCR_TYP_STRING, 'S', 0, 'E', 0, 'G', 0, 'A', 0};
#if defined(SIMPAD_V2_AE)
const uint8_c usbProdDesc[] = {0x12, USB_DESCR_TYP_STRING, 'S', 0, 'i', 0, 'm', 0, 'P', 0, 'a', 0, 'd', 0, ' ', 0, '2', 0};
#elif defined(SIMPAD_NANO_AE)
const uint8_c usbProdDesc[] = {0x19, USB_DESCR_TYP_STRING, 'S', 0, 'i', 0, 'm', 0, 'P', 0, 'a', 0, 'd', 0, ' ', 0, 'N', 0, 'a', 0, 'n', 0, 'o', 0};
#elif defined(SIMPAD_TOUCH)
const uint8_c usbProdDesc[] = {0x1A, USB_DESCR_TYP_STRING, 'S', 0, 'i', 0, 'm', 0, 'P', 0, 'a', 0, 'd', 0, ' ', 0, 'T', 0, 'o', 0, 'u', 0, 'c', 0, 'h', 0};
#elif defined(SIM_KEY)
const uint8_c usbProdDesc[] = {0x0E, USB_DESCR_TYP_STRING, 'S', 0, 'i', 0, 'm', 0, 'K', 0, 'e', 0, 'y', 0};
#elif defined(SIMPAD_V2)
const uint8_c usbProdDesc[] = {0x12, USB_DESCR_TYP_STRING, 'S', 0, 'i', 0, 'm', 0, 'P', 0, 'a', 0, 'd', 0, ' ', 0, '2', 0};
#elif defined(SIMPAD_NANO)
const uint8_c usbProdDesc[] = {0x18, USB_DESCR_TYP_STRING, 'S', 0, 'i', 0, 'm', 0, 'P', 0, 'a', 0, 'd', 0, ' ', 0, 'N', 0, 'a', 0, 'n', 0, 'o', 0};
#endif

#if defined(SEGA_IO_BOARD)
const uint8_c usbProdDesc[] = {0x16, USB_DESCR_TYP_STRING, 'O', 0, 'N', 0, 'G', 0, 'E', 0, 'E', 0, 'K', 0, 'i', 0, ' ', 0, 'I', 0, 'O', 0};
#endif

uint8_x usbSerialDesc[] = {
    0x12,
    USB_DESCR_TYP_STRING,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
};

#ifdef USE_EXT_STR
const uint8_c usbCfgStrDesc[] = {0x08, USB_DESCR_TYP_STRING, 'U', 0, 'S', 0, 'B', 0};
const uint8_c SgioStrDesc[]   = {0xBC, USB_DESCR_TYP_STRING, 'I', 0, '/', 0, 'O', 0, ' ', 0, 'C', 0, 'O', 0, 'N', 0, 'T', 0, 'R', 0, 'O', 0, 'L', 0, ' ', 0, 'B', 0, 'D', 0, ';', 0, '1', 0, '5', 0, '2', 0, '5', 0, '7', 0, ';', 0, '0', 0, '1', 0, ';', 0, '9', 0, '0', 0, ';', 0, '1', 0, '8', 0, '3', 0, '1', 0, ';', 0, '6', 0, '6', 0, '7', 0, '9', 0, 'A', 0, ';', 0, '0', 0, '0', 0, ';', 0, 'G', 0, 'O', 0, 'U', 0, 'T', 0, '=', 0, '1', 0, '4', 0, '_', 0, 'A', 0, 'D', 0, 'I', 0, 'N', 0, '=', 0, '8', 0, ',', 0, 'E', 0, '_', 0, 'R', 0, 'O', 0, 'T', 0, 'I', 0, 'N', 0, '=', 0, '4', 0, '_', 0, 'C', 0, 'O', 0, 'I', 0, 'N', 0, 'I', 0, 'N', 0, '=', 0, '2', 0, '_', 0, 'S', 0, 'W', 0, 'I', 0, 'N', 0, '=', 0, '2', 0, ',', 0, 'E', 0, '_', 0, 'U', 0, 'Q', 0, '1', 0, '=', 0, '4', 0, '1', 0, ',', 0, '6', 0, ';', 0};
// const uint8_c usbMseStrDesc[] = {0x08, USB_DESCR_TYP_STRING, 'M', 0, 's', 0, 'e', 0};
const uint8_c Com1StrDesc[] = {0x22, USB_DESCR_TYP_STRING, 'C', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'R', 0, 'e', 0, 'a', 0, 'd', 0, 'e', 0, 'r', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0, '3', 0};
const uint8_c Com2StrDesc[] = {0x1E, USB_DESCR_TYP_STRING, 'L', 0, 'E', 0, 'D', 0, ' ', 0, 'B', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, ' ', 0, 'C', 0, 'O', 0, 'M', 0, '3', 0};
#endif

uint8_x HIDMouse[4] = {0}; // 鼠标数据
/*
    byte 0: HID 数据包模式 ID
    byte 1:
        media key
        control key
        bit 0-7: lCtrl, lShift, lAlt, lGUI, rCtrl, rShift, rAlt, rGUI
    byte 2:
        media key
    byte 3-9: standard key
*/
uint8_x HIDKey[10] = {0}; // 键盘数据
// uint8_x        HIDInput[ENDP3_BUFFER_SIZE]  = {0};              // 自定义HID接收缓冲
// uint8_x        HIDOutput[ENDP3_BUFFER_SIZE] = {0};              // 自定义HID发送缓冲

uint16_i SetupLen;            // 包长度
uint8_i  SetupReq, UsbConfig; // FLAG,
uint8_t *pDescr;              // USB配置标志

uint32_t ledData;

uint8_t Ep1RequestReplay = 0;

volatile __bit HIDIN = 0;
#define UsbSetupBuf ((PUSB_SETUP_REQ)Ep0Buffer)

void __usbDeviceInterrupt() __interrupt(INT_NO_USB) __using(1)
{
  ET2 = 0;
  uint8_t len;
  if (UIF_TRANSFER)
  { // USB传输完成
    switch (USB_INT_ST & (MASK_UIS_TOKEN | MASK_UIS_ENDP))
    {                       // 分析操作令牌和端点号
    case UIS_TOKEN_OUT | 4: // endpoint 4# 中断端点下传
      if (U_TOG_OK)
      {
        len = USB_RX_LEN;
      }
      UEP4_CTRL ^= bUEP_R_TOG; // 手动翻转
      break;
    case UIS_TOKEN_IN | 4: // endpoint 4# 中断端点上传
      if (U_TOG_OK)
      {
        USB_EP4_IN_cb();
      }
      UEP4_CTRL ^= bUEP_T_TOG; // 手动翻转
      break;
    case UIS_TOKEN_OUT | 3: // endpoint 3# 中断端点下传
      if (U_TOG_OK)
      { // 不同步的数据包将丢弃
        len = USB_RX_LEN;
        // if (Ep3Buffer[0] == 0x55 && (len - 1) <= sizeof(HIDInput) && !HIDIN)
        // {
        //   memset(HIDInput, 0x00, sizeof(HIDInput));
        //   memcpy(HIDInput, Ep3Buffer + 1, len - 1);
        //   HIDIN = 1;
        // }
      }
      break;
    case UIS_TOKEN_IN | 3:                                      // endpoint 3# 中断端点上传
      UEP3_T_LEN = 0;                                           // 预使用发送长度一定要清空
      UEP3_CTRL  = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; // 默认应答NAK
      break;
    case UIS_TOKEN_OUT | 2: // endpoint 2# 中断端点下传
      if (U_TOG_OK)
      {
        len = USB_RX_LEN;
      }
      break;
    case UIS_TOKEN_IN | 2:                                      // endpoint 2# 中断端点上传
      UEP2_T_LEN = 0;                                           // 预使用发送长度一定要清空
      UEP2_CTRL  = UEP2_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; // 默认应答NAK
      break;
    case UIS_TOKEN_OUT | 1: // endpoint 1# 中断端点下传
      if (U_TOG_OK)
      {
        // len = USB_RX_LEN;
        UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_NAK;

        if (Ep1Buffer[0] == 0x10)
        {
          dataReceive   = (DataReceive *)(&(Ep1Buffer[1]));
          dataForUpload = (DataUpload *)(&(Ep1Buffer[65]));
          switch (dataReceive->command)
          {
          case SET_COMM_TIMEOUT:
            dataForUpload->systemStatus = 0x30;
            break;
          case SET_SAMPLING_COUNT:
            dataForUpload->systemStatus = 0x30;
            break;
          case CLEAR_BOARD_STATUS:
            dataForUpload->systemStatus = 0x00;

            dataForUpload->coin[0].count     = 0;
            dataForUpload->coin[0].condition = NORMAL;
            dataForUpload->coin[1].count     = 0;
            dataForUpload->coin[1].condition = NORMAL;

            break;
          case SET_GENERAL_OUTPUT:
            ledData = (uint32_t)(dataReceive->payload[0]) << 16 | (uint32_t)(dataReceive->payload[1]) << 8 | dataReceive->payload[2];
            for (len = 0; len < 3; len++)
            {
              // Left1, Left2, Left3, Right1, Right2, Right3
              rgbSet(len,
                  (((ledData >> bitPosMap[9 + len * 3]) & 1) ? 0xFF0000 : 0x00000000) |
                      (((ledData >> bitPosMap[9 + len * 3 + 1]) & 1) ? 0x00FF00 : 0x00000000) |
                      (((ledData >> bitPosMap[9 + len * 3 + 2]) & 1) ? 0x000000FF : 0x00000000)); // r
              // rgbSet(len + 3,
              //     (((ledData >> bitPosMap[len * 3]) & 1) ? 0xFF0000 : 0x00000000) |
              //         (((ledData >> bitPosMap[len * 3 + 1]) & 1) ? 0x00FF00 : 0x00000000) |
              //         (((ledData >> bitPosMap[len * 3 + 2]) & 1) ? 0x000000FF : 0x00000000)); // l
            }
            break;
          default:
            break;
          }
          Ep1RequestReplay = 1;
        }

        UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
      }
      break;
    case UIS_TOKEN_IN | 1:                                      // endpoint 1# 中断端点上传
      UEP1_T_LEN = 0;                                           // 预使用发送长度一定要清空
      UEP1_CTRL  = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK; // 默认应答NAK
      // FLAG = 1;                                                               /*传输完成标志*/
      break;
    case UIS_TOKEN_SETUP | 0: // endpoint 0# SETUP
      len = USB_RX_LEN;
      if (len == sizeof(USB_SETUP_REQ)) // SETUP包长度
      {
        SetupLen = ((uint16_t)UsbSetupBuf->wLengthH << 8) | (UsbSetupBuf->wLengthL);
        if (UsbSetupBuf->wLengthH != 0)
        {
          SetupLen = 0xFF; // 限制总长度
        }
        len      = 0; // 默认为成功并且上传0长度
        SetupReq = UsbSetupBuf->bRequest;
        if ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) == USB_REQ_TYP_STANDARD)
        { // 标准请求
          switch (SetupReq)
          { // 请求码
          case USB_GET_DESCRIPTOR:
            switch (UsbSetupBuf->wValueH)
            {
            case USB_DESCR_TYP_DEVICE: // 设备描述符
              pDescr = (uint8_t *)(&usbDevDesc[0]);
              len    = sizeof(usbDevDesc);
              break;
            case USB_DESCR_TYP_CONFIG: // 配置描述符
              pDescr = (uint8_t *)(&usbCfgDesc[0]);
              len    = sizeof(usbCfgDesc);
              break;
            case USB_DESCR_TYP_STRING: // 字符串描述符
              switch (UsbSetupBuf->wValueL)
              {
              case 0:
                pDescr = (uint8_t *)(&usbLangDesc[0]);
                len    = sizeof(usbLangDesc);
                break;
              case 1:
                pDescr = (uint8_t *)(&usbManuDesc[0]);
                len    = sizeof(usbManuDesc);
                break;
              case 2:
                pDescr = (uint8_t *)(&usbProdDesc[0]);
                len    = sizeof(usbProdDesc);
                break;
              case 3:
                pDescr = (uint8_t *)(&usbSerialDesc[0]);
                len    = sizeof(usbSerialDesc);
                break;
#ifdef USE_EXT_STR
              case 4:
                pDescr = (uint8_t *)(&usbCfgStrDesc[0]);
                len    = sizeof(usbCfgStrDesc);
                break;
              case 6:
                pDescr = (uint8_t *)(&SgioStrDesc[0]);
                len    = sizeof(SgioStrDesc);
                break;
              // case 6:
              //   pDescr = (uint8_t *)(&usbMseStrDesc[0]);
              //   len    = sizeof(usbMseStrDesc);
              //   break;
              case 7:
                pDescr = (uint8_t *)(&Com1StrDesc[0]);
                len    = sizeof(Com1StrDesc);
                break;
              case 8:
                pDescr = (uint8_t *)(&Com2StrDesc[0]);
                len    = sizeof(Com2StrDesc);
                break;
#endif
              default:
                len = 0xFF; // 不支持的字符串描述符
                break;
              }
              break;
            case USB_DESCR_TYP_REPORT: // 报表描述符
              switch (UsbSetupBuf->wIndexL)
              {
              case 0:
                pDescr = (uint8_t *)(&SegaRepDesc[0]); // 数据准备上传
                len    = sizeof(SegaRepDesc);
                break;
              case 1:
                pDescr = (uint8_t *)(&CustomRepDesc[0]); // 数据准备上传
                len    = sizeof(CustomRepDesc);
                break;
              default:
                len = 0xff; // 不支持的接口，这句话正常不可能执行
                break;
              }
              break;
            default:
              len = 0xFF; // 不支持的描述符类型
              break;
            }
            if (SetupLen > len)
            {
              SetupLen = len; // 限制总长度
            }
            len = SetupLen >= ENDP0_BUFFER_SIZE ? ENDP0_BUFFER_SIZE : SetupLen; // 本次传输长度
            memcpy(Ep0Buffer, pDescr, len);                                     /* 加载上传数据 */
            SetupLen -= len;
            pDescr += len;
            break;
          case USB_SET_ADDRESS:
            SetupLen = UsbSetupBuf->wValueL; // 暂存USB设备地址
            break;
          case USB_GET_CONFIGURATION:
            Ep0Buffer[0] = UsbConfig;
            if (SetupLen >= 1)
              len = 1;
            break;
          case USB_SET_CONFIGURATION:
            UsbConfig = UsbSetupBuf->wValueL;
            break;
          case USB_CLEAR_FEATURE:
            if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
            { // 端点
              if ((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == USB_FEATURE_ENDPOINT_HALT)
              {
                switch (UsbSetupBuf->wIndexL)
                {
                case 0x81:
                  UEP1_CTRL = UEP1_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                  break;
                case 0x82:
                  UEP2_CTRL = UEP2_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                  break;
                case 0x83:
                  UEP3_CTRL = UEP3_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                  break;
                case 0x84:
                  UEP4_CTRL = UEP4_CTRL & ~(bUEP_T_TOG | MASK_UEP_T_RES) | UEP_T_RES_NAK;
                  break;
                case 0x01:
                  UEP1_CTRL = UEP1_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                  break;
                case 0x02:
                  UEP2_CTRL = UEP2_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                  break;
                case 0x03:
                  UEP3_CTRL = UEP3_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                  break;
                case 0x04:
                  UEP4_CTRL = UEP4_CTRL & ~(bUEP_R_TOG | MASK_UEP_R_RES) | UEP_R_RES_ACK;
                  break;
                default:
                  len = 0xFF; // 不支持的端点
                  break;
                }
              }
              else
              {
                len = 0xFF;
              }
            }
            // if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
            // { // 设备
            //   break;
            // }
            else
            {
              len = 0xFF; // 不是端点不支持
            }
            break;
          case USB_SET_FEATURE: /* Set Feature */
            if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_DEVICE)
            {             /* 设置设备 */
                          // if ((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x01)
                          // {
                          //   if (usbCfgDesc[7] & 0x20)
                          //   {
                          //     /* 设置唤醒使能标志 */
                          //   }
                          //   else
                          //   {
                          //     len = 0xFF; /* 操作失败 */
                          //   }
                          // }
                          // else
                          // {
              len = 0xFF; /* 操作失败 */
              //}
            }
            else if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP)
            { /* 设置端点 */
              if ((((uint16_t)UsbSetupBuf->wValueH << 8) | UsbSetupBuf->wValueL) == 0x00)
              {
                switch (((uint16_t)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL)
                {
                case 0x81:
                  UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点1 IN STALL */
                  break;
                case 0x82:
                  UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点2 IN STALL */
                  break;
                case 0x83:
                  UEP3_CTRL = UEP3_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点3 IN STALL */
                  break;
                case 0x84:
                  UEP4_CTRL = UEP4_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL; /* 设置端点4 IN STALL */
                  break;
                case 0x01:
                  UEP1_CTRL = UEP1_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL; /* 设置端点1 OUT STALL */
                  break;
                case 0x02:
                  UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL; /* 设置端点2 OUT STALL */
                  break;
                case 0x03:
                  UEP3_CTRL = UEP3_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL; /* 设置端点3 OUT STALL */
                  break;
                case 0x04:
                  UEP4_CTRL = UEP4_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL; /* 设置端点4 OUT STALL */
                  break;
                default:
                  len = 0xFF; // 操作失败
                  break;
                }
              }
              else
              {
                len = 0xFF; // 操作失败
              }
            }
            else
            {
              len = 0xFF; // 操作失败
            }
            break;
          case USB_GET_INTERFACE:
            Ep0Buffer[0] = 0x00;
            if (SetupLen >= 1)
              len = 1;
            break;
          case USB_GET_STATUS:
            Ep0Buffer[0] = 0x00;
            Ep0Buffer[1] = 0x00;
            if (SetupLen >= 2)
              len = 2;
            else
              len = SetupLen;
            break;
          default:
            len = 0xFF; // 操作失败
            break;
          }
        }
        else if ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS)
        { // 类请求
          // USB CDC
          if (UsbSetupBuf->bRequestType & USB_REQ_TYP_IN)
          {
            // Device -> Host
            switch (SetupReq)
            {
            // case SERIAL_STATE:   //0x20
            //  Return CTS DTR state
            //  Return format: bit4:0 [RING BREAK DSR DCD]
            // break;
            case GET_LINE_CODING: // 0x21  currently configured
              pDescr = LineCoding;
              len    = LINECODING_SIZE;
              len    = SetupLen >= ENDP0_BUFFER_SIZE ? ENDP0_BUFFER_SIZE : SetupLen; // 本次传输长度
              memcpy(Ep0Buffer, pDescr, len);
              SetupLen -= len;
              pDescr += len;
              break;
            default:
              len = 0xFF;
              break;
            }
          }
          else
          { // USB_REQ_TYP_OUT
            // Host -> Device
            switch (SetupReq)
            {
            case SET_LINE_CODING: // 0x20
              break;
            case SET_CONTROL_LINE_STATE: // 0x22  generates RS-232/V.24 style control signals
              // 0x00 0x0[0 0 RTS DTR]
              break;
            // case SEND_BREAK:
            // break;
            default:
              len = 0xFF;
              break;
            }
          }
        }
        else
        { /* 非标准请求 */
          switch (SetupReq)
          {
          // case 0x01://GetReport
          //     break;
          // case 0x02://GetIdle
          //     break;
          // case 0x03://GetProtocol
          //     break;
          // case 0x09://SetReport
          //     break;
          // case 0x0A://SetIdle
          //     break;
          // case 0x0B://SetProtocol
          //     break;
          default:
            len = 0xFF; /*命令不支持*/
            break;
          }
        }
      }
      else
      {
        len = 0xFF; // SETUP包长度错误
      }
      if (len == 0xFF)
      { // 操作失败
        SetupReq  = 0xFF;
        UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL; // STALL
      }
      else if (len <= ENDP0_BUFFER_SIZE)
      { // 上传数据或者状态阶段返回0长度包
        UEP0_T_LEN = len;
        UEP0_CTRL  = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // 默认数据包是DATA1
      }
      else
      {                                                                       // 下传数据或其它
        UEP0_T_LEN = 0;                                                       // 虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
        UEP0_CTRL  = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK; // 默认数据包是DATA1
      }
      break;
    case UIS_TOKEN_IN | 0: // endpoint 0# IN
      switch (SetupReq)
      {
      case USB_GET_DESCRIPTOR:
        len = SetupLen >= ENDP0_BUFFER_SIZE ? ENDP0_BUFFER_SIZE : SetupLen; // 本次传输长度
        memcpy(Ep0Buffer, pDescr, len);                                     /* 加载上传数据 */
        SetupLen -= len;
        pDescr += len;
        UEP0_T_LEN = len;
        UEP0_CTRL ^= bUEP_T_TOG; // 翻转
        break;
      case USB_SET_ADDRESS:
        USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
        UEP0_CTRL  = UEP_R_RES_ACK | UEP_T_RES_NAK;
        break;
      default:
        UEP0_T_LEN = 0; // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
        UEP0_CTRL  = UEP_R_RES_ACK | UEP_T_RES_NAK;
        break;
      }
      break;
    case UIS_TOKEN_OUT | 0: // endpoint 0# OUT
      switch (SetupReq)
      {
      default:
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK; // 准备下一控制传输
        break;
      }
      UEP0_CTRL ^= bUEP_R_TOG; // 同步标志位翻转
      break;
    default:
      break;
    }
    UIF_TRANSFER = 0; // 清中断标志
  }
  else if (UIF_BUS_RST)
  { // USB总线复位
    UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP2_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
    UEP3_CTRL = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
    //    UEP4_CTRL    = UEP_R_RES_ACK | UEP_T_RES_NAK;
    USB_DEV_AD   = 0x00;
    UIF_SUSPEND  = 0;
    UIF_TRANSFER = 0;
    UIF_BUS_RST  = 0; // 清中断标志
  }
  else if (UIF_SUSPEND)
  { // USB总线挂起/唤醒完成
    UIF_SUSPEND = 0;
    if (USB_MIS_ST & bUMS_SUSPEND)
    { // 挂起
      //   while (XBUS_AUX & bUART0_TX)
      //     ; // 等待发送完成
      //   SAFE_MOD  = 0x55;
      //   SAFE_MOD  = 0xAA;
      //   WAKE_CTRL = bWAK_BY_USB | bWAK_RXD0_LO; // USB或者RXD0有信号时可被唤醒
      //   PCON |= PD;                             // 睡眠
      //   SAFE_MOD  = 0x55;
      //   SAFE_MOD  = 0xAA;
      //   WAKE_CTRL = 0x00;
      //
    }
    else
    { // 唤醒
    }
  }
  else
  {                    // 意外的中断,不可能发生的情况
    USB_INT_FG = 0xFF; // 清中断标志
  }

  ET2 = 1;
}

/**
 * 准备 USB 设备序列号描述符
 */
void usbSerialDescInit()
{
  uint8_t *pointer = &usbSerialDesc[16];
  uint32_t chipID  = getChipID();
  for (uint8_t i = 0; i < 8; i++)
  {
    *pointer = hexToChar(chipID);
    pointer -= 2;
    chipID = chipID >> 4;
  }
}

void usbDevInit()
{
  IE_USB    = 0;
  USB_CTRL  = 0x00;            // 先设定USB设备模式
  UDEV_CTRL = bUD_PD_DIS;      // 禁止DP/DM下拉电阻
  UDEV_CTRL &= ~bUD_LOW_SPEED; // 选择全速12M模式，默认方式
  USB_CTRL &= ~bUC_LOW_SPEED;

  UEP0_DMA   = (uint16_t)(&Ep0Buffer[0]);                               // 端点0数据传输地址
  UEP0_CTRL  = UEP_R_RES_ACK | UEP_T_RES_NAK;                           // OUT事务返回ACK，IN事务返回NAK
  UEP1_DMA   = (uint16_t)(&Ep1Buffer[0]);                               // 端点1数据传输地址
  UEP4_1_MOD = UEP4_1_MOD & ~bUEP1_BUF_MOD | bUEP1_RX_EN | bUEP1_TX_EN; // 端点1收发使能 128字节缓冲区
  UEP1_CTRL  = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;           // 端点1自动翻转同步标志位，OUT事务返回ACK，IN事务返回NAK
  UEP2_DMA   = (uint16_t)(&Ep2Buffer[0]);                               // 端点2数据传输地址
  UEP2_3_MOD = UEP2_3_MOD & ~bUEP2_BUF_MOD | bUEP2_RX_EN | bUEP2_TX_EN; // 端点2收发使能 128字节缓冲区
  UEP2_CTRL  = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;           // 端点2自动翻转同步标志位，OUT事务返回ACK，IN事务返回NAK
  UEP3_DMA   = (uint16_t)(&Ep3Buffer[0]);                               // 端点3数据传输地址
  UEP2_3_MOD = UEP2_3_MOD & ~bUEP3_BUF_MOD | bUEP3_RX_EN | bUEP3_TX_EN; // 端点3收发使能 128字节缓冲区
  UEP3_CTRL  = bUEP_AUTO_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;           // 端点3自动翻转同步标志位，OUT事务返回ACK，IN事务返回NAK

  //  UEP4_1_MOD = UEP4_1_MOD & (bUEP4_RX_EN | bUEP4_TX_EN);                // 端点4收发使能 128字节缓冲区
  //  UEP4_CTRL  = UEP_R_RES_ACK | UEP_T_RES_NAK;                           // 端点4不支持自动翻转同步标志位，OUT事务返回ACK，IN事务返回NAK

  USB_DEV_AD = 0x00;
  USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
  USB_CTRL |= bUC_SYS_CTRL1;
  UDEV_CTRL |= bUD_PORT_EN; // 允许USB端口
  USB_INT_FG = 0xFF;        // 清中断标志
  USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
  IE_USB     = 1;
  IP_EX |= bIP_USB;

  UEP1_T_LEN = 0;
  UEP2_T_LEN = 0;
  UEP3_T_LEN = 0;
  //  UEP4_T_LEN = 0;

  usbSerialDescInit();

  // FLAG = 1;
}

// void Enp1IntIn() {
//     memcpy(Ep1Buffer, HIDKey, sizeof(HIDKey));                                  //加载上传数据
//     UEP1_T_LEN = sizeof(HIDKey);                                                //上传数据长度
//     UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                   //有数据时上传数据并应答ACK
// }

/**
 * 上传 端点1 SEGA IO 数据包到上位机
 */
void Enp1IntIn()
{
  // memcpy(Ep1Buffer, HIDKey, 64);
  // usbReleaseAll();
  UEP1_T_LEN = 64;
  UEP1_CTRL  = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
}

// void Enp2IntIn( ) {
//     memcpy(Ep2Buffer, HIDMouse, sizeof(HIDMouse));                              //加载上传数据
//     UEP2_T_LEN = sizeof(HIDMouse);                                              //上传数据长度
//     UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                   //有数据时上传数据并应答ACK
// }

void Enp3IntIn()
{
  // Ep3Buffer[MAX_PACKET_SIZE] = 0xAA;
  // memcpy(Ep3Buffer + MAX_PACKET_SIZE + 1, HIDOutput, sizeof(HIDOutput)); // 加载上传数据
  UEP3_T_LEN = 64;                                          // 上传数据长度
  UEP3_CTRL  = UEP3_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK; // 有数据时上传数据并应答ACK
}

/**
 * 清空 HID设备 上传缓冲区
 */
void usbReleaseAll()
{
  memset(&Ep1Buffer[64], 0x00, 64);
  // memset(&Ep2Buffer[0], 0x01, 128);
  // memset(&Ep3Buffer[0], 0x02, 128);
  // memset(&Ep4BufferOut[0], 0x03, 64);
  // memset(&Ep4BufferIn[0], 0x04, 64);
}

uint8_t *GetEndpointInBuffer(uint8_t i)
{
  switch (i)
  {
  case 1:
    return &Ep1Buffer[64];
    break;
  case 2:
    return &Ep2Buffer[64];
    break;
  case 3:
    return &Ep3Buffer[64];
    break;
  case 4:
    return &Ep4BufferIn[0];
    break;
  default:
    return 0;
    break;
  }
}

uint8_t getHIDData(uint8_t index)
{
  // return HIDInput[index];
}

// void setHIDData(uint8_t index, uint8_t data)
// {
//   HIDOutput[index] = data;
// }

// __bit hasHIDData()
// {
//   return HIDIN;
// }

/**
 * 请求数据下发
 */
// void requestHIDData()
// {
//   HIDIN = 0;
// }

// void pushHIDData()
// {
//   Enp3IntIn();
// }

// uint8_t *fetchHIDData()
// {
//   // return HIDInput;
// }
