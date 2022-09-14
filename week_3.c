#include "stm32f10x.h"
#include <stdio.h>

#define DOWN 0x04
#define LEFT 0x08
#define RIGHT 0x10
#define UP 0x20

int main(void)
{
  // RCC_APB2ENR
  *((volatile unsigned int*) 0x40021018) &= ~0x000000FF;
  *((volatile unsigned int*) 0x40021018) |= (0x3<<4);
    
  // GPIO_CRL (포트 세팅)
  // led
  *((volatile unsigned int*) 0x40011400) &= 0x00000000;
  *((volatile unsigned int*) 0x40011400) = 0x30033300;
  
  // stick
  *((volatile unsigned int*) 0x40011000) &= 0x00000000;
  *((volatile unsigned int*) 0x40011000) = 0x00888800;
  
  *((volatile unsigned int*) 0x40011008) = 0x00000000;
  
  while(1) {
    if(!( *((volatile unsigned int*) 0x40011008) & UP)) {
      // GPIO_BSRR (불 켜기)
      *((volatile unsigned int*) 0x40011410) &= ~0x00000004;
      *((volatile unsigned int*) 0x40011410) |= ( 0x0C );
      printf("up");
      // *((volatile unsigned int*) 0x40011008) &= ~0x0000FFFF;
    }
    else if(!( *((volatile unsigned int*) 0x40011008) & DOWN)) {
      // GPIO_BSRR (불 켜기)
      *((volatile unsigned int*) 0x40011414) &= ~0x00000004;
      *((volatile unsigned int*) 0x40011414) |= ( 0x0C );
      printf("down");
    }
    else if(!( *((volatile unsigned int*) 0x40011008) & RIGHT)) {
      // GPIO_BSRR (불 켜기)
      *((volatile unsigned int*) 0x40011410) &= ~0x00000004;
      *((volatile unsigned int*) 0x40011410) |= ( 0x90 );
      printf("right");
    }
    else if(!( *((volatile unsigned int*) 0x40011008) & LEFT)) {
      // GPIO_BSRR (불 켜기)
      *((volatile unsigned int*) 0x40011414) &= ~0x00000004;
      *((volatile unsigned int*) 0x40011414) |= ( 0x90 );
      printf("left");
    }
  }
     
  return 0;
}