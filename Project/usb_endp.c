/**
  ******************************************************************************
  * @file    usb_endp.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Endpoint routines
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_mem.h"
#include "hw_config.h"
#include "usb_istr.h"
#include "usb_pwr.h"

#include "FreeRTOS.h"
#include "queue.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Interval between sending IN packets in frame number (1 frame = 1ms) */
#define VCOMPORT_IN_FRAME_INTERVAL             5
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern __IO uint32_t packet_sent;
extern __IO uint32_t packet_receive;
extern uint8_t Receive_Buffer[64];
uint32_t Receive_length;

extern xQueueHandle xInMessagesQueue;
extern xQueueHandle xOutMessagesQueue;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*******************************************************************************
* Function Name  : EP1_IN_Callback
* Description    :
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP1_IN_Callback(void)
{
//    UserToPMABufferCopy((unsigned char*)ptrBuffer, ENDP1_TXADDR, Send_length);
//    SetEPTxCount(ENDP1, Send_length);
//    SetEPTxValid(ENDP1);
    packet_sent = 1;
}

/*******************************************************************************
* Function Name  : EP3_OUT_Callback
* Description    :
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void EP3_OUT_Callback(void)
{
    int i;
    char c;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    Receive_length = GetEPRxCount(ENDP3);
    PMAToUserBufferCopy((unsigned char*)Receive_Buffer, ENDP3_RXADDR, Receive_length);

    //for (i = 0; i < Receive_length; i++) {
    //    xQueueSendToBackFromISR(xInMessagesQueue, (void *)&Receive_Buffer[i], &xHigherPriorityTaskWoken);
    //}
    //xQueueSendFromISR(xInMessagesQueue, &Receive_Buffer[0], &xHigherPriorityTaskWoken);
    c = '@';
    GPIOE->ODR ^= GPIO_Pin_14; /* LED8 */
    xQueueSendFromISR(xInMessagesQueue, &c, &xHigherPriorityTaskWoken);

    SetEPRxValid(ENDP3);
    GPIOE->ODR ^= GPIO_Pin_13;
    //portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken == pdTRUE) {
        // Actual macro used here is port specific.
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

void SOF_Callback(void)
{
}
