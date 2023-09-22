/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c-lcd.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
#define SLAVE_ADDRESS_EEPROM 0x50  
void write_eeprom (int address ,int value)
{
	uint8_t data_t[2];

	data_t[0] = address;  
	data_t[1] = value;  
	HAL_I2C_Master_Transmit (&hi2c2, SLAVE_ADDRESS_EEPROM << 1,(uint8_t *) data_t, 2, HAL_MAX_DELAY);
	HAL_Delay(10); // tWR Write Cycle Time 5 ms
	printf("write_eeprom at 0x%X \r\n",address);
}

uint8_t read_eeprom(uint16_t register_pointer)
{
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t return_value = 0;

	status = HAL_I2C_Mem_Read(&hi2c2, SLAVE_ADDRESS_EEPROM << 1, (uint16_t)register_pointer, I2C_MEMADD_SIZE_8BIT, &return_value, 1, 100);

	/* Check the communication status */
	if(status != HAL_OK)
	{
			
	}
	printf("Read EEPROM at [%d] : %d \r\n",register_pointer, return_value);
	return return_value;
}

void reset_eeprom()
{
	for(int i = 0 ; i < 20; i++)
	{
		write_eeprom(i,0+48);
		read_eeprom(i);
	}
}

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
 
// quet thay dia chi la 0x50, dich trai 1 bit >> 0xA0
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
typedef struct
{
 uint8_t trangthai;
 uint8_t left;//khai bao nut bam 
 uint8_t right;//khai bao nut bam 
	enum 
	{
		Mode_chung,//0
		Mode_maytinh,//1
		Mode_4led,//2
		mode_left,
		mode_right,
	};
	
} Global;

Global Global_NT;



typedef struct 
{
	int string_num[5];
	int num_1;
	int num_2;
	int vitri_pheptinh;
	int trang_thai;
	int ketqua;
	int quakhu;
	
} Mode1;

Mode1 Maytinh;


typedef struct 
{
	int string_num[5];
	int trang_thailed;
	int ketqua;
	
	
} Mode2;

Mode2 led;


// su dung uart debug
char buffer[20];
	int fputc(int ch, FILE * f) 
	{		
		HAL_UART_Transmit( & huart2, (uint8_t * ) & ch, 1, HAL_MAX_DELAY);
		return ch;
	}



	int row,cow =0;
	int so[6];
	int string_max = 0;
	char key;
	int	matrix_row ;  //debug printf LCD
	int matrix_col;
	char buffer[20];
	

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/////////////////////////////////////// code led 7 DOAN
void shiftout(int data)
{
	for(int i=0;i <8;i++) // gui tin hieu 1 SO CAN HIEN THI L?N 1 LED 
	{
		if(data&0x80)
		{
			HAL_GPIO_WritePin(DIN_GPIO_Port,DIN_Pin,GPIO_PIN_SET);
		}
		else
		{
			HAL_GPIO_WritePin(DIN_GPIO_Port,DIN_Pin,GPIO_PIN_RESET);
		}
		data<<=1;
		HAL_GPIO_WritePin(SCLK_GPIO_Port,SCLK_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(SCLK_GPIO_Port,SCLK_Pin,GPIO_PIN_RESET);
				
	}
}
char led_index[9]={0};
void display_4led(int so_int)
{
	unsigned char MA7DOAN[] = 
	{
		0xC0,// 0
		0xF9,// 1
		0xA4,// 2
		0xB0,// 3
		0x99,// 4
		0x92,// 5
		0x82,// 6
		0xF8,// 7
		0x80,// 8
		0x90,// 9
		0x88,// A 10
		0x83,// B 11
		0xC6,// C 12
		0xA1,// D 13
		0x86,// E 14
		0x8E,// F 15
		0xff // NONE 16  
	};
	led_index[8] = so_int/1000;
	led_index[4] = so_int%1000/100;			
	led_index[2] = so_int%100/10;			
	led_index[1] = so_int%10;	
	if(led_index[8] == 0)
	{
		led_index[8] = 16; // MA7DOAN[16] -> tat led 7 doan
		if(led_index[4] == 0)
		{
			led_index[4] = 16; // MA7DOAN[16] -> tat led 7 doan
					if(led_index[2] == 0)
					{
						led_index[2] = 16; // MA7DOAN[16] -> tat led 7 doan
					}
		}
	}		

	for(int i = 1; i < 9; i = i*2)
	{
		HAL_GPIO_WritePin(LATCH_GPIO_Port,LATCH_Pin,GPIO_PIN_RESET);
		shiftout(MA7DOAN[led_index[i]]);
		shiftout(i);
		HAL_GPIO_WritePin(LATCH_GPIO_Port,LATCH_Pin,GPIO_PIN_SET);
	}
}


 //////////////////////////////////////////code matrix
GPIO_TypeDef* row_port[4] = {ROW0_GPIO_Port, ROW1_GPIO_Port, ROW2_GPIO_Port, ROW3_GPIO_Port};
GPIO_TypeDef* col_port[4] = {COL0_GPIO_Port, COL1_GPIO_Port, COL2_GPIO_Port, COL3_GPIO_Port};
char scan_key()
{
	char keys [4][4] =
	{
		{'1', '2', '3', 'A'},
		{'4', '5', '6', 'B'},
		{'7', '8', '9', 'C'},
		{'*', '0', '#', 'D'},
	};
	// PHIA DUOI LA MATRIX 4X4
	char col_pin[4]={COL0_Pin,COL1_Pin,COL2_Pin,COL3_Pin};// quet cot
	char row_pin[4]={ROW0_Pin,ROW1_Pin,ROW2_Pin,ROW3_Pin};// quet hang
	// quet cot
	for (int col =0;col<4; col++)
	{
		
		HAL_GPIO_WritePin(GPIOD,col_pin[col],0);
		//quet hang
		for ( int row=0;row<4 ;row++)
		{
			if ( HAL_GPIO_ReadPin(GPIOD,row_pin[row]) ==0) 
			{
				
				HAL_Delay(50);
				while ( HAL_GPIO_ReadPin(GPIOD,row_pin[row]) ==0)
				{ 
				}
					 
				HAL_GPIO_WritePin(GPIOD,col_pin[col],1);	
				matrix_row = row;  //debug printf LCD
				matrix_col = col;
				return keys [row][col];
				
			}
			
		}
		HAL_GPIO_WritePin(GPIOD,col_pin[col],1);
	}
 return 32;// 32 KY TU "SPACE" TRONG MA ACSII
}



int scan_mode(void)
{
	if (HAL_GPIO_ReadPin(MODE_GPIO_Port, MODE_Pin))
	{
		HAL_Delay(20);
		if (HAL_GPIO_ReadPin(MODE_GPIO_Port, MODE_Pin))
		{
			Global_NT.trangthai =Global_NT.trangthai +1;
			if(Global_NT.trangthai==3)
			{
				Global_NT.trangthai=0;
			}
			while (HAL_GPIO_ReadPin(MODE_GPIO_Port,  MODE_Pin) == 1)
			{ 
			}
		}
	}
	return Global_NT.trangthai;
}
int scan_left(void)
{
	if ((HAL_GPIO_ReadPin(LEFT_GPIO_Port, LEFT_Pin)==0) )
	{
		 
		HAL_Delay(20);
		if (HAL_GPIO_ReadPin(LEFT_GPIO_Port, LEFT_Pin)==0)
		{
			Global_NT.left=1;
			

			while (HAL_GPIO_ReadPin(LEFT_GPIO_Port, LEFT_Pin) == 0)
			{ 
			}
		}
	}
	return Global_NT.left;
}


int scan_right(void)
{
	if (HAL_GPIO_ReadPin(RIGHT_GPIO_Port, RIGHT_Pin)==0)
	{
		 
		HAL_Delay(20);
		if (HAL_GPIO_ReadPin(RIGHT_GPIO_Port, RIGHT_Pin)==0)
		{
			Global_NT.right=0;
			printf("gia tri hien tai la  %d\r\n",Maytinh.ketqua );
			while (HAL_GPIO_ReadPin(RIGHT_GPIO_Port, RIGHT_Pin) == 0)
			{ 
				
			}
		}
	}
	return Global_NT.right;
}



int fn_getNum(int array[5],int array_max)
{
	for(int i =0;i<5;i++)
	{
		printf("array[%d] %d \r\n", i, array[i]);
		
	}
	
		
	return 1;
}


void fn_Mode_maytinh(void)
{
while(Global_NT.trangthai == Mode_maytinh)
	{
		lcd_put_cur(0,0);
		lcd_send_string("MODE1:MAY_TINH");
		scan_mode();
		scan_left();
		scan_right();
		if(Global_NT.left==1)
		{
			lcd_put_cur(0,0);
		  lcd_send_string("Gia tri truoc do la");
			lcd_put_cur(1,0);
			sprintf (buffer,"%d ", read_eeprom(0)); 
			lcd_send_string (buffer);	
			HAL_Delay(2000);
			lcd_init();
			Global_NT.left=2;	
		}
		
		key = scan_key();
		lcd_put_cur (1, string_max);
		if((key == '#') && (string_max > 2))
		{
			for(int i = 0; i < string_max; i++)
			{
				//printf("Maytinh.string_num[i] %d\r\n", Maytinh.string_num[i]);
				if (( Maytinh.string_num[i] < 0) || \
					( Maytinh.string_num[i] > 9))
				{
					printf("VI TRI PHEP TINH LA %d\r\n", i);
					if (Maytinh.string_num[i] == (65-48)) // dau +
					{
						
						printf("PHEP TINH HIEN TAI LA + \r\n");
						Maytinh.trang_thai=0;
					}
					else if (Maytinh.string_num[i] == (66-48)) // dau -
					{
						printf("PHEP TINH HIEN TAI LA - \r\n");
							Maytinh.trang_thai=1;
					}
					else if (Maytinh.string_num[i] == (67-48)) // dau *
					{
						printf("PHEP TINH HIEN TAI LA * \r\n");
						Maytinh.trang_thai=2;
					}
					Maytinh.vitri_pheptinh = i;
				}
			}		
			// lay so 1		 
			printf("Lay so 1:\r\n");
			Maytinh.num_1 = 0;
			for (int j = 0; j < Maytinh.vitri_pheptinh; j++) {
				Maytinh.num_1 = Maytinh.num_1 * 10 + Maytinh.string_num[j];
				write_eeprom(1,Maytinh.num_1);
			}					
			// lay so 2
			printf("Lay so 2:\r\n");
			Maytinh.num_2 = 0;
			for (int j = Maytinh.vitri_pheptinh+1; j < string_max; j++) {
				Maytinh.num_2 = Maytinh.num_2 * 10 + Maytinh.string_num[j];
				write_eeprom(2,Maytinh.num_2);
			}				
			printf("Maytinh.num_2 1a: %d\r\n", Maytinh.num_2);
			
				if(Maytinh.trang_thai==0)
			{
				Maytinh.ketqua=Maytinh.num_1+Maytinh.num_2; 
				Maytinh.quakhu=read_eeprom(1)+read_eeprom(2);
				write_eeprom(0,Maytinh.quakhu);
				printf("ket qua phep cong la:%d\r\n",Maytinh.ketqua);	 
			}
			
				if(Maytinh.trang_thai==1)
			{
				Maytinh.ketqua=Maytinh.num_1-Maytinh.num_2;
				
				printf("ket qua phep tru la : %d\r\n",Maytinh.ketqua);
			}
				if(Maytinh.trang_thai==2)
			{
				Maytinh.ketqua=Maytinh.num_1*Maytinh.num_2;
				;
				printf("ket qua phep nhan la  : %d\r\n",Maytinh.ketqua);
			}

				lcd_put_cur (1, string_max +1);
				lcd_send_string("=");
			  lcd_put_cur (1,string_max +2);
				printf("maytinh %d\r\n",Maytinh.ketqua);
		  	sprintf (buffer,"%d ", Maytinh.ketqua); 
			  lcd_send_string (buffer);	
		   	string_max = 0;
			  HAL_Delay(2500);
		    lcd_clear();
		}
		
		
		else if(key != 32)
		{
			lcd_put_cur (1,string_max);
		  Maytinh.string_num[string_max]=key-48;			
			printf("so thu %d la %d\r\n", string_max, Maytinh.string_num[string_max]);
			lcd_send_data(key);			
		
			if (Maytinh.string_num[string_max] == (65-48))			
			{		
				lcd_put_cur (1, string_max);
				lcd_send_string("+");
			}
			if (Maytinh.string_num[string_max] == (66-48))			
			{		
				lcd_put_cur (1, string_max);
				lcd_send_string("-");
			}
			if (Maytinh.string_num[string_max] == (67-48))			
			{		
				lcd_put_cur (1, string_max);
				lcd_send_string("*");
			}
			else if(Maytinh.string_num[string_max] == (68-48))
			{		
			
				lcd_clear();
				string_max=-1;	// vi tri -1
			}
		
			string_max++;	
		}
	}	
}

// ham matrix

void fn_Mode_4LED(void)
{	
	 while(Global_NT.trangthai == Mode_4led)
	 {
		lcd_put_cur(0,0);
		lcd_send_string("MODE2:4_led");
		 
		scan_mode();
		key = scan_key();
		if(key != 32)
		{
			lcd_put_cur (1,string_max);
		  led.string_num[string_max]=key-48;			
			printf("so thu %d la %d\r\n", string_max, led.string_num[string_max]);
			lcd_send_data(key);	
			string_max++;	
		 }
		if(key=='#')
		{
			write_eeprom(10,led.string_num[0]);
			write_eeprom(11,led.string_num[1]);
			write_eeprom(12,led.string_num[2]);
			write_eeprom(13,led.string_num[3]);
			string_max=0;	
			led.ketqua =1000*read_eeprom(10) +100*read_eeprom(11) + 10*read_eeprom(12)+read_eeprom(13);	 
			printf("so da luu la: %d\r\n",led.ketqua);
			lcd_init();
			lcd_put_cur(0,0);
			lcd_send_string("DA LUU EEPROM");
		}
		if(key=='*')
		{
	  	while(1)
			{
			  key = scan_key();
				scan_key();
		  	if (HAL_GPIO_ReadPin(MODE_GPIO_Port, MODE_Pin))
				{
				  Global_NT.trangthai=0;
					break;
				}
		display_4led(led.ketqua);	

		  }		
		   
			 		
		}				

 }	 
}
	

void display_key(void)
{
		lcd_put_cur(0,0);
		lcd_send_string("Row : ");
		lcd_send_data(matrix_row +48 );

		lcd_put_cur(1,0);
		lcd_send_string("Col : ");
		lcd_send_data(matrix_col + 48 );

	 
}




	

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
int ret = 0;
	for(int i=1; i<128; i++)
	{
			ret = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 3, 5);
			if (ret != HAL_OK) /* No ACK Received At That Address */
			{
					printf("khong co tai dia chi 0x%X \n",i);
			}
			else if(ret == HAL_OK)
			{
					printf("dia chi 0x%X \n",i);
					//break;
			}
			HAL_Delay(100);
	}
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_I2C2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	lcd_init ();
	

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	Global_NT.trangthai =0;
  while (1)
  {
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	
		
		scan_mode();		 
		if (Global_NT.trangthai == Mode_chung)
		{
			lcd_put_cur(0,0);
			lcd_send_string("MODE1:MAY_TINH");	
			lcd_put_cur(1,0);
			lcd_send_string("MODE2:4 LED");	
			printf("Global_NT.trangthai  Mode_chung \r\n");
		
		}
		else if (Global_NT.trangthai == Mode_maytinh)
		{
			
			lcd_init();
			printf(" Mode_maytinh = %d\r\n",Mode_maytinh);
			// void fn_Mode_maytinh
		  fn_Mode_maytinh();
			
		}
		else if(Global_NT.trangthai == Mode_4led)
		{
			lcd_init();
		 	printf("Global_NT.trangthai  Mode_4led \r\n");
			// void fn_Mode_4led
			fn_Mode_4LED();
		 	
		}
	 
	
		
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
