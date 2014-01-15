#include "main.h"

static int initialized = 0;

void rotary_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    /* connect PC6 and PC7 to TIM8_CH1, and TIM8_CH2 */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    /* alternate TIM8 CH1, TIM8 CH2 */
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_4);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_4);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Period = 0xffff;
    TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_EncoderInterfaceConfig(TIM8, TIM_EncoderMode_TI1, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM8, ENABLE);
}

int rotary_get(void)
{
    if (!initialized) {
        rotary_init();
        initialized = 1;
    }

    return TIM_GetCounter(TIM8) >> 1;
}
