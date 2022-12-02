#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <stdlib.h>
#include <stdio.h>

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


char* rhost = "192.168.10.129";
char dest_url[8192];
X509 *cert = NULL;
X509_NAME *certname = NULL;
const SSL_METHOD *method;
SSL_CTX *ctx;
SSL *ssl;
int server = 0;
int ret, i;
char *ptr = NULL;
const char* pCertPath = "domain.crt";
const char* pKeyPath = "domain.key";
void processHttpsRequest(char* request, int client_sock, SSL *ssl);
char* itoa(int num,	char* str,int radix);
int send_(int s, char *buf, int len);
long lhttpsport = 4181;

int main(int argc, char **argv){

    SSL_library_init();
    OpenSSL_add_all_algorithms();
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    SSL_load_error_strings();


    if (SSL_library_init() < 0) {
        printf("Could not initialize the OpenSSL library !\n");
    }
    method = TLS_server_method();
    if ((ctx = SSL_CTX_new(method)) == NULL) {
        printf("Unable to create a new SSL context structure.\n");
    }
    if (!SSL_CTX_set_default_verify_paths(ctx)) {
        printf("Unable to set default verify paths.\n");
    }

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "domain.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    printf("Cert loaded.\n");

    if (SSL_CTX_use_PrivateKey_file(ctx, "domain.key", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    printf("Private key loaded.\n");


    //init sock
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {         
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
    printf("https socket bind successful!\n");
     
    if (listen(sock, 10) == -1 ) {    
        return -1;
    }
    printf("listening on port %d ...\n", lhttpsport);

    struct sockaddr_in c_skt; 
    int s_size=sizeof(struct  sockaddr_in);
    int client = accept(sock, (struct sockaddr *)&c_skt, (socklen_t *)&s_size);
    printf("accept new connection %d\n", client);

	ssl = SSL_new(ctx);
    SSL_set_fd(ssl, client);

    if (SSL_accept(ssl) <= 0) { // Perform SSL/TLS handshake with peer
        ERR_print_errors_fp(stderr);
        printf("SSL connect failed.\n");
        return -1;
	}
    // char* msg = (char*)malloc(5000 * sizeof(char));
    // if (msg == NULL){
    //     printf("buffer malloc fail!\n");
    //     exit(-1);
    // }   
    // memset(msg, 0, 5000);

    char msg[5000];
    int ret = 0;
    while(ret <= 2 || !(msg[ret - 1] == '\n' && msg[ret - 2] == '\r')){
        int read = SSL_read(ssl, msg + ret, 5000 - ret);
        ret += read;
        if (read <= 1) {
            SSL_shutdown(ssl);
            close(client);
            SSL_free(ssl); // Free the SSL connection
            break;
        }        
    }
    printf("%s", msg);
    printf("Process Https request....\n");
    processHttpsRequest(msg, client, ssl);
    return 0;
    
}

void processHttpsRequest(char* request, int client_sock, SSL *ssl) {   
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
    printf("file --> %s\n", arguments);
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
	char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";	//索引表
	unsigned unum;											//中间变量
	int i=0,j,k;
	//确定unum的值
	if(radix==10&&num<0)									//十进制负数
	{
		unum=(unsigned)-num;
		str[i++]='-';
	}
	else unum=(unsigned)num;								//其他情况
	//逆序
	do
	{
		str[i++]=index[unum%(unsigned)radix];
		unum/=radix;
	}while(unum);
	str[i]='\0';
	//转换
	if(str[0]=='-') k=1;									//十进制负数
	else k=0;
	//将原来的“/2”改为“/2.0”，保证当num在16~255之间，radix等于16时，也能得到正确结果
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