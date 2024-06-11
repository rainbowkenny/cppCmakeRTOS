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
static constexpr uint8_t BUF_SIZE = 5;
static const int num_prod_tasks = 5;
static const int num_cons_tasks = 2;
static const int num_writes = 3;
static const int prodStackSize = 50;
static const int consStackSize = 200;
static const int debugStackSize=200;
static const int initStackSize=200;

// Globals
static int buf[BUF_SIZE];
static int head = 0;
static int tail = 0;
static SemaphoreHandle_t sem_prodTask;
static SemaphoreHandle_t mutex_buffer;
static SemaphoreHandle_t mutex_uart;
static SemaphoreHandle_t sem_empty;
static SemaphoreHandle_t sem_filled;
char task_name[12];
static int numProducers{0};
int producerCreatedSuccess{0};
int consumerCreatedSuccess{0};


void SystemClock_Config(void);

//*****************************************************************************
void debug(void*)
{
	while(true)
	{


		// xSemaphoreTake(uart_mutex,portMAX_DELAY);
		// printf("=====================\n\r");
		// printf("numProducers: %d\n\r",numProducers);
		// printf("producerCreatedSuccess: %d\n\r",producerCreatedSuccess);
		// printf("consumerCreatedSuccess: %d\n\r",consumerCreatedSuccess);
		// xSemaphoreGive(uart_mutex);
		vTaskDelay(pdMS_TO_TICKS(1000));

	}
}


void producer(void* pvParameters)
{
	int num = *(int*)pvParameters;
	numProducers++;
	xSemaphoreGive(sem_prodTask);
	for (int i=0;i<num_writes;i++)
	{
		xSemaphoreTake(sem_empty,portMAX_DELAY);
		xSemaphoreTake(mutex_buffer,portMAX_DELAY);
		buf[head]=num;
		head=(head+1)%BUF_SIZE;
		xSemaphoreGive(mutex_buffer);
		xSemaphoreGive(sem_filled);
	}
	vTaskDelete(nullptr);
}

void consumer(void*)
{
	consumerCreatedSuccess++;
	while(true)
	{
		int value;
		xSemaphoreTake(sem_filled,portMAX_DELAY);
		xSemaphoreTake(mutex_buffer,portMAX_DELAY);
		value  = buf[tail];
		tail=(tail+1)%BUF_SIZE;
		xSemaphoreGive(mutex_buffer);
		xSemaphoreTake(mutex_uart,portMAX_DELAY);
		printf("%d\n\r",value);
		xSemaphoreGive(mutex_uart);
		xSemaphoreGive(sem_empty);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
void initTask(void*)
{
	for ( int i = 0; i < num_prod_tasks; i++) {
		sprintf(task_name, "Producer %i", i);
		auto ret= xTaskCreate(producer,
				task_name,
				prodStackSize,
				(void *)&i,
				1,
				nullptr
				);
		if(pdPASS==ret)
		{
			// printf("%d\n\r",i);
		}
		producerCreatedSuccess+=ret;


		//this ensures a producer task is created and run before the next one is created.
		//It is needed so that i's value is passed to producer task before it changes
		auto status=xSemaphoreTake(sem_prodTask, portMAX_DELAY);
		// if(pdPASS==status)
		// {
		// initTaken++;
		// }
	}
	for ( int i = 0; i < num_cons_tasks; i++) {
		sprintf(task_name, "Consumer %i", i);
		auto ret= xTaskCreate(consumer,
				task_name,
				consStackSize,
				nullptr,
				1,
				nullptr
				);
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
	sem_prodTask = xSemaphoreCreateBinary();
	sem_empty = xSemaphoreCreateCounting(BUF_SIZE,BUF_SIZE);
	sem_filled = xSemaphoreCreateCounting(BUF_SIZE,0);
	mutex_buffer = xSemaphoreCreateMutex();
	mutex_uart = xSemaphoreCreateMutex();
	if(sem_prodTask){
		printf("bin_sem created\n\r");
	}

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
