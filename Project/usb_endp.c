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

uint8_t USB_Tx_state = 0;

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
    char buffer[VIRTUAL_COM_PORT_DATA_SIZE];
    int length = 0;

    if (USB_Tx_state == 1) {
        while (xQueueReceiveFromISR(xOutMessagesQueue, &buffer[length], 0) == pdPASS) {
            length++;
            if (length == VIRTUAL_COM_PORT_DATA_SIZE) {
                break;
            }
        }

        if (length > 0) {
            UserToPMABufferCopy((unsigned char*)buffer, ENDP1_TXADDR, length);
            SetEPTxCount(ENDP1, length);
            SetEPTxValid(ENDP1);
        } else {
            USB_Tx_state = 0;
        }
    }
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

    Receive_length = GetEPRxCount(ENDP3);
    PMAToUserBufferCopy((unsigned char*)Receive_Buffer, ENDP3_RXADDR, Receive_length);

    for (i = 0; i < Receive_length; i++) {
        //xQueueSendFromISR(xInMessagesQueue, &Receive_Buffer[i], &xHigherPriorityTaskWoken);
        xQueueSendFromISR(xInMessagesQueue, &Receive_Buffer[i], 0);
    }

    SetEPRxValid(ENDP3);
}

#ifdef SOF_CALLBACK
void SOF_Callback(void)
{
    char buffer[VIRTUAL_COM_PORT_DATA_SIZE];
    int length = 0;
    //portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    if (USB_Tx_state != 1) {
        USB_Tx_state = 0;
        while (xQueueReceiveFromISR(xOutMessagesQueue, &buffer[length], 0) == pdPASS) {
            length++;
            if (length == VIRTUAL_COM_PORT_DATA_SIZE) {
                break;
            }
        }

        if (length > 0) {
            UserToPMABufferCopy((unsigned char*)buffer, ENDP1_TXADDR, length);
            SetEPTxCount(ENDP1, length);
            SetEPTxValid(ENDP1);
            USB_Tx_state = 1;
        }
    }
}
#endif
