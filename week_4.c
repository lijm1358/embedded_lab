#include "stm32f10x.h"
#include <stdio.h>

#define DOWN 0x04
#define UP 0x20

// RCC 0x40021018
// PORT C 0x40011000
// PORT D 0x40011400
// PORT E 0x40011800 
#define RCC_APB2ENR *((volatile unsigned int*) 0x40021018)      // clock 설정용 레지스터

#define GPIOC_CRL *((volatile unsigned int*) 0x40011000)           // C 포트 설정(조이스틱)
#define GPIOC_IDR *((volatile unsigned int*) 0x40011008)            // C 포트 입력 받기(조이스틱)

#define GPIOD_CRH *((volatile unsigned int*) 0x40011404)          // D 포트 설정(스위치1. PD11)
#define GPIOD_IDR *((volatile unsigned int*) 0x40011408)            // D 포트 입력 받기(스위치1)

#define GPIOE_CRL *((volatile unsigned int*) 0x40011800)           // E포트 설정(PE2, 3 핀 출력)
#define GPIOE_BSRR *((volatile unsigned int*) 0x40011810)        // E포트 set, reset(PE2, 3 핀 in, out 설정)

#define D_ON 0x800                                                                          // 스위치 입력 들어왔는지 확인 용도

void delay() {
  int i;
  for (i=0; i<10000000; i++) {
  }
}

int main(void)
{
  // RCC_APB2ENR
  RCC_APB2ENR &= ~0x000000FF;
  RCC_APB2ENR |= (0x7<<4);
  
  // stick
  GPIOC_CRL &= 0x00000000;
  GPIOC_CRL = 0x00888800;
  
  //swtich
  GPIOD_CRH &= 0x00000000;
  GPIOD_CRH = 0x00008000;
  
  // out
  GPIOE_CRL &= ~0x0000FF00;
  GPIOE_CRL |= 0x00003300;      // PE2,3 에 신호
 
  while(1) {
    if(!( GPIOD_IDR & D_ON)) {
      GPIOE_BSRR &= ~0x000F0000; //  pe2, 3 끄기
      GPIOE_BSRR |= 0x000C0000;
      
      printf("on");
    }
    
    if(!( GPIOC_IDR & UP)) {
      GPIOE_BSRR &= ~0x0000000F; //  pe3 켜기
      GPIOE_BSRR |= 0x00000008;
      delay();
      GPIOE_BSRR &= ~0x000F0000;    // pe2 끄기
      GPIOE_BSRR |= 0x00040000;
      
      printf("up");
    }
    else if(!( GPIOC_IDR & DOWN)) {
      GPIOE_BSRR &= ~0x0000000F; //  pe2 켜기
      GPIOE_BSRR |= 0x00000004;
      delay();
      GPIOE_BSRR &= ~0x000F0000;    // pe3 끄기
      GPIOE_BSRR |= 0x00080000;
      
      printf("down");
    }
  }

  return 0;
}