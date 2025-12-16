#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "2223"

#pragma comment (lib, "Ws2_32.lib")

DWORD WINAPI ConnectionHandler(LPVOID CSocket);

void whoopsie() {
	__asm {
		jmp esp
		jmp eax
		pop eax
		pop eax
		ret
	}
}

int main(int argc, char* argv[]) {
	char PortNumber[6];

	strncpy(PortNumber, DEFAULT_PORT, 6);

	WSADATA wsaData;
	SOCKET ListenSocket = INVALID_SOCKET,
		ClientSocket = INVALID_SOCKET;
	struct addrinfo* result = NULL, hints;
	int Result;
	struct sockaddr_in ClientAddress;
	int ClientAddressL = sizeof(ClientAddress);

	Result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (Result != 0) {
		printf("WSAStartup failed with error: %d\n", Result);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	Result = getaddrinfo(NULL, PortNumber, &hints, &result);
	if (Result != 0) {
		printf("Getaddrinfo failed with error: %d\n", Result);
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	Result = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (Result == SOCKET_ERROR) {
		printf("Bind failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	Result = listen(ListenSocket, SOMAXCONN);
	if (Result == SOCKET_ERROR) {
		printf("Listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	while (ListenSocket) {
		printf("Waiting for client connections...\n");

		ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientAddress, &ClientAddressL);
		if (ClientSocket == INVALID_SOCKET) {
			printf("Accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		printf("Received a client connection from %s:%u\n", inet_ntoa(ClientAddress.sin_addr), htons(ClientAddress.sin_port));
		CreateThread(0, 0, ConnectionHandler, (LPVOID)ClientSocket, 0, 0);

	}

	closesocket(ListenSocket);
	WSACleanup();

	return 0;
}

/*
This function adds one to values that are...
-- 4 digits or longer
*/

char* inc(char* val) {
	int result;
	int len = strlen(val);
	if (len < 4) return NULL;
	char newval[8];
	sprintf(newval, "%s", val);
	result = atoi(newval) + 1;
	sprintf(newval, "%d\n\0", result);
	char* returnval = (char*)malloc(strlen(newval));
	strcpy(returnval, newval);
	return returnval;

}

/*
This function subtracts one to values that...
-- are 4 digits or longer
-- do not end in 0
*/

char* dec(char* val)
{
	int len = strlen(val);
	char temp[15];
	if (len < 4) return NULL;
	strcpy(temp, val);
	temp[strlen(temp) - 2] = temp[strlen(temp) - 2] - 1;
	char* returnval = (char*)malloc(9);
	strcpy(returnval, temp);
	return returnval;
}

DWORD WINAPI ConnectionHandler(LPVOID CSocket) {
	int RecvBufLen = DEFAULT_BUFLEN;
	char* RecvBuf = malloc(DEFAULT_BUFLEN);
	char BigEmpty[2000];
	int Result, SendResult, k;
	memset(BigEmpty, 0, 2000);
	memset(RecvBuf, 0, DEFAULT_BUFLEN);
	SOCKET Client = (SOCKET)CSocket;
	SendResult = send(Client, "Welcome to Increment Server! Enter HELP for help.\n", 50, 0);
	if (SendResult == SOCKET_ERROR) {
		printf("Send failed with error: %d\n", WSAGetLastError());
		closesocket(Client);
		return 1;
	}
	while (CSocket) {
		Result = recv(Client, RecvBuf, RecvBufLen, 0);
		if (Result > 0) {
			if (strncmp(RecvBuf, "HELP", 4) == 0) {
				const char ValidCommands[251] = "Valid Commands:\nHELP\nINC [inc_value]\nDEC [dec_value]\nEXIT\n";
				SendResult = send(Client, ValidCommands, sizeof(ValidCommands), 0);
			}
			else if (strncmp(RecvBuf, "INC ", 4) == 0) {
				char* returnval = inc(&(RecvBuf[4]));
				if (returnval) {
					SendResult = send(Client, returnval, strlen(returnval) + 1, 0);
				}
				else {
					SendResult = send(Client, "INVALID INC\n", 12, 0);
				}

			}
			else if (strncmp(RecvBuf, "DEC ", 4) == 0) {
				char* returnval = dec(&(RecvBuf[4]));
				if (returnval) {
					SendResult = send(Client, returnval, strlen(returnval) + 1, 0);
				}
				else {
					SendResult = send(Client, "INVALID DEC\n", 12, 0);
				}
			}
			else {
				SendResult = send(Client, "UNKNOWN COMMAND\n", 16, 0);
			}
			if (SendResult == SOCKET_ERROR) {
				printf("Send failed with error: %d\n", WSAGetLastError());
				closesocket(Client);
				return 1;
			}
		}
		else if (Result == 0) {
			printf("Connection closing...\n");
			closesocket(Client);
			return 0;
		}
		else {
			printf("Recv failed with error: %d\n", WSAGetLastError());
			closesocket(Client);
			return 1;
		}

	}
}
