#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stdio.h"
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

uint16_t xpos;
uint16_t ypos;
uint16_t realx;
uint16_t realy;
int lightVal;

void RCC_Configure(void)
{
	/* ADC1 Clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	
	/* Alternate Function IO clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
}

void ADC1_Configure(void) {
  ADC_InitTypeDef ADC_InitStructure;
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 1, ADC_SampleTime_239Cycles5);
  ADC_Init(ADC1, &ADC_InitStructure);
}

void ADC1_start(void) {
    ADC_ResetCalibration(ADC1);
    ADC_StartCalibration(ADC1);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;      // ADC12_IN10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;       // Analog In
    GPIO_Init(GPIOC, &GPIO_InitStructure);
  
}

void EXTI_Configure(void)
{
  EXTI_InitTypeDef EXTI_InitStructure;
  
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource0);   // GPIOC_0
  EXTI_InitStructure.EXTI_Line = EXTI_Line11;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
}

void NVIC_Configure() {
   NVIC_InitTypeDef NVIC_InitStructure;

   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

   NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
   NVIC_Init(&NVIC_InitStructure);
   
   ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
   ADC_Cmd(ADC1, ENABLE);

}

void ADC1_2_IRQHandler(void) {
    ADC_GetITStatus(ADC1, ADC_IT_EOC);
    lightVal = ADC_GetConversionValue(ADC1);
    //printf("%d\n", lightVal);
    ADC_ClearITPendingBit(ADC1, ADC_IT_EOC);
}

int main(void)
{

    SystemInit();

    RCC_Configure();

    GPIO_Configure();

    ADC1_Configure();
    
    EXTI_Configure();

    NVIC_Configure();
    
    LCD_Init();
    Touch_Configuration();
    Touch_Adjust();
    LCD_Clear(WHITE);
    
    ADC1_start();

    while (1) {
    	// TODO: implement 
        LCD_ShowString(50, 50, "MON_Team01", BLACK, WHITE);
        
        Touch_GetXY(&xpos, &ypos, 1);
        Convert_Pos(xpos, ypos, &realx, &realy);        // touch adjust ÇÊ¿ä
        
        LCD_DrawCircle(realx, realy, 10);
        LCD_ShowNum(50, 65, realx, 10, BLACK, WHITE);
        LCD_ShowNum(50, 80, realy, 10, BLACK, WHITE);
        LCD_ShowNum(50, 95, lightVal, 4, BLACK, WHITE);
    }
    return 0;
}    