/*
 * dht11.c
 *
 *  Created on: Jun 29, 2025
 *      Author: user
 */
#include "dht11.h"
#include "main.h"

uint8_t Temperature = 0;
uint8_t Humidity = 0;

void delay_us(uint32_t us)
{
  uint32_t cycles = (SystemCoreClock / 1000000L) * us;  // 168Mhz / 1000000 = 168
  uint32_t start = DWT->CYCCNT;							// start point
  while ((DWT->CYCCNT - start) < cycles);
}

int wait_pulse(int state)
{
    uint32_t timeout = 100; // 100us
    uint32_t startTick = DWT->CYCCNT;
    uint32_t ticks = timeout * (SystemCoreClock / 1000000);

    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != state)
    {
        if ((DWT->CYCCNT - startTick) > ticks)
        {
            return 0;
        }
    }
    return 1;
}

int dht11_read(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint8_t data[5] = {0};
    char debug_buffer[64];

    // Start signal
    GPIO_InitStruct.Pin = DHT11_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(20); // 20ms
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    delay_us(20);

    // change to input
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);

    // check response
    delay_us(40);
    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != GPIO_PIN_RESET)
    {
        sprintf(debug_buffer, "Error: No LOW after START\r\n");
        // HAL_UART_Transmit(&huart3, (uint8_t*)debug_buffer, strlen(debug_buffer), 100);
        return -1;
    }
    delay_us(80);
    if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) != GPIO_PIN_SET)
    {
        sprintf(debug_buffer, "Error: No HIGH after LOW\r\n");
        // HAL_UART_Transmit(&huart3, (uint8_t*)debug_buffer, strlen(debug_buffer), 100);
        return -2;
    }

    if (!wait_pulse(GPIO_PIN_RESET))
    {
        sprintf(debug_buffer, "Error: Timeout after HIGH\r\n");
        // HAL_UART_Transmit(&huart3, (uint8_t*)debug_buffer, strlen(debug_buffer), 100);
        return -3;
    }

    // read 5 bytes
    for (uint8_t i = 0; i < 5; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
            if (!wait_pulse(GPIO_PIN_SET))
            {
                sprintf(debug_buffer, "Error: Timeout waiting HIGH (Byte %d Bit %d)\r\n", i, j);
                // HAL_UART_Transmit(&huart3, (uint8_t*)debug_buffer, strlen(debug_buffer), 100);
                return -4;
            }

            delay_us(40);
            if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
                data[i] |= (1 << (7 - j));

            if (!wait_pulse(GPIO_PIN_RESET))
            {
                sprintf(debug_buffer, "Error: Timeout waiting LOW (Byte %d Bit %d)\r\n", i, j);
                // HAL_UART_Transmit(&huart3, (uint8_t*)debug_buffer, strlen(debug_buffer), 100);
                return -5;
            }
        }
    }

    // checksum
    if (data[4] != (data[0] + data[1] + data[2] + data[3]))
    {
        sprintf(debug_buffer, "Error: Checksum mismatch\r\n");
        // HAL_UART_Transmit(&huart3, (uint8_t*)debug_buffer, strlen(debug_buffer), 100);
        return -6;
    }

    Humidity    = data[0];
    Temperature = data[2];

    return 1;
}
