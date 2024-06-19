#include "main.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

namespace{
	constexpr uint8_t CMD_BUF_SIZE{10};
	constexpr uint16_t defaultStackSize{200};
	constexpr uint16_t defaultPriority{1};
	uint8_t cmdBuf[CMD_BUF_SIZE] ;
	uint8_t tempBuf[1] ;
	TaskHandle_t cmdTask{nullptr};
	TaskHandle_t handlerTask{nullptr};
	SemaphoreHandle_t mutex{nullptr};
	uint8_t counter{0};
	uint8_t i{0};
}

void SystemClock_Config(void);

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	cmdBuf[i]=tempBuf[0];//copy the received character to the cmd buffer
	i=(++i)%CMD_BUF_SIZE;
	counter ++;
	HAL_UART_Receive_IT(&huart2,tempBuf,1);//restart interrupt and receive new character

	// BaseType_t xHigherPriorityTaskWoken{pdFALSE};
	// portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void handleCommand(void*)
{
	while(1)
	{
		xSemaphoreTake(mutex,portMAX_DELAY);
		printf("Buffer: %s\r\n",cmdBuf);
		printf("Counter: %d\r\n",counter);
		xSemaphoreGive(mutex);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}



int main()
{

	HAL_Init();
	SystemClock_Config();
	MX_GPIO_Init();
	MX_USART2_UART_Init();

	mutex=xSemaphoreCreateMutex();
	if(!mutex)
	{
		printf("mutex not created\r\n");
	}

	printf("start\r\n");
	xTaskCreate(handleCommand,"handler task",defaultStackSize,nullptr,defaultPriority,&handlerTask);
	HAL_UART_Receive_IT(&huart2,tempBuf,1);
	vTaskStartScheduler();
	while (1)
	{
	}
}

int __io_putchar(int ch){
	HAL_UART_Transmit(&huart2,(uint8_t*)&ch,1,HAL_MAX_DELAY);
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
