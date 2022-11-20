 #if defined WIN32 || defined _WIN32

#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#define OS 0
#define LOCAL_SYSTEM "windows"
typedef int socklen_t;

#else 

# include <sys/types.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <sys/fcntl.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <string.h>
# include <stdlib.h>
# include <unistd.h>
# include <errno.h>
#include "sys/time.h"
#define OS 1
#define LOCAL_SYSTEM "Linux"
typedef int SOCKET;
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)
#endif
 
 
 
 #ifndef UTILS
 #define  UTILS

SOCKET cp_accept(SOCKET sock_default, struct sockaddr* addr, int* c);

int cp_getsockname(SOCKET hDatagramSocket,struct sockaddr* LocalAddr, int *iAddrLen);

void cp_closesocket(SOCKET socket);

int cp_recvfrom(SOCKET socket, char* buf, int len, int r, struct sockaddr* addr, int *slen);

void cp_sleep(int num);

int InitSocket();

void CloseSocket();

int getErrorCode();

int cp_setSockTimeout(int second, SOCKET * sock_fd);
 #endif


