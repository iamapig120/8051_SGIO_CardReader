#include "usb_sega_io.h"
#include "ch552.h"
#include "rgb.h"
#include "usb.h"

uint8_c  bitPosMap[]      = {23, 20, 22, 19, 21, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6};
uint32_x ledData          = 0;
__data uint32_t ledDataFor422    = 0;

DataReceive *dataReceive;
DataUpload  *dataForUpload;

void io4Init(void)
{
  // uint8_t i                   = 0;
  EP1_IN_BUF[0] = 1;
  dataForUpload = (DataUpload *)(&(EP1_IN_BUF[1]));

  dataForUpload->analog[0] = 0x8000;
  // dataForUpload->rotary[0]    = 0x8100;
  dataForUpload->buttons[1] = 0x40;
  dataForUpload->buttons[3] = 0x80;
  // dataForUpload->systemStatus = 0x02;
  dataForUpload->systemStatus = 0x30;
}

void USB_EP1_OUT_cb(void)
{
  uint8_t i;
  if (U_TOG_OK)
  {
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
        ledData       = (uint32_t)(dataReceive->payload[0]) << 16 | (uint32_t)(dataReceive->payload[1]) << 8 | dataReceive->payload[2];
        ledDataFor422 = 0;
        // Right1, Left1, Right2, Left2, Right3, Left3
        for (i = 0; i < 3; i++)
        {
          ledDataFor422 <<= 1;
          ledDataFor422 |= (ledData >> bitPosMap[9 + i * 3 + 2]) & 1;
          ledDataFor422 <<= 1;
          ledDataFor422 |= (ledData >> bitPosMap[9 + i * 3 + 1]) & 1;
          ledDataFor422 <<= 1;
          ledDataFor422 |= (ledData >> bitPosMap[9 + i * 3]) & 1;
          
          ledDataFor422 <<= 1;
          ledDataFor422 |= (ledData >> bitPosMap[i * 3 + 2]) & 1; // B
          ledDataFor422 <<= 1;
          ledDataFor422 |= (ledData >> bitPosMap[i * 3 + 1]) & 1; // G
          ledDataFor422 <<= 1;
          ledDataFor422 |= (ledData >> bitPosMap[i * 3]) & 1; // R
        }
        isLedDataChanged |= 0x01;
        break;
      default:
        break;
      }
      UEP1_T_LEN = 64;
      UEP1_CTRL  = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
    }
    UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_R_RES | UEP_R_RES_ACK;
  }
}