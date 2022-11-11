#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "lcd.h"
#include "touch.h"
#include "misc.h"

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void NVIC_Configure(void);
void ADC_Configure(void);
void Delay(void);

int color[12] = {WHITE,CYAN,BLUE,RED,MAGENTA,LGRAY,GREEN,YELLOW,BROWN,BRRED,GRAY};
//---------------------------------------------------------------------------------------------------
uint16_t value = 0;
uint16_t pos_x = 0;
uint16_t pos_y = 0;
uint16_t tim2_counter = 0;
volatile uint32_t ADC_Value;

void RCC_Configure(void) // stm32f10x_rcc.h Au¡Æi
{
	/* ADC1 Clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	
    /* Alternate Function IO clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

void ADC_Configure(void) {
  
  ADC_InitTypeDef ADC_InitStructure; 
  ADC_DeInit(ADC1); 
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent; 
  ADC_InitStructure.ADC_ScanConvMode = ENABLE; 
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_55Cycles5);
  ADC_Init(ADC1, &ADC_InitStructure);

  ADC_DMACmd(ADC1, ENABLE);
  ADC_Cmd(ADC1, ENABLE);
}

void ADC_start(void) {
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1))
        ;
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1))
        ;
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void DMA_Configure(void) {
    DMA_InitTypeDef DMA_InitStructure; 
    DMA_DeInit(DMA1_Channel1); 
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &ADC1->DR; 
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) &ADC_Value; 
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; 
    DMA_InitStructure.DMA_BufferSize = 1; 
    
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; 
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Word; 
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; 
    DMA_InitStructure.DMA_Priority = DMA_Priority_High; 
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 
    DMA_Init(DMA1_Channel1, &DMA_InitStructure); 
    DMA_Cmd(DMA1_Channel1, ENABLE);
}

void GPIO_Configure(void) // stm32f10x_gpio.h Au¡Æi
{
    GPIO_InitTypeDef GPIO_InitStructure;

  GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
}

int main(void)
{
    SystemInit();

    RCC_Configure();

    GPIO_Configure();
    ADC_Configure();
    DMA_Configure();
    
    LCD_Init();
    Touch_Configuration();

    LCD_Clear(WHITE);
    
    ADC_start();
    int threshold = 1500;

    while (1) {
        // TODO: implement 
      printf("light :%d\n", ADC_Value);
      if(ADC_Value < threshold) {
        LCD_Clear(GRAY);
        LCD_ShowNum(50, 50, ADC_Value, 4, BLACK, WHITE);
      } else {
        LCD_Clear(WHITE);
        LCD_ShowNum(50, 50, ADC_Value, 4, BLACK, WHITE);
      }
    }
    return 0;
}