#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <getopt.h>
#include <set>
#include <time.h>
int InitSocket() {  
    #if defined WIN32 || defined _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
            printf("Failed. Error Code : %d", WSAGetLastError());
            return -1;
        }
    #else 

    #endif
    
}

void CloseSocket() {
    #if defined WIN32 || defined _WIN32
        WSACleanup();
    #else 
    #endif    
}

SOCKET cp_accept(SOCKET sock_default, struct sockaddr* addr, int* c){
    #if defined WIN32 || defined _WIN32
        return accept(sock_default, addr, c);
    #else 
        return accept(sock_default, addr, (socklen_t*)c);
    #endif
}


int cp_getsockname(SOCKET hDatagramSocket,struct sockaddr* LocalAddr, int *iAddrLen){
    #if defined WIN32 || defined _WIN32
        return getsockname(hDatagramSocket,LocalAddr, iAddrLen);
    #else 
        return getsockname(hDatagramSocket, LocalAddr, (socklen_t *)iAddrLen);
    #endif
}

void cp_closesocket(SOCKET socket){
    #if defined WIN32 || defined _WIN32
        closesocket(socket);
    #else 
        close(socket);
    #endif
}


int cp_recvfrom(SOCKET socket, char* buf, int len, int r, struct sockaddr* addr, int *slen){
    #if defined WIN32 || defined _WIN32
        return recvfrom(socket, buf, len, 0,addr, slen);
    #else 
        return recvfrom(socket, buf, len, 0,addr, (socklen_t*)slen);
    #endif
}

int getErrorCode(){
    #if defined WIN32 || defined _WIN32
       return WSAGetLastError();
    #else 
        return errno;
    #endif
}

void cp_sleep(int num){
    #if defined WIN32 || defined _WIN32
       Sleep(num);
    #else 
        usleep(num*1000);
    #endif 
}
