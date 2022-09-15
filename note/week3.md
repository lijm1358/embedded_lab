## 1. 레지스터 접근
> n번째 bit는 0번째부터 시작. 즉, 2번째 bit의 의미는 오른쪽에서 세 칸째의 bit를 의미함.
* STM32 schematic(설계도) 확인을 통해 사용하고자 하는 부분의 핀 번호 확인
  * LED : PD2, PD3, PD4, PD7
  * Joystick : PC2, PC3, PC4, PC5
* Datasheet의 2.3 Overview를 확인하면 PC[2,3,4,5] 는 GPIO port C, PD[2,3,4,7]은 GPIO port D와 연결되어있는 것을 확인할 수 있음.
* 또한, Datasheet의 4 Memory Mapping을 확인하면 base주소가 각각 port C : `0x4001 1000`, port D : `0x4001 1400`, RCC : `0x4002 1000`임을 확인할 수 있음.
* 즉, LED관련 조작을 위한 레지스터는 `0x40011400 + offset`, 조이스틱 관련 조작을 위한 레지스터는 `0x40011000 + offset`을 통해 접근.
## 2. 레지스터 별 역할
* GPIO의 사용을 위해서는 1. 클락 설정 2. 포트 세팅 3. 포트에 값 설정 과정을 통해 진행(3주차 ppt)
* `RCC_APB2ENR` : 클락 설정용 레지스터
  * offset : `0x18`
  * reference manual 8.3.7 APB2 peripheral clock enable register 참고
  * `RCC_APB2ENR`에서, 각 bit별로 어떤 모듈에 클락을 줄 수 있는지 나와있고, 현재 GPIO port C와 D를 사용해야 하므로 4, 5번째 bit에 1bit를 설정해야함.
* `GPIO_CLR` : 포트 세팅을 위한 레지스터
  * offset : `0x00`
  * reference manual 9.2.1 Port configuration register low 참고
  * LED는 출력이므로 Output mode + General purpose output push-pull을 사용하고, Joystick은 입력이므로 Input mode + Input with pull-up / pull-down을 사용해야 함.
  * reference manual에 각 핀 번호 별 bit 위치가 나와있음. 즉, LED관련 조작을 위해서는 2, 3, 4, 7번 핀을 설정해야 함.
    * 1. LED의 base register `0x40011400` + offset `0x00`을 더한 `0x40011400`이 GPIO D에 대한 `GPIO_CLR`레지스터 주소값이 됨.
    * 2. 해당 레지스터에서 2, 3, 4, 7번핀 부분이 각각 [8~11], [12\~15], [16\~19], [28\~31]번째 bit이므로 해당 부분을 설정해줘야 함.
    * 3. output mode는 MODE부분 2bit를 0 이상, general purpose output push-pull은 CNF부분 2bit를 00으로 설정. 따라서 `*((volatile unsigned int*) 0x40011400) = 0x30033300;`으로 포트 설정 가능.
  * Joystick 관련 조작을 위해서는 2, 3, 4, 5번 핀을 설정해야 함.
    * 1. Joystick의 base register `0x40011000` + offset `0x00`을 더한 `0x40011000`이 GPIO C에 대한 `GPIO_CLR`레지스터 주소값이 됨.
    * 2. 해당 레지스터에서 2, 3, 4, 5번핀 부분이 각각 [8~11], [12\~15], [16\~19], [20\~23]번째 bit이므로 해당 부분을 설정해줘야 함.
    * 3. input mode는 MODE부분 2bit를 00, input with pull-up/pull-down은 CNF부분 2bit를 10으로 설정. 따라서 `*((volatile unsigned int*) 0x40011000) = 0x00888800;`으로 포트 설정 가능.
* `GPIO_IDR` : GPIO 입력을 통해 받은 값을 저장하는 레지스터
  * offset : `0x08`
  * reference manual 9.2.3 Port input data register 참고
  * joystick을 입력으로 받으므로, joystick의 각 방향별 입력을 받았을 때 2, 3, 4, 5번째 bit가 1로 세팅됨.
* `GPIO_BSRR` : GPIO 포트에 bit값 설정(led 불 켤때 사용)
  *  offset : `0x10`
  *  reference manual 9.2.5 Port bit set/reset register 참고
  *  led의 각 핀 번호가 2, 3, 4, 7이므로, `GPIO_BSRR`의 2, 3, 4, 7번째 bit에 1을 넣으면 해당 위치의 led가 켜짐.
* `GPIO_BRR` : GPIO 포트의 bit값 리셋(led 불 끌때 사용)
  * offset : `0x14`
  * reference manual 9.2.6 Port bit reset register 참고
  * led의 각 핀 번호가 2, 3, 4, 7이므로, `GPIO_BRR`의 2, 3, 4, 7번째 bit에 1을 넣으면 해당 위치의 led가 꺼짐.
## 3. 코드
```c
#include "stm32f10x.h"
#include <stdio.h>

// `GPIO_IDR`을 통해, 조이스틱을 어떤 방향으로 눌렀는 지 확인할 때 쓰는 bit.
#define DOWN 0x04   // 0b 0000 0100. 2번째 bit
#define LEFT 0x08   // 0b 0000 1000. 3번째 bit
#define RIGHT 0x10  // 0b 0001 0000. 4번째 bit
#define UP 0x20     // 0b 0010 0000. 5번째 bit
```

```c
int main(void)
{
  // RCC_APB2ENR. 클락 세팅.
  // RCC레지스터의 base주소는 0x4002 1000. offset은 0x18이므로, 이 둘을 더한 0x4002 1018로 레지스터 접근
  *((volatile unsigned int*) 0x40021018) &= ~0x000000FF;    // 레지스터 값 초기화. 4, 5번째 bit를 사용하므로 0~7번째 bit까지 0으로 초기화함
                                                            // (0x000000FF는 0~7번째 bit까지 전부다 1인 값이고, 앞에 ~을 붙였으므로 0~7번째 bit값이 전부다 0인 값으로 바뀜. 
                                                            // 이를 레지스터와 &연산하므로, RCC레지스터의 0~7번째 bit값이 전부 0으로 바뀌게 되고, 나머지는 기존 값 유지)
  *((volatile unsigned int*) 0x40021018) |= (0x3<<4);       // 0x3 << 4 == 0000 0011 << 4 == 0011 0000. 이 값을 or 연산하므로 RCC레지스터의 4, 5번째 bit를 1로 설정 
```

```c
  // GPIO_CRL (포트 세팅)
  // led
  // led가 포함된 GPIO port D의 레지스터 base주소는 0x4001 1400. offset은 0x00이므로 이 둘을 더한 0x4001 1400으로 레지스터 접근
  *((volatile unsigned int*) 0x40011400) &= 0x00000000; // GPIO_CRL의 bit를 모두 0으로 초기화
  *((volatile unsigned int*) 0x40011400) = 0x30033300;  // 2, 3, 4, 7번 핀의 설정을 전부 0x3 (0011)로 설정.(위 설명 참고)
  
  // stick
  // stick이 포함된 GPIO port C의 레지스터 base주소는 0x4001 1000. offset은 0x00이므로 이 둘을 더한 0x4001 1000으로 레지스터 접근
  *((volatile unsigned int*) 0x40011000) &= 0x00000000; // GPIO_CRL의 bit를 모두 0으로 초기화
  *((volatile unsigned int*) 0x40011000) = 0x00888800;  // 2, 3, 4, 5번 핀의 설정을 전부 0x8 (1000)로 설정.(위 설명 참고)
  
  *((volatile unsigned int*) 0x40011008) = 0x00000000;  // GPIO_IDR레지스터의 bit를 모두 0으로 초기화
```

```c
 // 코드를 실행시키면 그냥 1라인씩 쭉 실행한 뒤에 종료되어버리므로, 조이스틱 입력을 계속 받을 수 있도록 무한루포.
  while(1) {
    // joystick이 포함된 GPIO port C의 레지스터 base주소는 0x4001 1000. joystick의 입력을 받는 GPIO_IDR 레지스터의 offset은 0x08이므로 이 둘을 더한 0x4001 1008로 GPIO_IDR 접근 가능.
    // joystick을 위로 올리면 PC5번 핀의 입력이 들어가므로, GPIO_IDR의 5번째 bit가 1이 됨.
    // 따라서, joystick을 위로 올린 상태에서의 GPIO_IDR레지스터와 UP(0b 0010 0000)을 and연산하게 되면 ... 0010 0000이 됨. (만약, 조이스틱을 조작하지 않거나 다른 방향으로 놓은 상태에서 and 연산 시 0이 나옴)
    if(!( *((volatile unsigned int*) 0x40011008) & UP)) {
      // GPIO_BSRR (불 켜기)
      // led가 포함된 GPIO port D의 레지스터 base주소는 0x4001 1400. 불을 켜기 위한 GPIO_BSRR의 offset은 0x10이므로 이 둘을 더한 0x4001 1410으로 GPIO_BSRR 접근 가능.
      *((volatile unsigned int*) 0x40011410) &= ~0x00000004;    // 잘못 설정함. 없어도 됨 (이렇게 하면 2번째 bit만 0으로 초기화)
      *((volatile unsigned int*) 0x40011410) |= ( 0x0C );       // joystick을 위로 올리면 LED1, 2를 켜야 함. LED1, 2는 각각 PD2, PD3 핀을 사용.
                                                                // 즉, GPIO_BSRR의 2, 3번째 bit를 1로 설정해야 함.
                                                                // 0x0C == 0b 0000 1100이므로, 이 값을 GPIO_BSRR와 or 연산하면 2, 3번째 bit를 1로 만들 수 있음.
      printf("up");
    }

    // ...

    else if(!( *((volatile unsigned int*) 0x40011008) & RIGHT)) {
      // GPIO_BSRR (불 켜기)
      *((volatile unsigned int*) 0x40011410) &= ~0x00000004;    // 여기까지는 위와 동일
      *((volatile unsigned int*) 0x40011410) |= ( 0x90 );       // joystick을 오른쪽으로 놓으면 LED3, 4를 켜야 함. LED3, 4는 각각 PD4, PD7 핀을 사용.
                                                                // 즉, GPIO_BSRR의 4, 7번째 bit를 1로 설정해야 함.
                                                                // 0x90 == 0b 1001 0000이므로, 이 값을 GPIO_BSRR와 or 연산하면 4, 7번째 bit를 1로 만들 수 있음.
      printf("right");
    }

    // ...

  }
     
  return 0;
}
```