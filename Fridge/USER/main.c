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
//网络协议层
#include "onenet.h"
//网络设备
#include "esp8266.h" 
//C库
#include <string.h> 
#include "mpu6050.h"  
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h" 

//#define  CONNECTSTATE  1
#define  OV7725 1
#define  OV7670 2

//由于OV7725传感器安装方式原因,OV7725_WINDOW_WIDTH相当于LCD的高度，OV7725_WINDOW_HEIGHT相当于LCD的宽度
//注意：此宏定义只对OV7725有效
#define  OV7725_WINDOW_WIDTH		320 // <=320
#define  OV7725_WINDOW_HEIGHT		240 // <=240

extern u8 ov_sta;	//在exit.c里 面定义					//帧中断标记
extern u8 ov_frame;	//在timer.c里面定义 			//统计帧数

u8 flag=0;		//拍照标记

char stArray[1025]={0};
char buf[10];
//u32 sd_size=0;
unsigned int picture_file_size=153666;			//一张图片大小

//任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		128  
//任务堆栈
StackType_t StartTaskStack[START_STK_SIZE];
//任务控制块
StaticTask_t StartTaskTCB;
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

//任务优先级
#define TASK1_TASK_PRIO		3
//任务堆栈大小	
#define TASK1_STK_SIZE 		512  
//任务堆栈
StackType_t Task1TaskStack[TASK1_STK_SIZE];
//任务控制块
StaticTask_t Task1TaskTCB;
//任务句柄
TaskHandle_t Task1Task_Handler;
//任务函数
void LCD_Show(void *pvParameters);

//任务优先级
#define TASK2_TASK_PRIO		2
//任务堆栈大小	
#define TASK2_STK_SIZE 		512 
//任务堆栈
StackType_t Task2TaskStack[TASK2_STK_SIZE];
//任务控制块
StaticTask_t Task2TaskTCB;
//任务句柄
TaskHandle_t Task2Task_Handler;
//任务函数
void Photograph(void *pvParameters);

//任务优先级
#define TASK3_TASK_PRIO		2
//任务堆栈大小	
#define TASK3_STK_SIZE 		512 
//任务堆栈
StackType_t Task3TaskStack[TASK3_STK_SIZE];
//任务控制块
StaticTask_t Task3TaskTCB;
//任务句柄
TaskHandle_t Task3Task_Handler;
//任务函数
void SendData(void *pvParameters);

void OV7725_camera_refresh(void)
{
	u32 i,j;
 	u16 color;	 
	if(ov_sta)//有帧中断更新
	{
		LCD_Scan_Dir(U2D_L2R);//从上到下,从左到右
		LCD_Set_Window((lcddev.width - OV7725_WINDOW_WIDTH) / 2, (lcddev.height - OV7725_WINDOW_HEIGHT) / 2, OV7725_WINDOW_WIDTH, OV7725_WINDOW_HEIGHT);//将显示区域设置到屏幕中央
		if(lcddev.id == 0X1963)	//横竖屏判断
			LCD_Set_Window((lcddev.width-OV7725_WINDOW_WIDTH) / 2, (lcddev.height - OV7725_WINDOW_HEIGHT) / 2, OV7725_WINDOW_HEIGHT, OV7725_WINDOW_WIDTH);//将显示区域设置到屏幕中央
		LCD_WriteRAM_Prepare();     //开始写入GRAM	
		OV7725_RRST = 0;				//开始复位读指针 
		OV7725_RCK_L;
		OV7725_RCK_H;
		OV7725_RCK_L;
		OV7725_RRST = 1;				//复位读指针结束 
		OV7725_RCK_H; 
		for(i = 0; i < OV7725_WINDOW_HEIGHT; i++)
		{
			for(j = 0; j < OV7725_WINDOW_WIDTH; j++)
			{
				OV7725_RCK_L;
				color = GPIOC -> IDR & 0XFF;	//读数据
				OV7725_RCK_H; 
				color <<= 8;  
				OV7725_RCK_L;
				color |= GPIOC -> IDR & 0XFF;	//读数据
				OV7725_RCK_H; 
				LCD -> LCD_RAM = color;  
			}
		}
 		ov_sta = 0;					//清零帧中断标记
		ov_frame++; 
		LCD_Scan_Dir(DFT_SCAN_DIR);	//恢复默认扫描方向 
	} 
}

//文件命名
//组合成:形如"0:PHOTO/PIC00066.bmp"的文件名
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
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);//设置系统中断优先级分组4	 
	delay_init();	    				//延时函数初始化	 
	uart_init(460800);					//初始化串口
	LED_Init();		  					//初始化LED
	LCD_Init();							//初始化LCD
	KEY_Init();					//初始化按键
	usmart_dev.init(72);		//初始化USMART		
	Usart3_Init(460800);							//串口2，驱动ESP8266用
  ESP8266_Init();					//初始化ESP8266（若在传数据任务里初始化会影响画面）
  UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
	
    //创建开始任务
	StartTask_Handler = xTaskCreateStatic((TaskFunction_t	)start_task,		//任务函数
											(const char* 	)"start_task",		//任务名称
											(uint32_t 		)START_STK_SIZE,	//任务堆栈大小
											(void* 		  	)NULL,				//传递给任务函数的参数
											(UBaseType_t 	)START_TASK_PRIO, 	//任务优先级
											(StackType_t*   )StartTaskStack,	//任务堆栈
											(StaticTask_t*  )&StartTaskTCB);	//任务控制块              
    vTaskStartScheduler();          //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
	taskENTER_CRITICAL();           //进入临界区
  //创建TASK1任务
	Task1Task_Handler = xTaskCreateStatic((TaskFunction_t	)LCD_Show,		
											(const char* 	)"LCD_Show_task",		
											(uint32_t 		)TASK1_STK_SIZE,	
											(void* 		  	)NULL,				
											(UBaseType_t 	)TASK1_TASK_PRIO, 	
											(StackType_t*   )Task1TaskStack,	
											(StaticTask_t*  )&Task1TaskTCB);	
	//创建TASK2任务
	Task2Task_Handler = xTaskCreateStatic((TaskFunction_t	)Photograph,		
											(const char* 	)"Photograph_task",		
											(uint32_t 		)TASK2_STK_SIZE,	
											(void* 		  	)NULL,				
											(UBaseType_t 	)TASK2_TASK_PRIO, 	
											(StackType_t*   )Task2TaskStack,	
											(StaticTask_t*  )&Task2TaskTCB);
	//创建TASK3任务
	Task3Task_Handler = xTaskCreateStatic((TaskFunction_t	)SendData,		
											(const char* 	)"SendData_task",		
											(uint32_t 		)TASK3_STK_SIZE,	
											(void* 		  	)NULL,				
											(UBaseType_t 	)TASK3_TASK_PRIO, 	
											(StackType_t*   )Task3TaskStack,	
											(StaticTask_t*  )&Task3TaskTCB);									
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}

//task1任务函数
void LCD_Show(void *pvParameters)
{
	
		if(OV7725_Init() == 0)
		{
			LCD_ShowString(30, 250, 200, 16, 16, "OV7725 Init OK       ");
			OV7725_Window_Set(OV7725_WINDOW_WIDTH, OV7725_WINDOW_HEIGHT, 0);//QVGA模式输出(320*240)
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
			OV7725_camera_refresh();//更新显示
			vTaskDelay(150);        //延时100ms，同时也是休眠，期间低优先级的任务2可以运行（显示数据）	
		}
}

//task2任务函数
void Photograph(void *pvParameters)
{
	u8 temperature;  	    
	u8 humidity; 
	u8 tim = 0;
	u8 res;							 
	u8 *pname;				//带路径的文件名 	   					 
	u8 sd_ok = 1;				//0,sd卡不正常;1,SD卡正常.
	float pitch,roll,yaw; 		//欧拉角
	UBaseType_t uxPriority;
	
	MPU_Init();					//初始化MPU6050
 	POINT_COLOR = RED;			//设置字体为红色 
	while(mpu_dmp_init())
 	{
		LCD_ShowString(30, 130, 200, 16, 16, "MPU6050 Error");
		delay_ms(200);
		LCD_Fill(30, 130, 239, 130 + 16, WHITE);
 		delay_ms(200);
	}  
	W25QXX_Init();				//初始化W25Q128（SPI FLASH）
 	my_mem_init(SRAMIN);		//初始化内部内存池
	exfuns_init();				//为fatfs相关变量申请内存  
 	f_mount(fs[0], "0:", 1); 		//挂载SD卡 
 	f_mount(fs[1], "1:", 1); 		//挂载FLASH. 
	uxPriority = uxTaskPriorityGet( NULL );	//获取当前任务优先级
	while(font_init()) 				//检查字库
	{	    
		LCD_ShowString(30, 50, 200, 16, 16, "Font Error!");
		delay_ms(200);				  
		LCD_Fill(30, 50, 240, 66, WHITE);//清除显示	     
	}
	res = f_mkdir("0:/PHOTO");		//创建PHOTO文件夹
	if(res != FR_EXIST&&res != FR_OK) 	//发生了错误
	{		    
		Show_Str(30, 150, 240, 16, "SD卡错误!", 16, 0);
		delay_ms(200);				  
		Show_Str(30, 170, 240, 16, "拍照功能将不可用!", 16, 0);
		sd_ok = 0;  	
	}
	else
	{
		Show_Str(30, 150, 240, 16, "SD卡正常!", 16, 0);
		delay_ms(200);				  
		Show_Str(30, 170, 240, 16, "KEY0:拍照", 16, 0);
		sd_ok = 1;  	  
	}								
	pname=mymalloc(SRAMIN, 30);	//为带路径的文件名分配30个字节的内存		    
 	while(pname == NULL)			//内存分配出错
 	{	    
		Show_Str(30, 190, 240, 16, "内存分配失败!", 16, 0);
		delay_ms(200);				  
		LCD_Fill(30, 190, 240, 146, WHITE);//清除显示	     
		delay_ms(200);				  
	}   					
	TIM6_Int_Init(10000, 7199);			//10Khz计数频率,1秒钟中断									  
	EXTI8_Init();						//使能外部中断8,捕获帧中断	
	LCD_Clear(BLACK);
	POINT_COLOR=RED;			//设置字体为红色 
	while(DHT11_Init())	//DHT11初始化	
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
			DHT11_Read_Data(&temperature, &humidity);	//读取温湿度值
			tim = 0;
		}
		LCD_ShowNum(280 + 40, 350, temperature, 2, 16);	//显示温度	   		   
		LCD_ShowNum(280 + 40, 370, humidity, 2, 16);		//显示湿度	
		if(mpu_dmp_get_data(&pitch, &roll, &yaw) == 0)		//获取陀螺仪数据
		{
			if(yaw > 50)		//用航向角判断开门
			{
				if(sd_ok)
				{
					LED1 = 0;	//点亮DS1,提示正在拍照
					camera_new_pathname(pname);//得到文件名		
					vTaskPrioritySet( NULL, ( uxPriority + 2 ) );			//此时拍照任务最重要，提高优先级		
					LCD_ShowString(280, 350, 200, 16, 16, "Temp:  C");	 
					LCD_ShowString(280, 370, 200, 16, 16, "Humi:  %");
					LCD_ShowNum(280 + 40, 350, temperature, 2, 16);	//显示温度	   		   
					LCD_ShowNum(280 + 40, 370, humidity, 2, 16);		//显示湿度	
					bmp_encode(pname, (lcddev.width - 240) / 2, (lcddev.height - 320) / 2, 240, 320, 1);//拍照，即截屏，覆盖保存
					flag = 1;			//拍照成功标志
					vTaskPrioritySet( NULL, ( uxPriority - 2 ) );			//恢复优先级
					Show_Str(40, 130, 240, 12, "拍照成功!", 12, 0);
					Show_Str(40, 150, 240, 12, "保存为:", 12, 0);
					Show_Str(40 + 42, 150, 240, 12, pname, 12, 0);		    
				}
				else //提示SD卡错误
				{					    
					Show_Str(40, 130, 240, 12, "SD卡错误!", 12, 0);
					Show_Str(40, 150, 240, 12, "拍照功能不可用!", 12, 0);			    
				}
				LED1 = 1;//关闭DS1
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
	while(OneNet_DevLink())			//接入OneNET
		delay_ms(500);
	
	 while(1)
	{
			if(flag == 1)
			{
				if(OneNet_Ping() != 0)	//Ping不通
				{
					ESP8266_Clear();
					ESP8266_rejoin();		//8266初始化
					while(OneNet_DevLink())			//接入OneNET
						delay_ms(500);
					UsartPrintf(USART_DEBUG, "OneNet_SendData\r\n");
					OneNet_SendData();									//发送数据
					flag = 0;
				}
				else if(OneNet_Ping() == 0)
				{
					UsartPrintf(USART_DEBUG, "OneNet_SendData\r\n");
					OneNet_SendData();									//发送数据
					flag = 0;	
				}	
			}
			ESP8266_Clear();		
			dataPtr = ESP8266_GetIPD(0);
			if(dataPtr != NULL)
				OneNet_RevPro(dataPtr);
	} 
}



