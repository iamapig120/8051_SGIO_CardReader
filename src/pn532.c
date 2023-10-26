#include "pn532.h"
#include "bsp.h"

#include "pn532_i2c.h"

uint8_x *emety;
uint8_x  _uid[7];       // ISO14443A uid
uint8_x  _uidLen;       // uid len
uint8_x  _key[6];       // Mifare Classic key
uint8_x  inListedTag;   // Tg number of inlisted tag.
uint8_x  _felicaIDm[8]; // FeliCa IDm (NFCID2)
uint8_x  _felicaPMm[8]; // FeliCa PMm (PAD)
uint8_x  pn532_packetbuffer[64];

// void begin()
// {
//   _begin();
//   _wakeup();
// }

// uint32_t getFirmwareVersion(void)
// {
//   uint32_t response;

//   pn532_packetbuffer[0] = PN532_COMMAND_GETFIRMWAREVERSION;

//   if (_writeCommand(pn532_packetbuffer, 1, emety, 0))
//   {
//     return 0;
//   }

//   // read data packet
//   int16_t status = _readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), 1000);
//   if (0 > status)
//   {
//     return 0;
//   }

//   response = pn532_packetbuffer[0];
//   response <<= 8;
//   response |= pn532_packetbuffer[1];
//   response <<= 8;
//   response |= pn532_packetbuffer[2];
//   response <<= 8;
//   response |= pn532_packetbuffer[3];

//   return response;
// }

// uint8_t readPassiveTargetID(uint8_t cardbaudrate, uint8_t *uid, uint8_t *uidLength, uint16_t timeout, uint8_t inlist)
// {
//   pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
//   pn532_packetbuffer[1] = 1; // max 1 cards at once (we can set this to 2 later)
//   pn532_packetbuffer[2] = cardbaudrate;

//   if (_writeCommand(pn532_packetbuffer, 3, emety, 0))
//   {
//     return 0x0; // command failed
//   }

//   // read data packet
//   if (_readResponse(pn532_packetbuffer, sizeof(pn532_packetbuffer), timeout) < 0)
//   {
//     return 0x0;
//   }

//   // check some basic stuff
//   /* ISO14443A card response should be in the following format:

//     byte            Description
//     -------------   ------------------------------------------
//     b0              Tags Found
//     b1              Tag Number (only one used in this example)
//     b2..3           SENS_RES
//     b4              SEL_RES
//     b5              NFCID Length
//     b6..NFCIDLen    NFCID
//   */

//   if (pn532_packetbuffer[0] != 1)
//     return 0;

//   uint16_t sens_res = pn532_packetbuffer[2];
//   sens_res <<= 8;
//   sens_res |= pn532_packetbuffer[3];

//   /* Card appears to be Mifare Classic */
//   *uidLength = pn532_packetbuffer[5];

//   for (uint8_t i = 0; i < pn532_packetbuffer[5]; i++)
//   {
//     uid[i] = pn532_packetbuffer[6 + i];
//   }

//   if (inlist)
//   {
//     inListedTag = pn532_packetbuffer[1];
//   }

//   return 1;
// }

/*
int8_t felica_Polling(uint16_t systemCode, uint8_t requestCode, uint8_t *idm, uint8_t *pmm, uint16_t *systemCodeResponse, uint16_t timeout)
{
  pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
  pn532_packetbuffer[1] = 1;
  pn532_packetbuffer[2] = 1;
  pn532_packetbuffer[3] = FELICA_CMD_POLLING;
  pn532_packetbuffer[4] = (systemCode >> 8) & 0xFF;
  pn532_packetbuffer[5] = systemCode & 0xFF;
  pn532_packetbuffer[6] = requestCode;
  pn532_packetbuffer[7] = 0;

  if (_writeCommand(pn532_packetbuffer, 8, emety, 0))
  {
    return -1;
  }

  int16_t status = _readResponse(pn532_packetbuffer, 22, timeout);
  if (status < 0)
  {
    return -2;
  }

  // Check NbTg (pn532_packetbuffer[7])
  if (pn532_packetbuffer[0] == 0)
  {
    return 0;
  }
  else if (pn532_packetbuffer[0] != 1)
  {
    return -3;
  }

  inListedTag = pn532_packetbuffer[1];

  // length check
  uint8_t responseLength = pn532_packetbuffer[2];
  if (responseLength != 18 && responseLength != 20)
  {
    return -4;
  }

  uint8_t i;
  for (i = 0; i < 8; ++i)
  {
    idm[i]        = pn532_packetbuffer[4 + i];
    _felicaIDm[i] = pn532_packetbuffer[4 + i];
    pmm[i]        = pn532_packetbuffer[12 + i];
    _felicaPMm[i] = pn532_packetbuffer[12 + i];
  }

  if (responseLength == 20)
  {
    *systemCodeResponse = (uint16_t)((pn532_packetbuffer[20] << 8) + pn532_packetbuffer[21]);
  }

  return 1;
}

*/
