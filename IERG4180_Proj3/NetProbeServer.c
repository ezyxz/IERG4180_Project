// gcc hello.c src/tinycthread.c -lpthread -std=c99 -o hello 


#include <stdlib.h>
#include <stdio.h>
#include "src/tinycthread.h"
#include "src/utils.h"
#include "src/pipe.h"
#include<poll.h>
#define BUFSIZE 1024
#define MAX_THREADS 100

typedef struct ThreadPoolState {
	int     iThreadPoolSize;	
	thrd_t  *pThreadHandels;
	pipe_t  *pPipe;
	pipe_producer_t *pProducer;
	pipe_consumer_t **ppConsumer;

}ThreadPoolState;

typedef struct DOKI_packet {
    int mode;
    int proto;
    int pktsize;
    int pktnum;
    int pktrate;
    int port;
}DOKI_packet;

typedef struct SockDesc{
	SOCKET sock;
	int self_kill;
	// char description[50]; 
	DOKI_packet *config;
    struct sockaddr_in * sockaddr;
    int udp_flag_bit;

}SockDesc;



pipe_consumer_t* cons;
pipe_producer_t* pros;
struct sockaddr_in server, client;
ThreadPoolState threadPoolState;
SOCKET sock_default;
// parameters
char* lhost = "localhost";
char* lport = "4180";
char* rport = "4180";
int sbufsize = 0;
int rbufsize = 0;
int consumer_proc(int i);
int thread_pool_control_proc(int i);
void string2struct(DOKI_packet* packet, char* msg);
void struct2string(DOKI_packet* packet, char* msg);
void connPrint(char* ip, int port, int i, int j, int mode, int proto, int pktrate);
int tcpSend(SOCKET* pSock, int pktrate, int pktsize, int pktnum);
int udpSend(SOCKET* pSock, int pktrate, int pktsize, int pktnum, struct sockaddr_in* server_udp_addr, int poll_index);
int getUdpSenderSock(SOCKET newsfd, DOKI_packet doki, SOCKET* udp_sender_sock, struct sockaddr_in* temp_addr);
int getUdpReceiverSock(SOCKET newsfd, DOKI_packet doki, SOCKET *hDatagramSocket, struct sockaddr_in *LocalAddr);

int mointor_udp_close_proc(int random);
int *THREADS_STATUS; //0->killed 1->block 2->work
int THREAD_TAIL;
thrd_t t;
struct pollfd peerfd[1024];
mtx_t gMutex;

int main(int argc, char* argv[])
{	
	int pool_size = 8;
	THREAD_TAIL = pool_size;
	threadPoolState.pPipe = pipe_new(sizeof(int), 0);
	threadPoolState.pProducer =  pipe_producer_new(threadPoolState.pPipe);
	threadPoolState.ppConsumer = (pipe_consumer_t**)malloc(sizeof(pipe_consumer_t*)*pool_size);
	THREADS_STATUS = (int*)malloc(sizeof(int)*pool_size);
	memset(THREADS_STATUS, 0, pool_size * sizeof(int));

    mtx_init(&gMutex, mtx_plain);
	for(int i = 0; i < pool_size; i++){
		threadPoolState.ppConsumer[i] = pipe_consumer_new(threadPoolState.pPipe);
		if (thrd_create(&t, consumer_proc,  i) != thrd_success)
		{		
			printf("thread create fail!!!\n");
		}
	}
	if (thrd_create(&t, thread_pool_control_proc,  -1) != thrd_success)
	{		
		printf("thread create fail!!!\n");
	}
    if (thrd_create(&t, mointor_udp_close_proc,  -1) != thrd_success)
	{		
		printf("thread create fail!!!\n");
	}

	cp_sleep(100);
	printf("Thread Creation Complete.....\n");
	char* desc = "hello, pipe";
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
        printf("Exit.....Please rerun the programme\n");
        return -1;
    }
    printf("successful.\n");
	int c = sizeof(struct sockaddr_in);
	listen(sock_default, 10);
	while (1){
		processNewConnection();
	}
	

	sleep(1000);
	printf("Main thread stop....\n");
	CloseSocket();
	

	return 0;



	
	
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
		// connPrint(client_ip, client_port, j, j+1, doki.mode, doki.proto, doki.pktrate);
		SockDesc mySockDesc;
		mySockDesc.sock = newsfd;
		mySockDesc.config = &doki;
		mySockDesc.self_kill = -1;
		pipe_push(threadPoolState.pProducer, &mySockDesc, sizeof(mySockDesc));

    }
    //deal with udp
    else if (doki.proto == 0) {
        // server receive client send in udp
        if (doki.mode == 0) {
            SOCKET udp_receiver_sock;
            struct sockaddr_in temp_addr;
            getUdpReceiverSock(newsfd, doki, &udp_receiver_sock, &temp_addr);
            SockDesc mySockDesc;
		    mySockDesc.sock = udp_receiver_sock;
		    mySockDesc.config = &doki;
		    mySockDesc.self_kill = -1;
            mySockDesc.sockaddr = &temp_addr;
            // int ret = cp_setSockTimeout(3, &udp_receiver_sock);
            // if (ret < 0){
            //     printf("set error!!!\n");
            // }
            int j = 0;
            mtx_lock(&gMutex);
            for(; j < 100; ++j)
            {
                if(peerfd[j].fd < 0)
                {
                    peerfd[j].fd = newsfd;
                    printf("Reigister [%d] in poll\n", j);
                    break;
                }
            }
            peerfd[j].events = POLLIN;
            mySockDesc.udp_flag_bit = j;
            mtx_unlock(&gMutex);
            pipe_push(threadPoolState.pProducer, &mySockDesc, sizeof(mySockDesc));


        }
        //server send client receive in udp
        else if (doki.mode = 1) {
            SOCKET udp_sender_sock;
            struct sockaddr_in temp_addr;
            getUdpSenderSock(newsfd, doki, &udp_sender_sock, &temp_addr);
            SockDesc mySockDesc;
		    mySockDesc.sock = udp_sender_sock;
		    mySockDesc.config = &doki;
		    mySockDesc.self_kill = -1;
            mySockDesc.sockaddr = &temp_addr;
            int j = 0;
            mtx_lock(&gMutex);
            for(; j < 100; ++j)
            {
                if(peerfd[j].fd < 0)
                {
                    peerfd[j].fd = newsfd;
                    printf("Reigister [%d] in poll\n", j);
                    break;
                }
            }
            peerfd[j].events = POLLIN;
            mySockDesc.udp_flag_bit = j;
            mtx_unlock(&gMutex);
            pipe_push(threadPoolState.pProducer, &mySockDesc, sizeof(mySockDesc));

        }
    }

}


int mointor_udp_close_proc(int random){
    printf("Thread udp mointor start.....\n");
    int ret;
    int timeout = 100;
    for (int i = 0; i < 100; i++)
    {
        peerfd[i].fd = -1;
    }
    while(1)
    {
        switch(ret = poll(peerfd,100,timeout))
        {
            case 0:
                // printf("timeout...\n");
                break;
            case -1:
                perror("poll");
                break;
            default:
                {
                    mtx_lock(&gMutex);
                    for(int i = 0;i < 100;++i)
                    {
                        if(peerfd[i].revents & POLLIN)
                        {
                            // printf("read ready\n");
                            char buf[1024];
                            ssize_t s = read(peerfd[i].fd,buf, \
                                    sizeof(buf) - 1);
                            if(s > 0)
                            {
                                // buf[s] = 0;
                                // printf("client say#%s",buf);
                                // fflush(stdout);
                                // peerfd[i].events = POLLOUT;
                            }
                            else if(s <= 0)
                            {
                                close(peerfd[i].fd);
                                peerfd[i].fd = -1;
                                printf("Detect tcp Socket[%d] closing\n", i);
                            }
                        }
                    }//for
                    mtx_unlock(&gMutex);
                    
                }//default
                break;
        }
    }

    return 1;
}







int consumer_proc(int i)
{	
	printf("thread[%d] starting....\n", i);
	
	SockDesc mySockDesc;
 	size_t bytes_read;
	char msg[50000];


	THREADS_STATUS[i] = 1;
	while((bytes_read=pipe_pop(threadPoolState.ppConsumer[i], &mySockDesc, sizeof(mySockDesc)))){

		THREADS_STATUS[i] = 2;
		if (mySockDesc.self_kill == 1){
			printf("Thread[%d] killed......\n", i);
			THREADS_STATUS[i] = 0;
			return -1;
		}
		//Send
		if (mySockDesc.config->mode == 1){
			//TCP
			if (mySockDesc.config->proto == 1)
			{
				printf("Thread[%d] start SEND with TCP on [%d]Bps [%d]Pktsize [%d]Pktnum\n", i, mySockDesc.config->pktrate, mySockDesc.config->pktsize, mySockDesc.config->pktnum);
				tcpSend(&mySockDesc.sock, mySockDesc.config->pktrate, mySockDesc.config->pktsize, mySockDesc.config->pktnum);
                cp_closesocket(mySockDesc.sock);
				printf("Thread[%d] close socket.....\n", i);
			//UDP
			}else if(mySockDesc.config->proto == 0){
				printf("Thread[%d] start SEND with UDP on [%d]Bps [%d]Pktsize [%d]Pktnum\n", i, mySockDesc.config->pktrate, mySockDesc.config->pktsize, mySockDesc.config->pktnum);
                udpSend(&mySockDesc.sock, mySockDesc.config->pktrate, mySockDesc.config->pktsize, mySockDesc.config->pktnum, mySockDesc.sockaddr,mySockDesc.udp_flag_bit);
                printf("Thread[%d] close UDP SENDER socket.....\n", i);
                cp_closesocket(mySockDesc.sock);

			}
		//Recv
		}else if (mySockDesc.config->mode == 0)
		{
			//TCP
			if (mySockDesc.config->proto == 1)
			{
				printf("Thread[%d] start RECV with TCP on [%d]Bps [%d]Pktsize [%d]Pktnum\n", i, mySockDesc.config->pktrate, mySockDesc.config->pktsize, mySockDesc.config->pktnum);
				while (1)
				{	
					memset(msg, 0, 50000);
					int ret = recv(mySockDesc.sock, msg, 50000, 0);
					if (ret == 0) {
						printf("Thread[%d] close socket.....\n", i);
                        cp_closesocket(mySockDesc.sock);
						break;
					}		
					// printf("Thread[%d]->RECV [%d]\n", i, ret);
				}

			}else if(mySockDesc.config->proto == 0){
				printf("Thread[%d] start RECV with UDP on [%d]Bps [%d]Pktsize [%d]Pktnum\n", i, mySockDesc.config->pktrate, mySockDesc.config->pktsize, mySockDesc.config->pktnum);
                char buf[5000]; int len = 5000;
                int slen = sizeof(*mySockDesc.sockaddr);
                // char* client_ip = inet_ntoa(mySockDesc.sockaddr->sin_addr);
                // int client_port = ntohs(mySockDesc.sockaddr->sin_port);
                // printf("Thread[%d] from clinet ip %s | port %d\n", i, client_ip, client_port);
                struct timeval tv;
                tv.tv_sec = 1;  //  注意防core
	            tv.tv_usec = 0;
                setsockopt(mySockDesc.sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); 
                while (1)
                {   
                   int r = cp_recvfrom(mySockDesc.sock, buf, len, 0, (struct sockaddr*)mySockDesc.sockaddr, &slen);
                   if (peerfd[mySockDesc.udp_flag_bit].fd == -1){
                       printf("Thread %d closing udp RECV socket...\n", i);
                       cp_closesocket(mySockDesc.sock);
                       break;
                   }
                }
                
                

			}
		}
		
		
		THREADS_STATUS[i] = 1;
		
		
	}
	

	

	// pipe_consumer_free(p);
}

int thread_pool_control_proc(int i){
	printf("thread controller start.....\n");
	int current_live_thread = 0;
	int current_working_thread = 0;
    int time_gap = 0;
	while (1){
		for (int i = 0; i < THREAD_TAIL; i++){
			if (THREADS_STATUS[i] > 0){
				current_live_thread++;
				if (THREADS_STATUS[i] == 2){
					current_working_thread++;
				}
			}
		}
		//expansion
		if (current_live_thread == current_working_thread){
			threadPoolState.ppConsumer = (pipe_consumer_t**)realloc(threadPoolState.ppConsumer ,sizeof(pipe_consumer_t*)*(THREAD_TAIL + current_live_thread));
			THREADS_STATUS = (int*)realloc(THREADS_STATUS, sizeof(int)*(THREAD_TAIL + current_live_thread));
			memset(THREADS_STATUS+THREAD_TAIL*sizeof(int), 0, current_live_thread*sizeof(int));
			for(int i = THREAD_TAIL; i < THREAD_TAIL + current_live_thread; i++){
				threadPoolState.ppConsumer[i] = pipe_consumer_new(threadPoolState.pPipe);
				if (thrd_create(&t, consumer_proc,  i) != thrd_success)
				{		
					printf("thread create fail!!!\n");
				}
			}
			THREAD_TAIL += current_live_thread;
			printf("Thread expansion complete.....\n");
            time_gap = 0;
			//reduce
		}else if (current_live_thread / 2 > current_working_thread ){
            time_gap += 2;
            if (time_gap >= 60){
                int amount = current_live_thread / 2 ;
                for (int i = 0; i < amount; i++)
                {
                    SockDesc mySockDesc;
                    mySockDesc.self_kill = 1;
                    pipe_push(threadPoolState.pProducer, &mySockDesc, sizeof(mySockDesc));

                }
                time_gap = 0;
                printf("Thread reduce complete.....\n");
            }

			
		}else{
            time_gap = 0;
        }
		

		printf("Current available threads [%d] | working threads[%d]\n", current_live_thread, current_working_thread);
		cp_sleep(2000);
		current_live_thread = 0;
		current_working_thread = 0;
	}

}











int tcpSend(SOCKET* pSock, int pktrate, int pktsize, int pktnum) {

    char msg[50000];
	int if_error = 0;
    //char* msg = "Hello Client , I have received your connection. But I have to go now, bye\n";
	long long bytes_sent = 0;
    long long total_send = 0;
    int p_num_index = 1;
	clock_t current_clock, previous_clock = clock();
	double single_iter_pkt_threshold = pktrate * ((double)1/2);
    printf("single_iter_pkt_threshold %f\n", single_iter_pkt_threshold);
	int pkts_send_per_stat = 0;

    while (!if_error && (p_num_index < pktnum || pktnum == 0)) {

        if (total_send < single_iter_pkt_threshold || pktrate == 0) {
            bytes_sent = 0;
            //one pkt
            // encode_pkt(msg, p_num_index);
            while (bytes_sent < pktsize) {
                int r = send(*pSock, msg + bytes_sent, pktsize - bytes_sent, 0);
                if (r > 0) {
                    bytes_sent += r;
                }
                else {
                    printf("Sending error with %d\n", r);
                    if_error = 1;
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
        if (time_cost * 1000 >= (double)500) {
            // double throughput = (total_send * 8) / (time_cost * 1000000);
            // printf("Sender:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps\n", time_cost * 1000, pkts_send_per_stat, total_send, throughput);
            previous_clock = current_clock;
            total_send = 0;
            pkts_send_per_stat = 0;
        }

    }

    return -1;

}



int udpSend(SOCKET* pSock, int pktrate, int pktsize, int pktnum, struct sockaddr_in* server_udp_addr, int poll_index) {
    char* client_ip = inet_ntoa(server_udp_addr->sin_addr);
    int client_port = ntohs(server_udp_addr->sin_port);
    printf("Thread from clinet ip %s | port %d\n",  client_ip, client_port);
    int pkt_index = 1;
    char msg[50000];
    socklen_t slen = sizeof(*server_udp_addr);
    double single_iter_pkt_threshold = pktrate * ((double)1 / 2);
    clock_t current_time, previous_time = clock();
    int bytes_send_per_stat = 0;
    int  pkts_send_per_stat = 0;
    if (pktrate > 0) {
        while (1)
        {

            while (bytes_send_per_stat < single_iter_pkt_threshold) {
                // encode_pkt(msg, pkt_index);
                int r = sendto((*pSock), msg, pktsize, 0, (struct sockaddr*)server_udp_addr, slen);
                if (r <= 0) {
                    printf("Sending error\n");
                    break;
                }
                // printf("SEND[%s], BYTES[%d]\n", msg, r);
                pkt_index++;
                pkts_send_per_stat++;
                bytes_send_per_stat += r;
            }
            current_time = clock();
            double time_gap = ((double)current_time - previous_time);
            if (time_gap >= 500*1000) {
                // double throughput = ((double)bytes_send_per_stat * 8) / (time_gap * 1000);
                // printf("Sender:[Elapsed] %.2f ms, [Pkts] %d, [Bytes] %d B, [Rate] %.5f Mbps\n", time_gap/1000, pkts_send_per_stat, bytes_send_per_stat, throughput);
                previous_time = current_time;
                bytes_send_per_stat = 0;
                pkts_send_per_stat = 0;
            }
            if (peerfd[poll_index].fd == -1){
                break;
            }


        }
    }
    else {
        while (1)
        {
            // encode_pkt(msg, pkt_index);
            int r = sendto((*pSock), msg, pktsize, 0, (struct sockaddr*)server_udp_addr, slen);
            if (r <= 0) {
                printf("Sending error\n");
                break;
            }
            if (peerfd[poll_index].fd == -1){
                break;
            }

        }

    }
    return 1;

}





int getUdpReceiverSock(SOCKET newsfd, DOKI_packet doki, SOCKET *hDatagramSocket, struct sockaddr_in *LocalAddr) {
    (*hDatagramSocket) = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(LocalAddr, 0, sizeof(*LocalAddr));
    LocalAddr->sin_family = AF_INET;
    LocalAddr->sin_port = 0;
    LocalAddr->sin_addr.s_addr = INADDR_ANY;
    if (bind(*hDatagramSocket,(struct sockaddr*)LocalAddr,sizeof(struct sockaddr_in)) == SOCKET_ERROR) {
        printf("bind error==================\n");
    }
    int iAddrLen = sizeof(struct sockaddr_in);
    if (cp_getsockname((*hDatagramSocket),(struct sockaddr*)LocalAddr, &iAddrLen) == SOCKET_ERROR) {
        printf("getsocket error error==================\n");
    }
    DOKI_packet portPkt;
    portPkt.mode = 3;
    portPkt.proto = -1;
    portPkt.pktnum = -1;
    portPkt.pktsize = -1;
    portPkt.pktrate = -1;
    portPkt.port = htons(LocalAddr->sin_port);


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
    printf("Port number sending complete as %d\n", portPkt.port);

    return 1;
}

int getUdpSenderSock(SOCKET newsfd, DOKI_packet doki, SOCKET* udp_sender_sock, struct sockaddr_in* temp_addr) {
    DOKI_packet portPkt;
    printf("Waiting for Clinet send port number....\n");
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
    printf("Receiving port number %d\n", portPkt.port);
    
    doki.port = portPkt.port;
    memset(temp_addr, 0, sizeof(*temp_addr));
    temp_addr->sin_family = AF_INET;
    temp_addr->sin_port = htons(portPkt.port);
    temp_addr->sin_addr = client.sin_addr;

    if (((*udp_sender_sock) = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("Could not create socket : %d", getErrorCode());
    }

    return 1;
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

// int encode_pkt(char* msg, int i) {
//     char num[16] = { 0 };
//     memset(msg, 0, 50000);
//     itoa(i, num, 10);
//     memcpy(msg, num, strlen(num) + 1);
//     memcpy(msg + strlen(num), "#", 1);
//     return 1;
// }