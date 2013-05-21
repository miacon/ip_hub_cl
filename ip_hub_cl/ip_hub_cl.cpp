// ip_hub_cl.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
//#include <conio.h>
#include <time.h>
#include <stdint.h>

using namespace std;

#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")

// Порт сервера
//#define SERV_PORT 5000
// Размер приёмного буфера
#define BUFF_SIZE 256
#define QUATS_ARRAY_COUNT 4
#define MAX_QUATS 20
#define TCP_HEADER 0x55aa
#define PACKET_TYPE 0x0211

struct SOCK_RECORD {
	// Сокет сервера
	SOCKET srv_socket;
	// Длина использованного сокета
	int acc_sin_len;
	// Адрес использованного сокета
	SOCKADDR acc_sin;
	// Адрес сервера
	SOCKADDR_IN dest_sin;
};

int	UDP_PORT,TCP_PORT;

SOCK_RECORD s1,s2;

void ServerStart(SOCKET *srv_socket, int protocol);
int SendMsg(SOCK_RECORD *sr, int protocol, char *szBuf, int length);
int parsePacket(char *szTemp, int rc);
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
	int count_r;
	int count_s;

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
	count_r=count_s=0;

	do {
		if(need_reconnect==1){ 
			count_r=count_s=0;
			if(ServersStart()){
				need_reconnect=0;
				cout<< "Packets received/sent: " <<endl;
			}
			else {
				cout<< "Error: " <<WSAGetLastError() <<endl;
				return FALSE;
			}
		}

		rc = recv(s2.srv_socket, szTemp, BUFF_SIZE, 0);
		if(rc>0){
			count_r++;
			cout<< '\r' <<count_r;

			if((rc=parsePacket(szTemp,rc))>0){
				if(SendMsg(&s1, SOCK_STREAM, szTemp, rc)>0){
					count_s++;
					cout<< "/" <<count_s;
				}
				else {
					getTime();cout<<endl  <<curTime;
					cout<< "/" <<count_s;
					cout <<" TCP connection lost" <<endl;
					need_reconnect=1;
				}
			}
			else {
				cout<< "/" <<count_s;
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

int SendMsg(SOCK_RECORD *sr, int protocol, char *szBuf, int length)
{
int rc;
int port;
  
	port=(protocol==SOCK_STREAM)?TCP_PORT:UDP_PORT;

	// Посылаем сообщение
	if(protocol==SOCK_STREAM) {
		rc=send((*sr).srv_socket, szBuf, length, 0);
	}
/*
	else {
		// Устанавливаем адрес IP и номер порта
		(*sr).dest_sin.sin_family = AF_INET;

		// Другой способ указания адреса узла
		(*sr).dest_sin.sin_addr.s_addr = inet_addr("127.0.0.1");
		// Копируем номер порта
		(*sr).dest_sin.sin_port = htons(port);

		rc=sendto((*sr).srv_socket, szBuf, length, 0,
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
	cout<< " UDP server start Ok" <<endl;

	ServerStart(&s1.srv_socket, SOCK_STREAM);
	getTime();cout<< curTime;
	cout<< " TCP server start Ok" <<endl;

	if(listen(s1.srv_socket, 1) == SOCKET_ERROR){
		closesocket(s1.srv_socket);
		cout<< "listen Error" <<endl;
		return FALSE;
	}
	getTime();cout<< curTime;
	cout <<" TCP server listen port "<< TCP_PORT <<endl;

	s1.acc_sin_len=sizeof(s1.acc_sin);
	(s1.srv_socket)=accept(s1.srv_socket,&(s1.acc_sin),&(s1.acc_sin_len));
	if((s1.srv_socket) == INVALID_SOCKET){
		cout <<"accept Error, invalid socket" <<endl;
		return FALSE;
	}
	getTime();cout<< curTime;
	cout<< " client " << inet_ntoa(((struct sockaddr_in*)&(s1.acc_sin))->sin_addr) <<" connected" <<endl;

	return TRUE;
}

int parsePacket(char *szTemp, int rc){
	int i,count,quats_count;

//Input packet
	struct {
		uint32_t counter;
		uint32_t mask;
		uint32_t charge;
	} m_in;
	float quats[MAX_QUATS][QUATS_ARRAY_COUNT];
//------------

//Output packet
	struct {
		uint16_t Id;
		uint16_t Len;
		uint16_t Type;
		uint16_t Cntr;
		uint16_t Stat0;
		uint16_t Stat1;
		uint16_t Ubat;
	}m_out;

	struct {
		int16_t Quat[QUATS_ARRAY_COUNT];
		int16_t Rsrv;
	} sensors[MAX_QUATS];
	
	uint16_t check_sum;
//------------

	uint32_t mask_tmp;
	
	//Quatilions count calculated by size of input packet
	quats_count=((rc-sizeof(m_in))/sizeof(float))/QUATS_ARRAY_COUNT;

	//Проверки 1-не менее 3х uint32_t, 2-quats не более 20ти, 3-пакет полный(кратен 4м)
	if((rc<sizeof(m_in))||(quats_count>MAX_QUATS)||(QUATS_ARRAY_COUNT*sizeof(float)*quats_count!=(rc-sizeof(m_in)))){
		return 0;
	}

	memcpy(&m_in,&szTemp[0],sizeof(m_in));
	memcpy(&quats,&szTemp[sizeof(m_in)],quats_count*QUATS_ARRAY_COUNT*sizeof(float));

	//Проверка соответствия маски и реального количества кватерионов
	count=0;
	mask_tmp=m_in.mask;
	for(i=0;i<MAX_QUATS;i++){
			if(mask_tmp&1) count++;
			mask_tmp>>=1;
	}
	if(count!=quats_count){
		return 0;
	}

	//Fill headers
	m_out.Id=TCP_HEADER;
	m_out.Len=sizeof(m_out)+sizeof(sensors)+sizeof(check_sum);
	m_out.Type=PACKET_TYPE;
	m_out.Cntr=(uint16_t)(m_in.counter);
	m_out.Stat0=(uint16_t)(m_in.mask&0xFFFF);
	m_out.Stat1=(uint16_t)((m_in.mask>>16)&0x000F);
	m_out.Ubat=(uint16_t)(m_in.charge);

	//Zero check_sum
	check_sum=0;

	//Fill Sensors fields
	count=0;
	mask_tmp=m_in.mask;
	for(i=0;i<MAX_QUATS;i++){
		if(mask_tmp&1){
			for(int j=0;j<QUATS_ARRAY_COUNT;j++){
				sensors[i].Quat[j]=(int16_t)floor(quats[count][j]*32768);
				check_sum+=(uint16_t)sensors[i].Quat[j];
			}
			sensors[i].Rsrv=0;
			count++;
		}
		else {
			memset(&sensors[i],0,sizeof(sensors[i]));
		}
		mask_tmp>>=1;
	}

	//Continue Check_sum calculation
	check_sum+=m_out.Id;
	check_sum+=m_out.Len;
	check_sum+=m_out.Type;
	check_sum+=m_out.Cntr;
	check_sum+=m_out.Stat0;
	check_sum+=m_out.Stat1;
	check_sum+=m_out.Ubat;

	//Fill output buffer
	memcpy(&szTemp[0],&m_out,sizeof(m_out));
	memcpy(&szTemp[sizeof(m_out)],&sensors,sizeof(sensors));
	memcpy(&szTemp[sizeof(m_out)+sizeof(sensors)],&check_sum,sizeof(check_sum));

	return m_out.Len;
}

void getTime(void){
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf_s(curTime,"%4d.%02d.%02d %02d:%02d:%02d",st.wYear,st.wMonth,st.wDay,st.wHour, st.wMinute,st.wSecond);
}

void usage(void){
	cout<<"usage: ip_hub_cl.exe UDP_PORT TCP_PORT" <<endl;
}
