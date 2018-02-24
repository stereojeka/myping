// myping.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <WinSock2.h>
#include <conio.h>
#include <stdint.h>
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib") //winsock 2.2 library
#pragma warning(disable:4996)

#define ICMP_ECHO       8   /* Echo Request         */
#define	ICMP_HEADERSIZE 8

unsigned short in_cksum(unsigned short *ptr, int nbytes);

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;

struct ip 				// Структура заголовка IP
{
	BYTE ip_verlen;			// Version and header length
	BYTE ip_tos;			// Type of service
	WORD ip_len;			// Total packet length 
	UINT ip_id;			// Datagram identification 
	WORD ip_fragoff;		// Fragment offset 
	BYTE ip_ttl;			// Time to live 
	BYTE ip_proto;			// Protocol
	UINT ip_chksum;			// Checksum 
	IN_ADDR ip_src_addr;		// Source address 
	IN_ADDR ip_dst_addr;		// Destination address 
	BYTE ip_data[1];		// Variable length data area
};

struct icmphdr
{
	u_int8_t type;      /* message type */
	u_int8_t code;      /* type sub-code */
	u_int16_t checksum;
	union
	{
		struct
		{
			u_int16_t   id;
			u_int16_t   sequence;
		} echo;         /* echo datagram */
		u_int32_t   gateway;    /* gateway address */
		struct
		{
			u_int16_t   __unused;
			u_int16_t   mtu;
		} frag;         /* path mtu discovery */
	} un;
};

int main(int argc, char *argv[])
{
	char *packet, *data = NULL;

	SOCKET s;
	int k = 1, packet_size, payload_size = 512, sent = 0;
	int iHostAddrLength;		// Длина адреса сетевого компьютера
	int iReceivedBytes;		// Количество принятых байтов
	BYTE IcmpSendPacket[1024];	// Буфер для посылаемых данных
	BYTE IcmpRecvPacket[4096];	// Буфер для принимаемых данных
	SOCKADDR_IN	sockAddrHost;

	PDWORD pdwTimeStamp;	// Счетчик "тиков" при передаче
	DWORD dwReturnTime;		// Счетчик "тиков" при приеме
	DWORD dwRoundTrip;		// Счетчик "тиков" среднего времени пробега

	struct iphdr *iph = NULL;
	struct icmphdr *icmph = NULL;
	struct sockaddr_in dest;
	struct ip *pIpHeader;		// Указатель на структуру-заголовок IP

	//Initialise Winsock
	WSADATA wsock;
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsock) != 0)
	{
		fprintf(stderr, "WSAStartup() failed");
		exit(EXIT_FAILURE);
	}
	printf("Done");

	//Create Raw ICMP Packet
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == SOCKET_ERROR)
	{
		printf("Failed to create raw icmp packet");
		exit(EXIT_FAILURE);
	}

	dest.sin_family = AF_INET;
	InetPton(AF_INET, _T("216.58.213.196"), &dest.sin_addr.s_addr);

	packet_size = sizeof(struct icmphdr) + payload_size;
	packet = (char *)malloc(packet_size);

	//zero out the packet buffer
	memset(packet, 0, packet_size);

	icmph = (struct icmphdr*) packet;
	icmph->type = ICMP_ECHO;
	icmph->code = 0;
	icmph->un.echo.sequence = rand();
	icmph->un.echo.id = rand();

	// Initialize the TCP payload to some rubbish
	data = packet + sizeof(struct icmphdr);
	memset(data, '^', payload_size);

	//checksum
	icmph->checksum = 0;
	icmph->checksum = in_cksum((unsigned short *)icmph, packet_size);

	printf("\nSending packets...\n");

	while (sent < 4)
	{
		pdwTimeStamp = (PDWORD)&IcmpSendPacket[ICMP_HEADERSIZE];
		*pdwTimeStamp = GetTickCount();

		if (sendto(s, packet, packet_size, 0, (struct sockaddr *)&dest, sizeof(dest)) == SOCKET_ERROR)
		{
			printf("Error sending Packet : %d", WSAGetLastError());
			break;
		}

		printf("%d packets send\r", ++sent);

		iHostAddrLength = sizeof(sockAddrHost);

		iReceivedBytes = recvfrom(s, (LPSTR)IcmpRecvPacket,
			sizeof(IcmpRecvPacket), 0, (LPSOCKADDR)&sockAddrHost,
			&iHostAddrLength);

		if (iReceivedBytes == SOCKET_ERROR) {
			int iSocketError = WSAGetLastError();
			if (iSocketError == 10004) {
				std::cout << "Ping operation was cancelled." << std::endl;
				return(TRUE);
			}
			else {
				std::cout << "Socket Error from recvfrom(): " << iSocketError << std::endl;
				return(FALSE);
			}
		}

		dwReturnTime = GetTickCount();
		dwRoundTrip = dwReturnTime - *pdwTimeStamp;

		// Указываем на IP-заголовок принятого пакета
		pIpHeader = (struct ip *)IcmpRecvPacket;
	
		std::cout << "Round-trip travel time to " << "[" << inet_ntoa(sockAddrHost.sin_addr) << "] was " << dwRoundTrip << "ms." << std::endl;

	}

	WSACleanup();

	return 0;
}

/*
Function calculate checksum
*/
unsigned short in_cksum(unsigned short *ptr, int nbytes)
{
	register long sum;
	u_short oddbyte;
	register u_short answer;

	sum = 0;
	while (nbytes > 1) {
		sum += *ptr++;
		nbytes -= 2;
	}

	if (nbytes == 1) {
		oddbyte = 0;
		*((u_char *)& oddbyte) = *(u_char *)ptr;
		sum += oddbyte;
	}

	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = ~sum;

	return (answer);
}
