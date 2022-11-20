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