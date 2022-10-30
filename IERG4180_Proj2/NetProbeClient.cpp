/**
 * @Author Xinyuan Zuo
 * @SID 1155183193
 * */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <getopt.h>
#include <time.h>
#include "getopt.h"
#include "utils.h"
#include <set>
using namespace std;
#define MAX_BUFFER_RECV  10000000 
#define Jitter_max 100000000
SOCKET sock_default;
struct sockaddr_in server_addr, client, server_udp_addr;

SOCKET sock;
typedef struct DOKI_packet {
    int mode;
    int proto;
    int pktsize;
    int pktnum;
    int pktrate;
    int port;
}DOKI_packet;

DOKI_packet packet;
DOKI_packet portPkt;
/*  Arguments */
//mode init to -1; 0->send 1 -> recv 2->host
int MODE = 1;
//statistics display per time -stat
int mystat = 500;
//protocal 
char* proto = "tcp";
char* lhost = "127.0.0.1";
char* rhost = "192.168.10.128";
//char* rhost = "127.0.0.1";
char* rport = "4180";
struct sockaddr_in LocalAddr;
int pktsize = 100;
int pktrate = 1000;
int pktnum = 0;
long sbufsize = 0;
long rbufsize = 0;
double loss_cal(set<int> set, int start, int end);
int encode_pkt(char* msg, int i);
int decode_pkt(char* msg);
double calculate_mean_cost(double* arr, int count);
void struct2string(DOKI_packet* packet, char* msg);
void string2struct(DOKI_packet* packet, char* msg);
int InitSocket();
void CloseSocket();
bool isNumber(char* num);
bool validIPAddress(char* queryIP);
void printInfo();
void argParse(int argc, char* argv[]);
void sendConfigViaTCP();
void getUDPSock();
int tcpRecv();
int tcpSend();
int udpSend();
int udpRecv();
int tcpRecv();
// double loss_cal(set<int> set, int start, int end);

int main(int argc, char* argv[])
{

    argParse(argc, argv);

    printInfo();

    InitSocket();

    sendConfigViaTCP();

    if (strcmp(proto, "tcp") == 0 || strcmp(proto, "TCP") == 0) {
        if (MODE == 0) {
            tcpSend();
        }
        else if (MODE == 1) {
            tcpRecv();
        
        }
         
    }else{
        if (MODE == 0) {
            udpSend();

        }
        else if (MODE == 1) {
            udpRecv();
            
        }





    }
   

    cp_closesocket(sock);
    CloseSocket();
}








int udpSend() {
    int pkt_index = 1;
    char msg[50000];
    socklen_t slen = sizeof(server_udp_addr);
    double single_iter_pkt_threshold = pktrate * ((double)mystat / 1000);
    clock_t current_time, previous_time = clock();
    int bytes_send_per_stat = 0;
    int  pkts_send_per_stat = 0;
    if (pktrate > 0) {
        while (1)
        {

            while (bytes_send_per_stat < single_iter_pkt_threshold) {
                // encode_pkt(msg, pkt_index);
                int r = sendto(sock, msg, pktsize, 0, (struct sockaddr*)&server_udp_addr, slen);
                if (r <= 0) {
                    printf("Sending error\n");
                    break;
                }
                //printf("SEND[%s], BYTES[%d]\n", msg, r);
                pkt_index++;
                pkts_send_per_stat++;
                bytes_send_per_stat += r;
            }
            current_time = clock();
            double time_gap = ((double)current_time - previous_time);
            if (time_gap >= mystat*1000) {
                double throughput = ((double)bytes_send_per_stat * 8) / (time_gap * 1000);
                printf("Sender:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps\n", time_gap/1000, pkts_send_per_stat, bytes_send_per_stat, throughput);
                previous_time = current_time;
                bytes_send_per_stat = 0;
                pkts_send_per_stat = 0;
            }


        }
    }
    else {
        while (1)
        {
            // encode_pkt(msg, pkt_index);
            int r = sendto(sock, msg, pktsize, 0, (struct sockaddr*)&server_udp_addr, slen);
            if (r <= 0) {
                printf("Sending error\n");
                break;
            }
            //printf("SEND[%s], BYTES[%d]\n", msg, r);
            pkt_index++;
            pkts_send_per_stat++;
            bytes_send_per_stat += r;

            current_time = clock();
            double time_gap = ((double)current_time - previous_time);
            if (time_gap >= mystat *1000) {
                double throughput = ((double)bytes_send_per_stat * 8) / (time_gap * 1000);
                printf("Sender:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps\n", time_gap/1000, pkts_send_per_stat, bytes_send_per_stat, throughput);
                previous_time = current_time;
                bytes_send_per_stat = 0;
                pkts_send_per_stat = 0;
            }


        }
    // char* msg = "This msg sent by udp from client";
    // socklen_t slen = sizeof(server_udp_addr);
    // while (1) {
    //     int r = sendto(sock, msg, strlen(msg), 0, (struct sockaddr*)&server_udp_addr, slen);
    //     printf("[Send]%s\n", msg);
    //     cp_sleep(1000);
    // }
    }
    return 1;

}

int tcpSend() {
    double single_iter_pkt_threshold = pktrate * ((double)mystat / 1000);
    clock_t current_clock, previous_clock = clock();
    long long bytes_sent = 0;
    long long total_send = 0;
    bool if_error = false;
    int pkts_send_per_stat = 0;
    int p_num_index = 1;
    char msg[50000];
    while (!if_error && (p_num_index < pktnum || pktnum == 0)) {

        if (total_send < single_iter_pkt_threshold || pktrate == 0) {
            bytes_sent = 0;
            //one pkt
            // encode_pkt(msg, p_num_index);
            while (bytes_sent < pktsize) {
                int r = send(sock, msg + bytes_sent, pktsize - bytes_sent, 0);
                if (r > 0) {
                    bytes_sent += r;
                }
                else {
                    printf("Sending error with %d\n", r);
                    if_error = true;
                    break;

                }
            }
            total_send += bytes_sent;
            p_num_index++;
            pkts_send_per_stat++;
        }

        current_clock = clock();
        double time_cost = ((double)current_clock - previous_clock) / CLOCKS_PER_SEC;
        //printf("%.2f  ", time_cost * 1000);
        if (time_cost * 1000 >= (double)mystat) {
            double throughput = (total_send * 8) / (time_cost * 1000000);
            printf("Sender:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps\n", time_cost * 1000, pkts_send_per_stat, total_send, throughput);
            previous_clock = current_clock;
            total_send = 0;
            pkts_send_per_stat = 0;
        }

    }
    // char* msg = "This msg sent by tcp from client";
    // int bytes_sent = 0;
    // int pktsize = strlen(msg);
    // while (1) {
    //     while (bytes_sent < pktsize) {
    //         int r = send(sock, msg, pktsize - bytes_sent, 0);
    //         if (r > 0) {
    //             bytes_sent += r;
    //         }
    //         else {
    //             printf("Sending error with %d\n", r);
    //             break;

    //         }
            

    //     }
    //     bytes_sent = 0;
    //     printf("Send: %s\n", msg);
    //     cp_sleep(1000);
    // }
    

    return 1;


}

int udpRecv() {
    int sender_pkt_index;
    int pkt_recv = 0;
    int bytes_recv = 0;
    clock_t current_clock, previous_clock = clock();
    double* J_i_list = (double*)malloc(Jitter_max * sizeof(double));
    double* inter_arrival_list = (double*)malloc(Jitter_max * sizeof(double));
    double total_time = 0;
    int start_seq = 1;
    char msg[50000];
    socklen_t slen = sizeof(LocalAddr);
    while (1) {

        int r = recvfrom(sock, msg, 50000, 0, (struct sockaddr*)&LocalAddr, &slen);
        if (r == -1)
        {
            printf("Recv fail with error code %d\n", getErrorCode());
            return -1;
        }
        bytes_recv += r;
        current_clock = clock();
        double time_gap = ((double)current_clock - previous_clock) / CLOCKS_PER_SEC;
        inter_arrival_list[pkt_recv++] = time_gap;
        double D = calculate_mean_cost(inter_arrival_list, pkt_recv);
        J_i_list[pkt_recv - 1] = time_gap - D;
        previous_clock = current_clock;

        total_time += time_gap;


        if (total_time * 1000 >= mystat) {
            double throughput = ((double)bytes_recv * 8) / (total_time * 1000);
            double jitter = calculate_mean_cost(J_i_list, pkt_recv);
            printf("Receiver:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps, [Jitter] %.6f\n", total_time * 1000, pkt_recv, bytes_recv,  throughput, jitter);
            //printf("send_pkt_index %d | Loss %.5f\n", sender_pkt_index, loss);
            pkt_recv = 0;
            bytes_recv = 0;
            total_time = 0;
            start_seq = sender_pkt_index + 1;
        }
    }

    return 1;
}

int tcpRecv() {
    char msg[50000];
    int recv_size;
    bool if_exit = false;
    int cum_packet_number = 0, byte_recv = 0, total_packet_number = 0;
    double cum_time_cost = 0, cum_bytes_recv = 0, total_time_cost = 0;
    clock_t current_clock, previous_clock = clock(), current_msg_arrival_time, previous_msg_arrival_time = clock();
    double* inter_arrival_list = (double*)malloc(Jitter_max * sizeof(double));
    double* J_i_list = (double*)malloc(Jitter_max * sizeof(double));
    int start_pkt_index = 1;
    int curr_pkt_index;

    while (!if_exit) {

        while (byte_recv < pktsize) {
            int ret = recv(sock, msg, pktsize - byte_recv, 0);
            if (ret == SOCKET_ERROR) {
                printf("Recv fail with error code %d\n", getErrorCode());
                cp_sleep(3000);
                break;
            }
            else {
                if (ret == 0) {
                    if_exit = 1;
                    break;
                }
                else {
                    byte_recv += ret;
                }
            }
        }
        cum_bytes_recv += byte_recv;
        byte_recv = 0;

        current_clock = clock();
        double time_cost = ((double)current_clock - previous_clock)/CLOCKS_PER_SEC;
        inter_arrival_list[cum_packet_number++] = time_cost * 1000;
        double d = calculate_mean_cost(inter_arrival_list, cum_packet_number);
        J_i_list[cum_packet_number - 1] = time_cost * 1000 - d;
        previous_clock = current_clock;
        cum_time_cost += time_cost;
        // printf("cum_time_cost %f\n", cum_time_cost);
        if (cum_time_cost* 1000>= (double)mystat) {
            double throughput = (cum_bytes_recv * 8) / (cum_time_cost * 1000000);
            double jitter = calculate_mean_cost(J_i_list, cum_packet_number);
            printf("Reciver:[Elapsed] %.2f ms, [Pkts] %d, [Rate] %.5f Mbps,  [Jitter] %.6f ms\n", cum_time_cost * 1000, cum_packet_number, throughput,  jitter);
            cum_packet_number = 0;
            cum_time_cost = 0;
            cum_bytes_recv = 0;
            start_pkt_index = curr_pkt_index + 1;
        }

    }
    return 1;

}


void sendConfigViaTCP() {

    int recv_size;

    int port = strtol(rport, NULL, 10);
    if (port < 0) {
        printf("error port number %d \n", port);
        return;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", getErrorCode());
    }


    server_addr.sin_addr.s_addr = inet_addr(rhost);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    while (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("No Server connected.....\n");
        cp_sleep(1000);
    }

    printf("Successful connected!\n");


    
    packet.mode = MODE;
    if (strcmp(proto, "tcp") == 0 || strcmp(proto, "TCP") == 0) {
        packet.proto = 1;
    }
    else {
        packet.proto = 0;
    }

    packet.pktsize = pktsize;
    packet.pktnum = pktnum;
    packet.pktrate = pktrate;
    packet.port = -1;

    char conf[60];
    struct2string(&packet, conf);
    //conf[59] = 0;
    //printf("%s\n", conf);
    int bytes_sent = 0;
    int msg_size = 60;
    while (bytes_sent < msg_size) {
        int r = send(sock, conf + bytes_sent, msg_size - bytes_sent, 0);
        if (r > 0) {
            bytes_sent += r;
        }
        else {
            printf("Sending error with %d\n", r);
            break;

        }
    }
    printf("Configration sending complete!\n");
    printf("With Config Mode %d | proto %d | pktsize %d | pktnum %d | pktrate %d | port %d\n", packet.mode, packet.proto, packet.pktsize, packet.pktnum, packet.pktrate, packet.port);
    
    if (strcmp(proto, "udp") == 0 || strcmp(proto, "UDP") == 0) {
        
        DOKI_packet doki;
        if (MODE == 0) {
            //UDP SEND
            printf("Waiting for Server send port number....\n");
            char conf_string[60];
            int byte_recv = 0;
            int msg_size = 60;
            while (byte_recv < msg_size) {
                int r = recv(sock, conf_string, msg_size - byte_recv, 0);
                if (r > 0) {
                    byte_recv += r;
                }
                else {
                    printf("recv error with %d\n", r);
                    break;

                }
            }
            string2struct(&doki, conf_string);
            printf("Receiving port number %d\n",doki.port);
            memset((char*)&server_udp_addr, 0, sizeof(server_udp_addr));
            server_udp_addr.sin_family = AF_INET;
            server_udp_addr.sin_port = doki.port;
            server_udp_addr.sin_addr.s_addr = inet_addr(rhost);
            if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
            {
                printf("Could not create socket : %d", getErrorCode());
            }
           
        }
        else if (MODE == 1) {
            //udp recv
            SOCKET hDatagramSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            
            //memset((&LocalAddr), 0, sizeof(LocalAddr));
            LocalAddr.sin_family = AF_INET;
            LocalAddr.sin_port = 0;
            LocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(hDatagramSocket,
                (struct sockaddr*)&LocalAddr,
                sizeof(struct sockaddr_in)) == SOCKET_ERROR) { /* error handling */
            }
            socklen_t iAddrLen = sizeof(LocalAddr);
            if (getsockname(hDatagramSocket,
                (struct sockaddr*)&(LocalAddr), &iAddrLen) == SOCKET_ERROR) {
                /* error handling */
            }
            
            portPkt.mode = 3;
            portPkt.proto = -1;
            portPkt.pktnum = -1;
            portPkt.pktsize = -1;
            portPkt.pktrate = -1;
            portPkt.port = ntohs(LocalAddr.sin_port);


            char conf[60];
            int bytes_sent = 0;
            int msg_size = 60;
            struct2string(&portPkt, conf);
            while (bytes_sent < msg_size) {
                int r = send(sock, conf + bytes_sent, msg_size - bytes_sent, 0);
                if (r > 0) {
                    bytes_sent += r;
                }
                else {
                    printf("Sending error with %d\n", r);
                    break;

                }
            }
            printf("Port number sending complete as %d\n", portPkt.port);
            server_udp_addr = LocalAddr;
            sock = hDatagramSocket;
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


void argParse(int argc, char* argv[]) {
    const char* optstring = "a";
    int c;
    struct option opts[] = {
        {"send", 0, NULL, 0},
        {"recv", 0, NULL, 1},
        {"host", 0, NULL, 2},
        {"stat", 1, NULL, 3},
        {"rhost", 1, NULL, 4},
        {"rport", 1, NULL, 5},
        {"proto", 1, NULL, 6},
        {"pktsize", 1, NULL, 7},
        {"pktrate", 1, NULL, 8},
        {"pktnum", 1, NULL, 9},
        {"sbufsize", 1, NULL, 10},
        {"lhost", 1, NULL, 11},
        {"lport", 1, NULL, 12},
        {"rbufsize", 1, NULL, 13},
    };

    while ((c = getopt_long_only(argc, argv, optstring, opts, NULL)) != -1) {
        switch (c) {
        case 0:
            MODE = 0;
            break;
        case 1:
            MODE = 1;
            break;
        case 3:
            mystat = strtol(optarg, NULL, 10);
            if (mystat > 0) {
                //printf("stat is set to:%d\n", mystat);
            }
            else {
                mystat = 500;
            }
            break;
        case 4:
            if (validIPAddress(optarg)) {
                rhost = optarg;
            }
            //printf("rhost is set to:%s\n", rhost);

            break;
        case 5:
            if (isNumber(optarg)) {
                rport = optarg;
            }
            // printf("rport is set to:%s\n", rport);
            break;
        case 6:
            if (strcmp(optarg, "tcp") == 0 || strcmp(optarg, "udp") == 0 || strcmp(optarg, "TCP") == 0 || strcmp(optarg, "UDP") == 0) {
                proto = optarg;
            }
            //printf("proto is set to:%s\n", proto);

            break;
        case 7:
            pktsize = strtol(optarg, NULL, 10);
            if (pktsize > 0) {
                // printf("pktsize is set to:%d\n", pktsize);
            }
            else {
                pktsize = 1000;
            }
            break;
        case 8:
            pktrate = strtol(optarg, NULL, 10);
            if (pktrate >= 0) {
                //printf("pktrate is set to:%d\n", pktrate);
            }
            else {
                pktrate = 1000;
            }
            //printf("pktrate is set to:%d\n", pktrate);
            break;
        case 9:
            pktnum = strtol(optarg, NULL, 10);
            if (pktnum > 0) {
                //printf("pktnum is set to:%d\n", pktnum);
            }
            else {
                pktnum = 0;
            }
            //printf("pktnum is set to:%d\n", pktnum);
            break;
        case 10:
            sbufsize = strtol(optarg, NULL, 10);
            if (sbufsize < 0) {
                sbufsize = 0;
            }
            //printf("sbufsize:%s\n", sbufsize);

            break;
        case 11:
            if (validIPAddress(optarg)) {
                lhost = optarg;
            }
            // printf("lhost is set to:%s\n", lhost);
            break;
        case 13:
            rbufsize = strtol(optarg, NULL, 10);
            if (rbufsize < 0) {
                rbufsize = 0;
            }
            // printf("rbufsize:%s\n", rbufsize);
            break;
        default:
            printf("Error: invalid argument, exit...\n");
            break;

        }
    }
}

void printInfo() {
    printf("********************************\n");
    printf("********IERG4180 Proj2**********\n");
    printf("******@author:Xinyuan Zuo*******\n");
    printf("******@SID: 1155183193**********\n");
    printf("********************************\n\n");
    if (MODE == 0) {
        printf("Sending Mode\n");
        printf("Display freq stat: %d\n", mystat);
        printf("Hostname rhost: %s\n", rhost);
        printf("Port number rport: %s\n", rport);
        printf("Protocal proto : %s\n", proto);
        printf("pktsize : %d\n", pktsize);
        printf("pktrate : %d\n", pktrate);
        printf("pktnum : %d\n", pktnum);
        printf("sbufsize : %d\n", sbufsize);
        printf("rbufsize : %d\n", rbufsize);
    }
    else if (MODE == 1) {
        printf("Receiving Mode\n");
        printf("Display freq stat: %d\n", mystat);
        printf("Hostname rhost: %s\n", rhost);
        printf("Port number rport: %s\n", rport);
        printf("Protocal proto : %s\n", proto);
        printf("pktsiz : %d\n", pktsize);
        printf("pktnum : %d\n", pktnum);
        printf("sbufsize : %d\n", sbufsize);
        printf("rbufsize : %d\n", rbufsize);
       
    }
    printf("********************************\n");
}

bool validIPAddress(char* queryIP)
{
    int len = strlen(queryIP);
    if (strchr(queryIP, '.')) {
        int last = -1;
        for (int i = 0; i < 4; ++i) {
            int cur = -1;
            if (i == 3) {
                cur = len;
            }
            else {
                char* p = strchr(queryIP + last + 1, '.');
                if (p) {
                    cur = p - queryIP;
                }
            }
            if (cur < 0) {
                return false;
            }
            if (cur - last - 1 < 1 || cur - last - 1 > 3) {
                return false;
            }
            int addr = 0;
            for (int j = last + 1; j < cur; ++j) {
                if (!isdigit(queryIP[j])) {
                    return false;
                }
                addr = addr * 10 + (queryIP[j] - '0');
            }
            if (addr > 255) {
                return false;
            }
            if (addr > 0 && queryIP[last + 1] == '0') {
                return false;
            }
            if (addr == 0 && cur - last - 1 > 1) {
                return false;
            }
            last = cur;
        }
        return true;

    }
    else {
        return false;
    }
}

bool isNumber(char* num)
{
    int i;
    for (i = 0; num[i]; i++)
    {
        if (num[i] > '9' || num[i] < '0')
        {
            return false;;
        }
    }

    if (i > 100)
    {
        return false;
    }
    return true;
}

double calculate_mean_cost(double* arr, int count) {
    double ans = 0;
    for (int i = 0; i < count; i++) {
        ans += arr[i];
    }
    return ans / count;

}
double loss_cal(set<int> set, int start, int end) {
    double count = 0;
    for (int i = start; i <= end; i++) {
        if (set.count(i) == 1) {
            count++;
        }
    }
    return 1 - count / (end - start + 1);
}
int encode_pkt(char* msg, int i) {
    char num[16] = { 0 };
    memset(msg, 0, 50000);
    // itoa(i, num, 10);
    sprintf(num,"%d",i);
    memcpy(msg, num, strlen(num) + 1);
    memcpy(msg + strlen(num), "#", 1);
    return 1;
}

int decode_pkt(char* msg) {
    for (long i = 0; i < 50000; i++) {
        if (msg[i] == '#') {
            msg[i] = '\0';
            break;
        }
    }
    return atoi(msg);
}