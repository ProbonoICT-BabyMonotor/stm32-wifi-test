/*
 * ESP8266_HAL.c
 *
 *  Created on: Apr 14, 2020
 *      Author: Controllerstech
 */

#include "UartRingbuffer_multi.h"
#include "ESP8266_HAL.h"
#include "stdio.h"
#include "string.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

#define wifi_uart &huart2
#define pc_uart &huart3

uint8_t buffer[10000]; // 06/15 버퍼 사이즈를 20에서 10000으로 늘림

void ESP_Clear_Buffer()
{
	memset(buffer, 0, 10000);
}

/*****************************************************************************************************************************************/

void ESP_Init (char *SSID, char *PASSWD){
	char data[10000];
	int len;

	Ringbuf_init();

	HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT+RST\r\n", strlen("AT+RST\r\n"), 100);
	Uart_sendstring("RESETTING.", pc_uart);
	for (int i=0; i<5; i++)
	{
		Uart_sendstring(".", pc_uart);
		HAL_Delay(1000);
	}

	Uart_sendstring("\r\n", pc_uart);


	/********* 현재 와이파이 리스트 출력하기 **********/
	Uart_sendstring("\r\n[현재 와이파이 리스트를 출력합니다]", pc_uart);
	HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT+CWLAP\r\n", strlen("AT+CWLAP\r\n"), 100);
	while (!(Wait_for("CWLAP", wifi_uart)));
	while (!(Copy_upto("OK",buffer, wifi_uart)));
	while (!(Wait_for("OK\r\n", wifi_uart)));
	len = strlen (buffer);
	buffer[len-2] = '\0';
	sprintf (data, "%s\r\n", buffer);
	Uart_sendstring(data, pc_uart);
	ESP_Clear_Buffer();

	/********* AT **********/
	Uart_sendstring("\r[와이파이 연결 시도]\r\n", pc_uart);
	HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT\r\n", strlen("AT\r\n"), 100);
	while(!(Wait_for("AT\r\r\n\r\nOK\r\n", wifi_uart)));
	Uart_sendstring("\r-(완료) WIFI 모듈 자체 테스트 \r\n", pc_uart);
	ESP_Clear_Buffer();

	/********* AT+CWMODE=3 **********/
	HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT+CWMODE=3\r\n", strlen("AT+CWMODE=3\r\n"), 100);
	while (!(Wait_for("AT+CWMODE=3\r\r\n\r\nOK\r\n", wifi_uart)));
	Uart_sendstring("-(완료) AP모드 & Station Mode 동시 설정\r\n", pc_uart);
	ESP_Clear_Buffer();

	/***   Multiple Connection 허용. 연결 되기전에 명령어 실행해야함****/
	HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT+CIPMUX=1\r\n", strlen("AT+CIPMUX=1\r\n"), 100);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	Uart_sendstring("-(완료) 동시 접속 허용 설정\r\n", pc_uart);
	ESP_Clear_Buffer();

	/********* AT+CWJAP="SSID","PASSWD" : WIFI 연결 **********/
	Uart_sendstring("-(진행중) 와이파이 연결중..\n", pc_uart);
	sprintf (data, "AT+CWJAP=\"%s\",\"%s\"\r\n", SSID, PASSWD);
	HAL_UART_Transmit(wifi_uart, (uint8_t*)data, strlen(data), 100);
	while (!(Wait_for("WIFI GOT IP\r\n\r\nOK\r\n", wifi_uart)));
	sprintf (data, "\r\n-(완료) \"%s\"에 연결되었습니다!\r\n", SSID);
	Uart_sendstring(data,pc_uart);
	ESP_Clear_Buffer();

	/********* AT+CIFSR : IP 주소 확인 **********/
	Uart_sendstring("AT+CIFSR\r\n", wifi_uart);
	while (!(Wait_for("CIFSR:STAIP,\"", wifi_uart)));
	while (!(Copy_upto("\"",buffer, wifi_uart)));
	while (!(Wait_for("OK\r\n", wifi_uart)));

	// 06/15 아래 문제는 해결됨
	/** 이 부분이 문제, 여기서 데이터를 시리얼보드로 전송하면서 메모리에 오류가 나버림**/
	/** main으로 돌아가야할 메모리 주소가 날라가버림; **/
	len = strlen (buffer);
	buffer[16] = '\0';
	sprintf (data, "\r\nIP ADDR: %s\r\n", buffer);
	Uart_sendstring(data, pc_uart);
	ESP_Clear_Buffer();

	/********* AT+CIPSERVER : ESP8266에서 TCP 서버 열기 **********/
	HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT+CIPSERVER=1\r\n", strlen("AT+CIPSERVER=1\r\n"), 100);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	Uart_sendstring("\r\n-(완료) 333포트 번호를 열었습니다!\r\n", pc_uart);
	ESP_Clear_Buffer();


	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

	/********* AT+CIPSTART : 외부 서버 TCP에 연결하기 **********/
	//HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT+CIPSTART=0,\"TCP\",\"58.227.202.87\",9999,30\r\n", strlen("AT+CIPSTART=0,\"TCP\",\"192.168.45.179\",23,30\r\n"), 100);
	//while (!(Wait_for("OK", wifi_uart)));
	//Uart_sendstring("\r\n-(완료) TCP 연결에 성공했습니다!\r\n", pc_uart);
	//ESP_Clear_Buffer();

	/********* AT+CIPSEND : 연결된 서버에 데이터 전송하기 **********/
	//Uart_sendstring("\r\n-(진행중) 데이터 전송 시도 중..\r\n", pc_uart);

	//uint8_t temp[150], cipsend[50];
	//memset(cipsend, 0, 50);
	//memset(temp, 0, 150);

	//sprintf (temp, "GET /index.html\r\n");
	//sprintf(cipsend, "AT+CIPSEND=0,%i\r\n", strlen(temp));

	//* 이 부분이 강력히 문제임*//
	// 문제점 1 : CIPSEND 명령어를 주면 "OK >" 이 출력되었을 때 데이터를 전송해야함.
	// 문제점 1 : 하지만 "OK"는 반환되는 것 같으나 ">"이 리턴이 안됨
	//HAL_UART_Transmit(wifi_uart, (uint8_t*)cipsend, strlen(cipsend), 100);
	//while (!(Wait_for(">", wifi_uart)))
	//ESP_Clear_Buffer();

	// 문제점 2 : 문제점 1에 이어 SEND가 되는지도 모르겠음.
	// 이 두가지 문제만 고치면 됨.

	//HAL_UART_Transmit(wifi_uart, (uint8_t*)temp, strlen(temp), 100);
	//while (!(Wait_for("SEND OK", wifi_uart)));
	//ESP_Clear_Buffer();

	//Uart_sendstring("-(완료) TCP 데이터 전송 완료했습니다!\r\n", pc_uart);
	//**//

	/********* AT+CLOSE : TCP 서버 연결 종료하기 **********/
	//HAL_UART_Transmit(wifi_uart, (uint8_t*)"AT+CLOSE=0\r\n", strlen("AT+CLOSE=0\r\n"), 100);
	//while (!(Wait_for("OK", wifi_uart)));
	//ESP_Clear_Buffer();
	//Uart_sendstring("-(완료) TCP 연결이 종료 되었습니다.\r\n", pc_uart);


}


int Server_Send (char *str, int Link_ID)
{
	int len = strlen (str);
	char data[80] = {};
	sprintf (data, "AT+CIPSEND=%d,%d\r\n", Link_ID, len);
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for(">", wifi_uart)));
	Uart_sendstring (str, wifi_uart);
	while (!(Wait_for("SEND OK", wifi_uart)));
	sprintf (data, "AT+CIPCLOSE=5\r\n");
	Uart_sendstring(data, wifi_uart);
	while (!(Wait_for("OK\r\n", wifi_uart)));
	return 1;
}

int check_recv_brup(int milli_time){
	uint8_t received_data;
	if (HAL_UART_Receive(wifi_uart, &received_data, 1, milli_time) == HAL_OK) {
		Uart_sendstring("- 다른 데이터가 수신되었습니다. 트름 유도 기능을 종료하고, 다른 기능을 수행합니다.\r\n", pc_uart);
		ESP_Clear_Buffer();

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);

		// 데이터가 들어왔을 경우 break
		return 1;
	}
	return 0;
}

int check_recv_swing(int milli_time){
	uint8_t received_data;
	if (HAL_UART_Receive(wifi_uart, &received_data, 1, milli_time) == HAL_OK) {
		Uart_sendstring("- 다른 데이터가 수신되었습니다. 스윙 기능을 종료하고, 다른 기능을 수행합니다.\r\n", pc_uart);
		ESP_Clear_Buffer();

		// 보드 LED
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);

		// 스윙 기능
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);

		Uart_sendstring("%s\r\n", received_data);
		ESP_Clear_Buffer();

		// 데이터가 들어왔을 경우 break
		return 1;
	}
	return 0;
}

int check_recv_backdraft(int milli_time){
	uint8_t received_data;
	if (HAL_UART_Receive(wifi_uart, &received_data, 1, milli_time) == HAL_OK) {
		Uart_sendstring("- 다른 데이터가 수신되었습니다. 역류 방지 기능을 종료하고, 다른 기능을 수행합니다.\r\n", pc_uart);
		ESP_Clear_Buffer();


		// 역류 방지 기능 요청 내리기
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
		HAL_Delay(14000);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);

		Uart_sendstring("%s\r\n", received_data);
		ESP_Clear_Buffer();

		// 데이터가 들어왔을 경우 break
		return 1;
	}
	return 0;
}
void Server_Start (void){
	char buftocopyinto[255] = {0};
	char Link_ID = 0;
	while (!(Get_after("+IPD,", 1, &Link_ID, wifi_uart)));
	Link_ID -= 48;
	while (!(Copy_upto(" HTTP", buftocopyinto, wifi_uart)));
	if (Look_for("GET /backdraft/on", buftocopyinto) == 1){
		Uart_sendstring("- 역류방지를 위해 침대를 기울입니다.\r\n", pc_uart);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		if(check_recv_backdraft(14000)){
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		}
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
		Uart_sendstring("- [완료] 역류방지를 위해 침대를 기울입니다.\r\n", pc_uart);
		ESP_Clear_Buffer();
		Server_Send("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nContent-type: application/json\r\nDate: Mon, 20 Aug 2018 07:59:05 GMT\r\nConnection: close\r\n\r\n", Link_ID);
	}
	else if (Look_for("GET /backdraft/off", buftocopyinto) == 1){
		Uart_sendstring("- 역류방지 완료 후 침대를 내립니다.\r\n", pc_uart);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_RESET);
		HAL_Delay(14000);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
		Uart_sendstring("- [완료] 역류방지 완료 후 침대를 내립니다.\r\n", pc_uart);
		ESP_Clear_Buffer();
		Server_Send("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nContent-type: application/json\r\nDate: Mon, 20 Aug 2018 07:59:05 GMT\r\nConnection: close\r\n\r\n", Link_ID);
	}
	else if (Look_for("GET /swing/on", buftocopyinto) == 1){
		Uart_sendstring("- 스윙 기능을 실행합니다.\r\n", pc_uart);
		ESP_Clear_Buffer();
		Server_Send("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nContent-type: application/json\r\nDate: Mon, 20 Aug 2018 07:59:05 GMT\r\nConnection: close\r\n\r\n", Link_ID);
		HAL_Delay(1000);

		while(1){
			ESP_Clear_Buffer();
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // 보드 내 LED
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET); // 스윙 기능
			if(check_recv_swing(500)){
				break;
			}

			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); // 보드 내 LEDㄴ
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET); // 스윙 기능
			if(check_recv_swing(300)){
				break;
			}

			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); // 보드 내 LED
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET); // 스윙 기능
			if(check_recv_swing(500)){
				break;
			}

			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET); // 보드 내 LED
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET); // 스윙 기능
			if(check_recv_swing(300)){
				break;
			}
		}
	}
	else if (Look_for("GET /brup/on", buftocopyinto) == 1){
		while(1){
			int x;
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
			for(x=0; x<100; x=x+1){
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
				microDelay(1000);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
				microDelay(1000);
			}
			if(check_recv_brup(500)){
				break;
			}
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
			for(x=0; x<100; x=x+1)
			{
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
				microDelay(stepDelay);
				HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
				microDelay(stepDelay);
			}
			if(check_recv_brup(500)){
				break;
			}

		}
	}
	else if (Look_for("GET /done", buftocopyinto) == 1){
		Uart_sendstring("- 현재 실행 중인 기능이 없어 종료가 불가능 합니다.\r\n", pc_uart);
		ESP_Clear_Buffer();
		Server_Send("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nContent-type: application/json\r\nDate: Mon, 20 Aug 2018 07:59:05 GMT\r\nConnection: close\r\n\r\n", Link_ID);
	}


	//HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
	//	       HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
	//	       HAL_UART_Receive_IT(&huart2, &received_data, 1);
	////	       HAL_Delay(500);
	//	       if(check_recv(500)){
	//	    	   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	//	    	   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	//	    	   break;
	//	       }
	//
	//	       HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	//	       HAL_UART_Receive_IT(&huart2, &received_data, 1);
	////	       HAL_Delay(300);
	//
	//	       if(check_recv(300)){
	//	    	   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	//	    	   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	//	    	   break;
	//	       }
	//
	//
	//	       HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
	//	       HAL_UART_Receive_IT(&huart2, &received_data, 1);
	////	       HAL_Delay(500);
	//	       if(check_recv(500)){
	//	    	   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	//	    	   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	//	    	   break;
	//	       }
	//
	//	       HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	//	       HAL_UART_Receive_IT(&huart2, &received_data, 1);
	////	       HAL_Delay(300);
	//	       if(check_recv(300)){
	//	    	   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
	//	    	   HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
	//	    	   break;
	//	       }
	//	    }

	else if (Look_for("GET /swing/off", buftocopyinto) == 1){
		Uart_sendstring("- 스윙 기능을 멈춥니다.\r\n", pc_uart);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
		ESP_Clear_Buffer();
		Server_Send("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nContent-type: application/json\r\nDate: Mon, 20 Aug 2018 07:59:05 GMT\r\nConnection: close\r\n\r\n", Link_ID);
	}
}




/*
void Server_Handle (char *str, int Link_ID)
{
	char datatosend[1024] = {0};
	if (!(strcmp (str, "/ledon")))
	{
		sprintf (datatosend, Basic_inclusion);
		strcat(datatosend, LED_ON);
		strcat(datatosend, Terminate);
		Server_Send(datatosend, Link_ID);
	}

	else if (!(strcmp (str, "/ledoff")))
	{
		sprintf (datatosend, Basic_inclusion);
		strcat(datatosend, LED_OFF);
		strcat(datatosend, Terminate);
		Server_Send(datatosend, Link_ID);
	}

	else
	{
		sprintf (datatosend, Basic_inclusion);
		strcat(datatosend, LED_OFF);
		strcat(datatosend, Terminate);
		Server_Send(datatosend, Link_ID);
	}

}
 */
