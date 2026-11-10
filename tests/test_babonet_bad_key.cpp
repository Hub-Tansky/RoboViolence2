// ROBO-068: a peer whose first bytes, short of a full key, aren't "RND1" is dropped from the server's client list,
// as a wrong full key is. bb_serverUpdate reports the drop as -(NetID).
#include "baboNet.h"

#ifdef _WIN32
#	include <winsock2.h>
#else
#	include <arpa/inet.h>
#	include <netinet/in.h>
#	include <sys/socket.h>
#	include <unistd.h>
#endif
#include <chrono>
#include <cstdio>
#include <thread>

int main()
{
	const unsigned short port = 10209;
	if (bb_init() || bb_serverCreate(false, 4, port))
	{
		std::printf("FAIL: no server\n");
		return 1;
	}
	int fd = (int)socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in to = {};
	to.sin_family = AF_INET;
	to.sin_port = htons(port);
	to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (connect(fd, (sockaddr*)&to, sizeof(to)) != 0)
	{
		std::printf("FAIL: no connection\n");
		return 1;
	}
	INT4 accepted = 0, dropped = 0;
	for (int frame = 0; frame < 150 && !dropped; ++frame)
	{
		char ip[16];
		INT4 r = bb_serverUpdate(1 / 30.0f, UPDATE_SEND_RECV, ip);
		if (r > 0 && !accepted)
		{
			accepted = r;
			send(fd, "XY", 2, 0); // 2 of the 9 key bytes, wrong
		}
		else if (r < 0)
			dropped = r;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
#ifdef _WIN32
	closesocket(fd);
#else
	close(fd);
#endif
	if (!accepted || dropped != -accepted)
	{
		std::printf("FAIL: accepted %d, dropped %d (want -%d)\n", (int)accepted, (int)dropped, (int)accepted);
		return 1;
	}
	std::printf("ok: client %d dropped after a bad partial key\n", (int)accepted);
	return 0;
}
