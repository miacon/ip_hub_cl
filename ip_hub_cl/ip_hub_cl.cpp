/*
  ip_hub_cl.cpp:
  Version 1.0, 24 May 2013

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.
  
  This is TCP/IP server console application.
  - Listen to a TCP port (port number is configurable from a command line)
  - Catch UDP packets broadcast to a specific UDP port (port number is configurable from a command line)
  - Parse the packets, take certain data fields

  Usage:
  ip_hub_cl.exe UDP_PORT TCP_PORT
 
  Design for Inertial Labs, Inc. , BlueNet Solutions.
  Vladimir Mihaylov  t380689005276@gmail.com
*/


#include "stdafx.h"

//Input Buffer size
#define BUFF_SIZE 256

//Quaternion elements count 4 :)
#define QUATS_ARRAY_COUNT 4

//Maximum quaternions allowed
#define MAX_QUATS 20

//TCP answer preamble
#define TCP_HEADER 0x55aa

//Packet answer type
#define PACKET_TYPE 0x0211

struct SOCK_RECORD {
	//Server socket
	SOCKET srv_socket;
	//Server socket length
	int acc_sin_len;
	//Address of socket
	SOCKADDR acc_sin;
	//Server address
	SOCKADDR_IN dest_sin;
};

//UDP & TCP port numbers
int	UDP_PORT, TCP_PORT;

//s1 - record for UDP; s2 - for TCP
SOCK_RECORD s1,s2;

//Start server socket. One for UDP and one for TCP
void ServerStart(SOCKET *srv_socket, int protocol);

//Routine for datagramm send
int SendMsg(SOCK_RECORD *sr, int protocol, char *szBuf, int length);

//Converter from UDP to TCP packets, according given requarements
int parsePacket(char *szTemp, int rc);

//routine for start sequence
int ServersStart(void);

//tiny usage help
void usage(void);

//string for current time
char curTime[32];
//convert current time to string curTime
void getTime(void);

int _tmain(int argc, _TCHAR* argv[])
{
	int rc;
	int need_reconnect;
	char szTemp[BUFF_SIZE+1];

//readen packets counter
	int count_r;

//writen packets counter
	int count_s;

//check for correct command line parametres
	if(argc!=3){
		usage();
		return FALSE;
	}
	else {
		UDP_PORT=_ttoi(argv[1]);
		TCP_PORT=_ttoi(argv[2]);
	}
	cout<<"UDP_PORT=" <<UDP_PORT <<" TCP_PORT="<<TCP_PORT <<endl;

//First we need connection
	need_reconnect=1;
//Zero input counters
	count_r=count_s=0;

//endless loop
	do {
		//first start or restart after disconnect
		if(need_reconnect==1){ 
			//Zero input counters
			count_r=count_s=0;
			if(ServersStart()){
				//We are connected... Lets work!
				need_reconnect=0;
				cout<< "Packets received/sent: " <<endl;
			}
			else {
				//Fatality
				cout<< "Error: " <<WSAGetLastError() <<endl;
				return FALSE;
			}
		}

		//here receive UDP packet
		rc = recv(s2.srv_socket, szTemp, BUFF_SIZE, 0);
		if(rc>0){
			//nonempty input packet will be counted
			count_r++;
			//Carrige return and write on the same place input counter
			cout<< '\r' <<count_r;

			//Lets decode input packet
			if((rc=parsePacket(szTemp,rc))>0){
				//if all ok send TCP datagramm
				if(SendMsg(&s1, SOCK_STREAM, szTemp, rc)>0){
					//nonempty output packet will be counted
					count_s++;
					cout<< "/" <<count_s;
				}
				else {
					//We lost TCP connection
					//maybe exists better way to find out it? Who knows...
					getTime();cout<<endl  <<curTime;
					cout<< "/" <<count_s;
					cout <<" TCP connection lost" <<endl;
					//I'll be back. (c)
					need_reconnect=1;
				}
			}
			else {
				//output packet counter did not changed, but it must be renew in console
				cout<< "/" <<count_s;
			}
		}//if(rc)
	}//do
	while(1); //forewer

	//I must write this. Maybe in deep future somebody come here...
	closesocket(s1.srv_socket);
	closesocket(s2.srv_socket);
	WSACleanup();
	return 0;
}

void ServerStart(SOCKET *srv_socket, int protocol)
{
	struct sockaddr_in srv_address;
	int port;
  
	//Select port
	port=(protocol==SOCK_STREAM)?TCP_PORT:UDP_PORT;

	//Create socket
	*srv_socket = socket(AF_INET, protocol, 0);
	if(*srv_socket == INVALID_SOCKET) {
		cout<< "socket Error" <<endl;
		return;
	}

	//Set IP and Port
	srv_address.sin_family = AF_INET;
	srv_address.sin_addr.s_addr = INADDR_ANY;
	srv_address.sin_port = htons(port);

	//Bind Socket on IP
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

	//Send TCP datagramm
	if(protocol==SOCK_STREAM) {
		rc=send((*sr).srv_socket, szBuf, length, 0);
	}
/*
	else {
    //we dont need to sent UDP packets
		(*sr).dest_sin.sin_family = AF_INET;
		(*sr).dest_sin.sin_addr.s_addr = inet_addr("127.0.0.1");
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
	//Winsock startup
	if(WSAStartup(MAKEWORD(2, 2), &WSAData)!=0){
		cout<< "WSAStartup Error" <<endl;
		return FALSE;
	}

	//Start UDP server
	ServerStart(&s2.srv_socket, SOCK_DGRAM);
	getTime();cout<< curTime;
	cout<< " UDP server start Ok" <<endl;

	//Start TCP server
	ServerStart(&s1.srv_socket, SOCK_STREAM);
	getTime();cout<< curTime;
	cout<< " TCP server start Ok" <<endl;

	//Listen TCP port
	if(listen(s1.srv_socket, 1) == SOCKET_ERROR){
		closesocket(s1.srv_socket);
		cout<< "listen Error" <<endl;
		return FALSE;
	}
	getTime();cout<< curTime;
	cout <<" TCP server listen port "<< TCP_PORT <<endl;

	//Accept connection
	//Here we will wait, until somebody to connect
	s1.acc_sin_len=sizeof(s1.acc_sin);
	(s1.srv_socket)=accept(s1.srv_socket,&(s1.acc_sin),&(s1.acc_sin_len));
	if((s1.srv_socket) == INVALID_SOCKET){
		cout <<"accept Error, invalid socket" <<endl;
		return FALSE;
	}
	//Celebrate connection
	getTime();cout<< curTime;
	cout<< " client " << inet_ntoa(((struct sockaddr_in*)&(s1.acc_sin))->sin_addr) <<" connected" <<endl;

	return TRUE;
}

int parsePacket(char *szTemp, int rc){
	int i,count,quats_count;

//Input packet
//See documentation
	struct {
		uint32_t counter;
		uint32_t mask;
		uint32_t charge;
	} m_in;
	float quats[MAX_QUATS][QUATS_ARRAY_COUNT];
//------------

//Output packet
//See documentation
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

	//Check min packet size
	//quats no more MAX_QUATS
	//Packet size ok?
	if((rc<sizeof(m_in))||(quats_count>MAX_QUATS)||(QUATS_ARRAY_COUNT*sizeof(float)*quats_count!=(rc-sizeof(m_in)))){
		return 0;
	}

	//Copy input first 3 elements from buffer into structure
	memcpy(&m_in,&szTemp[0],sizeof(m_in));
	//Copy least buffer into quternions array
	memcpy(&quats,&szTemp[sizeof(m_in)],quats_count*QUATS_ARRAY_COUNT*sizeof(float));

	//Check mask and quternion count. It must be the same
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
				//Convert quaternion from float to fixed point format
				sensors[i].Quat[j]=(int16_t)floor(quats[count][j]*32768);
				check_sum+=(uint16_t)sensors[i].Quat[j];
			}
			sensors[i].Rsrv=0;
			count++;
		}
		else {
			//Zero unused sensors
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
