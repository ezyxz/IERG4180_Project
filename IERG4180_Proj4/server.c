//gcc server.c src/utils.c src/tinycthread.c src/pipe.c -lssl -lcrypto -o server

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <stdlib.h>
#include <stdio.h>
#include "getopt.h"

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
#include "src/tinycthread.h"
#include "src/utils.h"
#include "src/pipe.h"


// char* lhost = "192.168.10.129";
char* lhost = "127.0.0.1";
int THREAD_MODE = 1; 


char dest_url[8192];
X509 *cert = NULL;
X509_NAME *certname = NULL;
const SSL_METHOD *method;
SSL_CTX *ctx;
int server = 0;
int ret, i;
char *ptr = NULL;
void argParse(int argc, char* argv[]);
int HttpConnWorker(void * args);
int HttpAcceptWorker(void *args);
int HttpsConnWorker(void * args);
int ThreadMonitorWorker(int i);
int consumer_proc(int i);
char* itoa(int num,	char* str,int radix);
int send_(int s, char *buf, int len);
void processRequest(char* request, int client_sock, SSL *ssl) ;
void argPrint();
int POOL_SIZE = 8;
long lhttpsport = 4181;
long lhttpport = 4180;
thrd_t t;


typedef struct ThreadPoolState {
	int     iThreadPoolSize;	
	thrd_t  *pThreadHandels;
	pipe_t  *pPipe;
	pipe_producer_t *pProducer;
	pipe_consumer_t **ppConsumer;

}ThreadPoolState;


typedef struct SockDesc{
	SOCKET sock;
	int self_kill;
    int proto; //0->http 1->https
    SSL_CTX *ctx;
    int id;
}SockDesc;



typedef struct ThreadModeJob{
    SSL_CTX *ctx;
    int client;
    int id;
}ThreadModeJob;


pipe_consumer_t* cons;
pipe_producer_t* pros;
ThreadPoolState threadPoolState;
int *THREADS_STATUS; // in Thread Pool mode 0->killed 1->block 2->work 4->http 5->https
int THREAD_TAIL = 0;

void authorPrint(){
    printf("********************************\n");
    printf("********IERG4180 Proj4**********\n");
    printf("******@author:Xinyuan Zuo*******\n");
    printf("******@SID: 1155183193**********\n");
    printf("********************************\n\n");
}

int main(int argc, char **argv){
    authorPrint();
    argParse(argc, argv);
    //thread MODE one connection -> create one thread
    argPrint();
    int pool_size = POOL_SIZE;

    if (THREAD_MODE == 0){

        printf("Init Server Thread......");
        THREAD_TAIL = 1;
        THREADS_STATUS = (int*)malloc(sizeof(int)*THREAD_TAIL);
        printf("Done\n");

    }else if (THREAD_MODE == 1){

        printf("Init Server Thread Pool......");
        THREAD_TAIL = pool_size;
        threadPoolState.pPipe = pipe_new(sizeof(int), 0);
        threadPoolState.pProducer =  pipe_producer_new(threadPoolState.pPipe);
        threadPoolState.ppConsumer = (pipe_consumer_t**)malloc(sizeof(pipe_consumer_t*)*pool_size);
        THREADS_STATUS = (int*)malloc(sizeof(int)*pool_size);
        memset(THREADS_STATUS, 0, pool_size * sizeof(int));

        for(int i = 0; i < pool_size; i++){
            threadPoolState.ppConsumer[i] = pipe_consumer_new(threadPoolState.pPipe);
            if (thrd_create(&t, consumer_proc,  i) != thrd_success)
            {		
                printf("thread create fail!!!\n");
            }
	    }

    }

    if (thrd_create(&t, ThreadMonitorWorker,  -1) != thrd_success)
	{		
		printf("thread create fail!!!\n");
	}

    printf("Load open SSL....\n");
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();

    if (SSL_library_init() < 0) {
        printf("Could not initialize the OpenSSL library !\n");
        return -1;
    }
    method = TLS_server_method();
    if ((ctx = SSL_CTX_new(method)) == NULL) {
        printf("Unable to create a new SSL context structure.\n");
        return -1;
    }
    if (!SSL_CTX_set_default_verify_paths(ctx)) {
        printf("Unable to set default verify paths.\n");
        return -1;
    }

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "domain.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    printf("domain.crt loaded.\n");

    if (SSL_CTX_use_PrivateKey_file(ctx, "domain.key", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    printf("domain.key loaded.\n");
    printf("Init Https Socket....\n");
    //init sock 
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {  
        printf("Socket create fail\n");       
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(lhttpsport);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    memset(&(server_addr.sin_zero), '\0', 8);

    if (bind(sock, (struct sockaddr *)&server_addr,sizeof(server_addr)) == -1) {
        printf("bind error\n"); 
        close(sock);      
        return -1; 
    }     
    if (listen(sock, 10) == -1 ) {    
        return -1;
    }
    printf("Listening on port %d ...\n", lhttpsport);

    struct sockaddr_in c_skt; 
    int s_size=sizeof(struct  sockaddr_in);

    if (THREAD_MODE == 0){

    
        
        THREADS_STATUS = (int*)malloc(sizeof(int)*(5000));
        memset(THREADS_STATUS, 0, sizeof(int)*(5000));
        
        if (thrd_create(&t, HttpAcceptWorker, -1) != thrd_success)
        {		
            printf("thread create fail!!!\n");
        }
    
        while(1){
            int client = accept(sock, (struct sockaddr *)&c_skt, (socklen_t *)&s_size);
            char* client_ip = inet_ntoa(c_skt.sin_addr);
            int client_port = ntohs(c_skt.sin_port);
            printf("HTTPS PORT : TCP Connection accepted from clinet ip %s | port %d\n", client_ip, client_port);
            ThreadModeJob threadModeJob;
            threadModeJob.client = client;
            threadModeJob.ctx = ctx;
            // printf("THREAD_TAIL %d\n", THREAD_TAIL);
            threadModeJob.id = THREAD_TAIL;
            THREAD_TAIL++;
            // printf("THREAD_TAIL %d\n", THREAD_TAIL);
            if (thrd_create(&t, HttpsConnWorker, (void *)(&threadModeJob)) != thrd_success)
            {		
                printf("thread create fail!!!\n");
            }
        }

    }else if (THREAD_MODE == 1)
    {

       if (thrd_create(&t, HttpAcceptWorker, -1) != thrd_success)
        {		
            printf("thread create fail!!!\n");
        }
        while(1){
            int client = accept(sock, (struct sockaddr *)&c_skt, (socklen_t *)&s_size);
            char* client_ip = inet_ntoa(c_skt.sin_addr);
            int client_port = ntohs(c_skt.sin_port);
            printf("HTTPS PORT : TCP Connection accepted from clinet ip %s | port %d\n", client_ip, client_port);
            SockDesc mySockDesc;
            mySockDesc.id = THREAD_TAIL;
            mySockDesc.sock = client;
            mySockDesc.proto = 1;
            mySockDesc.self_kill = 0;
            mySockDesc.ctx = ctx;
            pipe_push(threadPoolState.pProducer, &mySockDesc, sizeof(mySockDesc));
        }

    }
    
    

	
    
}



int HttpAcceptWorker(void *args){
    printf("Init Http Socket....\n");

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {  
        printf("Socket create fail\n");       
        return -1;
    }

    struct sockaddr_in http_addr;
    http_addr.sin_family = AF_INET;
    http_addr.sin_port = htons(lhttpport);
    http_addr.sin_addr.s_addr = INADDR_ANY;

    memset(&(http_addr.sin_zero), '\0', 8);

    if (bind(sock, (struct sockaddr *)&http_addr,sizeof(http_addr)) == -1) {
        printf("bind error\n"); 
        close(sock);      
        return -1; 
    }     
    if (listen(sock, 10) == -1 ) {    
        return -1;
    }
    printf("Listening on port %d ...\n", lhttpport);
    struct sockaddr_in c_skt; 
    int s_size=sizeof(struct  sockaddr_in);
    if (THREAD_MODE == 0){
        while(1){
            int client = accept(sock, (struct sockaddr *)&c_skt, (socklen_t *)&s_size);
            char* client_ip = inet_ntoa(c_skt.sin_addr);
            int client_port = ntohs(c_skt.sin_port);
            printf("HTTP PORT : TCP Connection accepted from clinet ip %s | port %d\n", client_ip, client_port);
            ThreadModeJob threadModeJob;
            threadModeJob.client = client;
            threadModeJob.id = THREAD_TAIL;
            THREAD_TAIL++;
            if (thrd_create(&t, HttpConnWorker, (void *)(&threadModeJob)) != thrd_success)
            {		
                printf("thread create fail!!!\n");
            }
        }
    }else if (THREAD_MODE == 1){
        while(1){
            int client = accept(sock, (struct sockaddr *)&c_skt, (socklen_t *)&s_size);
            char* client_ip = inet_ntoa(c_skt.sin_addr);
            int client_port = ntohs(c_skt.sin_port);
            printf("HTTP PORT : TCP Connection accepted from clinet ip %s | port %d\n", client_ip, client_port);
            SockDesc mySockDesc;
            mySockDesc.id = THREAD_TAIL;
            mySockDesc.sock = client;
            mySockDesc.proto = 0;
            mySockDesc.self_kill = 0;
            pipe_push(threadPoolState.pProducer, &mySockDesc, sizeof(mySockDesc));
        }
    }

}


int HttpConnWorker(void * args){
    int client = ((ThreadModeJob*)args)->client;
    int id = ((ThreadModeJob*)args)->id - 1;
    THREADS_STATUS[id] = 4;
    while (1){
        char* buf = (char*)malloc(4096 * sizeof(char));
        if (buf == NULL){
            printf("buffer malloc fail!\n");
            exit(-1);
        }
        memset(buf, 0, 4096);
        int ret = 0;
        int recvLen = 0;
        while (recvLen < 2 || !(buf[recvLen - 1] == '\n' && buf[recvLen - 2] == '\r')){
            ret = recv(client, buf + recvLen, 4096 - recvLen, 0);
            recvLen += ret;
            if (ret <= 1) {
                THREADS_STATUS[id] = 0;
                printf("Thread[%d] Finish http request on socket [%d]\n", id, client);
                close(client);
                return 0;
            }
        }
        processRequest(buf, client, NULL);
    }
}





int HttpsConnWorker(void * args){
    SSL_CTX *ctx = ((ThreadModeJob*)args)->ctx;
    int client = ((ThreadModeJob*)args)->client;
    int id = ((ThreadModeJob*)args)->id - 1;
    THREADS_STATUS[id] = 5;
    printf("Thread[%d] to working https request on socket [%d]\n", id, client);
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client);
    int i;
    if (i = SSL_accept(ssl) <= 0) { // Perform SSL/TLS handshake with peer
        ERR_print_errors_fp(stderr);
        SSL_shutdown(ssl);
        close(client);
        SSL_free(ssl); // Free the SSL connection
        THREADS_STATUS[id] = 0;
        printf("Thread[%d] Finish https request on socket with error [%d]\n", id, client);    
        return -1;
	}
    char msg[5000];
    
    while(1){
        int ret = 0;
        while(ret <= 2 || !(msg[ret - 1] == '\n' && msg[ret - 2] == '\r')){
            int read = SSL_read(ssl, msg + ret, 5000 - ret);
            ret += read;
            if (read <= 1) {
                SSL_shutdown(ssl);
                close(client);
                SSL_free(ssl); // Free the SSL connection
                THREADS_STATUS[id] = 0;
                printf("Thread[%d] Finish https request on socket [%d]\n", id, client);    
                return 0;
            }        
        }
        // printf("%s", msg);
        processRequest(msg, client, ssl);
    }
    return 0;
}











int consumer_proc(int i)
{	
	printf("Consumer Thread[%d] starting....\n", i);
	
	SockDesc mySockDesc;
 	size_t bytes_read;
	char msg[50000];


	THREADS_STATUS[i] = 1;
    
	while((bytes_read=pipe_pop(threadPoolState.ppConsumer[i], &mySockDesc, sizeof(mySockDesc)))){
        int if_break = 0;
        THREADS_STATUS[i] = 1;
		if (mySockDesc.self_kill == 1){
			printf("Thread[%d] killed......\n", i);
			THREADS_STATUS[i] = 0;
			return -1;
		}
        if (mySockDesc.proto == 0){
            THREADS_STATUS[i] = 4;
            int client = mySockDesc.sock;
            while (1){
                char* buf = (char*)malloc(4096 * sizeof(char));
                if (buf == NULL){
                    printf("buffer malloc fail!\n");
                    exit(-1);
                }
                memset(buf, 0, 4096);
                int ret = 0;
                int recvLen = 0;
                while (recvLen < 2 || !(buf[recvLen - 1] == '\n' && buf[recvLen - 2] == '\r')){
                    ret = recv(client, buf + recvLen, 4096 - recvLen, 0);
                    recvLen += ret;
                    if (ret <= 1) {
                        printf("Thread[%d] Finish http request on socket with error [%d] to be block\n", i, client);
                        close(client);
                        if_break = 1;
                        break;
                    }
                }
                if (if_break == 1)
                    break;
                processRequest(buf, client, NULL);
            }


        }else if(mySockDesc.proto == 1){
            THREADS_STATUS[i] = 5;
            int client = mySockDesc.sock;
            int id = i;
            SSL *ssl = SSL_new(ctx);
            SSL_set_fd(ssl, client);
            int i;
            if (i = SSL_accept(ssl) <= 0) { // Perform SSL/TLS handshake with peer
                ERR_print_errors_fp(stderr);
                SSL_shutdown(ssl);
                close(client);
                SSL_free(ssl); // Free the SSL connection
                printf("Thread[%d] Finish https request on socket [%d] to be block\n", id, client); 
                THREADS_STATUS[i] = 1;
                continue;   
            }
            char msg[5000];
            
            while(1){
                int ret = 0;
                while(ret <= 2 || !(msg[ret - 1] == '\n' && msg[ret - 2] == '\r')){
                    int read = SSL_read(ssl, msg + ret, 5000 - ret);
                    ret += read;
                    if (read <= 1) {
                        SSL_shutdown(ssl);
                        close(client);
                        SSL_free(ssl); // Free the SSL connection
                        printf("Thread[%d] Finish https request on socket [%d] to be block\n", id, client);   
                        if_break = 1; 
                        break;
                    }        
                }
                // printf("%s", msg);
                if (if_break == 1)
                    break;
                processRequest(msg, client, ssl);
            }
        }
		
		THREADS_STATUS[i] = 1;
		
	}
	

	

	// pipe_consumer_free(p);
}

















int ThreadMonitorWorker(int i){
	printf("Thread monitor start.....\n");
    if (THREAD_MODE == 1){
        int current_live_thread = 0;
        int current_working_thread = 0;
        int http_thread = 0;
        int https_thread = 0;
        int time_gap = 0;
        int total_time = 0;
        while (1){
            for (int i = 0; i < THREAD_TAIL; i++){
                if (THREADS_STATUS[i] > 0){
                    current_live_thread++;
                    if (THREADS_STATUS[i] == 4){
                        current_working_thread++;
                        http_thread++;
                    }else if (THREADS_STATUS[i] == 5){
                        current_working_thread++;
                        https_thread++;
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
            
            total_time += 2;
            printf("Elapsed [%ds] ThreadPool [%d|%d] Http Clients [%d] Https Clients [%d]\n", total_time, current_live_thread, current_working_thread, http_thread,https_thread);
            cp_sleep(2000);
            current_live_thread = 0;
            current_working_thread = 0;
            http_thread = 0;
            https_thread = 0;
        }
    }else if (THREAD_MODE == 0){
        int https_conn = 0;
        int http_conn = 0;
        int total_time = 0;
        while (1){
            for (int i = 0; i < THREAD_TAIL; i++){
                if (THREADS_STATUS[i] == 4){
                    http_conn++;
                }else if (THREADS_STATUS[i] == 5){
                    https_conn++;
                }
            }
            cp_sleep(2000);
            printf("Elapsed [%ds] HTTP Clients [%d] HTTPS Clients [%d]\n", total_time, http_conn, https_conn);
            total_time += 2;
            https_conn = 0;
            http_conn = 0;
        }

    }
	

}




void processRequest(char* request, int client_sock, SSL *ssl) {   
    char arguments[BUFSIZ];  
    strcpy(arguments, "./");

    char command[BUFSIZ];     
    sscanf(request, "%s%s", command, arguments+2);

    char* extension = "text/html";   
    char* content_type = "text/plain";     
    char* body_length = "Content-Length: ";

    char length_buf[20];
    if(strcmp(arguments,".//") == 0){
        strcpy(arguments, ".//index.html");
    }
	FILE* rfile= fopen(arguments, "rb");
    if (rfile == NULL){

		int head_len, ctype_len, stat_len; 
		char* head = "HTTP/1.1 404 Not Found.\r\n";
		head_len = strlen(head);    
		char ctype[30] = "Content-type:text/html\r\n";   
		ctype_len = strlen(ctype);

		itoa(0, length_buf, 10);

		if (ssl == NULL){
			int yes = 1;
			int result = setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char *) &yes, sizeof(int));    // 1 - on, 0 - off
			send_(client_sock, head, head_len);
			send_(client_sock, ctype, ctype_len);
			send_(client_sock, body_length, strlen(body_length));
			send_(client_sock, length_buf, strlen(length_buf));
			send_(client_sock, "\r\n", 2);
			send_(client_sock, "\r\n", 2);
		} else {
			SSL_write(ssl, head, head_len);
			SSL_write(ssl, ctype, ctype_len);
			SSL_write(ssl, body_length, strlen(body_length));
			SSL_write(ssl, length_buf, strlen(length_buf));
			SSL_write(ssl, "\r\n", 2);
			SSL_write(ssl, "\r\n", 2);
		}
	} else {
		int head_len, ctype_len, stat_len; 
		char* head = "HTTP/1.1 200 OK\r\n";
		head_len = strlen(head);    
		char ctype[30] = "Content-type:text/html\r\n";   
		ctype_len = strlen(ctype);
		struct stat statbuf;
		fstat(fileno(rfile), &statbuf);
		itoa(statbuf.st_size, length_buf, 10);
		char* read_buf = (char*)malloc(65536 * sizeof(char));
		memset(read_buf, 0, 65536);
		stat_len = fread(read_buf ,1 , statbuf.st_size, rfile);

		if (ssl == NULL){
			send_(client_sock, head, head_len);
			send_(client_sock, ctype, ctype_len);
			send_(client_sock, body_length, strlen(body_length));
			send_(client_sock, length_buf, strlen(length_buf));
			send_(client_sock, "\n", 1);
			send_(client_sock, "\r\n", 2);
			send_(client_sock, read_buf, stat_len);
			send_(client_sock, "\r\n", 2);
			send_(client_sock, "\r\n", 2);
		} else {
			SSL_write(ssl, head, head_len);
			SSL_write(ssl, ctype, ctype_len);
			SSL_write(ssl, body_length, strlen(body_length));
			SSL_write(ssl, length_buf, strlen(length_buf));
			SSL_write(ssl, "\n", 1);
			SSL_write(ssl, "\r\n", 2);
			SSL_write(ssl, read_buf, stat_len);
			SSL_write(ssl, "\r\n", 2);
			SSL_write(ssl, "\r\n", 2);
		}
		free(read_buf);
	}
    return;
}

char* itoa(int num,	char* str,int radix) {  
	char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";	
	unsigned unum;											
	int i=0,j,k;
	if(radix==10&&num<0)									
	{
		unum=(unsigned)-num;
		str[i++]='-';
	}
	else unum=(unsigned)num;								
	do
	{
		str[i++]=index[unum%(unsigned)radix];
		unum/=radix;
	}while(unum);
	str[i]='\0';
	if(str[0]=='-') k=1;									
	else k=0;
	char temp;
	for(j=k;j<=(i-k-1)/2.0;j++)
	{
		temp=str[j];
		str[j]=str[i-j-1];
		str[i-j-1]=temp;
	}
	return str;
}

int send_(int s, char *buf, int len) {
    int total;          
    int bytesleft;                          
    int n;
    total=0;
    bytesleft = len;	
    while(total < len){
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) {
			printf("Send function fail\n");
            break;
        }
        total += n;
        bytesleft -= n;
    }
    len = total;          
    return n==-1?-1:0;         
}


void argParse(int argc, char* argv[]) {
    const char* optstring = "a";
    int c;
    struct option opts[] = {
        {"lhost", 1, NULL, 0},
        {"lhttpport", 1, NULL, 1},
        {"lhttpsport", 1, NULL, 2},
        {"server", 1, NULL, 3},
        {"poolsize", 1, NULL, 4},

    };

    while ((c = getopt_long_only(argc, argv, optstring, opts, NULL)) != -1) {
        switch (c) {
        case 0:
            if (strcmp("localhost", optarg) == 0){
                lhost = "127.0.0.1";    
            }else{
                lhost = optarg;
            }
            
            break;
        case 1:
            lhttpport = strtol(optarg, NULL, 10);
            break;
        case 2:
            lhttpsport = strtol(optarg, NULL, 10);
            break;
        case 3:
            if (strcmp("threadpool", optarg) == 0){
                THREAD_MODE == 1;
            }else if (strcmp("thread", optarg) == 0){
                THREAD_MODE == 0;
            }
            break;
        case 4:
            POOL_SIZE = strtol(optarg, NULL, 10);
            break;
        
        default:
            printf("Error: get invalid argument\n");
            break;

        }
    }
}

void argPrint(){

    printf("NetProbeSrv <parameters>, see below:\n");
    printf("   <-lhost hostname> [%s]\n", lhost);
    printf("   <-http port> [%d]\n", lhttpport);
    printf("   <-https port> [%d]\n", lhttpsport);
    printf("   <-server mode> [%d] 0->thread 1->threadpool\n\n\n", THREAD_MODE);


}