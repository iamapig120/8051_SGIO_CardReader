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
  EP1_IN_BUF[0] = 1;
  dataForUpload = (DataUpload *)(&(EP1_IN_BUF[1]));

  dataForUpload->analog[0] = 0x84F0;
  // dataForUpload->rotary[0]    = 0x8100;
  dataForUpload->buttons[1] = 0x40;
  dataForUpload->buttons[3] = 0x80;
  // dataForUpload->systemStatus = 0x02;
  dataForUpload->systemStatus = 0x30;
}

void USB_EP1_OUT_cb(void)
{
  uint32_t ledData;
  uint8_t  i;
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
        ledData = (uint32_t)(dataReceive->payload[0]) << 16 | (uint32_t)(dataReceive->payload[1]) << 8 | dataReceive->payload[2];
        for (i = 0; i < 3; i++)
        {
          // Left1, Left2, Left3, Right1, Right2, Right3
          rgbSet(i,
              (((ledData >> bitPosMap[9 + i * 3]) & 1) ? 0xFF0000 : 0x00000000) |
                  (((ledData >> bitPosMap[9 + i * 3 + 1]) & 1) ? 0x00FF00 : 0x00000000) |
                  (((ledData >> bitPosMap[9 + i * 3 + 2]) & 1) ? 0x000000FF : 0x00000000)); // r
          // rgbSet(i + 3,
          //     (((ledData >> bitPosMap[i * 3]) & 1) ? 0xFF0000 : 0x00000000) |
          //         (((ledData >> bitPosMap[i * 3 + 1]) & 1) ? 0x00FF00 : 0x00000000) |
          //         (((ledData >> bitPosMap[i * 3 + 2]) & 1) ? 0x000000FF : 0x00000000)); // l
        }
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