#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "lcd.h"
#include "touch.h"
#include "misc.h"

#define GPIO_PORT GPIOD
#define OPEN 1000
#define CLOSE 2000

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void NVIC_Configure(void);
void ADC_Configure(void);
void Delay(void);
//void show_4_digit(uint16_t number);
void show_4_digit_forcode();

//int keypad_col[3] = {3,1,5};
//int keypad_row[4] = {2,7,6,4};

uint16_t keypad_out_pins[3] = {
    GPIO_Pin_0,
    GPIO_Pin_1,
    GPIO_Pin_10,
};

uint16_t keypad_in_pins[4] = {
    GPIO_Pin_11,
    GPIO_Pin_7,
    GPIO_Pin_5,
    GPIO_Pin_6,
};

uint8_t keypad_map[3][4] = { {'1', '4', '7', '*'}, 
                             {'2', '5', '8', '0'},
                             {'3', '6', '9', '#'} };

int keypad_col_iter=0;

int PW_TEST = 1234;

int code[4]={0,0,0,0};
int codecount=0;

// 0에서 9까지 숫자 표현을 위한 세그먼트 a, b, c, d, e, f, g, dp의 패턴
uint8_t patterns[] = { 0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE4, 0xFE, 0xE6};

// segment 모듈 연결 핀 a, b, c, d, e, f, g, dp
uint16_t segmentpins[] = {  GPIO_Pin_8,  GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_11, 
                           GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15 };

// segment 모듈 자릿수 선택 핀
uint16_t digit_select_pin[] = { GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_2, GPIO_Pin_3 };

void RCC_Configure(void) // stm32f10x_rcc.h Au°i
{
    // clock for ultrasonic distance, keypad
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* Alternate Function IO clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    // clock for servo motor
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // clock for seven segment
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
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

void GPIO_distance_configure(void) {
  /* ultrasonic distance */
  GPIO_DeInit(GPIOA);
    GPIOA->CRH = (GPIOA->CRH&0xFFFFFF00)|(3<<0*4)|(4<<1*4); // PA8 Trig, PA9 Echo
}

void GPIO_Configure(void) // stm32f10x_gpio.h Au°i
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_DeInit(GPIOA);
    
    /* keypad */
    
    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10; // keypad 3, 1, 5th pin for output(v out)
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // set column output to 0 
    for(int i=0;i<3;i++) {
        GPIO_ResetBits(GPIOA, keypad_out_pins[i]);
    }

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_7 | GPIO_Pin_5 | GPIO_Pin_6; // keypad 2, 7, 6, 4th pin for input(v in)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOA,&GPIO_InitStructure);
    
    /* servo motor ??*/
    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed   = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_AF_PP;  // ADC ??????? ???
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /* 7segment */
    uint16_t pin = 0;
    for (int i = 0; i < 8; i++) {
        pin = pin | segmentpins[i];
    }
    for (int i = 0; i < 4; i++) {
        pin = pin | digit_select_pin[i];
    }
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 |  
                                                    GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15 |
                                                    GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOD, digit_select_pin[0]);
    GPIO_ResetBits(GPIOD, digit_select_pin[1]);
    GPIO_ResetBits(GPIOD, digit_select_pin[2]);
    GPIO_ResetBits(GPIOD, digit_select_pin[3]);
    for (int i = 0; i < 8; i++) {

        GPIO_SetBits(GPIOD, segmentpins[i]);
    }
}

void delay(int cnt) {
  for(int i=0;i<cnt;i++) {}
}

/**
 * Get input from keypad
 * 
 * Program stucks in loop until it gets keypad input.
 */
char getKeypadInput() {
    while(1) {
        show_4_digit_forcode();
        GPIO_SetBits(GPIOA, keypad_out_pins[keypad_col_iter]);
        delay(10);
        
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
        delay(10);
        
        keypad_col_iter++;
        if(keypad_col_iter==3)
            keypad_col_iter=0;
    }
}

/**
 * Loop until the pressed key has released.
 */
void waitForKeyRelease(char keyin) {
    if(keyin == '1' || keyin == '2' || keyin == '3') {
      while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[0])) {show_4_digit_forcode();};
    }
    if(keyin == '4' || keyin == '5' || keyin == '6') {
        while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[1])){show_4_digit_forcode();};
    }
    if(keyin == '7' || keyin == '8' || keyin == '9') {
        while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[2])){show_4_digit_forcode();};
    }
    if(keyin == '*' || keyin == '0' || keyin == '#') {
        while(GPIO_ReadInputDataBit(GPIOA, keypad_in_pins[3])){show_4_digit_forcode();};
    }
}

/**
 * Change numeric character to int
 */
int charToInt(char c) {
    if(c=='*')
      return 10;
    else if(c=='#')
      return 11;
    else
      return c-'0';
}

/**
 * check if the input password is same as registered password
 */
int checkPassword(int pw) {
    for(int i=3;i>=0;i--) {
        if(code[i] != pw%10)
          return 0;
        pw/=10;
    }
    return 1;
}

/**
 * Display segment with given number seg
 */
void segment_out(uint8_t seg) {
    uint8_t temp = patterns[seg];
    for (int i = 7; i >=0; i--) {
        if ((temp & 0x01) == 0x01) {
            GPIO_ResetBits(GPIOD, segmentpins[i]);
        }
        else {
            GPIO_SetBits(GPIOD, segmentpins[i]);
        }
        temp = temp >> 1;
    }
}

/**
  * Display segment with given number and position
  */
void show_digit(uint16_t position, uint16_t number) {
    for (int i = 0; i < 4; i++) {
        if (position == i) {
            GPIO_SetBits(GPIOD, digit_select_pin[i]);
        }
        else {
            GPIO_ResetBits(GPIOD, digit_select_pin[i]);
        }
    }
    
    segment_out(number);
}

/*
void show_4_digit(uint16_t number) {
    int size;
    int numarr[4] = {0,0,0,0};
    if(number/1000 != 0)
      size=4;
    else if(number/100!=0)
      size=3;
    else if(number/10!=0)
      size=2;
    else
      size=1;
    numarr[3] = number / 1000;
    number = number % 1000;
    numarr[2] = number / 100;
    number = number % 100;
    numarr[1] = number / 10;
    numarr[0] = number % 10;
    for(int i=0;i<size;i++) {
        show_digit(i, numarr[i]);
        delay(10000);
    }
}*/

void show_4_digit_forcode() {
    for(int i=0;i<codecount;i++) {
        show_digit(i, code[i]);
        delay(10000);
    }
}

/**
  * Clear the display out
  */
void clear_segment() {
    GPIO_ResetBits(GPIOD, digit_select_pin[0]);
    GPIO_ResetBits(GPIOD, digit_select_pin[1]);
    GPIO_ResetBits(GPIOD, digit_select_pin[2]);
    GPIO_ResetBits(GPIOD, digit_select_pin[3]);
}

void move_door(int status) {
  uint16_t prescale = (uint16_t) (SystemCoreClock / 1000000) - 1;
  
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  TIM_TimeBaseStructure.TIM_Period = 20000-1; 
  TIM_TimeBaseStructure.TIM_Prescaler = prescale;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
  TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
  
  
  TIM_OCInitTypeDef TIM_OCInitStructure;
  
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_Pulse = status; // 1000 for open, 2000 for close
  TIM_OC3Init(TIM4, &TIM_OCInitStructure);
  TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Disable); // TODO : OC1
  TIM_ARRPreloadConfig(TIM4, ENABLE);
  TIM_Cmd(TIM4, ENABLE);
}

void wait_to_close() {
    uint16_t distance;
    GPIO_distance_configure();
    int closing_count=0;
    
    while(1){
        GPIOA->BSRR = 0x01000100;
        TIM2->CNT=0; while(TIM2->CNT<12);  // 12us delay
        GPIOA->BSRR = 0x01000000;
        while(!(GPIOA->IDR&0x0200));
        TIM2->CNT=0; while(GPIOA->IDR&0x0200);
        distance=(TIM2->CNT+1)/58;  // cm
        if(distance<100) {
          closing_count++;
        }
        else {
          closing_count=0;
        }
        if(closing_count>30) {
          GPIO_Configure();
          return;
        }
        printf("%d, %d\n", distance, closing_count);
    }
}

int main(void)
{
    RCC_Configure();
    

    GPIO_Configure();
    init_TIM2();
    
    move_door(CLOSE);

    char keyin;
   

    while(1){
        keyin = getKeypadInput();
        printf("%c\n", keyin);
        waitForKeyRelease(keyin);
        code[codecount++] = charToInt(keyin);
        if(codecount == 4) {
            keyin = getKeypadInput();
            waitForKeyRelease(keyin);
            if(checkPassword(PW_TEST)) {
                printf("open\n");
                move_door(OPEN);
                wait_to_close();
                move_door(CLOSE);
            }
            else {
                printf("wrong\n");
            }
            for(int i=0;i<4;i++) {
                code[i] = 0;
            }
            codecount = 0;
            clear_segment();
        }
    }
    
    return 0;
}