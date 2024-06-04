#include "main.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "queue.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <assert.h>

namespace{
	static TaskHandle_t hTemp, hFetch;
	static QueueHandle_t sensorQ;
	static constexpr uint16_t queueLength {200} ;
	const uint32_t waitTime{};
	static constexpr uint32_t TASK_STACK_SIZE {100};
	enum class SENSOR:uint8_t{
		TEMPERATURE,
		HUMIDITY,
	};
	struct SensorReading
	{
		signed char value;
		SENSOR sensor;
	};
	using Item = SensorReading;
}


void SystemClock_Config(void);

void tempSensorTask(void *)
{
	SensorReading tempSensorReading;
	tempSensorReading.sensor=SENSOR::TEMPERATURE;
	tempSensorReading.value=-20;//fake data reading
	while(true){

		// printf("items in queue: %lu\n\r",queueLength-uxQueueSpacesAvailable(sensorQ));
		++tempSensorReading.value;
		auto status = xQueueSend(sensorQ,&tempSensorReading,waitTime);
		if(status!=pdPASS)
		{
			printf("Can't send data\n\r");

		}
		vTaskDelay(pdMS_TO_TICKS(1000 ));
	}
}


void humidiySensorTask(void *)
{
	SensorReading humidiySensorReading;
	humidiySensorReading.sensor=SENSOR::HUMIDITY;
	humidiySensorReading.value=0;
	while(true){

		++humidiySensorReading.value;
		auto status = xQueueSend(sensorQ,&humidiySensorReading,waitTime);
		if(status!=pdPASS)
		{
			printf("Can't send data\n\r");

		}
		vTaskDelay(pdMS_TO_TICKS(1000 ));
	}
}
void fetchDataTask(void *)
{
	SensorReading reading{};
	while(true)
	{
		auto status = xQueueReceive(sensorQ,&reading,waitTime);
		if(status==pdPASS)
		{
			if(reading.sensor==SENSOR::TEMPERATURE)
			{
				printf("Temperature reading: %d\n\r",reading.value);
			}
			if(reading.sensor==SENSOR::HUMIDITY)
			{
				printf("Humidiy reading: %d\n\r",int(reading.value));
			}

		}
		// vTaskDelay(pdMS_TO_TICKS(500 ));


	}
}


int main()
{

	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART2_UART_Init();


	xTaskCreate(tempSensorTask,"temp reading", TASK_STACK_SIZE,nullptr,1, &hTemp);
	xTaskCreate(humidiySensorTask,"humidity reading", TASK_STACK_SIZE,nullptr,1, nullptr);
	xTaskCreate(fetchDataTask,"fetch data", TASK_STACK_SIZE,nullptr,1, &hFetch);

	sensorQ = xQueueCreate(queueLength,sizeof(Item));
	if(sensorQ==nullptr)
	{
		while(true)
		{
			printf("Queue can't be allocated\n\r");
			HAL_Delay(1000);
		}
	}

	vTaskStartScheduler();
	while (1)
	{
	}
}

int __io_putchar(int ch){
	HAL_UART_Transmit(&huart2,(uint8_t*)&ch,1,0xffff);
	return ch;
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {};


	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL12;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
		|RCC_CLOCKTYPE_PCLK1;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
	PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM1) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
