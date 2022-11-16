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

//int keypad_col[3] = {3,1,5};
//int keypad_row[4] = {2,7,6,4};

uint16_t keypad_out_pins[3] = {
    GPIO_Pin_0,
    GPIO_Pin_1,
    GPIO_Pin_2,
};

uint16_t keypad_in_pins[4] = {
    GPIO_Pin_3,
    GPIO_Pin_7,
    GPIO_Pin_5,
    GPIO_Pin_6,
};

uint8_t keypad_map[3][4] = { {'1', '4', '7', '*'}, 
                             {'2', '5', '8', '0'},
                             {'3', '6', '9', '#'} };

int keypad_col_iter=0;

int PW_TEST = 1234;

void RCC_Configure(void) // stm32f10x_rcc.h Au¡Æi
{
    // clock for ultrasonic distance, keypad
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* Alternate Function IO clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}

void TIM2_IRQHandler(void) {
  if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
    printf("test\n");
  }
  
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
}

void initTimer() {
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_InitStructure.TIM_Prescaler = 72;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStructure.TIM_Period = 10;
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM_InitStructure);

    TIM_ARRPreloadConfig(TIM2, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
}

void init_TIM2(void){  // Output compare mode, PWM
    RCC->APB1ENR |= (1<<0); // Bit 0 TIM2EN
 
    TIM2->PSC = 72-1;   // 1us
    TIM2->EGR = (1<<0); // Bit 0 UG
    TIM2->CR1 = (1<<0); // Bit 0 CEN
}

void GPIO_Configure(void) // stm32f10x_gpio.h Au¡Æi
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* ultrasonic distance */
    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_8; // PA8(Trig)
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_9; // PA9(Echo)
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* keypad */
    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2; // keypad 3, 1, 5th pin for output(v out)
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // set column output to 0 
    for(int i=0;i<3;i++) {
        GPIO_ResetBits(GPIOA, keypad_out_pins[i]);
    }

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_7 | GPIO_Pin_5 | GPIO_Pin_6; // keypad 2, 7, 6, 4th pin for input(v in)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOA,&GPIO_InitStructure);
}

void delay() {
  for(int i=0;i<10;i++) {}
}

char getKeypadInput() {
    while(1) {
        GPIO_SetBits(GPIOA, keypad_out_pins[keypad_col_iter]);
        delay();
        
        if(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[0])) {
            return keypad_map[keypad_col_iter][0];
        }
        else if(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[1])) {
            return keypad_map[keypad_col_iter][1];
        }
        else if(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[2])) {
            return keypad_map[keypad_col_iter][2];
        }
        else if(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[3])) {
            return keypad_map[keypad_col_iter][3];
        }
        
        GPIO_ResetBits(GPIOA, keypad_out_pins[keypad_col_iter]);
        delay();
        
        keypad_col_iter++;
        if(keypad_col_iter==3)
            keypad_col_iter=0;
    }
}

void waitForKeyRelease(char keyin) {
    if(keyin == '1' || keyin == '2' || keyin == '3') {
        while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[0]));
    }
    if(keyin == '4' || keyin == '5' || keyin == '6') {
        while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[1]));
    }
    if(keyin == '7' || keyin == '8' || keyin == '9') {
        while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[2]));
    }
    if(keyin == '*' || keyin == '0' || keyin == '#') {
        while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[3]));
    }
}

int charToInt(char c) {
    if(c=='*')
      return 10;
    else if(c=='#')
      return 11;
    else
      return c-'0';
}

int checkPassword(int input, int pw) {
    return input==pw;
}

int main(void)
{
    RCC_Configure();

    GPIO_Configure();
    init_TIM2();

    char keyin;
    int code=0;
    int digit=1;

    while(1){
        keyin = getKeypadInput();
        printf("%c\n", keyin);
        waitForKeyRelease(keyin);
        code*=10;
        code += charToInt(keyin);
        if(code/1000 != 0) {
            if(checkPassword(code, PW_TEST)) {
                printf("open\n");
                code = 0;
                digit = 1;
            }
            else {
                printf("wrong\n");
                code = 0;
                digit = 1;
            }
        }
    }
    
    return 0;
}