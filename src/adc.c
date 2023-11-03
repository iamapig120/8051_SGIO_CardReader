#include "adc.h"
#include "ch552.h"

uint8_t adc_data = 0x80;

/*******************************************************************************
* Function Name  : ADCInit(UINT8 div)
* Description    : ADC采样时钟设置,模块开启，中断开启
* Input          : UINT8 div 时钟设置
                   1 慢  384个Fosc
                   0 快  96个Fosc
* Output         : None
* Return         : None
*******************************************************************************/
void ADCInit(uint8_t div)
{
  ADC_CFG &= ~bADC_CLK | div;
  ADC_CFG |= bADC_EN; // ADC电源使能
#if ADC_INTERRUPT
  ADC_IF = 0; // 清空中断
  IE_ADC = 1; // 使能ADC中断
#endif
}

/*******************************************************************************
* Function Name  : ADC_ChannelSelect(UINT8 ch)
* Description    : ADC采样启用
* Input          : UINT8 ch 采用通道
* Output         : None
* Return         : 成功 SUCCESS
                   失败 FAIL
*******************************************************************************/
void ADC_ChannelSelect(uint8_t ch)
{
  if (ch == 0)
  {
    ADC_CHAN1 = 0;
    ADC_CHAN0 = 0;
    P1_DIR_PU &= ~bAIN0;
  } // AIN0
  else if (ch == 1)
  {
    ADC_CHAN1 = 0;
    ADC_CHAN0 = 1;
    P1_DIR_PU &= ~bAIN1;
  } // AIN1
  else if (ch == 2)
  {
    ADC_CHAN1 = 1;
    ADC_CHAN0 = 0;
    P1_DIR_PU &= ~bAIN2;
  } // AIN2
  else if (ch == 3)
  {
    ADC_CHAN1 = 1;
    ADC_CHAN0 = 1;
    P3_DIR_PU &= ~bAIN3;
  } // AIN3
}

/*******************************************************************************
 * Function Name  : ADCInterrupt()
 * Description    : ADC 中断服务程序
 *******************************************************************************/
void __ADCInterrupt() __interrupt(INT_NO_ADC) __using(3) // ADC中断服务程序,使用寄存器组1
{
  adc_data = ADC_DATA; // 取走ADC采样数据
  ADC_IF   = 0;        // 清空ADC中断标志
}
