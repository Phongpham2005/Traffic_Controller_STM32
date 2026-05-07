/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    MODE_AUTO = 0,
    MODE_NIGHT,
    MODE_NS_GO,
    MODE_EW_GO,
    MODE_SETTING
} SystemMode_t;

volatile SystemMode_t current_mode = MODE_AUTO;
volatile uint8_t fsm_state = 0; // 0: GR, 1: YR, 2: RG, 3: RY

// Button debouncing structure
typedef struct {
    uint16_t pin;
    uint8_t filter_cnt;
    uint8_t state;
    bool flag_pressed;
} Button_t;

Button_t btns[8] = {
    {GPIO_PIN_0, 0, 1, false}, {GPIO_PIN_1, 0, 1, false}, {GPIO_PIN_2, 0, 1, false}, {GPIO_PIN_3, 0, 1, false},
    {GPIO_PIN_4, 0, 1, false}, {GPIO_PIN_5, 0, 1, false}, {GPIO_PIN_6, 0, 1, false}, {GPIO_PIN_7, 0, 1, false}
};
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// --- GPIO DEFINITIONS ---
#define LIGHT_PORT      GPIOA
#define NS_RED_PIN      GPIO_PIN_1
#define NS_YEL_PIN      GPIO_PIN_2
#define NS_GRN_PIN      GPIO_PIN_3
#define EW_RED_PIN      GPIO_PIN_4
#define EW_YEL_PIN      GPIO_PIN_5
#define EW_GRN_PIN      GPIO_PIN_6

#define DIG_PORT        GPIOA
#define DIG_NS_T_PIN    GPIO_PIN_8
#define DIG_NS_U_PIN    GPIO_PIN_9
#define DIG_EW_T_PIN    GPIO_PIN_10
#define DIG_EW_U_PIN    GPIO_PIN_11

#define SEG_PORT        GPIOB
#define SEG_A_PIN       GPIO_PIN_8
#define SEG_B_PIN       GPIO_PIN_9
#define SEG_C_PIN       GPIO_PIN_10
#define SEG_D_PIN       GPIO_PIN_11
#define SEG_E_PIN       GPIO_PIN_12
#define SEG_F_PIN       GPIO_PIN_13
#define SEG_G_PIN       GPIO_PIN_14

#define BTN_PORT        GPIOB
// --- END GPIO DEFINITIONS ---
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
// --- TIME VARIABLES (Volatile because they change in ISR) ---
volatile uint32_t time_green = 5;
volatile uint32_t time_yellow = 2;
volatile uint32_t time_red = 8;

volatile uint32_t countdown_ns = 0;
volatile uint32_t countdown_ew = 0;
volatile uint8_t setting_target = 0; // 0: Green, 1: Yellow, 2: Red

// --- INTERRUPT FLAGS & COUNTERS ---
volatile bool flag_1s = false;       // Tells main loop 1 second has passed
volatile bool flag_blink = false;    // 500ms blink toggle flag
volatile bool update_fsm = false;     // Forces FSM to update display immediately

// Common Anode 7-Segment Codes (0-9)
const uint8_t SEG_CODE[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Helper to push data to 7-segment
void SetSegmentData(uint8_t number);

// Helper to turn off all traffic lights
void TurnOffAllLights(void);


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim2); 
  update_fsm = true;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		// --- 1. HANDLE BUTTON INPUTS ---
      if (btns[0].flag_pressed) { btns[0].flag_pressed = false; current_mode = MODE_AUTO; countdown_ns = 0; countdown_ew = 0; update_fsm = true; }
      if (btns[1].flag_pressed) { btns[1].flag_pressed = false; current_mode = MODE_NIGHT; update_fsm = true; }
      if (btns[2].flag_pressed) { btns[2].flag_pressed = false; current_mode = MODE_NS_GO; update_fsm = true; }
      if (btns[3].flag_pressed) { btns[3].flag_pressed = false; current_mode = MODE_EW_GO; update_fsm = true; }

      if (btns[4].flag_pressed) { // SELECT
          btns[4].flag_pressed = false;
          if (current_mode != MODE_SETTING) {
              current_mode = MODE_SETTING; setting_target = 0; update_fsm = true;
          } else {
              setting_target = (setting_target + 1) % 3;
          }
      }

      if (current_mode == MODE_SETTING) {
          if (btns[5].flag_pressed) { // UP
              btns[5].flag_pressed = false;
              if (setting_target == 0)      { time_green++; time_red = time_green + time_yellow + 1; }
              else if (setting_target == 1) { time_yellow++; time_red = time_green + time_yellow + 1; }
              else if (setting_target == 2) { time_red++; time_green = time_red - time_yellow - 1; }
          }
          if (btns[6].flag_pressed) { // DOWN
              btns[6].flag_pressed = false;
              if (setting_target == 0 && time_green > 1) { time_green--; time_red = time_green + time_yellow + 1; }
              else if (setting_target == 1 && time_yellow > 1) { time_yellow--; time_red = time_green + time_yellow + 1; }
              else if (setting_target == 2 && time_red > time_yellow + 1) { time_red--; time_green = time_red - time_yellow - 1; }
          }
          if (btns[7].flag_pressed) { // SET
              btns[7].flag_pressed = false;
              current_mode = MODE_AUTO; countdown_ns = 0; countdown_ew = 0; update_fsm = true;
          }
      }

      // --- 2. COUNTDOWN LOGIC (Executed strictly once per second) ---
      if (flag_1s) {
          flag_1s = false;
          if (current_mode == MODE_AUTO) {
              if (countdown_ns == 0 || countdown_ew == 0) {
                  update_fsm = true; // Request state transition
              } else {
                  countdown_ns--;
                  countdown_ew--;
              }
          }
      }

      // --- 3. TRAFFIC LIGHT FSM (Only updates when requested to save CPU) ---
      if (update_fsm) {
          update_fsm = false;
          TurnOffAllLights();
				
          if (current_mode == MODE_AUTO) {
              if (countdown_ns == 0 || countdown_ew == 0) { 
                  if (fsm_state == 0)      { fsm_state = 1; countdown_ns = time_yellow; countdown_ew = time_red - time_green - 1; }
                  else if (fsm_state == 1) { fsm_state = 2; countdown_ns = time_red; countdown_ew = time_green; }
                  else if (fsm_state == 2) { fsm_state = 3; countdown_ns = time_red - time_green - 1; countdown_ew = time_yellow; }
                  else if (fsm_state == 3) { fsm_state = 0; countdown_ns = time_green; countdown_ew = time_red; }
                  else                     { fsm_state = 0; countdown_ns = time_green; countdown_ew = time_red; } // Fallback
              }

              if (fsm_state == 0)      HAL_GPIO_WritePin(LIGHT_PORT, NS_GRN_PIN | EW_RED_PIN, GPIO_PIN_SET);
              else if (fsm_state == 1) HAL_GPIO_WritePin(LIGHT_PORT, NS_YEL_PIN | EW_RED_PIN, GPIO_PIN_SET);
              else if (fsm_state == 2) HAL_GPIO_WritePin(LIGHT_PORT, NS_RED_PIN | EW_GRN_PIN, GPIO_PIN_SET);
              else if (fsm_state == 3) HAL_GPIO_WritePin(LIGHT_PORT, NS_RED_PIN | EW_YEL_PIN, GPIO_PIN_SET);
          }
          else if (current_mode == MODE_NIGHT) {
              HAL_GPIO_WritePin(LIGHT_PORT, NS_YEL_PIN | EW_YEL_PIN, flag_blink ? GPIO_PIN_SET : GPIO_PIN_RESET);
          }
          else if (current_mode == MODE_NS_GO) {
              HAL_GPIO_WritePin(LIGHT_PORT, NS_GRN_PIN | EW_RED_PIN, GPIO_PIN_SET);
          }
          else if (current_mode == MODE_EW_GO) {
              HAL_GPIO_WritePin(LIGHT_PORT, NS_RED_PIN | EW_GRN_PIN, GPIO_PIN_SET);
          }
          else if (current_mode == MODE_SETTING) {
              if (flag_blink) {
                  if (setting_target == 0) HAL_GPIO_WritePin(LIGHT_PORT, EW_GRN_PIN, GPIO_PIN_SET);
                  if (setting_target == 1) HAL_GPIO_WritePin(LIGHT_PORT, EW_YEL_PIN, GPIO_PIN_SET);
                  if (setting_target == 2) HAL_GPIO_WritePin(LIGHT_PORT, EW_RED_PIN, GPIO_PIN_SET);
              }
          }
      }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, NS_RED_Pin|NS_YEL_Pin|NS_GRN_Pin|EW_RED_Pin
                          |EW_YEL_Pin|EW_GRN_Pin|DIG_NS_TENS_Pin|DIG_NS_UNITS_Pin
                          |DIG_EW_TENS_Pin|DIG_EW_UNITS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SEG_C_Pin|SEG_D_Pin|SEG_E_Pin|SEG_F_Pin
                          |SEG_G_Pin|SEG_A_Pin|SEG_B_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : NS_RED_Pin NS_YEL_Pin NS_GRN_Pin EW_RED_Pin
                           EW_YEL_Pin EW_GRN_Pin DIG_NS_TENS_Pin DIG_NS_UNITS_Pin
                           DIG_EW_TENS_Pin DIG_EW_UNITS_Pin */
  GPIO_InitStruct.Pin = NS_RED_Pin|NS_YEL_Pin|NS_GRN_Pin|EW_RED_Pin
                          |EW_YEL_Pin|EW_GRN_Pin|DIG_NS_TENS_Pin|DIG_NS_UNITS_Pin
                          |DIG_EW_TENS_Pin|DIG_EW_UNITS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_AUTO_Pin BTN_NIGHT_Pin BTN_NS_GO_Pin BTN_EW_GO_Pin
                           BTN_SELECT_Pin BTN_UP_Pin BTN_DOWN_Pin BTN_SET_Pin */
  GPIO_InitStruct.Pin = BTN_AUTO_Pin|BTN_NIGHT_Pin|BTN_NS_GO_Pin|BTN_EW_GO_Pin
                          |BTN_SELECT_Pin|BTN_UP_Pin|BTN_DOWN_Pin|BTN_SET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SEG_C_Pin SEG_D_Pin SEG_E_Pin SEG_F_Pin
                           SEG_G_Pin SEG_A_Pin SEG_B_Pin */
  GPIO_InitStruct.Pin = SEG_C_Pin|SEG_D_Pin|SEG_E_Pin|SEG_F_Pin
                          |SEG_G_Pin|SEG_A_Pin|SEG_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void SetSegmentData(uint8_t number) {
    if(number > 9) number = 0;
    uint8_t code = SEG_CODE[number];
    HAL_GPIO_WritePin(SEG_PORT, SEG_A_PIN, (code & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_PORT, SEG_B_PIN, (code & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_PORT, SEG_C_PIN, (code & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_PORT, SEG_D_PIN, (code & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_PORT, SEG_E_PIN, (code & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_PORT, SEG_F_PIN, (code & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SEG_PORT, SEG_G_PIN, (code & 0x40) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}


void TurnOffAllLights(void) {
    HAL_GPIO_WritePin(LIGHT_PORT, NS_RED_PIN|NS_YEL_PIN|NS_GRN_PIN|EW_RED_PIN|EW_YEL_PIN|EW_GRN_PIN, GPIO_PIN_RESET);
}

// --- HARDWARE TIMER INTERRUPT CALLBACK (Runs every 1ms) ---
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        static uint16_t tick_ms = 0;
        static uint8_t mux_digit = 0;

        tick_ms++;

        // 1. BUTTON DEBOUNCING (Every 1ms)
        for(int i=0; i<8; i++) {
            uint8_t current_val = HAL_GPIO_ReadPin(BTN_PORT, btns[i].pin);
            if (current_val == GPIO_PIN_RESET) {
                if (btns[i].filter_cnt < 30) btns[i].filter_cnt++; // 30ms debounce
                else if (btns[i].state == 1) {
                    btns[i].state = 0;
                    btns[i].flag_pressed = true; // Signal main loop
                }
            } else {
                if (btns[i].filter_cnt > 0) btns[i].filter_cnt--;
                else btns[i].state = 1;
            }
        }

        // 2. DISPLAY MULTIPLEXING (Every 5ms)
        if (tick_ms % 5 == 0) {
            HAL_GPIO_WritePin(DIG_PORT, DIG_NS_T_PIN|DIG_NS_U_PIN|DIG_EW_T_PIN|DIG_EW_U_PIN, GPIO_PIN_RESET); // Ghosting prevention

            uint8_t ns_val = 0, ew_val = 0;
            bool show_ns = true, show_ew = true;

            if (current_mode == MODE_AUTO) {
                ns_val = (countdown_ns > 99) ? 99 : countdown_ns;
                ew_val = (countdown_ew > 99) ? 99 : countdown_ew;
            } 
            else if (current_mode == MODE_NIGHT) { ns_val = 0; ew_val = 0; } 
            else if (current_mode == MODE_NS_GO || current_mode == MODE_EW_GO) { ns_val = 99; ew_val = 99; } 
            else if (current_mode == MODE_SETTING) {
                show_ns = false; // NS off during setting
                show_ew = flag_blink; // EW blinks
                if (setting_target == 0) ew_val = time_green;
                if (setting_target == 1) ew_val = time_yellow;
                if (setting_target == 2) ew_val = time_red;
                ew_val = (ew_val > 99) ? 99 : ew_val;
            }

            if (mux_digit == 0 && show_ns) { SetSegmentData(ns_val / 10); HAL_GPIO_WritePin(DIG_PORT, DIG_NS_T_PIN, GPIO_PIN_SET); }
            else if (mux_digit == 1 && show_ns) { SetSegmentData(ns_val % 10); HAL_GPIO_WritePin(DIG_PORT, DIG_NS_U_PIN, GPIO_PIN_SET); }
            else if (mux_digit == 2 && show_ew) { SetSegmentData(ew_val / 10); HAL_GPIO_WritePin(DIG_PORT, DIG_EW_T_PIN, GPIO_PIN_SET); }
            else if (mux_digit == 3 && show_ew) { SetSegmentData(ew_val % 10); HAL_GPIO_WritePin(DIG_PORT, DIG_EW_U_PIN, GPIO_PIN_SET); }

            mux_digit = (mux_digit + 1) % 4;
        }

        // 3. SYSTEM FLAGS (500ms and 1000ms)
        if (tick_ms % 500 == 0) { flag_blink = !flag_blink; if (current_mode != MODE_AUTO){update_fsm = true;}} // Trigger visual updates
        if (tick_ms >= 1000) {
            tick_ms = 0;
            flag_1s = true; // Signal main loop that 1 second elapsed
        }
    }
}
/* USER CODE END 4 */

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
#ifdef USE_FULL_ASSERT
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
