#include "main.h"

#define MAX_SIN_SLOTS 30
#define SIN_OUTPUT_AMP 2000 /* [mV] */
#define BUFFER_SIZE 1024
#define DAC_DHR12R1_ADDRESS   0x40007414

#define PI (float)3.14159265f

static int sin_frequency[MAX_SIN_SLOTS] = {0};
static int sin_amplitude[MAX_SIN_SLOTS] = {0};
static int sin_counter = 0;
static uint16_t Input_Buffer[BUFFER_SIZE];

void do_sin_init(char *args)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    int i;

    for (i = 0; i < MAX_SIN_SLOTS; i++) {
        sin_frequency[i] = 0;
        sin_amplitude[i] = 0;
    }

    /*********************************************************************/
    /* DMA2 clock enable (to be used with DAC) */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2, ENABLE);

    /* DAC Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

    /* Enable PortA */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    /* Configure DAC output PA.4 */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*********************************************************************/
    /* TIM2 Periph clock enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    /* Time base configuration */
    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    //TIM_TimeBaseStructure.TIM_Period = 0xff;
    TIM_TimeBaseStructure.TIM_Period = 10000; /* (1/SystemCoreClock) * period = 1/72MHz * period */
    TIM_TimeBaseStructure.TIM_Prescaler = 0x0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

    /* TIM2 TRGO selection */
    TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);

    /* TIM2 enable counter */
    TIM_Cmd(TIM2, ENABLE);

    _dprintf("Sinus mesh table initialized\r\n");
}

void DMA2_Channel4_IRQHandler(void)
{
    int i;

    /* Test on DMA Stream Half Transfer interrupt */
    if (DMA_GetITStatus(DMA2_IT_HT4)) {
        /* Update data buffer */
        for (i = 0; i < (BUFFER_SIZE / 2); i++) {
            //Input_Buffer[i] = sin_counter++;
            Input_Buffer[i] = 2048 + 2048 * sin(2 * PI * sin_counter++ / BUFFER_SIZE);
        }

        /* Clear DMA Stream Half Transfer interrupt pending bit */
        DMA_ClearITPendingBit(DMA2_IT_HT4);
    }

    /* Test on DMA Stream Transfer Complete interrupt */
    if (DMA_GetITStatus(DMA2_IT_TC4)) {
        /* Update data buffer */
        for (i = BUFFER_SIZE / 2; i < BUFFER_SIZE; i++) {
            //Input_Buffer[i] = sin_counter++;
            Input_Buffer[i] = 2048 + 2048 * sin(2 * PI * sin_counter++ / BUFFER_SIZE);
        }

        /* Clear DMA Stream Transfer Complete interrupt pending bit */
        DMA_ClearITPendingBit(DMA2_IT_TC4);
    }
}

void do_sin(char *args)
{
    DAC_InitTypeDef DAC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;

    int slot = atoi(_strtok(NULL, " ,.-"));
    int frequency = _atoi(_strtok(NULL, " ,.-"));
    int i;
    int count;
    int amp;

    _dprintf("slot: %d\r\n", slot);
    _dprintf("freq: %d\r\n", frequency);

    if ((slot < 0) || (slot >= MAX_SIN_SLOTS)) {
        for (i = 0; i < MAX_SIN_SLOTS; i++) {
            _dprintf("[Slot %d] frequency = %dHz, amp = %d\r\n", i, sin_frequency[i], sin_amplitude[i]);
        }
        return;
    }

    if (frequency < 0) {
        _dprintf("[Slot %d] frequency = %dHz, amp = %d\r\n", slot, sin_frequency[slot], sin_amplitude[slot]);
        return;
    }

    if (frequency > 10000000) {
        _dprintf("Frequency too high. Max = 10000000\r\n");
        return;
    }

    sin_frequency[slot] = frequency;

    /* get active number of slots */
    count = 0;
    for (i = 0; i < MAX_SIN_SLOTS; i++) {
        if (sin_frequency[slot] > 0) {
            count++;
        }
    }
    /* recalculate amplitude for every slot */
    amp = SIN_OUTPUT_AMP / count;
    for (i = 0; i < MAX_SIN_SLOTS; i++) {
        if (sin_frequency[slot] > 0) {
            sin_amplitude[i] = amp;
        }
    }

    for (i = 0; i < BUFFER_SIZE; i++) {
        //Input_Buffer[i] = 4096 / i;
        //Input_Buffer[i] = 2048 + 2048 * sin(2 * PI * i * frequency / 1024); /* i in rad */
        Input_Buffer[i] = 2048 + 2048 * sin(2 * PI * sin_counter / BUFFER_SIZE);
        //Input_Buffer[i] = i;
    }
    sin_counter = BUFFER_SIZE;

    /**********************************************************************************/
    /* DAC channel2 Configuration */
    DAC_DeInit();
    DAC_InitStructure.DAC_Trigger = DAC_Trigger_T2_TRGO;
    DAC_InitStructure.DAC_WaveGeneration = DAC_WaveGeneration_None;
    DAC_InitStructure.DAC_LFSRUnmask_TriangleAmplitude = DAC_LFSRUnmask_Bits11_0;
    DAC_InitStructure.DAC_OutputBuffer = DAC_OutputBuffer_Enable;

    /* DAC Channel2 Init */
    DAC_Init(DAC_Channel_2, &DAC_InitStructure);

    /* Enable DAC Channel2 */
    DAC_Cmd(DAC_Channel_2, ENABLE);

    /* DMA2 channel4 configuration */
    DMA_DeInit(DMA2_Channel4);
    DMA_InitStructure.DMA_PeripheralBaseAddr = DAC_DHR12R1_ADDRESS;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)&Input_Buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA2_Channel4, &DMA_InitStructure);

    /* Enable DMA Stream Half / Transfer Complete interrupt */
    DMA_ITConfig(DMA2_Channel4, DMA_IT_TC | DMA_IT_HT, ENABLE);

    /* Enable DMA2 Channel3 */
    DMA_Cmd(DMA2_Channel4, ENABLE);

    /* Enable DMA for DAC Channel2 */
    DAC_DMACmd(DAC_Channel_2, ENABLE);
}
