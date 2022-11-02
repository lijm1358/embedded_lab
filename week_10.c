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

int counter = 0;

uint16_t xpos;
uint16_t ypos;
uint16_t realx;
uint16_t realy;
int led1=0;
int led2=0;
int buttonStatus = 0;

void RCC_Configure(void)
{
	/* TIM2 Clock enable */
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
	/* Alternate Function IO clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;      // ADC12_IN10
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;       // Analog In
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
  
}

void NVIC_Configure() {
   NVIC_InitTypeDef NVIC_InitStructure;
   NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

   NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
   NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
   NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
   NVIC_Init(&NVIC_InitStructure);
   
   
}

void initTimer() {
  TIM_TimeBaseInitTypeDef TIM_InitStructure;
  TIM_InitStructure.TIM_Prescaler = 7200;
  TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_InitStructure.TIM_Period = 10000;
  TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseInit(TIM2, &TIM_InitStructure);
  
  TIM_ARRPreloadConfig(TIM2, ENABLE);
  TIM_Cmd(TIM2, ENABLE);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void TIM2_IRQHandler(void) {
  if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
    printf("test\n");
    if(led1) {
      GPIO_SetBits(GPIOD, GPIO_Pin_2);
    } else {
      GPIO_ResetBits(GPIOD, GPIO_Pin_2);
    }
    
    led1 = !led1;
    
    if(led2) {
      GPIO_SetBits(GPIOD, GPIO_Pin_3);
    } else {
      GPIO_ResetBits(GPIOD, GPIO_Pin_3);
    }
    
    if(counter%5 == 0) {
      led2 = !led2;
    }
    counter++;
  }
  
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

int isOnButton(uint16_t x, uint16_t y) {
  if (x>=70 && x<=130 && y>=90 && y<=150) {
    return 1;
  }
  else {
    return 0;
  }
}

void delay() {
  for(int i=0;i<500;i++) {}
}

int main(void)
{

    SystemInit();

    RCC_Configure();

    GPIO_Configure();
    
    LCD_Init();
    Touch_Configuration();
    Touch_Adjust();
    LCD_Clear(WHITE);
    
    initTimer();

    NVIC_Configure();

    while (1) {
    	// TODO: implement 
        LCD_ShowString(50, 50, "MON_Team01", BLUE, WHITE);
        if(!buttonStatus) {
          LCD_ShowString(50, 65, "OFF", BLACK, WHITE);
          TIM_Cmd(TIM2, DISABLE);
          GPIO_ResetBits(GPIOD, GPIO_Pin_2);
          GPIO_ResetBits(GPIOD, GPIO_Pin_3);
          counter = 0;
          led1=0;
          led2=0;
        } else {
          LCD_ShowString(50, 65, "ON ", RED, WHITE);
          TIM_Cmd(TIM2, ENABLE);
        }
        
        LCD_DrawRectangle(70, 90, 130, 150);
        LCD_ShowString(80, 100, "But", BLACK, WHITE);
        
        Touch_GetXY(&xpos, &ypos, 1);
        Convert_Pos(xpos, ypos, &realx, &realy);        // touch adjust ÇÊ¿ä
        
        if(isOnButton(realx, realy)) {
          buttonStatus = !buttonStatus;
          delay();
        }
    }
    return 0;
}    