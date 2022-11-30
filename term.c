#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "lcd.h"
#include "touch.h"
#include "misc.h"
#include "main.h"

#define GPIO_PORT GPIOD
#define OPEN 1000
#define CLOSE 2000
#define MAX_BUFF 15

/* function prototype */
void RCC_Configure(void);
void GPIO_Configure(void);
void NVIC_Configure(void);
void ADC_Configure(void);
void Delay(void);
void show_4_digit_forcode();
void clear_segment(void);
void lockdown_mode(void);
void cam_init(void);
void UART_BulkOut(uint32_t, char *);

/*int keypad_col[3] = {3,1,5};*/
/*int keypad_row[4] = {2,7,6,4};*/
// Keypad output for keypad input iteration
uint16_t keypad_out_pins[3] = {
    GPIO_Pin_7,
    GPIO_Pin_8,
    GPIO_Pin_9,
};
// Keypad input
uint16_t keypad_in_pins[4] = {
    GPIO_Pin_10,
    GPIO_Pin_11,
    GPIO_Pin_12,
    GPIO_Pin_13,
};
// Keypad mapping with GPIO input and integer number
uint8_t keypad_map[3][4] = { {'1', '4', '7', '*'}, 
                             {'2', '5', '8', '0'},
                             {'3', '6', '9', '#'} };
// keypad column iterator index
int keypad_col_iter=0;

// Password constant for test
int PW_TEST = 1234;

// External password input array
int code[4]={0,0,0,0};
int codecount=0; // Code length of externel password input
int shockcount=0; // Shock counter for lockdown mode
int wrongcount=0; // wrong password input counter for lockdown mode
int lockmode=0;

int threshold = 100;// distance threshold to determine closed state

int picture_mode = 0; //1 to bmp, 0 to jpeg

char usartIn[MAX_BUFF];

int unlock = 0;

// segment patterns for 0 to 9
uint8_t patterns[] = { 0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE4, 0xFE, 0xE6};

// segment module connection pin a, b, c, d, e, f, g, dp
uint16_t segmentpins[] = {  GPIO_Pin_8,  GPIO_Pin_9, GPIO_Pin_10, GPIO_Pin_11, 
                           GPIO_Pin_12, GPIO_Pin_13, GPIO_Pin_14, GPIO_Pin_15 };

// segment module digit selection pin
uint16_t digit_select_pin[] = { GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_2, GPIO_Pin_3 };


/**************************************
 * Initializer for RCC, TIM, GPIO, etc.
 *
 * can be used at main start or in function
 **************************************/ 
void RCC_Configure(void) // stm32f10x_rcc.h Au°i
{
    // clock for ultrasonic distance
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    // clock for keypad
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

    // Alternate Function IO clock enable 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    // clock for servo motor
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    // clock for seven segment
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    
    /* UART1, 2 TX/RX port clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
}

/*
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
}*/

void init_TIM2(void){  // Output compare mode, PWM
    /**
      * TIM2 Initializer for hypersonic distance sensor
      */
    RCC->APB1ENR |= (1<<0); // Bit 0 TIM2EN
 
    TIM2->PSC = 72-1;   // 1us
    TIM2->EGR = (1<<0); // Bit 0 UG
    TIM2->CR1 = (1<<0); // Bit 0 CEN
}

void GPIO_distance_configure(void) {
    /**
     * GPIO Initializer for hypersonic distance sensor
     * 
     * Called when the locker door is opened. other GPIO settings will be deinitialized.
     * DOES NOT comparable with GPIO_Configure.
     */
  /* ultrasonic distance */
  GPIO_DeInit(GPIOA);
    GPIOA->CRH = (GPIOA->CRH&0xFFFFFF00)|(3<<0*4)|(4<<1*4); // PA8 Trig, PA9 Echo
}

void GPIO_Configure(void)
{
    /**
     * Universal GPIO settings
     */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_DeInit(GPIOA);
    GPIO_DeInit(GPIOE);
    
    /* keypad */
    
    GPIO_InitStructure.GPIO_Pin     = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9; // keypad 3, 1, 5th pin for output(v out)
    GPIO_InitStructure.GPIO_Mode    = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    // set column output to 0 
    for(int i=0;i<3;i++) {
        GPIO_ResetBits(GPIOA, keypad_out_pins[i]);
    }

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13; // keypad 2, 7, 6, 4th pin for input(v in)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOE,&GPIO_InitStructure);
    
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
    
    /* shock sensor*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
    
    //USART1 TX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    //USART1 RX
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    //USART2 TX(PA2)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    //USART2 RX(PA3)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD | GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void USART1_Init(void)
{
    USART_InitTypeDef USART1_InitStructure;

    USART_Cmd(USART1, ENABLE);

    USART1_InitStructure.USART_BaudRate = 57600;
    USART1_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART1_InitStructure.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);
    USART1_InitStructure.USART_Parity = USART_Parity_No;
    USART1_InitStructure.USART_StopBits = USART_StopBits_1;
    USART1_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART1_InitStructure);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
}

void USART2_Init(void)
{
    USART_InitTypeDef USART2_InitStructure;

    USART_Cmd(USART2, ENABLE);

    USART2_InitStructure.USART_BaudRate = 57600;
    USART2_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART2_InitStructure.USART_Mode = (USART_Mode_Rx | USART_Mode_Tx);
    USART2_InitStructure.USART_Parity = USART_Parity_No;
    USART2_InitStructure.USART_StopBits = USART_StopBits_1;
    USART2_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART2, &USART2_InitStructure);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
}

void EXTI_Configure(void)
{
    /**
     * EXTI Configuration for shock detection
     */
    EXTI_InitTypeDef EXTI_InitStructure;
	
    /* shock */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6);
    EXTI_InitStructure.EXTI_Line = EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void EXTI_Disable(void) {
    /**
     * Disable EXTI for shock detection
     */
    EXTI_InitTypeDef EXTI_InitStructure;
	
    /* shock */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6);
    EXTI_InitStructure.EXTI_Line = EXTI_Line6;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void NVIC_Configure(void) {
    /**
     * NVIC Configuration for interrupt
     */
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
    // Shock Detected
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // UART1
    NVIC_EnableIRQ(USART1_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // UART2
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void EXTI9_5_IRQHandler(void) {
   /**
    * Interrupt handler after the shock detection
    */
  if (EXTI_GetITStatus(EXTI_Line6) != RESET) {
            printf("shock\n");
            shockcount++;
  }
  EXTI_ClearITPendingBit(EXTI_Line6);
  if(shockcount>30)
    lockdown_mode();
}
uint8_t idx=0;
/*
void USART1_IRQHandler() {
    uint16_t word;
    
    if(USART_GetITStatus(USART1, USART_IT_RXNE)!=RESET){
        word = USART_ReceiveData(USART1);
        USART_SendData(USART2, word);
    	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}*/

void USART2_IRQHandler() {
    uint16_t word;
    if(USART_GetITStatus(USART2, USART_IT_RXNE)!=RESET){
        word = USART_ReceiveData(USART2);
        if(word == 'c')
            unlock = 1;
        USART_SendData(USART1, word);

    	USART_ClearITPendingBit(USART2,USART_IT_RXNE);
    }
}

/*
void USART1_IRQHandler() {
    uint16_t word;
    if(USART_GetITStatus(USART1, USART_IT_RXNE)!=RESET){
        word = USART_ReceiveData(USART1);
        USART_SendData(USART2, word);
    	USART_ClearITPendingBit(USART1,USART_IT_RXNE);
    }
}*/

void delay(int cnt) {
  /**
   * Pseudo delay
   */
  for(int i=0;i<cnt;i++) {}
}


/**************************************
 * Fuctions for keypad input
 *
 * contains digit code output function for password output
 * TODO : set timer for 7 segment output
 **************************************/ 

char getKeypadInput() {
    /**
     * Get input from keypad
     * 
     * Program stucks in loop until it gets keypad input.
     */
    while(1) {
        show_4_digit_forcode();
        GPIO_SetBits(GPIOE, keypad_out_pins[keypad_col_iter]);
        delay(10);
        
        if(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[0])) {
            return keypad_map[keypad_col_iter][0];
        }
        else if(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[1])) {
            return keypad_map[keypad_col_iter][1];
        }
        else if(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[2])) {
            return keypad_map[keypad_col_iter][2];
        }
        else if(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[3])) {
            return keypad_map[keypad_col_iter][3];
        }
        
        GPIO_ResetBits(GPIOE, keypad_out_pins[keypad_col_iter]);
        delay(10);
        
        keypad_col_iter++;
        if(keypad_col_iter==3)
            keypad_col_iter=0;
    }
}

void waitForKeyRelease(char keyin) {
    /**
     * Loop until the pressed key has released.
     */
    if(keyin == '1' || keyin == '2' || keyin == '3') {
      while(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[0])) {show_4_digit_forcode();};
    }
    if(keyin == '4' || keyin == '5' || keyin == '6') {
        while(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[1])){show_4_digit_forcode();};
    }
    if(keyin == '7' || keyin == '8' || keyin == '9') {
        while(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[2])){show_4_digit_forcode();};
    }
    if(keyin == '*' || keyin == '0' || keyin == '#') {
        while(GPIO_ReadInputDataBit(GPIOE, keypad_in_pins[3])){show_4_digit_forcode();};
    }
}

int charToInt(char c) {
    /**
     * Change numeric character to int
     */
    if(c=='*')
      return 10;
    else if(c=='#')
      return 11;
    else
      return c-'0';
}

int checkPassword(int pw) {
    /**
     * check if the input password is same as registered password
     */
    for(int i=3;i>=0;i--) {
        if(code[i] != pw%10)
          return 0;
        pw/=10;
    }
    return 1;
}

/**************************************
 * Fuctions for segment output
 **************************************/
void segment_out(uint8_t seg) {
    /**
     * Display segment with given number seg
     */
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

void show_digit(uint16_t position, uint16_t number) {
    /**
      * Display segment with given number and position
      */
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
    /**
     * Show 4 digit password code to 7 segment
     * TODO : don't use
     */
    for(int i=0;i<codecount;i++) {
        show_digit(i, code[i]);
        delay_ms(1);
    }
}

void clear_segment() {
     /**
      * Clear the display out
      */
    GPIO_ResetBits(GPIOD, digit_select_pin[0]);
    GPIO_ResetBits(GPIOD, digit_select_pin[1]);
    GPIO_ResetBits(GPIOD, digit_select_pin[2]);
    GPIO_ResetBits(GPIOD, digit_select_pin[3]);
}

/**************************************
 * Fuctions for door open and close
 **************************************/
void move_door(int status) {
   /**
    * Function to move door using step motor
    * 
    * status : OPEN for open door, CLOSE to close door
    */
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
   /**
    * Function to wait until the door closed
    * 
    * The door open/close state is determined with ultrasonic distance sensor
    * the sensor does not read distance right after the lock has released
    * when the distance reached out of the threshold, it begins to read distance
    */
    uint16_t distance;
    GPIO_distance_configure();
    EXTI_Disable();
    int closing_count=0;
    uint8_t opened = 0;
    clear_segment();
    
    
    
    while(1){
        GPIOA->BSRR = 0x01000100;
        TIM2->CNT=0; while(TIM2->CNT<12);  // 12us delay
        GPIOA->BSRR = 0x01000000;
        while(!(GPIOA->IDR&0x0200));
        TIM2->CNT=0; while(GPIOA->IDR&0x0200);
        distance=(TIM2->CNT+1)/58;  // cm
        if(distance>threshold)
            opened = 1;
        if(opened && distance<threshold) {
          closing_count++;
        }
        else {
          closing_count=0;
        }
        if(closing_count>30) {
            GPIO_Configure();
            EXTI_Configure();
            NVIC_Configure();
            
            UART_BulkOut(7, "closees");
            
            wrongcount = 0;
            shockcount = 0;
            
            return;
        }
        printf("%d, %d\n", distance, closing_count);
    }
}

void lockdown_mode() {
    EXTI_Disable();
    unlock = 0;
    
    printf("lockdown!\n");
    UART_BulkOut(10, "lockdownes");
    
    delay_ms(1000);
    
    cam_init();
    OV2640_set_JPEG_size(OV2640_160x120);
	printf("ACK CMD switch to OV2640_160x120\r\n");
    delay_ms(1000);
    if(picture_mode == 0) {
        SingleCapTransfer();
        SendbyUSART1(); 
        UART_BulkOut(2, "es");
    } else if(picture_mode == 1) {
        StartBMPcapture();
    }

    clear_segment();
    
    while(!unlock) {};
    
    unlock = 0;
    printf("unlock\n");
    
    wrongcount=0;
    shockcount=0;
    EXTI_Configure();
}

void UART_BulkOut(uint32_t len, char *p)
{
	uint32_t	cnt =0;
	
	for(cnt=0;cnt!=len;cnt++)
	{	    
		while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        //printf("%c", *p);
		USART_SendData(USART2, *p);
        delay_us(100);
		p++;    
	}
}

void cam_init(void) {
    uint8_t vid, pid, temp ;
    
    ArduCAM_LED_init();
	ArduCAM_CS_init();
	sccb_bus_init();
	SPI1_Init();

    while(1)
    {
        write_reg(ARDUCHIP_TEST1, 0x55);
        temp = read_reg(ARDUCHIP_TEST1);
        if (temp != 0x55)
        {
            printf("ACK CMD SPI interface Error!\n");
            delay_ms(1000);
            continue;
        }
        else
        {
            printf("ACK CMD SPI interface OK!\r\n");
            break;
        }
    }
    while(1)
    {
        sensor_addr = 0x60;
        wrSensorReg8_8(0xff, 0x01);
        rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
        rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
        if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 )))
            printf("ACK CMD Can't find OV2640 module!\r\n");
        else
        {
          sensor_model =  OV2640 ;
          printf("ACK CMD OV2640 detected.\r\n");   
          break;
        }
    }  
    ArduCAM_Init(sensor_model);
    
    if(picture_mode == 0) {
        set_format(JPEG);
        ArduCAM_Init(sensor_model);
        #if !defined(OV2640)
            set_bit(ARDUCHIP_TIM,VSYNC_MASK);
        #endif
    } else if(picture_mode == 1) {
        set_format(BMP);
        ArduCAM_Init(sensor_model);
        if(sensor_model != OV2640)
        {
            clear_bit(ARDUCHIP_TIM, VSYNC_LEVEL_MASK);
        }
        wrSensorReg16_8(0x3818,0x81);
        wrSensorReg16_8(0x3621,0xa7);		
        printf("ACK CMD SetToBMP \r\n");
    }
}

int main(void)
{
    SystemInit();
    
    RCC_Configure();
    
    GPIO_Configure();
    
    EXTI_Configure();
    NVIC_Configure();
    delay_init();
    USART1_Init();
    
    USART2_Init();
    
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
                UART_BulkOut(8, "openes");
                move_door(OPEN);
                shockcount=0;
                wait_to_close();
                move_door(CLOSE);
            }
            else {
                printf("wrong\n");
                wrongcount++;
                printf("wrongcount=%d", wrongcount);
                if(wrongcount==3)
                  lockdown_mode();
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