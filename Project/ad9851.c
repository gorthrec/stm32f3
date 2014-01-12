#include "main.h"
#include "ad9851.h"

#define AD9851_GPIO_CLK_PORT GPIOD
#define AD9851_GPIO_CLK_PIN GPIO_Pin_13
#define AD9851_GPIO_CLK_CLK RCC_AHBPeriph_GPIOD

#define AD9851_GPIO_DATA_PORT GPIOD
#define AD9851_GPIO_DATA_PIN GPIO_Pin_11
#define AD9851_GPIO_DATA_CLK RCC_AHBPeriph_GPIOD

#define AD9851_GPIO_FQ_PORT GPIOD
#define AD9851_GPIO_FQ_PIN GPIO_Pin_15
#define AD9851_GPIO_FQ_CLK RCC_AHBPeriph_GPIOD

#define AD9851_GPIO_RESET_PORT GPIOC
#define AD9851_GPIO_RESET_PIN GPIO_Pin_6
#define AD9851_GPIO_RESET_CLK RCC_AHBPeriph_GPIOC

static int ad9851_initialized = 0;

void ad9851_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable ports */
    RCC_AHBPeriphClockCmd(AD9851_GPIO_CLK_CLK | AD9851_GPIO_DATA_CLK | AD9851_GPIO_FQ_CLK | AD9851_GPIO_RESET_CLK, ENABLE);

    /* Configure outputs */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin =  AD9851_GPIO_CLK_PIN;
    GPIO_Init(AD9851_GPIO_CLK_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  AD9851_GPIO_DATA_PIN;
    GPIO_Init(AD9851_GPIO_DATA_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  AD9851_GPIO_FQ_PIN;
    GPIO_Init(AD9851_GPIO_FQ_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin =  AD9851_GPIO_RESET_PIN;
    GPIO_Init(AD9851_GPIO_RESET_PORT, &GPIO_InitStructure);
}

/* Reset AD9851 */
void ad9851_reset(void)
{
    AD9851_GPIO_RESET_PORT->BSRR = AD9851_GPIO_RESET_PIN;
    AD9851_GPIO_RESET_PORT->BRR = AD9851_GPIO_RESET_PIN;
    vTaskDelay(1);

    /* Enable serial mode */
    AD9851_GPIO_CLK_PORT->BSRR = AD9851_GPIO_CLK_PIN;
    AD9851_GPIO_CLK_PORT->BRR = AD9851_GPIO_CLK_PIN;
    AD9851_GPIO_FQ_PORT->BSRR = AD9851_GPIO_FQ_PIN;
    AD9851_GPIO_FQ_PORT->BRR = AD9851_GPIO_FQ_PIN;
    vTaskDelay(1);
}

/* Set frequency and phase of output signal
    frequency in Hz
    phase in deg
*/
void ad9851_set(int frequency, int phase)
{
    int control;
    int i;

    if (!ad9851_initialized) {
        ad9851_init();
        ad9851_reset();
        ad9851_initialized = 1;
    }

    control = frequency * 4294967296.0 / 120.0e6;
    for (i = 0; i < 32; i++) {
        if (control & (1 << i)) {
            AD9851_GPIO_DATA_PORT->BSRR = AD9851_GPIO_DATA_PIN;
        } else {
            AD9851_GPIO_DATA_PORT->BRR = AD9851_GPIO_DATA_PIN;
        }
        /* clock pulse */
        AD9851_GPIO_CLK_PORT->BSRR = AD9851_GPIO_CLK_PIN;
        AD9851_GPIO_CLK_PORT->BRR = AD9851_GPIO_CLK_PIN;
    }

    control = (((phase / 32) & 0x1f) << 3) | (0 << 2) | (0 << 1) | (1 << 0); /* phase | power_down | serial mode | x6 mode */
    for (i = 0; i < 8; i++) {
        if (control & (1 << i)) {
            AD9851_GPIO_DATA_PORT->BSRR = AD9851_GPIO_DATA_PIN;
        } else {
            AD9851_GPIO_DATA_PORT->BRR = AD9851_GPIO_DATA_PIN;
        }
        /* clock pulse */
        AD9851_GPIO_CLK_PORT->BSRR = AD9851_GPIO_CLK_PIN;
        AD9851_GPIO_CLK_PORT->BRR = AD9851_GPIO_CLK_PIN;
    }

    AD9851_GPIO_FQ_PORT->BSRR = AD9851_GPIO_FQ_PIN;
    AD9851_GPIO_FQ_PORT->BRR = AD9851_GPIO_FQ_PIN;
}
