/**
	************************************************************
	************************************************************
	************************************************************
	*	�ļ����� 	onenet.c
	*
	*	���ߣ� 		�ż���
	*
	*	���ڣ� 		2017-05-08
	*
	*	�汾�� 		V1.1
	*
	*	˵���� 		��onenetƽ̨�����ݽ����ӿڲ�
	*
	*	�޸ļ�¼��	V1.0��Э���װ�������ж϶���ͬһ���ļ������Ҳ�ͬЭ��ӿڲ�ͬ��
	*				V1.1���ṩͳһ�ӿڹ�Ӧ�ò�ʹ�ã����ݲ�ͬЭ���ļ�����װЭ����ص����ݡ�
	************************************************************
	************************************************************
	************************************************************
**/

//��Ƭ��ͷ�ļ�
#include "stm32f10x.h"

//�����豸
#include "esp8266.h"

//Э���ļ�
#include "onenet.h"
#include "edpkit.h"

//Ӳ������
#include "usart.h"
#include "delay.h"
#include "led.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"   

//ͼƬ
#include "image_2k.h"

//C��
#include <string.h>
#include <stdio.h>


#define DEVID	"906152666"

#define APIKEY	"XZ=jyWgZn=obDTjbntQxFYxVs=4="

extern char stArray[1024];
extern unsigned int picture_file_size;
//==========================================================
//	�������ƣ�	OneNet_DevLink
//
//	�������ܣ�	��onenet��������
//
//	��ڲ�����	��
//
//	���ز�����	1-�ɹ�	0-ʧ��
//
//	˵����		��onenetƽ̨��������
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	EDP_PACKET_STRUCTURE edpPacket = {NULL, 0, 0, 0};				//Э���

	unsigned char *dataPtr;
	
	unsigned char status = 1;
	
	UsartPrintf(USART_DEBUG, "OneNet_DevLink\r\n"
                        "DEVID: %s,     APIKEY: %s\r\n"
                        , DEVID, APIKEY);

	if(EDP_PacketConnect1(DEVID, APIKEY, 256, &edpPacket) == 0)		//����devid �� apikey��װЭ���
	{
		
		
		ESP8266_SendData(edpPacket._data, edpPacket._len);			//�ϴ�ƽ̨
		
		dataPtr = ESP8266_GetIPD(250);								//�ȴ�ƽ̨��Ӧ
		if(dataPtr != NULL)
		{
			
			
			if(EDP_UnPacketRecv(dataPtr) == CONNRESP)
			{
				
				
				switch(EDP_UnPacketConnectRsp(dataPtr))
				{
					case 0:UsartPrintf(USART_DEBUG, "Tips:	���ӳɹ�\r\n");status = 0;break;
					
					case 1:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�Э�����\r\n");break;
					case 2:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��豸ID��Ȩʧ��\r\n");break;
					case 3:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�������ʧ��\r\n");break;
					case 4:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��û�ID��Ȩʧ��\r\n");break;
					case 5:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�δ��Ȩ\r\n");break;
					case 6:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ���Ȩ����Ч\r\n");break;
					case 7:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ�������δ����\r\n");break;
					case 8:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ����豸�ѱ�����\r\n");break;
					case 9:UsartPrintf(USART_DEBUG, "WARN:	����ʧ�ܣ��ظ��������������\r\n");break;
					
					default:UsartPrintf(USART_DEBUG, "ERR:	����ʧ�ܣ�δ֪����\r\n");break;
				}
			}
		}
		
		EDP_DeleteBuffer(&edpPacket);								//ɾ��
	}
	else
		UsartPrintf(USART_DEBUG, "WARN:	EDP_PacketConnect Failed\r\n");
	
	return status;
	
}

unsigned char OneNet_FillBuf(char *buf)
{
	
	char text[16];
	
	memset(text, 0, sizeof(text));
	
	strcpy(buf, "{");
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Red_Led\":%d,", led_status.Led4Sta);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Green_Led\":%d,", led_status.Led5Sta);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Yellow_Led\":%d,", led_status.Led6Sta);
	strcat(buf, text);
	
	memset(text, 0, sizeof(text));
	sprintf(text, "\"Blue_Led\":%d", led_status.Led7Sta);
	strcat(buf, text);
	
	strcat(buf, "}");
	
	return strlen(buf);

}

//==========================================================
//	�������ƣ�	OneNet_SendData_EDPType2
//
//	�������ܣ�	�ϴ����������ݵ�ƽ̨
//
//	��ڲ�����	devid���豸ID(�Ƽ�ΪNULL)
//				picture��ͼƬ����
//				pic_len��ͼƬ���ݳ���
//
//	���ز�����	��
//
//	˵����		���ǵ����豸����������ʱ������ʹ�������豸��͸��ģʽ
//				�ϴ�ͼƬ�ǣ�ǿ�ҽ���devid�ֶ�Ϊ�գ�����ƽ̨�ὫͼƬ�����·����豸
//==========================================================
#define PKT_SIZE 1400
void OneNet_SendData_Picture(char *devid, const char* picture, unsigned int pic_len)
{
	
	EDP_PACKET_STRUCTURE edpPacket = {NULL, 0, 0, 0};					//Э���

	char type_bin_head[] = "{\"ds_id\":\"pic\"}";						//ͼƬ����ͷ
	unsigned char *pImage = (unsigned char *)picture;
	
	if(EDP_PacketSaveData(devid, pic_len, type_bin_head, kTypeBin, &edpPacket) == 0)
	{	
		ESP8266_Clear();
		
		UsartPrintf(USART_DEBUG, "Send %d Bytes\r\n", edpPacket._len);
		ESP8266_SendData(edpPacket._data, edpPacket._len);				//�ϴ����ݵ�ƽ̨
		
		EDP_DeleteBuffer(&edpPacket);									//ɾ��
		
		UsartPrintf(USART_DEBUG, "image len = %d\r\n", pic_len);
		
		f_open(file,"0:PHOTO/PIC00066.bmp",FA_READ);
		
		while(pic_len > 0)
		{
			
			delay_ms(200);												//��ͼʱ��ʱ�������һ�㣬�����������һ����ʱ
			
			if(pic_len >= PKT_SIZE)
			{
				
				memset(stArray, 0, sizeof(stArray));	//����
				f_read(file,stArray,1400,&bw);
				bw+=1400;
				
				ESP8266_SendData(pImage, PKT_SIZE);						//���ڷ��ͷ�Ƭ-1024��pImageָ��stArray
				
//				pImage += PKT_SIZE;        
				pic_len -= PKT_SIZE;       
			}
			else
			{
				memset(stArray, 0, sizeof(stArray));	
				f_read(file,stArray,pic_len,&bw);
				
				ESP8266_SendData(pImage, (unsigned short)pic_len);		//���ڷ������һ����Ƭ-δ֪
				pic_len = 0;
			}
		}
		f_close(file);
		UsartPrintf(USART_DEBUG, "image send ok\r\n");
	}
	else
		UsartPrintf(USART_DEBUG, "EDP_PacketSaveData Failed\r\n");

}

//==========================================================
//	�������ƣ�	OneNet_SendData
//
//	�������ܣ�	�ϴ����ݵ�ƽ̨
//
//	��ڲ�����	type���������ݵĸ�ʽ
//
//	���ز�����	��
//
//	˵����		
//==========================================================

void OneNet_SendData(void)
{
	OneNet_SendData_Picture(NULL, stArray, picture_file_size);
}

//==========================================================
//	�������ƣ�	OneNet_RevPro
//
//	�������ܣ�	ƽ̨�������ݼ��
//
//	��ڲ�����	dataPtr��ƽ̨���ص�����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	EDP_PACKET_STRUCTURE edpPacket = {NULL, 0, 0, 0};	//Э���
	
	char *cmdid_devid = NULL;
	char *req = NULL;
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = EDP_UnPacketRecv(cmd);
	switch(type)										//�ж���pushdata���������·�
	{
		case SAVEACK:
			
			if(cmd[3] == MSG_ID_HIGH && cmd[4] == MSG_ID_LOW)
			{
				UsartPrintf(USART_DEBUG, "Tips:	Send %s\r\n", cmd[5] ? "Err" : "Ok");
			}
			else
				UsartPrintf(USART_DEBUG, "Tips:	Message ID Err\r\n");
			
		break;
			
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//��ջ���
	
	if(result == -1)
		return;
	
	dataPtr = strchr(req, '}');							//����'}'
	
	if(dataPtr != NULL)									//����ҵ���
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//�ж��Ƿ����·��������������
		{
			numBuf[num++] = *dataPtr++;
		}
		numBuf[num] = 0;
		
		num = atoi((const char *)numBuf);				//תΪ��ֵ��ʽ
		
		
	}
	
	if(type == CMDREQ && result == 0)						//���������� �� ����ɹ�
	{
		EDP_FreeBuffer(cmdid_devid);						//�ͷ��ڴ�
		EDP_FreeBuffer(req);
															//�ظ�����
		ESP8266_SendData(edpPacket._data, edpPacket._len);	//�ϴ�ƽ̨
		EDP_DeleteBuffer(&edpPacket);						//ɾ��
	}

}


/////////////////////////////////////////////////////////////////////
_Bool OneNet_Ping(void)
{
	EDP_PACKET_STRUCTURE edpPacket = {NULL, 0, 0, 0};	//Э���

	unsigned char *dataPtr;
	
	_Bool status = 1;
		if(EDP_PacketPing(&edpPacket)== 0)
	{
		ESP8266_SendData(edpPacket._data, edpPacket._len);				//�ϴ�ƽ̨
		dataPtr = ESP8266_GetIPD(2000);									//�ȴ�ƽ̨��Ӧ250
		if(dataPtr != NULL)
		{
			if(EDP_UnPacketRecv(dataPtr) == PINGRESP)
			{
				status=0;
			}
		}
		else
		{
			return status;
		}
		EDP_DeleteBuffer(&edpPacket);								//ɾ��		
	}
	return status;
}


