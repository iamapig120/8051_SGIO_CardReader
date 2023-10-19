#include "usb_sega_io.h"
#include "ch552.h"
#include "rgb.h"
#include "usb.h"

uint8_c bitPosMap[] = {23, 20, 22, 19, 21, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6};

DataReceive *dataReceive;
DataUpload  *dataForUpload;

void io4Init(void)
{
  // uint8_t i                   = 0;
  GetEndpointInBuffer(1)[0] = 1;
  dataForUpload = (DataUpload *)(&(GetEndpointInBuffer(1)[1]));

  dataForUpload->analog[0]    = 0x84F0;
  // dataForUpload->rotary[0]    = 0x8100;
  dataForUpload->buttons[1]   = 0x40;
  dataForUpload->buttons[3]   = 0x80;
  // dataForUpload->systemStatus = 0x02;
  dataForUpload->systemStatus = 0x30;
}

void io4Upload(void)
{
  // GetEndpointInBuffer(1)[0] = 0x01;
  Enp1IntIn();
  // usbReleaseAll();
}
