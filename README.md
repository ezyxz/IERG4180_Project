# IERG4180_Project
Hello TA, I am Xinyuan Zuo.
My student ID is 1155183193.

## Proj1 - description
compile line: g++ NetProbe.cpp -o NetProbe -lwsock32

set pktsize as same with sender and receiver

report：[IERG4180_Report1_1155183193.pdf](IERG4180_Report1_1155183193.pdf)

## Proj2 - description

Linux compile line: g++ NetProbeServer.cpp utils.cpp -o LinuxServer

​				g++ NetProbeClient.cpp utils.cpp -o LinuxClient

Win compile line: g++ NetProbeServer.cpp utils.cpp -o WinServer -lwsock32

​				g++ NetProbeClient.cpp utils.cpp -o WinClient -lwsock32

report: [IERG4180_Report2_1155183193.pdf](IERG4180_Report2_1155183193.pdf) 

### Remarks

If running client and server on same Linux machine and set the pktrate > 0, 

the info print interval(stat) may shows very late.

This issue **won't** happened on two different Linux machine and on same windows machine **neither**.



## Proj3 - description

 Linux compile：

【Server】

gcc NetProbeServer.c src/utils.c src/pipe.c src/tinycthread.c -lpthread -std=c99 -o server

【Clinet】

g++ NetProbeClient.cpp src/utils.c -o Linuxclient

Win compile：

g++ NetProbeClient.cpp src/utils.c -o Winclient -lwsock32

### Remark

1. how the server makes sure to close all the sockets.

   When using tcpSend， the send function returns -1 means the client close the socket.

   When using tcpRecv, the recv function returns -1 means the client close the socket.

   When using udp, I use Poll, register

   struct pollfd peerfd[1024];

   peerfd[j].fd = newsfd;

   peerfd[j].events = POLLIN; (in void processNewConnection())

   and also send j as sturct to pipe.

   When The thread function mointor_udp_close_proc notice the tcp is closing,

   put peerfd[j].fd = -1, then the consumer know to close udp socket.

2. describe how you control the pool size.

   I maintain a array called int *THREADS_STATUS;

   Here are the specific number meanning. 

   //0->killed  1->block   2->tcp working   3->udp working

   When a thread starting or working, it should update his status in THREADS_STATUS;

   Thread function thread_pool_control_proc check the THREADS_STATUS at intervals.

   When it needs to reduce thread, put self_kill=1 in struct into pipe.

   When the blocked thread get this job, and judge self_kill==1 just return to end this thread.

   When it needs to double thread, realloc THREADS_STATUS and threadPoolState.ppConsumer.

3. Do you provide an option to disable Nagle algorithm? (small bonus)

   No

4. Others: 

   Finish response mode

   Executable files are in bin.

## Proj4 - description

Server compile: gcc server.c src/utils.c src/tinycthread.c src/pipe.c -lssl -lcrypto -o server
Server run: ./server 
Client compile:  gcc client.c  -lssl -lcrypto -o client
Client run: ./client   default: http://127.0.0.1:4180/
to get index.html to run ./client -url http://127.0.0.1:4180/ -file index.html
in https: ./client -url https://127.0.0.1:4181/ -file index.html
to get file.txt to run ./client -url http://127.0.0.1:4180/ -file file.txt
in https: ./client -url https://127.0.0.1:4181/ -file file.txt

to connect Internet:
./client -url https://www.baidu.com

Using web browser https://192.168.10.129:4181/


### Remarks
 Using thrid part library to parse URL by https://gitee.com/yikoulinux/url/tree/master