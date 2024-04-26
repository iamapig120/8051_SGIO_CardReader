#ifndef __ADC_H
#define __ADC_H

#include "bsp.h"
#include "sys.h"
#include "ch552.h"

#define ADC_INTERRUPT 0

void ADCInit(uint8_t div);
void ADC_ChannelSelect(uint8_t ch);
void ADC_update();

extern uint16_t adc_var;

#endif