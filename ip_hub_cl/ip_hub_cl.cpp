// ip_hub_cl.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
//#include <conio.h>
#include <time.h>


using namespace std;

#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

// Порт сервера
//#define SERV_PORT 5000
// Размер приёмного буфера
#define BUFF_SIZE 256

struct SOCK_RECORD {
	// Сокет сервера
	SOCKET srv_socket;
	// Длина использованного сокета
//	int acc_sin_len;
	// Адрес использованного сокета
//	SOCKADDR_IN acc_sin;
	// Адрес сервера
	SOCKADDR_IN dest_sin;
};

int	UDP_PORT,TCP_PORT;

SOCK_RECORD s1,s2;

void ServerStart(SOCKET *srv_socket, int protocol);
int SendMsg(SOCK_RECORD *sr, int protocol, char *szBuf);
void parsePacket(char *szTemp, int rc);
int ServersStart(void);
void usage(void);

char curTime[32];
void getTime(void);

int _tmain(int argc, _TCHAR* argv[])
//int _tmain(int argc, char **argv)
{
	int rc;
	int need_reconnect;
	char szTemp[BUFF_SIZE+1];
	int count;

	if(argc!=3){
		usage();
		return FALSE;
	}
	else {
		UDP_PORT=_ttoi(argv[1]);
		TCP_PORT=_ttoi(argv[2]);
	}
	cout<<"UDP_PORT=" <<UDP_PORT <<" TCP_PORT="<<TCP_PORT <<endl;

	need_reconnect=1;
	count=0;

	do {
		if(need_reconnect==1) 
			if(ServersStart()){
				need_reconnect=0;
				cout<< "Packets received: " <<endl;
			}
			else {
				cout<< "Error: " <<WSAGetLastError() <<endl;
				return FALSE;
			}

		rc = recv(s2.srv_socket, szTemp, BUFF_SIZE, 0);
		if(rc>0)
		{
			szTemp[rc] = '\0';
			//cout<< szTemp <<endl;
			parsePacket(szTemp,rc);
			count++;
			cout<< '\r' <<count;

			if(SendMsg(&s1, SOCK_STREAM, szTemp)>0){
//				cout<< '.';
			}
			else {
				getTime();cout<< curTime;
				cout<< " TCP connection lost" <<endl;
				need_reconnect=1;
			}
		}
//		cout<< '*';
	}
	while(1);
//	while(!_kbhit());

	closesocket(s1.srv_socket);
	closesocket(s2.srv_socket);
	WSACleanup();
	return 0;
}

void ServerStart(SOCKET *srv_socket, int protocol)
{
	struct sockaddr_in srv_address;
	int port;
  
	port=(protocol==SOCK_STREAM)?TCP_PORT:UDP_PORT;

	// Создаем сокет сервера для работы с потоком данных
	*srv_socket = socket(AF_INET, protocol, 0);
	if(*srv_socket == INVALID_SOCKET) {
		cout<< "socket Error" <<endl;
		return;
	}

	// Устанавливаем адрес IP и номер порта
	srv_address.sin_family = AF_INET;
	srv_address.sin_addr.s_addr = INADDR_ANY;
	srv_address.sin_port = htons(port);

/*
	const char on = 1;
	setsockopt(*srv_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
*/

	// Связываем адрес IP с сокетом  
	if(bind(*srv_socket, (LPSOCKADDR)&srv_address, sizeof(srv_address)) == SOCKET_ERROR) {
		// При ошибке закрываем сокет
		closesocket(*srv_socket);
		cout<< "bind Error: " <<WSAGetLastError() <<endl;
		return;
	}
}

int SendMsg(SOCK_RECORD *sr, int protocol, char *szBuf)
{
int rc;
int port;
  
	port=(protocol==SOCK_STREAM)?TCP_PORT:UDP_PORT;

	// Посылаем сообщение
	if(protocol==SOCK_STREAM) {
		rc=send((*sr).srv_socket, szBuf, strlen(szBuf), 0);
	}
/*
	else {
		// Устанавливаем адрес IP и номер порта
		(*sr).dest_sin.sin_family = AF_INET;

		// Другой способ указания адреса узла
		(*sr).dest_sin.sin_addr.s_addr = inet_addr("127.0.0.1");
		// Копируем номер порта
		(*sr).dest_sin.sin_port = htons(port);

		rc=sendto((*sr).srv_socket, szBuf, strlen(szBuf), 0,
			(PSOCKADDR)&((*sr).dest_sin), sizeof((*sr).dest_sin));
	}
*/
	return rc;
}

int ServersStart(void){
	WSADATA WSAData;

	WSACleanup();
	if(WSAStartup(MAKEWORD(2, 2), &WSAData)!=0){
		cout<< "WSAStartup Error" <<endl;
		return FALSE;
	}

	ServerStart(&s2.srv_socket, SOCK_DGRAM);
	getTime();cout<< curTime;
	cout<< " UDP Server start Ok" <<endl;

	ServerStart(&s1.srv_socket, SOCK_STREAM);
	getTime();cout<< curTime;
	cout<< " TCP Server start Ok" <<endl;

	if(listen(s1.srv_socket, 1) == SOCKET_ERROR){
		closesocket(s1.srv_socket);
		cout<< "listen Error" <<endl;
		return FALSE;
	}
	getTime();cout<< curTime;
	cout <<" TCP server listen" <<endl;

	(s1.srv_socket)=accept(s1.srv_socket,0,0);
	if((s1.srv_socket) == INVALID_SOCKET){
		cout <<"accept Error, invalid socket" <<endl;
		return FALSE;
	}
	getTime();cout<< curTime;
	cout<< " client connected" <<endl;

	return TRUE;
}

void parsePacket(char *szTemp, int rc){
	int i;
	for(i=0;i<rc;i++)
		if(szTemp[i]>' ')szTemp[i]++;
}

void getTime(void){
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf_s(curTime,"%4d.%02d.%02d %02d:%02d:%02d",st.wYear,st.wMonth,st.wDay,st.wHour, st.wMinute,st.wSecond);
}

void usage(void){
	cout<<"usage: ip_hub_cl.exe UDP_PORT TCP_PORT" <<endl;
}
