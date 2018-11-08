#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
/* LCD kutuphanesi */
#include "lcd48x84.h"

/* Yilanin basinin baslangic adresleri */
#define FIRST_SNAKE_X_ADDRESS 14
#define FIRST_SNAKE_Y_ADDRESS 24

/* Yilanin adreslerinin tutuldugu dizi */
#define MAX_SNAKE_LENGHT 100
uint8_t snake[MAX_SNAKE_LENGHT][2];

/* Yilanin uzunlugunun tutuldugu degisken ve ilk degeri 5 */
#define FIRST_SNAKE_LENGTH 5
uint8_t snake_length;

/* Yilan yonu */
#define SNAKE_MOVE_UP 	 0x00
#define SNAKE_MOVE_DOWN  0x01
#define SNAKE_MOVE_LEFT  0x02
#define SNAKE_MOVE_RIGHT 0x03 
/* Yilanin yonu */
uint8_t snake_direction;

/* Skor */
uint16_t score;
uint8_t bait[2];									

#define GAME_OVER    0x00
uint8_t Game_State = !GAME_OVER;

uint32_t count = 0;

/* Hardware konfigurasyon fonksiyonlari */
void RCC_Config(void);
void GPIO_Config(void);
void EXTI_Config(void);
void NVIC_Config(void);

/* LCD konfigurasyon fonksiyonu */
void LCD48x84_Config(void);
										
/* Oyun fonksiyonlari */
#define Score_Table(__S__) 				Write_Number(44, 0, __S__)
void Put_4px_Block(uint8_t x, uint8_t y, uint8_t sr);
void Put_A_Bait(uint8_t *x, uint8_t *y);
void Snake_Move(uint8_t d);
void Game_Init(void);
void Game_Over(void);
void Snake_Add(uint8_t l);
								

void EXTI15_10_IRQHandler(void)
{
	/* Oyun bittiyse */
	if(Game_State == GAME_OVER)
	{
		Game_Init();
		Game_State = !GAME_OVER;
	}
	/* Yukari butonu */
	else if(EXTI_GetITStatus(EXTI_Line12))
	{
		if(snake_direction != SNAKE_MOVE_DOWN)
			snake_direction = SNAKE_MOVE_UP;
		EXTI_ClearITPendingBit(EXTI_Line12);
	}
	/* Asagi butonu */
	else if(EXTI_GetITStatus(EXTI_Line13))
	{
		if(snake_direction != SNAKE_MOVE_UP)
			snake_direction = SNAKE_MOVE_DOWN;
		EXTI_ClearITPendingBit(EXTI_Line13);
	}
	/* Sola butonu */
	else if(EXTI_GetITStatus(EXTI_Line14))
	{
		if(snake_direction != SNAKE_MOVE_RIGHT)
			snake_direction = SNAKE_MOVE_LEFT;
		EXTI_ClearITPendingBit(EXTI_Line14);
	}
	/*Saga butonu */
	else if(EXTI_GetITStatus(EXTI_Line15))
	{
		if(snake_direction != SNAKE_MOVE_LEFT)
			snake_direction = SNAKE_MOVE_RIGHT;
		EXTI_ClearITPendingBit(EXTI_Line15);
	}
}
void SysTick_Handler(void)
{
	++count;
	if(count == UINT32_MAX) 
		count = 0;
}
int main(void)
{
	uint32_t i;

	RCC_Config();
	GPIO_Config();
	EXTI_Config();
	NVIC_Config();
	LCD48x84_Config();
	Game_Init();

	if (SysTick_Config(SystemCoreClock / 1000))
	{ 
		while (1);
	}
	while(1)
	{
		/* Yilanin yemi yemesi */
		if(snake[0][0] == bait[0] && snake[0][1] == bait[1] && Game_State != GAME_OVER)
		{
			Score_Table(++score);
			Snake_Add(++snake_length);
			Put_A_Bait(&bait[0], &bait[1]);
		}
		
		/* Yilanin haraket ettirilmesi */
		if(Game_State != GAME_OVER)
		{
			Snake_Move(snake_direction);
		}	
		
		/* Oyun hizi */
		for(i = 0; i < 0xaddff; ++i);	
		
		/* Yilanin kendi kendine carpmasi */
		for(i = 1; i < snake_length; ++i)
		{
			if(snake[0][0] == snake[i][0] && snake[0][1] == snake[i][1] && Game_State != GAME_OVER)
			{	
				Game_Over();
				Game_State = GAME_OVER;
			}
		}	
		
		/* Yilanin cerceveye carpmasi */
		if((snake[0][0] == 0 || snake[0][0] == 82 || snake[0][1] == 8 || snake[0][1] == 46) && Game_State != GAME_OVER)
		{
			Game_Over();
			Game_State = GAME_OVER;
		}
	}
}
void Game_Over(void)
{
	/* Oyun bittiginde gerceklecekler */
	Clear_Display();
	Write_String(align(ALIGN_CENTER, 9), 1, "GAME OVER");
	Write_String(align(ALIGN_CENTER, 5), 2, "SCORE");
	Write_Number(align(ALIGN_CENTER, 4), 3, score);
}
void Game_Init(void)
{
	uint8_t i;
	Clear_Display();
	
	/* Ilk degerler */
	snake_direction = SNAKE_MOVE_RIGHT;
	snake_length = FIRST_SNAKE_LENGTH;
	score = 0;
	
	/*Skor Tablosu*/ 
	Write_String(align(ALIGN_CENTER, 10), 0, "SCORE:");
	Score_Table(score);
	
	/* Cerceve */
	for(i = 1; i < 82; ++i)
		Write_A_Pixel(i, 9, PIXEL_SET);
	for(i = 1; i < 82; ++i)
		Write_A_Pixel(i, 46, PIXEL_SET);	
	for(i = 9; i < 47; ++i)
		Write_A_Pixel(82, i, PIXEL_SET);
	for(i = 9; i < 47; ++i)
		Write_A_Pixel(1, i, PIXEL_SET);	
	
	/* Adresler snake[][] e kopyalaniyor. */
	for(i = 0; i < FIRST_SNAKE_LENGTH; ++i)
	{
		snake[i][0] = FIRST_SNAKE_X_ADDRESS - (2 * i);
	  snake[i][1] = FIRST_SNAKE_Y_ADDRESS;
	}
	
	/* Yilan ilk konumuna ciziliyor. */
	for(i = 0; i < FIRST_SNAKE_LENGTH; ++i)
		Put_4px_Block(snake[i][0], snake[i][1], PIXEL_SET);

	/* Ilk yemin konulmasi */
	bait[0] = 50;
	bait[1] = 24;
	Put_4px_Block(bait[0], bait[1], PIXEL_SET);;
}

void Put_A_Bait(uint8_t *x, uint8_t *y)
{
	uint8_t i;
	/* Random yem yeri */
	*x = (count * 2) % 80 + 2;
	*y = (count * 2) % 36 + 10;
	
	/*Yem yilanin uzerine mi geldi */
	for(i = 0; i < snake_length; ++i)
	{
		if(*x == snake[i][0] && snake[i][1] == *y)
		{	
			*x = (count * 2) % 80 + 2;
			*y = (count * 2) % 36 + 10;
			i = 0;
		}
	}
	
	/* Yemin ekrana cizilmesi */
	Put_4px_Block(*x, *y, PIXEL_SET);
}

void Snake_Add(uint8_t l)
{	
	/* Son iki parca karsilastirmasi ve parcanin nereye ekleneceginin belirlenmesi */
	if(snake[l - 2][0] == snake[l - 3][0])
	{
		if(snake[l - 2][1] > snake[l - 3][1])	snake[l - 1][1] = snake[l - 2][1] + 2;
		else																	snake[l - 1][1] = snake[l - 2][1] - 2;
		snake[l - 1][0] = snake[l - 2][0];
	}
	else if(snake[l - 2][1] == snake[l - 3][1])
	{
		if(snake[l - 2][0] > snake[l - 3][0])	snake[l - 1][0] = snake[l - 2][0] + 2;
		else																	snake[l - 1][0] = snake[l - 2][0] - 2;
		snake[l - 1][1] = snake[l - 2][1];
	}
	
	/* Yilana eklenen parcanin cizilmesi */ 
	Put_4px_Block(snake[l - 1][0], snake[l - 1][1], PIXEL_SET);
	
}

void Snake_Move(uint8_t d)
{
	uint8_t i, temp[2];
	temp[0] = snake[snake_length - 1][0];
	temp[1] = snake[snake_length - 1][1];
	
	/* Snake dizisinde birer eleman kaydirma */
	for(i = 1; i < snake_length; ++i)
	{
		snake[snake_length - i][0] = snake[snake_length - i - 1][0];
		snake[snake_length - i][1] = snake[snake_length - i - 1][1];
	}
	/* Yon tayini */
	switch(d)
	{
		case SNAKE_MOVE_UP:
			snake[0][1] -= 2; break;
		case SNAKE_MOVE_DOWN:
			snake[0][1] += 2; break;
		case SNAKE_MOVE_LEFT:
			snake[0][0] -= 2; break;
		case SNAKE_MOVE_RIGHT:
			snake[0][0] += 2; break;
	}
	/* Yilani ciz */
	for(i = 0; i < snake_length; ++i)
		Put_4px_Block(snake[i][0], snake[i][1], PIXEL_SET);
	
	/* Yilanin son parcasini sil */
	Put_4px_Block(temp[0], temp[1], PIXEL_RESET);
}

void Put_4px_Block(uint8_t x, uint8_t y, uint8_t sr)
{
	Write_A_Pixel(x, y, sr);
	Write_A_Pixel(x, y + 1, sr);	
	Write_A_Pixel(x + 1, y, sr);
	Write_A_Pixel(x + 1, y + 1, sr);
}

void LCD48x84_Config(void)
{
	LCD48x84_InitTypedef InitStruct;
	
	InitStruct.Chip_Mode = Chip_Active;
	InitStruct.Addressing = Horizontal_Adressing;
	InitStruct.Bias_System = 0x15;
	InitStruct.Set_Vop = 0xB1;
	InitStruct.Display_Mode = Normal_Mode;
	InitStruct.Temperature_Control = Vlcd_Temp_Coef_0;
	LCD48x84_Init(&InitStruct);
}

void RCC_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
}

void GPIO_Config(void)
{
	GPIO_InitTypeDef InitStruct;
	
	InitStruct.GPIO_Pin = GPIO_Pin_0 | \
												GPIO_Pin_1 | \
												GPIO_Pin_2 | \
												GPIO_Pin_3 | \
			  							  GPIO_Pin_4;
	InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOA, &InitStruct);
	
	InitStruct.GPIO_Pin = GPIO_Pin_12 | \
											  GPIO_Pin_13 | \
												GPIO_Pin_14 | \
												GPIO_Pin_15;
	InitStruct.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOB, &InitStruct);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource12);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource13);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource14);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource15);
}

void EXTI_Config(void)
{
	EXTI_InitTypeDef InitStruct;
	
	InitStruct.EXTI_Line = EXTI_Line12 | \
												 EXTI_Line13 | \
												 EXTI_Line14 | \
												 EXTI_Line15;
	InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&InitStruct);
}	

void NVIC_Config(void)
{
	NVIC_InitTypeDef InitStruct;
	
	InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn;
	InitStruct.NVIC_IRQChannelPreemptionPriority = 0x0F;
	InitStruct.NVIC_IRQChannelSubPriority = 0x0F;
	InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&InitStruct);
}
