/*
 * dht11.h
 *
 *  Created on: Jun 29, 2025
 *      Author: user
 */

#ifndef INC_DHT11_H_
#define INC_DHT11_H_

#include "stm32f4xx_hal.h"

// 핀 정의
#define DHT11_PORT GPIOE
#define DHT11_PIN  GPIO_PIN_15

// 함수 프로토타입
int dht11_read(void);

// 데이터 값
extern uint8_t Temperature;
extern uint8_t Humidity;

#endif /* INC_DHT11_H_ */
