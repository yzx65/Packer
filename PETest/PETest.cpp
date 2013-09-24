#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

int main()
{
	WSADATA wd = {0, };
	int result = WSAStartup(MAKEWORD(2, 2), &wd);
	std::cout << wd.szDescription;

	addrinfo *info;
	getaddrinfo("dlunch.net", "9008", NULL, &info);
	std::cout << info;

	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	connect(s, info->ai_addr, info->ai_addrlen);
	send(s, "test", 4, 0);
	closesocket(s);
	return 0;
}
