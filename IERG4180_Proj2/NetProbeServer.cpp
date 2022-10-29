/**
 * @Author Xinyuan Zuo
 * @SID 1155183193
 * */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <getopt.h>
#include <set>
#include <time.h>
#include "getopt.h"
#include "utils.h"

//Config struct
typedef struct DOKI_packet {
    int mode;
    int proto;
    int pktsize;
    int pktnum;
    int pktrate;
    int port;
}DOKI_packet;
//TCP Connection Socket
SOCKET sock_default;

struct sockaddr_in server, client;
// parameters
char* lhost = "localhost";
char* lport = "4180";
char* rport = "4180";
int sbufsize = 0;
int rbufsize = 0;


const int maxSockets = 10;
SOCKET socketHandles[maxSockets]; // Array for the socket handles
bool socketValid[maxSockets]; // Bitmask to manage the array
int numActiveSockets = 1;
DOKI_packet socketConfigList[maxSockets];
struct sockaddr_in socket_client_address[maxSockets];


void argPrint();
void authorPrint();
void connPrint(char* ip, char* port, int i, int j, int mode, int proto, int pktrate);
void processNewConnection();
void udpRecv(int i);
void udpSend(int i);
void tcpSend(int i);
void tcpRecv(int i);
int getUdpSenderSock(SOCKET newsfd, DOKI_packet doki);
int getUdpReceiverSock(SOCKET newsfd, DOKI_packet doki);
long SetSendBufferSize(SOCKET sock, long size);
long SetReceiveBufferSize(SOCKET sock, long size);
//os 1->linux 0->win
int main(){
    printf("programme running on %s\n", LOCAL_SYSTEM);

    authorPrint();
    
    argPrint();
    
    InitSocket();

    printf("Binding local socket to port number %s with late binding ... ", lport);

    if ((sock_default = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", getErrorCode());
    }
    int port = strtol(lport, NULL, 10);
    if (port < 0) {
        printf("error port number %d \n", port);
        return -1;
    }

    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    //Bind
    if (bind(sock_default, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d", getErrorCode());
    }
    printf("successful.\n");

    listen(sock_default, 10);
    
    unsigned long ul_val = 1; // 1 means enable
    for (int i = 1; i < maxSockets; i++) socketValid[i] = false;
    socketHandles[0] = sock_default; socketValid[0] = true;

    fd_set fdReadSet, fdWriteSet;
    int c;
    printf("Listening to incoming connection request ...\n");
    while (1) {
        // Setup the fd_set //
        int topActiveSocket = 0;
        FD_ZERO(&fdReadSet);
        FD_ZERO(&fdWriteSet);
        for (int i = 0; i < maxSockets; i++) {
            if (socketValid[i] == true) {
                if (socketConfigList[i].mode == 0)
                    FD_SET(socketHandles[i], &fdReadSet);
                else if (socketConfigList[i].mode == 1)
                    FD_SET(socketHandles[i], &fdWriteSet);
                if (socketHandles[i] > topActiveSocket)
                    topActiveSocket = socketHandles[i]; // for Linux compatibility
            }
        }

        // Block on select() //
        int ret;
        if ((ret = select(topActiveSocket + 1, &fdReadSet,
            &fdWriteSet, NULL, NULL)) == SOCKET_ERROR) {
            printf("\nselect() failed. Error code: %i\n", getErrorCode());
            return 0;
        }
        // Process the active sockets //
        for (int i = 0; i < maxSockets; i++) {
            if (!socketValid[i]) continue; // Only check for valid sockets.
            if (FD_ISSET(socketHandles[i], &fdReadSet)) { // Is socket i active?
                if (i == 0) {
                    processNewConnection();
                }
                else {
                    //TCP RECV
                    if (socketConfigList[i].proto == 1) {
                        tcpRecv(i);
                        //UDP RECV
                    }
                    else if (socketConfigList[i].proto == 0)
                    {
                        udpRecv(i);

                    }


                }
                if (--ret == 0) break; // All active sockets processed.
            }
        }
        //handle write
        for (int i = 0; i < maxSockets; i++) {
            if (!socketValid[i]) continue; // Only check for valid sockets.
            if (FD_ISSET(socketHandles[i], &fdWriteSet)) { // Is socket i active?
                if (i == 0) {

                }
                else {
                    if (socketConfigList[i].proto == 0) {
                        udpSend(i);
                    }
                    else if (socketConfigList[i].proto == 1) {
                        tcpSend(i);
                    }

                }
                if (--ret == 0) break; // All active sockets processed.
            }
        }
    }
    
    CloseSocket();
}

void authorPrint(){
    printf("********************************\n");
    printf("********IERG4180 Proj2**********\n");
    printf("******@author:Xinyuan Zuo*******\n");
    printf("******@SID: 1155183193**********\n");
    printf("********************************\n\n");
}

void argPrint(){
    printf("NetProbeSrv <parameters>, see below:\n");
    printf("    <-sbufsize bsize>   set the outgoing socket buffer size to bsize bytes.\n");
    printf("    <-rbufsize bsize>   set the incoming socket buffer size to bsize bytes.\n");
    printf("    <-lhost hostname>   hostname to bind to. (Default late binding)\n");
    printf("    <-lport portnum>    port number to bind to. (Default '4180')\n\n");
}

void connPrint(char* ip, int port, int i, int j, int mode, int proto, int pktrate){
    if (mode == 0){
        if (proto == 0){
            printf("Connected to %s port %d [%d,%d] SEND, UDP, %d Bps\n", ip, port, i, j, pktrate);
        }else if (proto == 1){
            printf("Connected to %s port %d [%d,%d] SEND, TCP, %d Bps\n", ip, port, i, j, pktrate);
        }
    }else if (mode == 1){
        if (proto == 0){
            printf("Connected to %s port %d [%d,%d] RECV, UDP, %d Bps\n", ip, port, i, j, pktrate);
        }else if (proto == 1){
            printf("Connected to %s port %d [%d,%d] RECV, TCP, %d Bps\n", ip, port, i, j, pktrate);
        }
    }
}


void struct2string(DOKI_packet* packet, char* msg) {
    int int_size = sizeof(int);
    memset(msg, '#', 60 * sizeof(char));
    char buf[10];
    snprintf(buf, 10, "%d", packet->mode);
    memcpy(msg, buf, strlen(buf));
    snprintf(buf, 10, "%d", packet->proto);
    memcpy(msg + 10, buf, strlen(buf));
    snprintf(buf, 10, "%d", packet->pktsize);
    memcpy(msg + 20, buf, strlen(buf));
    snprintf(buf, 10, "%d", packet->pktnum);
    memcpy(msg + 30, buf, strlen(buf));
    snprintf(buf, 10, "%d", packet->pktrate);
    memcpy(msg + 40, buf, strlen(buf));
    snprintf(buf, 10, "%d", packet->port);
    memcpy(msg + 50, buf, strlen(buf));
}

void string2struct(DOKI_packet* packet, char* msg) {
    packet->mode = atoi(msg);
    packet->proto = atoi(msg + 10);
    packet->pktsize = atoi(msg + 20);
    packet->pktnum = atoi(msg + 30);
    packet->pktrate = atoi(msg + 40);
    packet->port = atoi(msg + 50);
}


void processNewConnection() {
    DOKI_packet doki;
    int c;
    c = sizeof(struct sockaddr_in);

    SOCKET newsfd = cp_accept(sock_default, (struct sockaddr*)&client, &c);
    char* client_ip = inet_ntoa(client.sin_addr);
    int client_port = ntohs(client.sin_port);
    printf("TCP Config Channel Connection accepted from clinet ip %s | port %d\n", client_ip, client_port);
    if (newsfd == SOCKET_ERROR) return;
    char conf_string[60];
    int byte_recv = 0;
    int msg_size = 60;
    while (byte_recv < msg_size) {
        int r = recv(newsfd, conf_string, msg_size - byte_recv, 0);
        if (r > 0) {
            byte_recv += r;
        }
        else {
            printf("recv error with %d\n", r);
            break;

        }
    }
    string2struct(&doki, conf_string);
    printf("With Config Mode %d | proto %d | pktsize %d | pktnum %d | pktrate %d | port %d\n", doki.mode, doki.proto, doki.pktsize, doki.pktnum, doki.pktrate, doki.port);
    

    //deal with tcp
    if (doki.proto == 1) {

        int j = 1;
        for (; j < maxSockets; j++) {
            if (socketValid[j] == false) {
                connPrint(client_ip, client_port, j, j, doki.mode, doki.proto, doki.pktrate);
                socketValid[j] = true;
                socketHandles[j] = newsfd;
                socketConfigList[j] = doki;
                socket_client_address[j] = client;
                ++numActiveSockets;
                if (numActiveSockets == maxSockets) {
                    // Ignore new accept()
                    socketValid[0] = false;
                }
                break;
            }
        }
    }
    //deal with udp
    else if (doki.proto == 0) {
        //server receive client send in udp
        if (doki.mode == 0) {
            int index = getUdpReceiverSock(newsfd, doki);
            connPrint(client_ip, client_port, index, index, doki.mode, doki.proto, doki.pktrate);

        }
        //server send client receive in udp
        else if (doki.mode = 1) {
            int index = getUdpSenderSock(newsfd, doki);
            connPrint(client_ip, client_port, index, index, doki.mode, doki.proto, doki.pktrate);
        }
    }

}

int getUdpReceiverSock(SOCKET newsfd, DOKI_packet doki) {
    SOCKET hDatagramSocket = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in LocalAddr;
    memset((&LocalAddr), 0, sizeof(LocalAddr));
    LocalAddr.sin_family = AF_INET;
    LocalAddr.sin_port = 0;
    LocalAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(hDatagramSocket,
        (struct sockaddr*)&LocalAddr,
        sizeof(struct sockaddr_in)) == SOCKET_ERROR) { /* error handling */
    }
    int iAddrLen = sizeof(LocalAddr);
    if (cp_getsockname(hDatagramSocket,
        (struct sockaddr*)&(LocalAddr), &iAddrLen) == SOCKET_ERROR) {
        /* error handling */
    }
    DOKI_packet portPkt;
    portPkt.mode = 3;
    portPkt.proto = -1;
    portPkt.pktnum = -1;
    portPkt.pktsize = -1;
    portPkt.pktrate = -1;
    portPkt.port = LocalAddr.sin_port;


    char conf[60];
    int bytes_sent = 0;
    int msg_size = 60;
    struct2string(&portPkt, conf);
    while (bytes_sent < msg_size) {
        int r = send(newsfd, conf + bytes_sent, msg_size - bytes_sent, 0);
        if (r > 0) {
            bytes_sent += r;
        }
        else {
            printf("Sending error with %d\n", r);
            break;

        }
    }
    // printf("Port number sending complete as %d\n", portPkt.port);

    int j = 1;
    for (; j < maxSockets; j++) {
        if (socketValid[j] == false) {
            socketValid[j] = true;
            socketHandles[j] = hDatagramSocket;
            socketConfigList[j] = doki;
            socket_client_address[j] = LocalAddr;
            ++numActiveSockets;
            if (numActiveSockets == maxSockets) {
                // Ignore new accept()
                socketValid[0] = false;
            }
            break;
        }
    }
    return j;
}

int getUdpSenderSock(SOCKET newsfd, DOKI_packet doki) {
    DOKI_packet portPkt;
    // printf("Waiting for Clinet send port number....\n");
    char conf_string[60];
    int byte_recv = 0;
    int msg_size = 60;
    while (byte_recv < msg_size) {
        int r = recv(newsfd, conf_string, msg_size - byte_recv, 0);
        if (r > 0) {
            byte_recv += r;
        }
        else {
            printf("recv error with %d\n", r);
            break;

        }
    }

    string2struct(&portPkt, conf_string);
    // printf("Receiving port number %d\n", portPkt.port);
    struct sockaddr_in temp_addr;
    SOCKET temp_sock;
    doki.port = portPkt.port;
    memset(&temp_addr, 0, sizeof(temp_addr));
    temp_addr.sin_family = AF_INET;
    temp_addr.sin_port = htons(portPkt.port);
    temp_addr.sin_addr.s_addr = client.sin_addr.s_addr;

    if ((temp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("Could not create socket : %d", getErrorCode());
    }
    int j = 1;
    for (; j < maxSockets; j++) {
        if (socketValid[j] == false) {
            socketValid[j] = true;
            socketHandles[j] = temp_sock;
            socketConfigList[j] = doki;
            socket_client_address[j] = temp_addr;
            ++numActiveSockets;
            if (numActiveSockets == maxSockets) {
                // Ignore new accept()
                socketValid[0] = false;
            }
            break;
        }
    }
    return j;
}

void udpRecv(int i) {
    char buf[256]; int len = 255;
    int slen = sizeof(socket_client_address[i]);
    int r = cp_recvfrom(socketHandles[i], buf, len, 0, (struct sockaddr*)&socket_client_address[i], &slen);
    // printf("[%i]RECV: ", i);
    // for (int x = 0; x < r; x++) putchar(buf[x]);
    // printf("\n");

}

void udpSend(int i) {

    char* msg = "This msg sent by udp from server";
    socklen_t    len;
    // printf("%d \n", socketConfigList[i].port);
    len = sizeof(socket_client_address[i]);
    int r = sendto(socketHandles[i], msg, strlen(msg), 0, (struct sockaddr*)&socket_client_address[i], len);
    // printf("[%d]Send: %s\n", i, msg);
    cp_sleep(1000);


}

void tcpSend(int i) {
    char* msg = "This msg sent by tcp from server";
    int bytes_sent = 0;
    int pktsize = strlen(msg);

    while (bytes_sent < pktsize) {
        int r = send(socketHandles[i], msg, pktsize - bytes_sent, 0);
        if (r > 0) {
            bytes_sent += r;
        }
        else {
            printf("Sending error with %d ....", r);
            printf("Closing SOCKET  %d\n", i);
            socketValid[i] = false;
            --numActiveSockets;
            if (numActiveSockets == (maxSockets - 1))
                socketValid[0] = true;
            cp_closesocket(socketHandles[i]);
            break;

        }

    }
    // printf("[%d]Send: %s\n", i, msg);
    cp_sleep(1000);
}

void tcpRecv(int i) {
    char buf[256]; int len = 255;
    int numread = recv(socketHandles[i], buf, len, 0);
    if (numread == SOCKET_ERROR) {
        printf("\nrecv() failed. Error code: %i\n",
            getErrorCode());
        socketValid[i] = false;
        return;
    }
    else if (numread == 0) { // connection closed
        // Update the socket array
        socketValid[i] = false;
        --numActiveSockets;
        if (numActiveSockets == (maxSockets - 1))
            socketValid[0] = true;
        cp_closesocket(socketHandles[i]);
    }
    else {
        // printf("[%i]RECV: ", i);
        // for (int x = 0; x < numread; x++) putchar(buf[x]);
        // printf("\n");
    }
}

long SetSendBufferSize(SOCKET sock, long size) {
    int Size = size;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)(&Size), sizeof(Size)) < 0)
        return -1;
    else
        return Size;
}

long SetReceiveBufferSize(SOCKET sock, long size) {
    int Size = size;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)(&Size), sizeof(Size)) < 0)
        return -1;
    else
        return Size;
}
