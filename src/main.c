#include "debug.h"

#define PERIOD  5000 // 5 sec.

#define PULLDOWN_PINS

static void UART_Flush(void) {
    while (! USART_GetFlagStatus(USART1, USART_FLAG_TC)) {}
}

static void delay(uint16_t ms) {
    SysTick->CMP = SystemCoreClock / 1000 * ms - 1;
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CTLR = (1 << 2) | (1 << 0); // STCLK | STE
    while (! (SysTick->SR & 0x01)) {}
    SysTick->CTLR = 0;
}

static void light_sleep(uint16_t ms) {
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CMP = SystemCoreClock / 1000 * ms - 1;
    SysTick->SR = 0;
    SysTick->CNT = 0;
    SysTick->CTLR = (1 << 2) | (1 << 1) | (1 << 0); // STCLK | STIE | STE
    while (SysTick->CNT < SysTick->CMP) {
        __WFI();
    }
    SysTick->CTLR = 0;
}

static void deep_sleep(uint16_t ms) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {}

    {
        const EXTI_InitTypeDef EXTI_InitStructure = {
            .EXTI_Line = EXTI_Line9,
            .EXTI_Mode = EXTI_Mode_Event,
            .EXTI_Trigger = EXTI_Trigger_Falling,
            .EXTI_LineCmd = ENABLE
        };

        EXTI_Init((EXTI_InitTypeDef*)&EXTI_InitStructure);
    }

    if (ms <= 64) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_128);
    } else if (ms <= 128) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_256);
        ms >>= 1;
    } else if (ms <= 256) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_512);
        ms >>= 2;
    } else if (ms <= 512) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_1024);
        ms >>= 3;
    } else if (ms <= 1024) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_2048);
        ms >>= 4;
    } else if (ms <= 2048) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_4096);
        ms >>= 5;
    } else if (ms <= 5120) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_10240);
        ms /= 80;
    } else if (ms <= 30720) {
        PWR_AWU_SetPrescaler(PWR_AWU_Prescaler_61440);
        ms /= 480;
    }
    PWR_AWU_SetWindowValue(ms - 1);
    PWR_AutoWakeUpCmd(ENABLE);
#if (SDI_PRINT == SDI_PR_CLOSE)
    UART_Flush();
#endif
#ifdef PULLDOWN_PINS
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);

    {
        const GPIO_InitTypeDef GPIO_InitStructure = {
            .GPIO_Pin = GPIO_Pin_All,
            .GPIO_Mode = GPIO_Mode_IPD
        };

        GPIO_Init(GPIOA, (GPIO_InitTypeDef*)&GPIO_InitStructure);
        GPIO_Init(GPIOC, (GPIO_InitTypeDef*)&GPIO_InitStructure);
        GPIO_Init(GPIOD, (GPIO_InitTypeDef*)&GPIO_InitStructure);
    }
#endif
    PWR_EnterSTANDBYMode(PWR_STANDBYEntry_WFE);

#ifdef PULLDOWN_PINS
//    RCC_APB2PeriphResetCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, ENABLE);
    {
        const GPIO_InitTypeDef GPIO_InitStructure = {
            .GPIO_Pin = GPIO_Pin_All,
            .GPIO_Mode = GPIO_Mode_IN_FLOATING
        };

        GPIO_Init(GPIOA, (GPIO_InitTypeDef*)&GPIO_InitStructure);
        GPIO_Init(GPIOC, (GPIO_InitTypeDef*)&GPIO_InitStructure);
        GPIO_Init(GPIOD, (GPIO_InitTypeDef*)&GPIO_InitStructure);
    }

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD, DISABLE);
#endif
    SystemInit();
//    SystemCoreClockUpdate();
    PWR_AutoWakeUpCmd(DISABLE);
#if (SDI_PRINT == SDI_PR_OPEN)
    SDI_Printf_Enable();
#else
    USART_Printf_Init(115200);
#endif
}

int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();

#if (SDI_PRINT == SDI_PR_OPEN)
    SDI_Printf_Enable();
#else
    USART_Printf_Init(115200);
#endif

    while(1) {
        printf("Normal mode\n");
        delay(PERIOD);
        printf("Sleep mode\n");
        light_sleep(PERIOD);
        printf("Deep sleep mode\n");
        deep_sleep(PERIOD);
    }
}

void __attribute__((interrupt("WCH-Interrupt-fast"))) SysTick_Handler(void) {
    SysTick->SR = 0;
}
