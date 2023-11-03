#ifndef __ADC_H
#define __ADC_H

#include "bsp.h"
#include "sys.h"

#define ADC_INTERRUPT 1

extern uint8_t adc_data;

void ADCInit(uint8_t div);
void ADC_ChannelSelect(uint8_t ch);

#endif