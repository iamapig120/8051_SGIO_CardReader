#ifndef __USB_H
#define __USB_H

#include <stdint.h>
#include "bsp.h"

void usbDevInit();                              // USB设备初始化

uint8_t* GetEndpointInBuffer(uint8_t i);    // 设置键值

void usbReleaseAll();                           // 清除上传缓冲区

void Enp1IntIn();                          // 上传按键数据

#define ENDP0_BUFFER_SIZE 64                      // 端点0 缓冲区大小
#define ENDP1_BUFFER_SIZE 64                      // 端点1 缓冲区大小；键盘、音频控制器、手柄 端点最大缓冲大小
#define ENDP2_BUFFER_SIZE 64                      // 端点2 缓冲区大小；CDC 设备数据缓冲大小
#define ENDP3_BUFFER_SIZE 64                      // 端点3 缓冲区大小；HID 自定义设备数据缓冲大小
#define ENDP4_BUFFER_SIZE 64                      // 端点4 缓冲区大小；CDC 设备数据缓冲大小

#define USE_EXT_STR                           // 是否启用额外的字符串描述符

// uint8_t getHIDData(uint8_t index);              // 从HID接收缓冲中读取一个字节
// void setHIDData(uint8_t index, uint8_t data);   // 设置HID发送缓冲区对应偏移的数据
__bit hasHIDData();                             // 是否有数据下发
void requestHIDData();                          // 请求数据下发
void pushHIDData();                             // 将HID发送缓冲区内的数据上传
uint8_t* fetchHIDData();                        // 获取HID接收缓冲区的数据

#endif
