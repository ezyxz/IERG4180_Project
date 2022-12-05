// -lssl -lcrypto/
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
#include "getopt.h"

//URL PARSE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>			/* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>


#define _URL_DEBUG 0

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


#define HEAD_FTP_P "ftp://"
#define HEAD_FTPS_P "ftps://"   
#define HEAD_FTPES_P "ftpes://"
#define HEAD_HTTP_P "http://"
#define HEAD_HTTPS_P "https://"


#define PORT_FTP  21
#define PORT_FTPS_I  990    //implicit
#define PORT_FTPS_E  21     //explicit
#define PORT_HTTP 80
#define PORT_HTTPS 443

#define URL_ERROR -1
#define URL_OK 0


struct pro_port{
	char pro_s[32];
	unsigned short port;
};

struct pro_port g_pro_port[]={
	{HEAD_FTP_P,PORT_FTP},
	{HEAD_FTPS_P,PORT_FTPS_I},	
	{HEAD_FTPES_P,PORT_FTPS_E},	
	{HEAD_HTTP_P,PORT_HTTP},	
	{HEAD_HTTPS_P,PORT_HTTPS},
};

#define MAX_COMM_NAME_LEN 128

#define MAX_URL_LEN 1024
#define INET_ADDRSTRLEN 16
#define INET_DOMAINSTRLEN 128
#define MAX_PORT_LEN 6
#define MAX_PATH_FILE_LEN 256
#define MAX_IP_STR_LEN 32

#define MAX_USER_LEN 32
#define MAX_PASS_LEN 32

int PROTO = -1;
typedef struct
{
	char user[MAX_USER_LEN];
	char pass[MAX_PASS_LEN];
	char domain[INET_DOMAINSTRLEN];//域名
	char svr_dir[MAX_PATH_FILE_LEN]; //文件路径
	char svr_ip[MAX_IP_STR_LEN];
	int port;
}URL_RESULT_T;


URL_RESULT_T url_result_t;

int  parse_domain_dir(char *url,URL_RESULT_T *result)
{
	char *p;
	char *token = NULL;
	char buf[MAX_URL_LEN] = {0};
	char dir_str[MAX_PATH_FILE_LEN*2] = {0};
	char svr_dir[MAX_COMM_NAME_LEN] = {0}; 	
	char ip_port[MAX_COMM_NAME_LEN*2] = {0};//域名:port
	char domain[INET_DOMAINSTRLEN] = {0};
	char port_s[MAX_PORT_LEN] = {0};

	char user_pass[MAX_USER_LEN + MAX_PASS_LEN] = {0};
	char user[MAX_USER_LEN] = {0};
	char pass[MAX_PASS_LEN] = {0};

	int test;

	p = buf;

	//strtok后 buf中内容会被修改，所以先把url内容拷贝出来
	strncpy(buf,url,sizeof(buf));
	
	//"baidu.com:8080/dir/index.html"

	token = strtok(buf, "@");
	if(token != NULL) {
#if _URL_DEBUG
		printf("token=%s\n",token);
#endif
		if(strlen(token) != strlen(url)) //找不到@后，返回值指向头
		{
			if(strlen(user_pass) == 0) {
				strncpy(user_pass, token,sizeof(user_pass));
				p += strlen(user_pass)+1; //指向@后字符串
			} 
		}
	}
#if _URL_DEBUG
	printf("user_pass=%s\n",user_pass);
#endif	

	//提取用户名密码
	if(strlen(user_pass) > 0)
	{
		token = strtok(user_pass, ":");
		 while(token != NULL) {
			if(strlen(user) == 0) {
				strncpy(user, token,sizeof(user));
				if((strlen(user)==0)||(strlen(user)>MAX_USER_LEN))
				{
					printf("%s() %d\n",__func__,__LINE__);
					return URL_ERROR;
				}
			} else{
				strcat(pass,token);
			}
			token = strtok(NULL, "/");
		}
	}
	
//192.168.1.100:990/LE2.0    192.168.20.10/dir1/dir2/index.html   baidu.com/down   baidu.com
//body: baidu.com:8080/dir/index.html

	
	 token = strtok(p, "/");
	 while(token != NULL) {
		if(strlen(ip_port) == 0) {
			strncpy(ip_port, token,sizeof(ip_port));
			if((strlen(ip_port)==0)||(strlen(ip_port)>INET_DOMAINSTRLEN + MAX_PORT_LEN + 1))
			{
				printf("%s() %d\n",__func__,__LINE__);
				return URL_ERROR;
			}
		} else{
			strcat(dir_str,"/");
			strcat(dir_str,token);
		}

		token = strtok(NULL, "/");
	}
#if _URL_DEBUG
	printf("ip_port=%s\n",ip_port);
	printf("svr_dir:%s\n\n",dir_str);
#endif	 

	
	token = strtok(ip_port, ":");
	 while(token != NULL) {
		if(strlen(domain) == 0) {
			strncpy(domain, token,sizeof(domain));
			if((strlen(domain)==0)||(strlen(domain)>INET_DOMAINSTRLEN))
			{
				printf("%s() %d invalid daomain length\n",__func__,__LINE__);
				return URL_ERROR;
			}
		}else if(strlen(port_s) == 0){
			strncpy(port_s, token,sizeof(port_s));
			if((strlen(port_s)==0)||(strlen(port_s)>MAX_PORT_LEN))
			{
				printf("%s() %d invalid port length\n",__func__,__LINE__);
				return URL_ERROR;
			}
		}
		token = strtok(NULL, ":");
	} 
	if(strlen(domain)==0)
	{
		printf("%s() %d there is no domain\n",__func__,__LINE__);
		return URL_ERROR;
	}
	if(strlen(port_s) > 0)
	{
		result->port = atoi(port_s);		
	}

	if(strlen(dir_str)==0){
		//baidu.com  没有路径 用默认/
		strcat(dir_str,"/");
	}

	strncpy(result->user,user,sizeof(result->user));
	strncpy(result->pass,pass,sizeof(result->pass));
	strncpy(result->domain,domain,sizeof(result->domain));
	strncpy(result->svr_dir,dir_str,sizeof(svr_dir));

#if _URL_DEBUG
	
	printf("user:%s\n",result->user);
	printf("pass:%s\n",result->pass);
	printf("port:%d\n",result->port);
	printf("domain:%s\n",result->domain);
	printf("svr_dir:%s\n\n\n",result->svr_dir);
#endif
	return URL_OK;
}



void remove_quotation_mark(char *input)
{
    int i=0;
    char tmp_buf[MAX_URL_LEN];
    char *tmp_ptr = input;

    while(*tmp_ptr != '\0') {
	if(*tmp_ptr != '"') {
	    tmp_buf[i] = *tmp_ptr;
	    i++;
	}
	tmp_ptr++;
    }
    tmp_buf[i] = '\0';
    strcpy(input, tmp_buf);
}


/*
return code
-1 : invalid param  0:domain   1 :ipv4 
*/
 int check_is_ipv4(char *domain)
 {
	struct in_addr s;  
	char IPdotdec[20] = {0}; 

	if((strlen(domain) == 0) || (strlen(domain)>INET_DOMAINSTRLEN))
	{
		printf("invalid domain length!\n");	
		return -1;
	}
	if(inet_pton(AF_INET, domain, (void *)&s)==1)
	{
		inet_ntop(AF_INET, (void *)&s, IPdotdec, 16);
		//printf("fota_check_is_ipv4 %s\n", IPdotdec);
		return 1;
	}else{
		//printf("may be domain\n");	
		return -1;
	}
 }


int parse_url(char *raw_url,URL_RESULT_T *result)
{
	int i ;
	int ret;
	char *p = NULL;
	char out_buf[MAX_URL_LEN];//url
	char body[MAX_URL_LEN];



	strcpy(out_buf, "");
	strcpy(body, "");		


	strncpy(out_buf, raw_url,sizeof(out_buf));
	p = out_buf;

	if(strstr(out_buf, "\"") != NULL) 
	{	               
		remove_quotation_mark(out_buf);	            
	}

	for(i=0;i<ARRAY_SIZE(g_pro_port);i++)
	{	
		if(strncasecmp(g_pro_port[i].pro_s,p,strlen(g_pro_port[i].pro_s))==0)
		{	
			p += strlen(g_pro_port[i].pro_s);
			strncpy(body, p,sizeof(body));
			result->port = g_pro_port[i].port;
            PROTO = i;
			break;
		}
	}
	if(i == ARRAY_SIZE(g_pro_port))
	{
		printf("%s: Error: invalid protocol %s\n", __FUNCTION__,out_buf);
		return URL_ERROR;	
	}

#if _URL_DEBUG
	printf("%s():\nbody: %s\n", __FUNCTION__,body);
#endif

	if(strstr(body, "\"") != NULL) 
	{	               
		remove_quotation_mark(body);	            
	}

//解析域名、端口号、文件目录
	ret = parse_domain_dir(body,result);
	if(ret == URL_ERROR)
	{
		ret = printf("parse_domain_dir() err\n");
		return URL_ERROR;
	}	
	return ret;
}



int dns_resoulve(char *svr_ip,const char *domain)
{
    char   **pptr;
    struct hostent *hptr;
    char   str[MAX_IP_STR_LEN];

    if((hptr = gethostbyname(domain)) == NULL)
    {
        printf(" gethostbyname error for host:%s\n", domain);
        return URL_ERROR;
    }
#if _URL_DEBUG

    printf("official hostname:%s\n",hptr->h_name);
    for(pptr = hptr->h_aliases; *pptr != NULL; pptr++)
        printf(" alias:%s\n",*pptr);
#endif
    switch(hptr->h_addrtype)
    {
        case AF_INET:
        case AF_INET6:
            pptr=hptr->h_addr_list;
		#if _URL_DEBUG
            for(; *pptr!=NULL; pptr++)
            {
                printf(" address:%s\n",inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str)));
            }
            printf(" first address: %s\n",inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str)));
		#else	
			inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
		#endif	
			strncpy(svr_ip,str,MAX_IP_STR_LEN);
		
        break;
        default:
            printf("unknown address type\n");
        break;
    }

    return URL_OK;
}

//END URL PARSE



char* rhost = "127.0.0.1";
char* lhost = "127.0.0.1";
int rport = 4180;
char dest_url[8192];
X509 *cert = NULL;
X509_NAME *certname = NULL;
const SSL_METHOD *method;
SSL_CTX *ctx;
SSL *ssl;
int server = 0;
int ret, i;
char *ptr = NULL;
char* URL = "http://127.0.0.1:4180/";
char* FILE_NAME = "/";


void argParse(int argc, char* argv[]);
void argPrint();
int create_socket();


void certsPrint(SSL* ssl);

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
    argPrint();

    char url[256];
    strcpy (url, URL);
    strcat (url, "/");
    strcat (url, FILE_NAME);
    printf("Clinet start go url [%s]....\n", url);
    parse_url(url,&url_result_t);

	ret = check_is_ipv4(url_result_t.domain);	
	if(ret != 1)
	{	
		//dns
		dns_resoulve(url_result_t.svr_ip,url_result_t.domain);
	}
    PROTO -= 3;
	printf("port:%d\n",url_result_t.port);
	printf("domain:%s\n",url_result_t.domain);
	printf("svr_dir:%s\n",url_result_t.svr_dir);
	printf("svr_ip:%s\n",url_result_t.svr_ip);
    printf("Proto [%d]  0->http 1->https\n", PROTO);

    rhost = url_result_t.domain;
    rport = url_result_t.port;
    if (strlen(url_result_t.svr_ip) > 1){
        rhost = url_result_t.svr_ip;

    }
    
    server = create_socket();
    if (server != 0)
        printf("Successfully made the TCP connection to: %s.\n", rhost);
    if (PROTO == 1){


        OpenSSL_add_all_algorithms();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();
        SSL_load_error_strings();
        if (SSL_library_init() < 0)
            printf("Could not initialize the OpenSSL library !\n");

        method = TLS_method();
        if ((ctx = SSL_CTX_new(method)) == NULL)
            printf("Unable to create a new SSL context structure.\n");
        
        // ret = SSL_CTX_load_verify_file(ctx, "domain.crt");
        // if (ret == 1){
        //     printf("crt added to cert store.\n");
        // }
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, server);
        SSL_set_tlsext_host_name(ssl, rhost); // Enable Server-Name-Indication
        if (SSL_connect(ssl) != 1)
            printf("Error: Could not build a SSL session to: %s.\n", rhost);
        else
            printf("Successfully enabled SSL/TLS session to: %s.\n", rhost);
        

        certsPrint(ssl);
        cert = SSL_get_peer_certificate(ssl);
        if (cert == NULL)
            printf("Error: Could not get a certificate from: %s.\n", rhost);
        else
            printf("Retrieved the server's certificate from: %s.\n", rhost);
        
        ret = SSL_get_verify_result(ssl);
            
        if (ret != X509_V_OK)
            printf("Warning: Validation failed for certificate from: %s.\n", rhost);
        else
            printf("Successfully validated the server's certificate from: %s.\n",rhost);

        char request[8192];
         sprintf(request,
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",url_result_t.svr_dir, rhost);
        SSL_write(ssl, request, strlen(request));
        // Receive and print out the HTTP response
        printf("------------------ RESPONSE RECEIVED ---------------------\n");
        int len = 0;
        do {
            char buff[1536];
            len = SSL_read(ssl, buff, sizeof(buff));
            if (len > 0) 
                fwrite(buff, len, 1, stdout);
        } while (len > 0);
        printf("\n----------------------------------------------------------\n");
    
    }else if (PROTO == 0){
        char request[8192];
         sprintf(request,
            "GET %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Connection: close\r\n\r\n",url_result_t.svr_dir, rhost);
        // printf("[%s]", request);
        int total = strlen(request);
        int sent = 0;
        while (sent < total)
        {
            int r = send(server, request+sent, total-sent, 0);
            sent += r;
        }
        // Receive and print out the HTTP response
        printf("------------------ RESPONSE RECEIVED ---------------------\n");
        int len = 0;
        do {
            char buff[1536];
            len = recv(server, buff, sizeof(buff),0);
            if (len > 0) 
                fwrite(buff, len, 1, stdout);
        } while (len > 0);
        printf("\n----------------------------------------------------------\n");
    
        
        
    }
    
}


int create_socket() {
    int sockfd;
    struct sockaddr_in dest_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(rport);
    dest_addr.sin_addr.s_addr = inet_addr(rhost);
    if (connect(sockfd, (struct sockaddr *) &dest_addr,
    sizeof(struct sockaddr)) == -1) {
        printf("Error: Cannot connect to host %s  on port %d.\n",rhost, rport);
        exit(0);
    }
    return sockfd;
}

void certsPrint(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}

void argParse(int argc, char* argv[]) {
    const char* optstring = "a";
    int c;
    struct option opts[] = {
        {"URL", 1, NULL, 0},
        {"url", 1, NULL, 1},
        {"file", 1, NULL, 2},
        {"FILE", 1, NULL, 3},
    };

    while ((c = getopt_long_only(argc, argv, optstring, opts, NULL)) != -1) {
        switch (c) {
        case 0:
            URL = optarg;
            break;
        case 1:
            URL = optarg;
            break;
        case 2:
            FILE_NAME = optarg;
            break;
        case 3:
            FILE_NAME = optarg;
            break;
        default:
            printf("Error: get invalid argument\n");
            break;

        }
    }
}

void argPrint(){

    printf("NetProbeClinet <parameters>, see below:\n");
    printf("   <-URL http/https request> [%s]\n", URL);
    printf("   <-FILE file name> [%s]\n\n\n", FILE_NAME);
}
