/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "spi.h"
#include "lwip.h"
#include "lwip/api.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId SPI_MasterHandle;
osThreadId UDPTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
QueueHandle_t sensorDataQueue;
SensorData_t sensorData = {0};

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartSPI_Master(void const * argument);
void StartUDPTask(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	sensorDataQueue = xQueueCreate(5, sizeof(SensorData_t));
	if (sensorDataQueue == NULL) {
	    Error_Handler();
	}
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of SPI_Master */
  osThreadDef(SPI_Master, StartSPI_Master, osPriorityNormal, 0, 512);
  SPI_MasterHandle = osThreadCreate(osThread(SPI_Master), NULL);

  /* definition and creation of UDPTask */
  osThreadDef(UDPTask, StartUDPTask, osPriorityAboveNormal, 0, 512);
  UDPTaskHandle = osThreadCreate(osThread(UDPTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  Uart3_Printf("MX_LWIP_Init done.\r\n");

  if (gnetif.ip_addr.addr == 0) {
      Uart3_Printf("IP is 0.0.0.0, maybe link not up.\r\n");
  } else {
      char buffer[64];
      sprintf(buffer, "IP address: %s\r\n", ip4addr_ntoa(&gnetif.ip_addr));
      HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 100);
  }

  // flush time 확보
  osDelay(100);

  vTaskDelete(NULL); // 자기 자신 종료
  for(;;)
  {

  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartSPI_Master */
/**
* @brief Function implementing the SPI_Master thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSPI_Master */
void StartSPI_Master(void const * argument)
{
  /* USER CODE BEGIN StartSPI_Master */
  /* Infinite loop */
	SensorData_t data;

	uint8_t txDummy_Internal[4] = {0};
	uint8_t Internal_rxData[4] = {0};
	uint8_t txDummy_External[4] = {0};
	uint8_t External_rxData[4] = {0};

	HAL_StatusTypeDef Internal_status;
	HAL_StatusTypeDef External_status;

	uint16_t lux = 0;
	uint16_t rain_adc = 0;
	uint16_t co2_ppm = 0;
	for(;;)
	{
		// Receive data from Internal ST
		CS_Select_PA4();
		Internal_status = HAL_SPI_TransmitReceive(&hspi1, txDummy_Internal, Internal_rxData, 4, HAL_MAX_DELAY);
		CS_Deselect_PA4();

		osDelay(1);

		// Receive data from External ST
		CS_Select_PA15();
		External_status = HAL_SPI_TransmitReceive(&hspi1, txDummy_External, External_rxData, 4, HAL_MAX_DELAY);
		CS_Deselect_PA15();

		// Enqueue data for UDP Send
		if(Internal_status == HAL_OK && External_status == HAL_OK) {
			co2_ppm = (Internal_rxData[2] << 8) | Internal_rxData[3];
			lux = (External_rxData[0] << 8) | External_rxData[1];
			rain_adc = (External_rxData[2] << 8) | External_rxData[3];
			Uart3_Printf("humi=%d temp=%d co2_ppm=%d lux=%d rain_adc=%d\r\n", Internal_rxData[0], Internal_rxData[1], co2_ppm, lux, rain_adc);

			data.humid = Internal_rxData[0];
			data.temp = Internal_rxData[1];
			data.co2 = co2_ppm;
			data.lux = lux;
			data.rain = rain_adc;

			xQueueSend(sensorDataQueue, &data, portMAX_DELAY);
		}
		else
		{
			Uart3_Printf("SPI fail Internal:%d External:%d\r\n", Internal_status, External_status);
		}

	osDelay(500);

}
  /* USER CODE END StartSPI_Master */
}

/* USER CODE BEGIN Header_StartUDPTask */
/**
* @brief Function implementing the UDPTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUDPTask */
void StartUDPTask(void const * argument)
{
  /* USER CODE BEGIN StartUDPTask */
  /* Infinite loop */
	for(;;)
	{
		struct udp_pcb *pcb;
		ip_addr_t dest_ip;

		// Wait for link-up
		while (netif_is_link_up(&gnetif) == 0)
		{
			HAL_UART_Transmit(&huart3, (uint8_t*)"Waiting for link-up...\r\n", 24, 100);
			osDelay(1000);
		}

		// Create socket
		pcb = udp_new();
		if (!pcb)
		{
			HAL_UART_Transmit(&huart3, (uint8_t*)"udp_new failed\r\n", 16, 100);
			osThreadTerminate(NULL);
		}

		// Set up destination Address
		IP4_ADDR(&dest_ip,192,168,1,3);

		// Bind
		udp_connect(pcb, &dest_ip, 5006);

		HAL_UART_Transmit(&huart3, (uint8_t*)"UDP Task Ready\r\n", 16, 100);

		for(;;)
		{
			SensorData_t received;

			// Dequeue received data
			if (xQueueReceive(sensorDataQueue, &received, portMAX_DELAY) == pdTRUE)
			{
				char msg[64];
				snprintf(msg, sizeof(msg), "{\"temp\":%d,\"humi\":%d,\"co2\":%d,\"lux\":%d,\"rain\":%d}", received.temp, received.humid, received.co2,received.lux, received.rain);

				struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);
				if (!p)
				{
					HAL_UART_Transmit(&huart3, (uint8_t*)"pbuf_alloc failed\r\n", 20, 100);
					continue;
				}

				pbuf_take(p, msg, strlen(msg));

				// Transmit data to Pi#2
				udp_send(pcb, p);
				pbuf_free(p);

				HAL_UART_Transmit(&huart3, (uint8_t*)"UDP Sent\r\n", 10, 100);
			}
		}

		// Delete socket
		udp_remove(pcb);
	}
  /* USER CODE END StartUDPTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
