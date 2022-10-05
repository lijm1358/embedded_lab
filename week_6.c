
#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"

#include "misc.h"

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void EXTI_Configure(void);
void USART1_Init(void);
void NVIC_Configure(void);

void EXTI15_10_IRQHandler(void);

void EXTI2_IRQHandler(void);
void EXTI9_5_IRQHandler(void);

void Delay(void);

void sendDataUART1(uint16_t data);

char msg[] = "TEAM01.\r\n";

int mode = 0;

//---------------------------------------------------------------------------------------------------

void RCC_Configure(void)
{
	// TODO: Enable the APB2 peripheral clock using the function 'RCC_APB2PeriphClockCmd'
	
	/* UART TX/RX port clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	/* JoyStick Up/Down port clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	/* JoyStick Selection port clock enable */
	// RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	/* LED port clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	/* USART1 clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	/* Alternate Function IO clock enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure_JoyStick;
    GPIO_InitTypeDef GPIO_InitStructure_Button;
    GPIO_InitTypeDef GPIO_InitStructure_LED;
    GPIO_InitTypeDef GPIO_InitStructure_UART_TX;
    GPIO_InitTypeDef GPIO_InitStructure_UART_RX;

	// TODO: Initialize the GPIO pins using the structure 'GPIO_InitTypeDef' and the function 'GPIO_Init'
	
    /* JoyStick up, down pin setting */
    GPIO_InitStructure_JoyStick.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_5;
    GPIO_InitStructure_JoyStick.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOC, &GPIO_InitStructure_JoyStick);
	
    /* JoyStick selection pin setting */
    
    /* button pin setting */
    GPIO_InitStructure_Button.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure_Button.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure_Button);
    
    /* LED pin setting*/
    GPIO_InitStructure_LED.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7;
    GPIO_InitStructure_LED.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure_LED.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure_LED);
	
    /* UART pin setting */
    //TX
    GPIO_InitStructure_UART_TX.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure_UART_TX.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure_UART_TX.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure_UART_TX);
	//RX
    GPIO_InitStructure_UART_RX.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure_UART_RX.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure_UART_RX);
}

void EXTI_Configure(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

	// TODO: Select the GPIO pin (Joystick, button) used as EXTI Line using function 'GPIO_EXTILineConfig'
	// TODO: Initialize the EXTI using the structure 'EXTI_InitTypeDef' and the function 'EXTI_Init'
	
    /* Joystick Down */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource2);
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Joystick Up */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource5);
    EXTI_InitStructure.EXTI_Line = EXTI_Line5;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
	
	/* Joystick Selection */

    /* Button */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource11);
    EXTI_InitStructure.EXTI_Line = EXTI_Line11;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
	// NOTE: do not select the UART GPIO pin used as EXTI Line here
}

void USART1_Init(void)
{
	USART_InitTypeDef USART1_InitStructure;

	// Enable the USART1 peripheral
	USART_Cmd(USART1, ENABLE);
	
	// TODO: Initialize the USART using the structure 'USART_InitTypeDef' and the function 'USART_Init'
	USART1_InitStructure.USART_BaudRate = 9600;
        USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART1_InitStructure.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);
        USART1_InitStructure.USART_Parity = USART_Parity_No;
        USART1_InitStructure.USART_StopBits = USART_StopBits_1;
        USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_Init(USART1, &USART1_InitStructure);
	
	// TODO: Enable the USART1 RX interrupts using the function 'USART_ITConfig' and the argument value 'Receive Data register not empty interrupt'
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

void NVIC_Configure(void) {

    NVIC_InitTypeDef NVIC_InitStructure;
    
    // TODO: fill the arg you want
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	// TODO: Initialize the NVIC using the structure 'NVIC_InitTypeDef' and the function 'NVIC_Init'
	
    // Joystick Down
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI_Line2;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    // Joystick Up
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI_Line5;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // User S1 Button
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = EXTI_Line11;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // UART1
	// 'NVIC_EnableIRQ' is only required for USART setting
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // TODO
    NVIC_InitStructure.NVIC_IRQChannelSubPriority =3; // TODO
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void USART1_IRQHandler() {
	uint16_t word;
    if(USART_GetITStatus(USART1,USART_IT_RXNE)!=RESET){
    	// the most recent received data by the USART1 peripheral
        word = USART_ReceiveData(USART1);

        // TODO implement
        if(word == 'a') {
          //TODO
          mode=0;
        }
        else if(word == 'b') {
          //TODO
          mode=1;
        }

        // clear 'Read data register not empty' flag
    	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}

void EXTI15_10_IRQHandler(void) { // when the button is pressed

	if (EXTI_GetITStatus(EXTI_Line11) != RESET) {
		if (GPIO_ReadInputDataBit(GPIOD, GPIO_Pin_11) == Bit_RESET) {
			// TODO implement
			for(int i=0; msg[i]!='\0'; i++) {
                        sendDataUART1((msg[i]);
                    }
		}
        EXTI_ClearITPendingBit(EXTI_Line11);
	}
}

// TODO: Create Joystick interrupt handler functions
void EXTI2_IRQHandler(void) {
        if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
            //TODO
            mode = 1;
        }
        EXTI_ClearITPendingBit(EXTI_Line2);
}

void EXTI9_5_IRQHandler(void) {
  if (EXTI_GetITStatus(EXTI_Line5) != RESET) {
            //TODO
            mode = 0;
  }
  EXTI_ClearITPendingBit(EXTI_Line5);
}

void Delay(void) {
	int i;

	for (i = 0; i < 2000000; i++) {}
}

void sendDataUART1(uint16_t data) {
	/* Wait till TC is set */
	while ((USART1->SR & USART_SR_TC) == 0);
	USART_SendData(USART1, data);
}

int main(void)
{

    SystemInit();

    RCC_Configure();

    GPIO_Configure();

    EXTI_Configure();

    USART1_Init();

    NVIC_Configure();
    
    uint16_t led_pins[4] = {
      GPIO_Pin_7,
      GPIO_Pin_4,
      GPIO_Pin_3,
      GPIO_Pin_2,
    };
    int i = 0;;
    
    while (1) {
    	// TODO: implement 
        GPIO_SetBits(GPIOD, led_pins[i]);
        Delay();
        GPIO_ResetBits(GPIOD, led_pins[i]);
        
        if(mode == 0) {
          i = (i+1) % 4;
        }
        else if(mode == 1) {
          i = ((i-1) + 4) % 4;
        }
    }
    return 0;
}
