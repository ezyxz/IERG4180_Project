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

### Remarks

If running client and server on same Linux machine and set the pktrate > 0, 

the info print interval(stat) may shows very late.

This issue **won't** happened on two different Linux machine and on same windows machine **neither**.



 