#include "main.h"
#include "FreeRTOS.h"
#include <string.h>
#include "projdefs.h"
#include "task.h"
#include "semphr.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <assert.h>

/**
 * FreeRTOS Counting Semaphore Challenge
 *
 * Challenge: use a mutex and counting semaphores to protect the shared buffer
 * so that each number (0 throguh 4) is printed exactly 3 times to the Serial
 * monitor (in any order). Do not use queues to do this!
 *
 * Hint: you will need 2 counting semaphores in addition to the mutex, one for
 * remembering number of filled slots in the buffer and another for
 * remembering the number of empty slots in the buffer.
 *
 * Date: January 24, 2021
 * Author: Shawn Hymel
 * License: 0BSD
 */

// You'll likely need this on vanilla FreeRTOS


// Settings
static constexpr uint8_t BUF_SIZE = 5;                  // Size of buffer array
static const int num_prod_tasks = 3;  // Number of producer tasks
static const int num_cons_tasks = 2;  // Number of consumer tasks
static const int num_writes = 3;      // Num times each producer writes to buf
static const int prodStackSize = 50;
static const int debugStackSize=200;
static const int initStackSize=200;

// Globals
static int buf[BUF_SIZE];             // Shared buffer
static int head = 0;                  // Writing index to buffer
static int tail = 0;                  // Reading index to buffer
static SemaphoreHandle_t bin_sem;     // Waits for parameter to be read
static SemaphoreHandle_t buffer_lock;     // Waits for parameter to be read
char task_name[12];
static int initTaken{0};
int counter{0};
static int numProducers{0};
int producerCreatedSuccess{0};


void SystemClock_Config(void);

//*****************************************************************************
void debug(void*)
{
	while(true)
	{

		printf("=====================\n\r");
		printf("counter: %d\n\r",counter);
		printf("numProducers: %d\n\r",numProducers);
		printf("producerCreatedSuccess: %d\n\r",producerCreatedSuccess);
		printf("initTaken: %d\n\r",initTaken);
		vTaskDelay(pdMS_TO_TICKS(1000));

	}
}


void producer(void* pvParameters)
{
	int num = *(int*)pvParameters;

	// xSemaphoreTake(buffer_lock);
	for (int i=0;i<num_writes;i++)
	{
		buf[head]=num;
		head=(head+1)%BUF_SIZE;
	}
	// xSemaphoreGive(buffer_lock);
	numProducers++;
	xSemaphoreGive(bin_sem);
	vTaskDelete(nullptr);
}
// void consumer(void*)
// {
//	while(true)
//	{
//		xSemaphoreTake(bin_sem, portMAX_DELAY);
//		printf("task2 ran:%d times\r\n",task2Ran++);
//		xSemaphoreGive(bin_sem);
//		vTaskDelay(pdMS_TO_TICKS(100));
//	}
// }
void initTask(void*)
{
	for ( counter = 0; counter < num_prod_tasks; counter++) {
		sprintf(task_name, "Producer %i", counter);
		auto ret= xTaskCreate(producer,
				task_name,
				prodStackSize,
				(void *)&counter,
				1,
				nullptr
				);
		producerCreatedSuccess+=ret;
		auto status=xSemaphoreTake(bin_sem, portMAX_DELAY);
		if(pdPASS==status)
		{
			initTaken++;
		}
	}
	vTaskDelete(nullptr);
}

int main()
{

	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART2_UART_Init();
	printf("---FreeRTOS Semaphore Alternate Solution---\r\n");
	bin_sem = xSemaphoreCreateBinary();
	buffer_lock = xSemaphoreCreateBinary();
	if(bin_sem){
		printf("bin_sem created\n\r");
	}
	// Wait a moment to start (so we don't miss Serial output)
	// HAL_Delay(500);


	// Start consumer tasks
	// for (int i = 0; i < num_cons_tasks; i++) {
	//	sprintf(task_name, "Consumer %i", i);
	//	xTaskCreate(consumer,
	//			task_name,
	//			100,
	//			nullptr,
	//			1,
	//			nullptr
	//		   );
	// }
	// printf("consumer task created\n\r");

	xTaskCreate(debug,
			"debug",
			debugStackSize,
			nullptr,
			2,
			nullptr
		   );
	xTaskCreate(initTask,
			"task2",
			initStackSize,
			nullptr,
			1,
			nullptr
		   );
	// printf("All tasks created\n\r");

	// Create mutexes and semaphores before starting tasks

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
