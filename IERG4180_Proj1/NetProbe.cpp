/*
@Autohr Xinyuan Zuo
@Student ID 1155183193
*/
#include <iostream>
#include <WinSock2.h>
#include <getopt.h>
#include <set>
#include <time.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;
#define MAX_BUFFER_RECV  10000000 
#define Jitter_max 100000000


/*  Arguments */
//mode init to -1; 0->send 1 -> recv 2->host
int MODE = -1;
//statistics display per time -stat
int mystat = 500;
//protocal 
char* proto = "UDP";
//send mode
char* rhost = "127.0.0.1";
char* rport = "4180";
int pktsize = 100;
int pktrate = 1000;
int pktnum = 0;
long sbufsize = 0;

//recv mode
char* lhost = "127.0.0.1";
char* lport = "4180";
long rbufsize = 0;

//host mode
char* myhost = "localhost";


WSADATA wsa;
SOCKET sock;

boolean validIPAddress(char* queryIP);
boolean isNumber(char* num);
void argParse(int argc, char* argv[]);
void printInfo();
int InitSocket();
void closeSocket();
int tcpRecv();
int tcpSend();
int udpSend();
int udpRecv();
double calculate_mean_cost(double* arr, int count);
int encode_pkt(char* msg, int i);
int decode_pkt(char* msg);
double loss_cal(set<int> set, int start, int end);
void getIPaddress();
long SetSendBufferSize (long size);
long SetReceiveBufferSize (long size);


int main(int argc, char* argv[])
{
    //argument parsing
    argParse(argc, argv);
    //print argements
    printInfo();
    //create socket
    InitSocket();

    if (MODE == 0) {
        if (strcmp(proto, "tcp") == 0 || strcmp(proto, "TCP") == 0) {
            tcpSend();
        }
        else {
            udpSend();
        }

    }
    else if (MODE == 1) {
        if (strcmp(proto, "tcp") == 0 || strcmp(proto, "TCP") == 0) {
            tcpRecv();
        }
        else {
            udpRecv();
        }

    }
    else if (MODE == 2) {
        getIPaddress();
    }
    closeSocket();
    return 0;

}

int InitSocket() {
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        return -1;
    }
}

void closeSocket() {
    closesocket(sock);
    WSACleanup();
}


void getIPaddress() {
    struct hostent* host = gethostbyname(myhost);
    if (!host) {
        puts("Get IP address error!");
        system("pause");
        exit(0);
    }

    for (int i = 0; host->h_aliases[i]; i++) {
        printf("Aliases %d: %s\n", i + 1, host->h_aliases[i]);
    }
    printf("Address type: %s\n", (host->h_addrtype == AF_INET) ? "AF_INET" : "AF_INET6");

    for (int i = 0; host->h_addr_list[i]; i++) {
        printf("IP addr %d: %s\n", i + 1, inet_ntoa(*(struct in_addr*)host->h_addr_list[i]));
    }

    

}



int tcpSend() {
    SOCKET new_sock;
    struct sockaddr_in server, client;
    int c;
    char msg[5000];

    long long bytes_sent = 0;
    long long total_send = 0;
    int p_num_index = 1;

    double single_iter_pkt_threshold = pktrate * ((double)mystat / 1000);
    clock_t current_clock, previous_clock = clock();

    bool if_error = false;
    int pkts_send_per_stat = 0;



    int port = strtol(rport, NULL, 10);
    if (port < 0) {
        printf("error port number %d \n", port);
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", WSAGetLastError());
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    //Bind
    if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
        printf("Bind failed with error code : %d", WSAGetLastError());
    }
    if (sbufsize > 0)
        SetSendBufferSize(sbufsize);


    listen(sock, 3);

    c = sizeof(struct sockaddr_in);
    new_sock = accept(sock, (struct sockaddr*)&client, &c);

    if (new_sock == INVALID_SOCKET)
    {
        printf("accept failed with error code : %d", WSAGetLastError());
    }

    char* client_ip = inet_ntoa(client.sin_addr);
    int client_port = ntohs(client.sin_port);
    printf("Connection accepted from clinet ip %s | port %d\n", client_ip, client_port);

    //char* msg = "Hello Client , I have received your connection. But I have to go now, bye\n";
    

    printf("single_iter_pkt_threshold %f\n", single_iter_pkt_threshold);

    
    while (!if_error && (p_num_index < pktnum || pktnum == 0)) {

        if (total_send < single_iter_pkt_threshold || pktrate == 0) {
            bytes_sent = 0;
            //one pkt
            encode_pkt(msg, p_num_index);
            while (bytes_sent < pktsize) {
                int r = send(new_sock, msg + bytes_sent, pktsize - bytes_sent, 0);
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

    return 0;

}

int tcpRecv() {
    struct sockaddr_in server;

    char msg[50000];
    int recv_size;

    int port = strtol(rport, NULL, 10);
    if (port < 0) {
        printf("error port number %d \n", port);
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket : %d", WSAGetLastError());
    }


    server.sin_addr.s_addr = inet_addr(lhost);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    if (rbufsize > 0)
        SetReceiveBufferSize(rbufsize);

    while (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
    {
        printf("No Server connected.....\n");
        Sleep(1000);
    }

    printf("Successful connected!\n");

    bool if_exit = false;
    int cum_packet_number = 0, byte_recv = 0, total_packet_number = 0;
    double cum_time_cost = 0, cum_bytes_recv = 0, total_time_cost = 0;
    clock_t current_clock, previous_clock = clock(), current_msg_arrival_time, previous_msg_arrival_time = clock();
    double* inter_arrival_list = (double*)malloc(Jitter_max * sizeof(double));
    double* J_i_list = (double*)malloc(Jitter_max * sizeof(double));
    set<int> set;
    int start_pkt_index = 1;
    int curr_pkt_index;

    while (!if_exit) {

        while (byte_recv < pktsize) {
            int ret = recv(sock, msg, pktsize - byte_recv, 0);
            if (ret == SOCKET_ERROR) {
                printf("Recv fail with error code %d\n", WSAGetLastError());
                Sleep(3000);
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
        int curr_pkt_index = decode_pkt(msg);
        set.insert(curr_pkt_index);
        cum_bytes_recv += byte_recv;
        byte_recv = 0;

        current_clock = clock();
        double time_cost = ((double)current_clock - previous_clock) / CLOCKS_PER_SEC;

        //printf("%d\n", cum_packet_number);

        inter_arrival_list[cum_packet_number++] = time_cost * 1000;
        double d = calculate_mean_cost(inter_arrival_list, cum_packet_number);
        J_i_list[cum_packet_number - 1] = time_cost * 1000 - d;
        previous_clock = current_clock;
        cum_time_cost += time_cost;
        //printf("%.2f | %d\n", cum_time_cost * 1000, mystat);
        if (cum_time_cost * 1000 >= (double)mystat) {
            double throughput = (cum_bytes_recv * 8) / (cum_time_cost * 1000000);
            double jitter = calculate_mean_cost(J_i_list, cum_packet_number);
            double loss = loss_cal(set, start_pkt_index, curr_pkt_index);
            printf("Reciver:[Elapsed] %.2f ms, [Pkts] %d, [Rate] %.5f Mbps, [Lost] %.5f%%, [Jitter] %.6f ms\n", cum_time_cost * 1000, cum_packet_number, throughput, loss*100, jitter);
            cum_packet_number = 0;
            cum_time_cost = 0;
            cum_bytes_recv = 0;
            set.clear();
            start_pkt_index = curr_pkt_index+1;
        }

        


    }




    //if (( recv_size = recv(sock, server_reply, pktsize, 0)) == SOCKET_ERROR)
    //{
    //    printf("recv failed with code %d\n", recv_size);
    //}
    //server_reply[recv_size] = '\0';
    //printf("GET[%s]\n", server_reply);
    return 0;


}

int udpSend() {

    struct sockaddr_in client;
    int slen = sizeof(client);
    int pkt_index = 1;
    char msg[10000];
    int port = strtol(rport, NULL, 10);

    if (port < 0) {
        printf("error port number %d \n", port);
        return -1;
    }

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("Could not create socket : %d", WSAGetLastError());
    }

    memset((char*)&client, 0, sizeof(client));
    client.sin_family = AF_INET;
    client.sin_port = htons(port);
    client.sin_addr.s_addr = inet_addr(rhost);
    memset(msg, 0, 5000);


    double single_iter_pkt_threshold = pktrate * ((double)mystat / 1000);
    printf("single_iter_pkt_threshold %f\n", single_iter_pkt_threshold);
    clock_t current_time, previous_time = clock();
    int bytes_send_per_stat = 0;
    int  pkts_send_per_stat = 0;
    if (pktrate > 0) {
        while (1)
        {

            while (bytes_send_per_stat < single_iter_pkt_threshold) {
                encode_pkt(msg, pkt_index);
                int r = sendto(sock, msg, pktsize, 0, (struct sockaddr*)&client, slen);
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
            if (time_gap >= mystat) {
                double throughput = ((double)bytes_send_per_stat * 8) / (time_gap * 1000);
                printf("Sender:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps\n", time_gap, pkts_send_per_stat, bytes_send_per_stat, throughput);
                previous_time = current_time;
                bytes_send_per_stat = 0;
                pkts_send_per_stat = 0;
            }


        }
    }
    else {
        while (1)
        {
            encode_pkt(msg, pkt_index);
            int r = sendto(sock, msg, pktsize, 0, (struct sockaddr*)&client, slen);
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
            if (time_gap >= mystat) {
                double throughput = ((double)bytes_send_per_stat * 8) / (time_gap * 1000);
                printf("Sender:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps\n", time_gap, pkts_send_per_stat, bytes_send_per_stat, throughput);
                previous_time = current_time;
                bytes_send_per_stat = 0;
                pkts_send_per_stat = 0;
            }


        }
    }
    
    
    return 0;
}

int udpRecv() {
    struct sockaddr_in client, server;
    int s, i, slen = sizeof(client);
    char msg[5000];
    int port = strtol(rport, NULL, 10);
    if (port < 0) {
        printf("error port number %d \n", port);
        return -1;
    }

    memset((char*)&client, 0, sizeof(client));

    client.sin_family = AF_INET;
    client.sin_port = htons(port);
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        printf("socket error\n");
    }
    if (bind(sock, (struct sockaddr*)&client, sizeof(client)) == -1)
    {
        printf("bind error\n");
        return 0;
    }
    int sender_pkt_index;
    int pkt_recv = 0;

    int bytes_recv = 0;
    clock_t current_clock, previous_clock = clock();
    double* J_i_list = (double*)malloc(Jitter_max * sizeof(double));
    double* inter_arrival_list = (double*)malloc(Jitter_max * sizeof(double));
    double total_time = 0;
    set<int> recv_set;
    int start_seq = 1;
    while (1) {

        int r = recvfrom(sock, msg, 5000, 0, (struct sockaddr*)&server, &slen);
        if (r == -1)
        {
            printf("Recv fail with error code %d\n", WSAGetLastError());
            return -1;
        }
        //printf("Received packet from %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
        //printf("GET[%s]\n", msg);
        sender_pkt_index = decode_pkt(msg);
        recv_set.insert(sender_pkt_index);
        bytes_recv += r;
        current_clock = clock();
        double time_gap = ((double)current_clock - previous_clock);
        inter_arrival_list[pkt_recv++] = time_gap;
        double D = calculate_mean_cost(inter_arrival_list, pkt_recv);
        J_i_list[pkt_recv - 1] = time_gap - D;
        previous_clock = current_clock;

        total_time += time_gap;
       

        if (total_time >= mystat) {
            double throughput = ((double)bytes_recv * 8) / (total_time * 1000);
            double jitter = calculate_mean_cost(J_i_list, pkt_recv);
            double loss = loss_cal(recv_set, start_seq, sender_pkt_index);
            printf("Receiver:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Lost] %.5f%%,[Rate] %.5f Mbps, [Jitter] %.6f\n", total_time, pkt_recv, bytes_recv, loss*100, throughput, jitter);
            //printf("send_pkt_index %d | Loss %.5f\n", sender_pkt_index, loss);
            pkt_recv = 0;
            bytes_recv = 0;
            total_time = 0;
            recv_set.clear();
            start_seq = sender_pkt_index+1;
        }


    }

    return 0;
}



long SetSendBufferSize (long size) {
    int Size = size;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,(char *)(&Size), sizeof(Size)) < 0)
        return -1;
    else
        return Size;
}

long SetReceiveBufferSize (long size) {
    int Size = size;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,(char *)(&Size), sizeof(Size)) < 0)
        return -1;
    else
        return Size;
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
    char num[16] = {0};
    memset(msg, 0,  5000);
    itoa(i, num, 10);
    memcpy(msg, num, strlen(num)+1);
    memcpy(msg+ strlen(num), "#", 1);
    return 1;
}

int decode_pkt(char* msg) {
    for (long i = 0; i < 5000; i++) {
        if (msg[i] == '#') {
            msg[i] = '\0';
            break;
        }
    }
    return atoi(msg);
}

double calculate_mean_cost(double* arr, int count) {
    double ans = 0;
    for (int i = 0; i < count; i++) {
        ans += arr[i];
    }
    return ans / count;

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
        case 2:
            MODE = 2;
            if (optarg)
                myhost = optarg;
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
        case 12:
            if (isNumber(optarg)) {
                lport = optarg;
            }
            // printf("lport is set to:%s\n", lport);
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
    printf("====== IERG4180 Proj1 ===========\n");
    printf("@Author Xinyuan Zuo *************\n");
    printf("@Id 1155183193 ******************\n");
    printf("============NetProbe=============\n");
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
    }
    else if (MODE == 1) {
        printf("Receiving Mode\n");
        printf("Display freq stat: %d\n", mystat);
        printf("Hostname lhost: %s\n", lhost);
        printf("Port number lport: %s\n", lport);
        printf("Protocal proto : %s\n", proto);
        printf("pktsiz : %d\n", pktsize);
        printf("sbufsize : %d\n", sbufsize);
    }
    else if (MODE == 2) {
        printf("Host Mode\n");
        printf("Hostname : %s\n", myhost);
    }
    printf("=================================\n");
}

boolean validIPAddress(char* queryIP)
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

boolean isNumber(char* num)
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
