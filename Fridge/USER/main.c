#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "timer.h"
#include "lcd.h"
#include "FreeRTOS.h"
#include "task.h"
#include "key.h"
#include "string.h"
#include "ov7725.h"
#include "ov7670.h"
#include "exti.h"
#include "usmart.h"
#include "dht11.h" 	
#include "malloc.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"   
#include "text.h"	
#include "math.h"	 
#include "beep.h"  
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "exfuns.h"  
//����Э���
#include "onenet.h"
//�����豸
#include "esp8266.h" 
//C��
#include <string.h> 
#include "mpu6050.h"  
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 

//#define  CONNECTSTATE  1
#define  OV7725 1
#define  OV7670 2

//����OV7725��������װ��ʽԭ��,OV7725_WINDOW_WIDTH�൱��LCD�ĸ߶ȣ�OV7725_WINDOW_HEIGHT�൱��LCD�Ŀ��
//ע�⣺�˺궨��ֻ��OV7725��Ч
#define  OV7725_WINDOW_WIDTH		320 // <=320
#define  OV7725_WINDOW_HEIGHT		240 // <=240

extern u8 ov_sta;	//��exit.c�� �涨��					//֡�жϱ��
extern u8 ov_frame;	//��timer.c���涨�� 			//ͳ��֡��

u8 flag=0;		//���ձ��

char stArray[1025]={0};
char buf[10];
//u32 sd_size=0;
unsigned int picture_file_size=153666;			//һ��ͼƬ��С

//�������ȼ�
#define START_TASK_PRIO		1
//�����ջ��С	
#define START_STK_SIZE 		128  
//�����ջ
StackType_t StartTaskStack[START_STK_SIZE];
//������ƿ�
StaticTask_t StartTaskTCB;
//������
TaskHandle_t StartTask_Handler;
//������
void start_task(void *pvParameters);

//�������ȼ�
#define TASK1_TASK_PRIO		3
//�����ջ��С	
#define TASK1_STK_SIZE 		512  
//�����ջ
StackType_t Task1TaskStack[TASK1_STK_SIZE];
//������ƿ�
StaticTask_t Task1TaskTCB;
//������
TaskHandle_t Task1Task_Handler;
//������
void LCD_Show(void *pvParameters);

//�������ȼ�
#define TASK2_TASK_PRIO		2
//�����ջ��С	
#define TASK2_STK_SIZE 		512 
//�����ջ
StackType_t Task2TaskStack[TASK2_STK_SIZE];
//������ƿ�
StaticTask_t Task2TaskTCB;
//������
TaskHandle_t Task2Task_Handler;
//������
void Photograph(void *pvParameters);

//�������ȼ�
#define TASK3_TASK_PRIO		2
//�����ջ��С	
#define TASK3_STK_SIZE 		512 
//�����ջ
StackType_t Task3TaskStack[TASK3_STK_SIZE];
//������ƿ�
StaticTask_t Task3TaskTCB;
//������
TaskHandle_t Task3Task_Handler;
//������
void SendData(void *pvParameters);

void OV7725_camera_refresh(void)
{
	u32 i,j;
 	u16 color;	 
	if(ov_sta)//��֡�жϸ���
	{
		LCD_Scan_Dir(U2D_L2R);//���ϵ���,������
		LCD_Set_Window((lcddev.width - OV7725_WINDOW_WIDTH) / 2, (lcddev.height - OV7725_WINDOW_HEIGHT) / 2, OV7725_WINDOW_WIDTH, OV7725_WINDOW_HEIGHT);//����ʾ�������õ���Ļ����
		if(lcddev.id == 0X1963)	//�������ж�
			LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH) / 2, (lcddev.height - OV7725_WINDOW_HEIGHT) / 2, OV7725_WINDOW_HEIGHT, OV7725_WINDOW_WIDTH);//����ʾ�������õ���Ļ����
		LCD_WriteRAM_Prepare();     //��ʼд��GRAM	
		OV7725_RRST = 0;				//��ʼ��λ��ָ�� 
		OV7725_RCK_L;
		OV7725_RCK_H;
		OV7725_RCK_L;
		OV7725_RRST = 1;				//��λ��ָ����� 
		OV7725_RCK_H; 
		for(i = 0; i < OV7725_WINDOW_HEIGHT; i++)
		{
			for(j = 0; j < OV7725_WINDOW_WIDTH; j++)
			{
				OV7725_RCK_L;
				color = GPIOC -> IDR & 0XFF;	//������
				OV7725_RCK_H; 
				color <<= 8;  
				OV7725_RCK_L;
				color |= GPIOC -> IDR & 0XFF;	//������
				OV7725_RCK_H; 
				LCD -> LCD_RAM = color;  
			}
		}
 		ov_sta = 0;					//����֡�жϱ��
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//�ָ�Ĭ��ɨ�跽�� 
	} 
}

//�ļ�����
//��ϳ�:����"0:PHOTO/PIC00066.bmp"���ļ���
void camera_new_pathname(u8 *pname)
{	 

	while(1)
	{
		sprintf((char*)pname, "0:PHOTO/PIC00066.bmp");
		break;
	}
}

int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//����ϵͳ�ж����ȼ�����4	 
	delay_init();	    				//��ʱ������ʼ��	 
	uart_init(460800);					//��ʼ������
	LED_Init();		  					//��ʼ��LED
	LCD_Init();							//��ʼ��LCD
	KEY_Init();					//��ʼ������
	usmart_dev.init(72);		//��ʼ��USMART		
	Usart3_Init(460800);							//����2������ESP8266��
  ESP8266_Init();					//��ʼ��ESP8266�����ڴ������������ʼ����Ӱ�컭�棩
  UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
	
    //������ʼ����
	StartTask_Handler = xTaskCreateStatic((TaskFunction_t	)start_task,		//������
											(const char* 	)"start_task",		//��������
											(uint32_t 		)START_STK_SIZE,	//�����ջ��С
											(void* 		  	)NULL,				//���ݸ��������Ĳ���
											(UBaseType_t 	)START_TASK_PRIO, 	//�������ȼ�
											(StackType_t*   )StartTaskStack,	//�����ջ
											(StaticTask_t*  )&StartTaskTCB);	//������ƿ�              
    vTaskStartScheduler();          //�����������
}

//��ʼ����������
void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();           //�����ٽ���
  //����TASK1����
	Task1Task_Handler = xTaskCreateStatic((TaskFunction_t	)LCD_Show,		
											(const char* 	)"LCD_Show_task",		
											(uint32_t 		)TASK1_STK_SIZE,	
											(void* 		  	)NULL,				
											(UBaseType_t 	)TASK1_TASK_PRIO, 	
											(StackType_t*   )Task1TaskStack,	
											(StaticTask_t*  )&Task1TaskTCB);	
	//����TASK2����
	Task2Task_Handler = xTaskCreateStatic((TaskFunction_t	)Photograph,		
											(const char* 	)"Photograph_task",		
											(uint32_t 		)TASK2_STK_SIZE,	
											(void* 		  	)NULL,				
											(UBaseType_t 	)TASK2_TASK_PRIO, 	
											(StackType_t*   )Task2TaskStack,	
											(StaticTask_t*  )&Task2TaskTCB);
	//����TASK3����
	Task3Task_Handler = xTaskCreateStatic((TaskFunction_t	)SendData,		
											(const char* 	)"SendData_task",		
											(uint32_t 		)TASK3_STK_SIZE,	
											(void* 		  	)NULL,				
											(UBaseType_t 	)TASK3_TASK_PRIO, 	
											(StackType_t*   )Task3TaskStack,	
											(StaticTask_t*  )&Task3TaskTCB);									
    vTaskDelete(StartTask_Handler); //ɾ����ʼ����
    taskEXIT_CRITICAL();            //�˳��ٽ���
}

//task1������
void LCD_Show(void *pvParameters)
{
	
		if(OV7725_Init() == 0)
		{
			LCD_ShowString(30, 250, 200, 16, 16, "OV7725 Init OK       ");
			OV7725_Window_Set(OV7725_WINDOW_WIDTH, OV7725_WINDOW_HEIGHT, 0);//QVGAģʽ���(320*240)
			OV7725_CS = 0;
		}
		else
		{
			LCD_ShowString(30, 230, 200, 16, 16, "OV7725_OV7670 Error!!");
			delay_ms(200);
			LCD_Fill(30, 230, 239, 246, WHITE);
			delay_ms(200);
		}
		LCD_Clear(BLACK);
		while(1)
		{
			OV7725_camera_refresh();//������ʾ
			vTaskDelay(150);        //��ʱ100ms��ͬʱҲ�����ߣ��ڼ�����ȼ�������2�������У���ʾ���ݣ�	
		}
}

//task2������
void Photograph(void *pvParameters)
{
	u8 temperature;  	    
	u8 humidity; 
	u8 tim = 0;
	u8 res;							 
	u8 *pname;				//��·�����ļ��� 	   					 
	u8 sd_ok = 1;				//0,sd��������;1,SD������.
	float pitch,roll,yaw; 		//ŷ����
	UBaseType_t uxPriority;
	
	MPU_Init();					//��ʼ��MPU6050
 	POINT_COLOR = RED;			//��������Ϊ��ɫ 
	while(mpu_dmp_init())
 	{
		LCD_ShowString(30, 130, 200, 16, 16, "MPU6050 Error");
		delay_ms(200);
		LCD_Fill(30, 130, 239, 130 + 16, WHITE);
 		delay_ms(200);
	}  
	W25QXX_Init();				//��ʼ��W25Q128��SPI FLASH��
 	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	exfuns_init();				//Ϊfatfs��ر��������ڴ�  
 	f_mount(fs[0], "0:", 1); 		//����SD�� 
 	f_mount(fs[1], "1:", 1); 		//����FLASH. 
	uxPriority = uxTaskPriorityGet( NULL );	//��ȡ��ǰ�������ȼ�
	while(font_init()) 				//����ֿ�
	{	    
		LCD_ShowString(30, 50, 200, 16, 16, "Font Error!");
		delay_ms(200);				  
		LCD_Fill(30, 50, 240, 66, WHITE);//�����ʾ	     
	}
	res = f_mkdir("0:/PHOTO");		//����PHOTO�ļ���
	if(res != FR_EXIST&&res != FR_OK) 	//�����˴���
	{		    
		Show_Str(30, 150, 240, 16, "SD������!", 16, 0);
		delay_ms(200);				  
		Show_Str(30, 170, 240, 16, "���չ��ܽ�������!", 16, 0);
		sd_ok = 0;  	
	}
	else
	{
		Show_Str(30, 150, 240, 16, "SD������!", 16, 0);
		delay_ms(200);				  
		Show_Str(30, 170, 240, 16, "KEY0:����", 16, 0);
		sd_ok = 1;  	  
	}								
	pname=mymalloc(SRAMIN, 30);	//Ϊ��·�����ļ�������30���ֽڵ��ڴ�		    
 	while(pname == NULL)			//�ڴ�������
 	{	    
		Show_Str(30, 190, 240, 16, "�ڴ����ʧ��!", 16, 0);
		delay_ms(200);				  
		LCD_Fill(30, 190, 240, 146, WHITE);//�����ʾ	     
		delay_ms(200);				  
	}   					
	TIM6_Int_Init(10000, 7199);			//10Khz����Ƶ��,1�����ж�									  
	EXTI8_Init();						//ʹ���ⲿ�ж�8,����֡�ж�	
	LCD_Clear(BLACK);
	POINT_COLOR=RED;			//��������Ϊ��ɫ 
	while(DHT11_Init())	//DHT11��ʼ��	
	{
		LCD_ShowString(30, 130, 200, 16, 16, "DHT11 Error");
		delay_ms(200);
		LCD_Fill(30, 130, 239, 130 + 16, WHITE);
 		delay_ms(200);
	}				
	
	while(1)
	{
		LCD_ShowString(280, 350, 200, 16, 16, "Temp:  C");	 
		LCD_ShowString(280, 370, 200, 16, 16, "Humi:  %");
		tim++;
		if(tim == 30)
		{
			DHT11_Read_Data(&temperature, &humidity);	//��ȡ��ʪ��ֵ
			tim = 0;
		}
		LCD_ShowNum(280 + 40, 350, temperature, 2, 16);	//��ʾ�¶�	   		   
		LCD_ShowNum(280 + 40, 370, humidity, 2, 16);		//��ʾʪ��	
		if(mpu_dmp_get_data(&pitch, &roll, &yaw) == 0)		//��ȡ����������
		{
			if(yaw > 50)		//�ú�����жϿ���
			{
				if(sd_ok)
				{
					LED1 = 0;	//����DS1,��ʾ��������
					camera_new_pathname(pname);//�õ��ļ���		
					vTaskPrioritySet( NULL, ( uxPriority + 2 ) );			//��ʱ������������Ҫ��������ȼ�		
					LCD_ShowString(280, 350, 200, 16, 16, "Temp:  C");	 
					LCD_ShowString(280, 370, 200, 16, 16, "Humi:  %");
					LCD_ShowNum(280 + 40, 350, temperature, 2, 16);	//��ʾ�¶�	   		   
					LCD_ShowNum(280 + 40, 370, humidity, 2, 16);		//��ʾʪ��	
					bmp_encode(pname, (lcddev.width - 240) / 2, (lcddev.height - 320) / 2, 240, 320, 1);//���գ������������Ǳ���
					flag = 1;			//���ճɹ���־
					vTaskPrioritySet( NULL, ( uxPriority - 2 ) );			//�ָ����ȼ�
					Show_Str(40, 130, 240, 12, "���ճɹ�!", 12, 0);
					Show_Str(40, 150, 240, 12, "����Ϊ:", 12, 0);
					Show_Str(40 + 42, 150, 240, 12, pname, 12, 0);		    
				}
				else //��ʾSD������
				{					    
					Show_Str(40, 130, 240, 12, "SD������!", 12, 0);
					Show_Str(40, 150, 240, 12, "���չ��ܲ�����!", 12, 0);			    
				}
				LED1 = 1;//�ر�DS1
				LCD_Clear(BLACK);
			}
		}
		else 
			delay_ms(5);
	}
}

void SendData(void *pvParameters)
{
	unsigned char *dataPtr = NULL;
	while(OneNet_DevLink())			//����OneNET
		delay_ms(500);
	
	 while(1)
	{
			if(flag == 1)
			{
				if(OneNet_Ping() != 0)	//Ping��ͨ
				{
					ESP8266_Clear();
					ESP8266_rejoin();		//8266��ʼ��
					while(OneNet_DevLink())			//����OneNET
						delay_ms(500);
					UsartPrintf(USART_DEBUG, "OneNet_SendData\r\n");
					OneNet_SendData();									//��������
					flag = 0;
				}
				else if(OneNet_Ping() == 0)
				{
					UsartPrintf(USART_DEBUG, "OneNet_SendData\r\n");
					OneNet_SendData();									//��������
					flag = 0;	
				}	
			}
			ESP8266_Clear();		
			dataPtr = ESP8266_GetIPD(0);
			if(dataPtr != NULL)
				OneNet_RevPro(dataPtr);
	} 
}



